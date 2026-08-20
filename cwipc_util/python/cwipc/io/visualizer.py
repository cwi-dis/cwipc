import os
import sys
from typing import Dict, Any
import time
import subprocess
import argparse
import queue
from .. import cwipc_window, cwipc_tilefilter, cwipc_write, cwipc_pointcloud_wrapper
from ..util import cwipc_sink_wrapper
from ..net.abstract import *
from ..filters.abstract import cwipc_abstract_filter
from ..filters.colorize import ColorizeFilter
import numpy as np

class Visualizer(cwipc_sink_abstract):
    """Asynchronous point cloud viewer. The API is very similar to cwipc_window, but this class runs the UI
    in a separate thread so interaction (and feeding of the point clouds) does not cause the whole program to grind
    to a halt.
    """
    HELP="""
space         Pause/resume
.             Single step (for recordings)
<             Rewind (for recordings)
mouse_left    Rotate viewpoint
mouse_scroll  Zoom in/out
mouse_right   Up/down viewpoint
+/-           Increase/decrease point size
0-9           Select single tile to view ( 0=All )
n             Select next tile to view
a             Show all tiles
m             Toggle tile selection mask mode
i             Toggle tile selection index mode
f             Colorize points to show contributing cameras
r             Toggle skeleton rendering (only if executed with --skeleton)
w             Write PLY file
t             Timelapse: like w but after a 5 second delay
p             Timelapse pause: pause after 5 seconds
z             Next tile quality selection (if available)
c             Reload cameraconfig
e             Edit cameraconfig
?,h           Help
q,ESC         Quit
    """

    def __init__(self, verbose=False, nodrop=False, args : Optional[argparse.Namespace]=None, title="cwipc_view", **kwargs):
        self.title : str = title
        self.visualiser : Optional[cwipc_sink_wrapper] = None
        self.producer : Optional[cwipc_producer_abstract] = None
        self.source : Optional[cwipc_source_abstract] = None
        if args:
            self.args = args
        else:
            self.args = argparse.Namespace(
                cameraconfig=None,
                rgb=False,
                rgb_cw=False,
                rgb_ccw=False,
                rgb_full=False,
                timestamps=False,
                endpaused=False,
                nodrop=False,
                verbose=False,
            )
        if self.args.rgb_cw or self.args.rgb_ccw or self.args.rgb_full:
            self.args.rgb = True
        self.forcedrop : bool = False
        self.paused : bool = False
        self.single_step : bool = False
        self.recompute_display_pc : bool = False
        self.display_filter : Optional[cwipc_abstract_filter] = None
        # If we want paused we actually set single_step (so we get the first point cloud)
        if 'paused' in self.args and self.args.paused:
            self.paused = True
            self.single_step = True
        self.endpaused = 'endpaused' in self.args and self.args.endpaused

        for k in kwargs:
            # Should only be the ones that can also be in args, but hey...
            setattr(self.args, k, kwargs[k])
        queue_size = 1 if self.args.timestamps else 2
        self.input_queue : queue.Queue[Optional[cwipc_pointcloud_wrapper]] = queue.Queue(maxsize=queue_size)
        self.current_pc : Optional[cwipc_pointcloud_wrapper] = None
        self.display_pc : Optional[cwipc_pointcloud_wrapper] = None
        self.tilefilter : Optional[int] = None
        self.filter_mode : str = 'mask'
        self.alive : bool = True
        self.stop_requested = False
        self.point_size_min : float = 0.0005
        self.point_size_power : int = 0
        self.display_fps : int = 30
        # Timelapse settings, while timelapse command active. Zero means not active.
        self.timelapse_write_at : float = 0
        self.timelapse_beep_at : float = 0
        self.timelapse_pause_at : float = 0
        # Let's go!
        self.start_window()
        
    def statistics(self) -> None:
        pass

    def start(self) -> None:
        pass

    def stop(self) -> None:
        self.stop_requested = True
    
    def set_producer(self, producer : cwipc_producer_abstract) -> None:
        self.producer = producer
        
    def set_source(self, source : cwipc_source_abstract) -> None:
        self.source = source
        
    def is_alive(self) -> bool:
        return self.alive  
        
    def _source_is_alive(self) -> bool:
        if self.producer:
            rv = self.producer.is_alive()
        elif self.source:
            rv = not self.source.eof()
        return rv

    def _continue_running(self) -> bool:
        if self.stop_requested:
            return False
        if self._source_is_alive():
            return True
        if self.endpaused:
            self.paused = True
            return True
        return False
    
    def _get_next_pc(self, drop=False) -> bool:
        get_timeout = (1.0 / self.display_fps)
        try:
            pc = self.input_queue.get(timeout=get_timeout)
        except queue.Empty:
            return False
        if drop:
            pc = None
            return False
        if pc:
            self.current_pc = pc
            if self.args.timestamps:
                self._show_timestamps(pc, "timestamps")
            return True
        return False

    def _get_new_display_pc(self) -> None:
        assert self.current_pc
        assert self.current_pc._cwipc
        pc_to_show = self.current_pc
        if self.display_filter:
            pc_to_show = self.display_filter.filter(pc_to_show)
        if self.tilefilter:
            pc_to_show = cwipc_tilefilter(pc_to_show, self.tilefilter)
        self.display_pc = pc_to_show
        cellsize = max(self.display_pc.cellsize(), self.point_size_min)
        self.display_pc._set_cellsize(cellsize*pow(2,self.point_size_power))
            
    def run(self) -> None:
        while self._continue_running():
            got_new_pc = False
            if self.current_pc is None:
                # If we have no point cloud we get one.
                got_new_pc = self._get_next_pc()
            elif self.paused:
                if self.forcedrop or not self.args.nodrop:
                    got_new_pc = self._get_next_pc(drop=True)
            else:
                got_new_pc = self._get_next_pc()
            if self.current_pc is None:
                assert not got_new_pc
                continue
            if self.single_step and self.current_pc != None:
                self._show_timestamps(self.current_pc, 'single_step')
                self.paused = True
                self.single_step = False
            if got_new_pc or self.recompute_display_pc:
                self._get_new_display_pc()
            self._draw_pc()
        self.alive = False
        # Empty queue. First clear nodrop so the producer won't block.
        self.forcedrop = True
        try:
            while True:
                pc = self.input_queue.get(block=False)
        except queue.Empty:
            pass
        if self.args.rgb:
            import cv2
            if self.args.rgb_full:
                cv2.destroyAllWindows()
            else:
                cv2.destroyWindow("RGB")
        self.visualiser = None
                
    def feed(self, pc : cwipc_pointcloud_wrapper) -> None:
        try:
            if self.args.nodrop and not self.forcedrop:
                self.input_queue.put(pc)
            else:
                self.input_queue.put(pc, timeout=0.5)
        except queue.Full:
            pass
            
    def start_window(self):
        cwd = os.getcwd()   # Workaround for cwipc_window changing working directory
        self.visualiser = cwipc_window(self.title)
        os.chdir(cwd)
        if self.args.verbose: print('display: started', flush=True)
        self.visualiser.feed(None, True)

    def _show_timestamps(self, pc: cwipc_pointcloud_wrapper, label : str) -> None:
        print(f'{label}: ts={pc.timestamp()}')
        metadata = pc.access_metadata()
        if metadata != None and metadata.count() > 0:
            for i in range(metadata.count()):
                metadataname = metadata.name(i)
                if not "timestamps" in metadataname:
                    continue
                descr = metadata.description(i)
                print(f'{label}:    {metadataname}: {descr}')

    def _draw_pc(self) -> None:
        """Draw pointcloud and interact with the visualizer.
        If None is passed as the pointcloud it will only interact (keeping the previous pointcloud visible)
        """
        assert self.visualiser
        if self.current_pc and self.args.rgb:
            # First show RGB, if wanted, from current PC read.
            if self.args.rgb:
                self.draw_rgb(self.current_pc)
        if self.display_pc:
            if self.args.verbose:
                if not self.paused:
                    print(f'display: showing pointcloud timestamp={self.display_pc.timestamp()} cellsize={self.display_pc.cellsize()} latency={time.time() - self.display_pc.timestamp()/1000000.0:.3f}')
            ok = self.visualiser.feed(self.display_pc, True)
            if not ok: 
                print('display: window.feed() returned False')
                self.stop()
                return
        self.interact_visualiser() # Note we pass the original pc to interact, not the modified pc.

    def interact_visualiser(self) -> None:
        """Allow user interaction with the visualizer."""
        assert self.visualiser
        interaction_duration = 500 // self.display_fps
        cmd = self.visualiser.interact(None, "?h\x1bq .<+-cefwtpamirsnz0123456789", interaction_duration)
        # First handle the timelapse
        if self.timelapse_write_at > 0:
            now = time.time()
            if now >= self.timelapse_write_at:
                print(f"timelapse: Capture point cloud.\x07", file=sys.stderr)
                self.timelapse_beep_at = 0
                self.timelapse_write_at = 0
                self.write_current_pointcloud()
            if now >= self.timelapse_beep_at:
                print(f"timelapse: {int(self.timelapse_write_at - now)}\x07", file=sys.stderr)
                self.timelapse_beep_at += 1
        if self.timelapse_pause_at > 0:
            now = time.time()
            if now >= self.timelapse_pause_at:
                print(f"timelapse: pause", file=sys.stderr)
                self.paused = True
                self.timelapse_pause_at = 0
                if self.current_pc:
                    self._show_timestamps(self.current_pc, 'pause')
        # Now handle the commands
        if cmd == "q" or cmd == "\x1b":
            self.stop()
            return
        elif cmd == '?' or cmd == 'h':
            print(self.HELP)
        elif cmd == "w" and self.current_pc:
            self.write_current_pointcloud()
        elif cmd == "t":
            now = time.time()
            self.timelapse_beep_at = now + 1
            self.timelapse_write_at = now + 5
            self.paused = False
            print(f"timelapse: capture in 5 seconds", file=sys.stderr)
        elif cmd == "p":
            now = time.time()
            self.timelapse_pause_at = now + 5
            print(f"timelapse: pause in 5 seconds", file=sys.stderr)
            self.paused = False
        elif cmd == " ":
            self.paused = not self.paused
            if self.paused:
                if self.current_pc:
                    self._show_timestamps(self.current_pc, 'pause')
        elif cmd == ".":
            self.single_step = True
            self.paused = False
        elif cmd == "<":
            if not self.source.seek(0):
                print("Input source does not support seek")
            self.paused = False
        elif cmd == 'a':
            self.select_tile(all=True)
            self.recompute_display_pc = True
        elif cmd == 'm':
            self.select_mode('mask')
        elif cmd == 'i':
            self.select_mode('index')
        elif cmd == 'n':
            self.select_tile(increment=True)
            self.recompute_display_pc = True
        elif cmd == 'r':
            pass # toggle skeleton rendering (managed on cwipc_view)
        elif cmd in '0123456789':
            self.select_tile(number=int(cmd))
            self.recompute_display_pc = True
        elif cmd == '+':
            self.point_size_power += 1
            self.recompute_display_pc = True
        elif cmd == '-':
            if self.point_size_power>0:
                self.point_size_power -= 1
                self.recompute_display_pc = True
        elif cmd == '\0':
            pass
        elif cmd == 'c':
            self.paused = False
            print("reload: reloading cameraconfig.json...", file=sys.stderr)
            self.reload_cameraconfig()
            print("reload: reloaded cameraconfig.json.", file=sys.stderr)
        elif cmd == 'e':
            self.edit_cameraconfig()
        elif cmd == 'f':
            if self.display_filter == None:
                self.display_filter = ColorizeFilter(0.8, "camera")
                self.display_filter.set_keep_source()
            else:
                self.display_filter = None
            self.recompute_display_pc = True
        elif cmd == 'z':
            if self.source and hasattr(self.source, "select_next_tile_quality"):
                selection = self.source.select_next_tile_quality()
                print(f"Selected tile quality: {selection}")
            else:
                print("Input source does not support select_next_tile_quality")
        else:
            print(f"Unknown command {repr(cmd)}")
            print(self.HELP, flush=True)
    
    def write_current_pointcloud(self):
        """Save the current point cloud. May be overridden in subclasses to do something else."""
        assert self.current_pc
        filename = f'pointcloud_{self.current_pc.timestamp()}.ply'
        cwipc_write(filename, self.current_pc, True) #writing in binary
        print(f'Saved as {filename} in {os.getcwd()}')

    def draw_rgb(self, pc : cwipc_pointcloud_wrapper) -> None:
        """Draw a window with the RGB data of all cameras."""
        import cv2
        metadata = pc.access_metadata()
        if not metadata:
            return
        per_camera_images = metadata.get_all_images("rgb.")
        if self.args.rgb_full:
            
            for name, image in per_camera_images.items():
                cv2.imshow(name, image)
            cv2.waitKey(1)
            return
        # Combine images into a single window to show
        all_images = list(per_camera_images.values())

        if len(all_images) == 0:
            return
        if self.args.rgb_cw or self.args.rgb_ccw:
            full_image = cv2.hconcat(all_images)
        else:
            full_image = cv2.vconcat(all_images)
        # Scale to something reasonable
        h, w, _ = full_image.shape
        hscale = 1024 / h
        wscale = 1024 / w
        scale = min(hscale, wscale)
        if scale < 1:
            new_h = int(h*scale)
            new_w = int(w*scale)
            full_image = cv2.resize(full_image, (new_w, new_h), interpolation=cv2.INTER_LINEAR)
        cv2.imshow("RGB", full_image)
        cv2.waitKey(1)
    
    def reload_cameraconfig(self) -> None:
        """Reload the cameras. Call after changing cameraconfig.json."""
        assert self.source
        assert hasattr(self.source, "reload_config")
        try:
            ok = self.source.reload_config(self.args.cameraconfig) # type: ignore
            if not ok:
                print("reload_cameraconfig: failed to reload cameraconfig")
        except Exception as e:
            print(f"reload_cameraconfig: Exception: {e}")
    
    def edit_cameraconfig(self) -> None:
        editor = os.environ.get("EDITOR", "code")
        cameraconfig = self.args.cameraconfig or "cameraconfig.json"
        print(f"edit_cameraconfig: run: {editor} {cameraconfig}")
        subprocess.run([editor, cameraconfig])
        print(f"edit_cameraconfig: don't forget to use 'c' command to reload cameraconfig.json when done")
    
    def select_mode(self, newmode):
        self.filter_mode = newmode
        print(f"tilefilter mask mode: {self.filter_mode}. Showing all tiles", flush=True)
        self.tilefilter = None
        self.select_tile(all=True)
        
    def select_tile(self, *, number : Optional[int]=None, all=False, increment=False):
        assert self.source
    
        if all:
            self.tilefilter = None
            print("Showing all tiles")
        elif increment:
            if not self.tilefilter:
                self.tilefilter = 1
            else:
                self.tilefilter = self.tilefilter + 1
            print(f"Showing tile number {self.tilefilter} mask 0x{self.tilefilter:x}", flush=True)
        else:
            assert number != None
            if number == 0:
                self.tilefilter = 0
                print("Showing all tiles", flush=True)
            else:
                if self.filter_mode == 'mask':
                    self.tilefilter = pow(2,number-1)
                else:
                    self.tilefilter = number
                print(f"Showing tile number {self.tilefilter} mask 0x{self.tilefilter:x}", flush=True)
