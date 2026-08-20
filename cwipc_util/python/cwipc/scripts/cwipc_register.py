"""
Initialize RGBD camera setup or raw recording so they produce overlapping point clouds.
"""
import sys
import os
import time
import shutil
import argparse
import json
from typing import Optional, List, cast, Tuple, Dict
import numpy
import cwipc
from cwipc.net.abstract import *
from ._scriptsupport import *
from cwipc.registration.abstract import *
from cwipc.registration.util import *
from cwipc.registration.cameraconfig import CameraConfig
import cwipc.registration.multicoarse
import cwipc.registration.fine
import cwipc.registration.multicamera
import cwipc.registration.analyze
import cwipc.registration.plot
from cwipc.io.visualizer import Visualizer

try:
    import cwipc.realsense2
except ModuleNotFoundError:
    cwipc.realsense2 = None
except FileNotFoundError:
    cwipc.realsense2 = None
try:
    import cwipc.kinect
except ModuleNotFoundError:
    cwipc.kinect = None
except FileNotFoundError:
    cwipc.kinect = None


DEFAULT_FILENAME = "cameraconfig.json"

class RegistrationVisualizer(Visualizer):
    reload_cameraconfig_callback : Optional[Callable[[], None]]

    def __init__(self, *args, **kwargs):
        Visualizer.__init__(self, *args, **kwargs)
        self.captured_pc : Optional[cwipc_pointcloud_wrapper] = None
        self.reload_cameraconfig_callback = None

    def write_current_pointcloud(self):
        self.captured_pc = self.current_pc
        # We need to detach the current point cloud from the visualizer, otheriwse it will free it.
        self.current_pc = None
        self.display_pc = None
        self.stop()

    def reload_cameraconfig(self) -> None:
        super().reload_cameraconfig()
        if self.reload_cameraconfig_callback:
            self.reload_cameraconfig_callback()

def main():
    assert __doc__ is not None
    parser = ArgumentParser(description=__doc__.strip(), formatter_class=argparse.RawDescriptionHelpFormatter)
    def twofloats(s):
        f1, f2 = s.split(',')
        return float(f1), float(f2)
    parser.add_argument("--guided", action="store_true", help="Guide me through the whole registration procedure")
    parser.add_argument("--tabletop", action="store_true", help="Do static registration of one camera, 1m away at 1m height")
    parser.add_argument("--noregister", action="store_true", help="Don't do any registration, only create cameraconfig.json if needed")
    parser.add_argument("--clean", action="store_true", help=f"Remove old {DEFAULT_FILENAME} and calibrate from scratch")

    parser.add_argument("--interactive", action="store_true", help="Interactive mode: show pointclouds (and optional RGB images). w command will attempt registration.")
    parser.add_argument("--paused", action="store_true", help="Start paused. Use this with --guided for registering recordings")
    parser.add_argument("--rgb", action="store_true", help="Show RGB captures in addition to point clouds")
    parser.add_argument("--rgb_cw", action="store_true", help="When showing RGB captures first rotate the 90 degrees clockwise")
    parser.add_argument("--rgb_ccw", action="store_true", help="When showing RGB captures first rotate the 90 degrees counterclockwise")
    parser.add_argument("--rgb_full", action="store_true", help="When showing RGB captures don't scale and combine but show every image in its own window")
    parser.add_argument("--timestamps", action="store_true", help="Print detailed timestamp information about every point cloud displayed")

    parser.add_argument("--coarse", action="store_true", help="Do coarse registration (default: only if needed)")
    parser.add_argument("--nofloor", action="store_true", help="Don't do a floor registration (default: always do it)")
    parser.add_argument("--nofine", action="store_true", help="Don't do fine registration (default: always do it)")
    parser.add_argument("--correspondence", type=float, metavar="FLOAT", help="Correspondence threshold for fine calibration alignment (default: use analysis result)")
    
    parser.add_argument("--algorithm_analyzer", action="store", help="Analyzer algorithm to use")
    parser.add_argument("--algorithm_multicamera", action="store", help="Fine alignment outer algorithm to use, for multiple cameras")
    parser.add_argument("--algorithm_fine", action="store", help="Fine alignment inner registration algorithm to use")
    parser.add_argument("--help_algorithms", action="store_true", help="Show available algorithms and a short description of them")

    parser.add_argument("--nograb", metavar="PLYFILE", action="store", help=f"Don't use grabber but use .ply file grabbed earlier, using {DEFAULT_FILENAME} from same directory.")
    parser.add_argument("--skip", metavar="N", type=int, action="store", help="Skip the first N captures")
    parser.add_argument("--no_aruco", action="store_true", help="Do coarse alignment with interactive selection (default: find aruco marker)")
    parser.add_argument("--conf_init", action="append", metavar="PATH=VALUE", help="If creating cameraconfig.json, set PATH to VALUE. Example: postprocessing.depthfilterparameters.threshold_far=3.0")
    
    parser.add_argument("--plot", action="store_true", help="After each fine aligner step show a graph of the results")
    parser.add_argument("--plotstyle", action="store", help="Plot style for the fine alignment step. Comma-separated list of 'count', 'cumulative', 'delta', 'all', 'log'.")
    parser.add_argument("--dry_run", action="store_true", help="Don't modify cameraconfig file")
    
    parser.add_argument("recording", nargs='?', help="A directory with recordings (realsense or kinect) for which to do registration")
    args = parser.parse_args()
    beginOfRun(args)
    if args.help_algorithms:
        print(cwipc.registration.analyze.HELP_ANALYZER_ALGORITHMS)
        print(cwipc.registration.fine.HELP_FINE_ALIGNMENT_ALGORITHMS)
        print(cwipc.registration.multicamera.HELP_MULTICAMERA_ALGORITHMS)
        return 0
    reg = Registrator(args)
    reg.run()



class Registrator:
    def __init__(self, args : argparse.Namespace):
        self.progname = sys.argv[0]
        self.capturerFactory : Optional[cwipc_activesource_factory_abstract] = None
        self.capturer = None
        self.args = args
        self.verbose = self.args.verbose
        self.show_plot = self.args.plot
        if args.plotstyle:
            self.show_plot = True
        self.dry_run = self.args.dry_run
        self.check_coarse_alignment = False # This can be a very expensive operation...
        #
        if self.args.guided:
            self.args.interactive = True
            self.args.rgb = True
        #
        # Coarse aligner class depends on what input we have. So there's no --algorithm option for this.
        #
        self.no_aruco = args.no_aruco
        if self.no_aruco:
            self.coarse_aligner_class = cwipc.registration.multicoarse.MultiCameraCoarseColorTarget
        elif self.args.rgb:
            self.coarse_aligner_class = cwipc.registration.multicoarse.MultiCameraCoarseArucoRgb
        else:
            self.coarse_aligner_class = cwipc.registration.multicoarse.MultiCameraCoarseAruco

        if self.args.algorithm_multicamera:
            self.multicamera_aligner_class = getattr(cwipc.registration.multicamera, args.algorithm_multicamera)
        else:
            self.multicamera_aligner_class = cwipc.registration.multicamera.DEFAULT_MULTICAMERA_ALGORITHM

        if self.args.algorithm_fine:
            self.alignment_class = getattr(cwipc.registration.fine, args.algorithm_fine)
        else:
            self.alignment_class = None # The fine alignment class is determined by the multicamera aligner chosen.

        if self.args.algorithm_analyzer:
            self.analyzer_class = getattr(cwipc.registration.analyze, args.algorithm_analyzer)
        else:
            self.analyzer_class = cwipc.registration.analyze.DEFAULT_ANALYZER_ALGORITHM

        if self.args.recording:
            if self.args.cameraconfig:
                print("cwipc_register: Cannot use --cameraconfig with a recording")
                sys.exit(1)
            self.args.cameraconfig = os.path.realpath(os.path.join(self.args.recording, DEFAULT_FILENAME))
            if self.args.guided:
                self.args.paused = True
                self.args.nodrop = True
                print("cwipc_register: --guided implies --paused and --nodrop when registering a recording")
        if not self.args.cameraconfig:
            self.args.cameraconfig = DEFAULT_FILENAME
        self.cameraconfig = CameraConfig(self.args.cameraconfig)

    def __del__(self):
        self.capturer = None

    def prompt(self, message : str):
        print(f"{message}")

    def ask(self, prompt : str, default : str) -> str:
        sys.stdout.write(f"{prompt} [{default}] ? ")
        sys.stdout.flush()
        line = sys.stdin.readline()
        line = line.strip()
        if not line: 
            return default
        return line

    def run(self) -> bool:
        if self.args.recording:
            # Special case: register a recording. First make sure we actually have a
            # cameraconfig file, otherwise generate it.
            if not self.initialize_recording():
                return False
            self.args.nodrop = True
        self.args.nodecode = True
        self.capturerFactory = activesource_factory_from_args(self.args)
        #
        # Now we really start
        #
        if self.args.nograb:
            self.create_nograb_cameraconfig()
        else:
            if not self.open_capturer():
                if self.args.recording:
                    print(f"cwipc_register: Cannot open capturer, probably an issue with {self.args.cameraconfig}")
                    return False
                print("cwipc_register: Cannot open capturer. Presume missing cameraconfig, try to create it.")
                self.create_cameraconfig()
                if not self.open_capturer():
                    print("cwipc_register: Still cannot open capturer. Giving up.")
                    return False
            assert self.capturer

            self._capture_some_frames(self.capturer)
                        
            self.cameraconfig.load(self.capturer.get_config())
        must_reload = False
        if not self.dry_run:
            anyChanged = False
            if self.args.conf_init:
                for setting in self.args.conf_init:
                    if self.cameraconfig.set_entry_from_string(setting):
                        anyChanged = True
            self.cameraconfig.save()
            if anyChanged:
                # Need to reload camera config if we made changes
                must_reload = True
        if self.args.noregister:
            return True
        if must_reload:
            self._reload_cameraconfig_to_capturer()
            must_reload = False
        if self.args.tabletop:
            assert self.cameraconfig.camera_count() == 1
            t = self.cameraconfig.get_transform(0)
            matrix = transformation_identity()
            t.set_matrix(matrix)
            self.cameraconfig.save()
            return True
        if self.args.coarse and not self.cameraconfig.is_identity():
            print(f"cwipc_register: reset matrix")
            pass 
            for i in range(self.cameraconfig.camera_count()):
                t = self.cameraconfig.get_transform(i)
                t.set_matrix(transformation_identity())
            self.cameraconfig.save()
            self._reload_cameraconfig_to_capturer()
        if self.args.coarse or self.cameraconfig.is_identity():
            if self.args.guided:
                self.args.interactive = True
                self.args.rgb = True
                print(f"\n===================================================================", file=sys.stderr)
                print(f"===== All commands should by typed in the point cloud window (the blue one).", file=sys.stderr)
                print(f"===== ? will print help here.", file=sys.stderr)
                print(f"===== c to reload the cameraconfig.json file will sometimes fail. Restart cwipc_register in that case.", file=sys.stderr)
                print(f"===== If you want to redo the coarse registration run cwipc_register --guided --coarse", file=sys.stderr)
                print(f"===== If you get surprising results try re-running with the --verbose option.", file=sys.stderr)
                print(f"===== See https://github.com/cwi-dis/cwipc/blob/master/doc/registration.md for more information.", file=sys.stderr)
                print(f"\n===================================================================", file=sys.stderr)
                print(f"===== Place Aruco marker at origin. ", file=sys.stderr)
                print(f"===== Examine RGB images and adjust cameras so marker is visible to all.", file=sys.stderr)
                print(f"===== Examine RGB images colors and exposure.", file=sys.stderr)
                print(f"===== Edit cameraconfig.json and change exposure, gain, backlight, etc. ", file=sys.stderr)
                print(f"===== Ensure there are no negative values (auto-exposure, etc) in the hardware parameters (but Depth exposure can be on auto)", file=sys.stderr)
                print(f"===== Ensure the Depth and Color width and height are to your liking", file=sys.stderr)
                print(f"===== For realsense, turn off all filters and use map_color_to_depth=-1", file=sys.stderr)
                print(f"===== If you are using sync cables ensure the sync parameters are set correctly", file=sys.stderr)
                print(f"===== If you are using a Kinect, ensure the depth and color cameras are aligned", file=sys.stderr)
                print(f"===== Press c in point cloud window (or restart cwipc_register) to reload cameraconfig.json", file=sys.stderr)
                print(f"===== When all is good press w in point cloud window, otherwise edit and c again", file=sys.stderr)
                print(f"===================================================================\n", file=sys.stderr)
            new_pc = None
            while new_pc == None:
                self.prompt("Coarse registration: capturing aruco/color target")
                pc = self.capture()
                if self.args.guided:
                    print(f"===== The windows will now close, the algorithms will run, and after that the windows will reopen.", file=sys.stderr)
                new_pc = self.coarse_registration(pc)
                pc = None
            assert new_pc

            new_pc = None
            if not self.dry_run:
                if self.verbose:
                    print(f"cwipc_register: save {self.cameraconfig.filename}")
                self.cameraconfig.save()
                must_reload = True
        else:
            if self.verbose:
                print(f"cwipc_register: skipping coarse registration, cameraconfig already has matrices")
        if not self.args.nofloor:
            if must_reload:
                self._reload_cameraconfig_to_capturer()
                must_reload = False
            if self.args.guided:
                self.args.rgb = False
                print(f"\n===================================================================", file=sys.stderr)
                print(f"===== Examine point cloud window. You want to capture lots of floor, so we can align it to Y=0.", file=sys.stderr)
                print(f"===== Edit cameraconfig.json and change near, far (or threshold_min_distance and threshold_max_distance), height_min, height_max, radius_filter. ", file=sys.stderr)
                print(f"===== At this point you want to include some floor, so set height_min to a small negative value.", file=sys.stderr)
                print(f"===== Press c in point cloud window (or restart cwipc_register) to reload cameraconfig.json", file=sys.stderr)
                print(f"===== Ensure you get a clean capture including the floor.", file=sys.stderr)
                print(f"===== There could be a human in the view, this doesn't matter.", file=sys.stderr)
                print(f"===== You can use 1234 or 0 to see only per-camera point clouds. ", file=sys.stderr)
                print(f"===== Press w in the point cloud window to capture.", file=sys.stderr)
                print(f"===================================================================\n", file=sys.stderr)

            self.prompt("Floor registration: capturing some floor")
            pc = self.capture()
            if self.args.guided:
                print(f"===== The window will now close, the floor alignment will run, and after that the windows will reopen.", file=sys.stderr)
            new_pc = self.fine_registration(pc, 
                            multicam_aligner_class=cwipc.registration.multicamera.MultiCameraToFloor, 
                            aligner_class=cwipc.registration.fine.RegistrationComputer_ICP_Point2Point,
                            analyzer_class=cwipc.registration.analyze.RegistrationAnalyzer)
            pc = None

            if new_pc:
                new_pc = None
                if not self.dry_run:
                    self.cameraconfig.save()
                must_reload = True
        if self.cameraconfig.camera_count() > 1 and not self.args.nofine:
            while True:
                if must_reload:
                    self._reload_cameraconfig_to_capturer()
                    must_reload = False
                if self.args.guided:
                    self.args.rgb = False
                    print(f"\n===================================================================", file=sys.stderr)
                    print(f"===== Examine point cloud window.", file=sys.stderr)
                    print(f"===== Edit cameraconfig.json and change near, far (or threshold_min_distance and threshold_max_distance), height_min, height_max, radius_filter. ", file=sys.stderr)
                    print(f"===== At this point you want to include some floor, so set height_min to a small negative value.", file=sys.stderr)
                    print(f"===== Also look at depth_x_erosion and depth_y_erosion, to get rid of background colors that leak onto the subject", file=sys.stderr)
                    print(f"===== For Realsense, experiment with the filters and map_color_to_depth (-1, 0, 1)", file=sys.stderr)
                    print(f"===== Press c in point cloud window (or restart cwipc_register) to reload cameraconfig.json", file=sys.stderr)
                    print(f"===== Ensure you get a clean capture including the floor.", file=sys.stderr)
                    print(f"===== Have a human (could be you) stand at the origin and ensure they are fully captured. ", file=sys.stderr)
                    print(f"===== You can use 1234 or 0 to see only per-camera point clouds. ", file=sys.stderr)
                    print(f"===== Press w in the point cloud window to capture.", file=sys.stderr)
                    print(f"===== Alternatively, press t (for timelapse) and ensure you yourself are in position in 5 seconds.", file=sys.stderr)
                    print(f"===== Or, if you are happy with the registration, press q")
                    print(f"===================================================================\n", file=sys.stderr)

                self.prompt("Fine registration: capturing human-sized object")
                pc = self.capture()
                if self.args.guided:
                    print(f"===== The window will now close, the algorithms will run, and after that the windows will reopen.", file=sys.stderr)
                new_pc = self.fine_registration(pc)
                pc = None

                if new_pc:
                    new_pc = None
                    if not self.dry_run:
                        self.cameraconfig.save()
                    must_reload = True
                if not self.args.guided:
                    break
        else:
            if self.verbose:
                print(f"cwipc_register: skipping fine registration, not needed or skipped because of --nofine")
        
        return False
    
    def _reload_cameraconfig_from_file(self) -> None:
        assert self.capturer
        print(f"cwipc_register: reload camerconfig from {self.cameraconfig.filename}")
        self.cameraconfig.load(self.capturer.get_config())

    def _reload_cameraconfig_to_capturer(self) -> None:
        assert self.capturer
        if True:
            print(f"cwipc_register: reload {self.cameraconfig.filename} by closing and reopening capturer")
            self.capturer = None
            time.sleep(1)
            ok = self.open_capturer()
            assert ok
        else:
            print(f"cwipc_register: reload {self.cameraconfig.filename} to capturer")
            self.capturer.reload_config(self.cameraconfig.filename)

    def initialize_recording(self) -> bool:
        if os.path.exists(self.args.cameraconfig):
            return True
        allfiles = []
        is_kinect = False
        is_realsense = False
        for fn in os.listdir(self.args.recording):
            if fn.startswith("."):
                continue
            if fn.lower().endswith(".mkv"):
                allfiles.append(fn)
                is_kinect = True
            if fn.lower().endswith(".bag"):
                allfiles.append(fn)
                is_realsense = True
        if is_realsense and is_kinect:
            print(f"cwipc_register: Directory {self.args.recording} contains both .mkv and .bag files")
            return False
        if not is_realsense and not is_kinect:
            print(f"cwipc_register: Directory {self.args.recording} contains neither .mkv nor .bag files")
            return False
        if is_realsense:
            camtype = "realsense_playback"
            # Create the camera definitions
            camera = [
                dict(filename=fn, type=camtype)
                    for fn in allfiles
            ]
            cameraconfig = dict(
                version=4,
                type=camtype,
                system=dict(),
                hardware=dict(),
                processing=dict(),
                filtering=dict(),
                camera=camera
            )
        elif is_kinect:
            camtype = "kinect_playback"
            # Create the camera definitions
            camera = [
                dict(filename=fn, type=camtype)
                    for fn in allfiles
            ]
            cameraconfig = dict(
                version=3,
                type=camtype,
                system=dict(),
                postprocessing=dict(
                    depthfilterparameters=dict(

                    )
                ),
                skeleton=dict(),
                camera=camera
            )
        else:
            print(f"cwipc_register: Directory {self.args.recording} contains neither .mkv nor .bag files")
            return False

        json.dump(cameraconfig, open(self.args.cameraconfig, "w"), indent=4)
        if self.verbose:
            print(f"cwipc_register: Created {self.args.cameraconfig}")
        return True
    
    def open_capturer(self) -> bool:
        assert self.capturerFactory
        # xxxjack this will eventually fail for generic capturer
        # Step one: Try to open with an existing cameraconfig.
        self.capturer = None
        try:
            self.capturer = self.capturerFactory()
        except cwipc.CwipcError:
            return False
        self.capturer.request_metadata("rgb")
        self.capturer.request_metadata("depth")
        self.capturer.request_metadata("timestamps")
        ok = self.capturer.start()
        return ok
    
    def create_cameraconfig(self) -> None:
        """Attempt to open a capturer without cameraconfig file. Then save the empty cameraconfig."""
        tmpFactory = activesource_factory_from_args(self.args, autoConfig=True)
        tmpCapturer = tmpFactory()
        tmpCapturer.start()
        new_config = tmpCapturer.get_config()
        if not new_config or new_config == b"{}":
            raise RuntimeError("Cannot get cameraconfig")
        self.cameraconfig.load(new_config)
        tmpCapturer = None

        if not self.dry_run:
            self.cameraconfig.save()
            if self.verbose:
                print(f"cwipc_register: Saved {self.args.cameraconfig}")
        else:
            print(f"cwipc_register: Not saving {self.args.cameraconfig}, --dry-run specified.")
    
    def create_nograb_cameraconfig(self) -> None:
        self.cameraconfig.load(open("cameraconfig.json", "rb").read())

    def capture(self) -> cwipc.cwipc_pointcloud_wrapper:
        if self.args.nograb:
            pc = cwipc.cwipc_read(self.args.nograb, 0)
            return pc
        assert self.capturer
        if self.args.skip:
            if self.verbose:
                print(f"cwipc_register: skipping {self.args.skip} captures")
            for i in range(self.args.skip):
                ok = self.capturer.available(True)
                if ok:
                    pc = self.capturer.get()
                    pc = None
        if self.args.interactive:
            return self.interactive_capture()
        ok = self.capturer.available(True)
        assert ok
        pc = self.capturer.get()
        assert pc
        assert pc.count() > 0
        return cast(cwipc.cwipc_pointcloud_wrapper, pc)
    
    def interactive_capture(self) -> cwipc.cwipc_pointcloud_wrapper:
        visualizer = RegistrationVisualizer(self.args.verbose, nodrop=True, args=self.args, title="cwipc_register")
        visualizer.reload_cameraconfig_callback = self._reload_cameraconfig_from_file
        assert self.capturer
        sourceServer = SourceServer(self.capturer, visualizer, self.args, owns_grabber=False)
        sourceThread = threading.Thread(target=sourceServer.run, args=(), name="cwipc_view.SourceServer")
        visualizer.set_producer(sourceThread)
        visualizer.set_source(cast(cwipc_source_abstract, self.capturer))
        sourceThread.start()
        visualizer.run()
        captured_pc = visualizer.captured_pc
        sourceServer.stop()
        sourceThread.join()
        del visualizer
        del sourceServer
        del sourceThread
        if not captured_pc:
            print("cwipc_register: no capture selected in interactive mode. Exiting.")
            sys.exit(1)
        return captured_pc

    def coarse_registration(self, pc : cwipc_pointcloud_wrapper) -> Optional[cwipc_pointcloud_wrapper]:
        if True or self.verbose:
            print(f"cwipc_register: Use coarse alignment class {self.coarse_aligner_class.__name__}")
        assert self.capturer
        aligner = self.coarse_aligner_class()
        aligner.verbose = self.verbose > 2
        aligner.debug = self.verbose > 3
        aligner.set_tiled_pointcloud(pc)
        serial_dict = self.cameraconfig.get_serial_dict()
        aligner.set_serial_dict(serial_dict)
        aligner.set_grabber(self.capturer)
        start_time = time.time()
        ok = aligner.run()
        stop_time = time.time()
        if self.verbose:
            print(f"cwipc_register: coarse aligner ran for {stop_time-start_time:.3f} seconds")
        if not ok:
            print("cwipc_register: Could not do coarse registration")
            return None
        # Get the resulting transformations, and store them in cameraconfig.
        transformations = aligner.get_result_transformations()
        for cam_num in range(len(transformations)):
            matrix = transformations[cam_num]
            t = self.cameraconfig.get_transform(cam_num)
            t.set_matrix(matrix)
        # Get the newly aligned pointcloud to test for alignment, and return it
        new_pc = aligner.get_result_pointcloud_full()
        if self.check_coarse_alignment:
            correspondence = self.check_alignment(new_pc, "after coarse registration")
            self.cameraconfig["correspondence"] = correspondence
        return new_pc

    def ask_aligner_class(self, default: type[MulticamAlignmentAlgorithm]) -> Optional[type[MulticamAlignmentAlgorithm]]:
        defaultName = default.__name__
        allNames = ' / '.join(["None"]+[klass.__name__ for klass in cwipc.registration.multicamera.ALL_MULTICAMERA_ALGORITHMS])
        klassName = self.ask(f"Multicamera alignment algorithm to use ({allNames})", defaultName)
        if klassName == "None":
            return None
        klass = getattr(cwipc.registration.multicamera, klassName)
        return klass

    def fine_registration(self, pc : cwipc_pointcloud_wrapper, multicam_aligner_class=None, aligner_class=None, analyzer_class=None) -> Optional[cwipc_pointcloud_wrapper]:
        if analyzer_class is None:
            analyzer_class = self.analyzer_class
        fixed_multicam_aligner = multicam_aligner_class != None
        if not fixed_multicam_aligner:
            multicam_aligner_class = self.multicamera_aligner_class
            if self.args.guided:
                multicam_aligner_class = self.ask_aligner_class(multicam_aligner_class)
                if multicam_aligner_class == None:
                    print(f"cwpic_register: skipping registration")
                    return None
        if not self.verbose:
            # We only do this if we are not running verbosely (because then the alignment classes will print this info)
            self.check_alignment(pc, f"before {multicam_aligner_class.__name__} registration", analyzer_class)
        if True or self.verbose:
            print(f"cwipc_register: Fine multicamera alignment using aligner class {multicam_aligner_class.__name__}")
        multicam = multicam_aligner_class()
        multicam.verbose = self.verbose > 2
        multicam.debug = self.verbose > 3
        if not fixed_multicam_aligner and self.args.correspondence:
            multicam.set_max_correspondence(self.args.correspondence)
            if True or self.verbose:
                print(f"cwipc_register: override max correspondence to {self.args.correspondence}")
        if aligner_class == None:
            aligner_class = self.alignment_class
        if aligner_class:
            multicam.set_aligner_class(aligner_class)
        if True or self.verbose:
            assert multicam.aligner_class
            print(f"cwipc_register: Use fine aligner class {multicam.aligner_class.__name__}")
        multicam.set_analyzer_class(analyzer_class)

        multicam.set_tiled_pointcloud(pc)
        for cam_index in range(self.cameraconfig.camera_count()):
            multicam.set_original_transform(cam_index, self.cameraconfig.get_transform(cam_index).get_matrix())
        start_time = time.time()
        ok = multicam.run()
        stop_time = time.time()
        if self.verbose:
            print(f"cwipc_register: {multicam_aligner_class.__name__} ran for {stop_time-start_time:.3f} seconds")
        if not ok:
            print(f"cwipc_register: Could not do {multicam_aligner_class.__name__} registration")
            sys.exit(1)
        # Get the newly aligned pointcloud to test for alignment, and return it
        new_pc = multicam.get_result_pointcloud_full()
        correspondence = self.check_alignment(new_pc, f"after {multicam_aligner_class.__name__} registration", analyzer_class=analyzer_class)
        #
        # For --guided, give the user the option to accept, reject or inspect the results.
        #
        if self.args.guided:
            while True:
                answer = self.ask("Accept (yes/no/show/plot)", "no default")
                if answer == "yes":
                    break
                if answer == "no":
                    return None
                if answer == "plot":
                    pass
                if answer == "show":
                    show_pc = multicam.get_result_pointcloud_full()
                    show_pointcloud("Result after alignment", show_pc)
                if answer == "plot":
                    results = multicam.results  # type: ignore
                    plotter = cwipc.registration.plot.Plotter(title="Results after alignment")
                    plotter.set_results(results)
                    plotter.plot(show=True)
        # Get the resulting transformations, and store them in cameraconfig.
        transformations = multicam.get_result_transformations()
        for cam_num in range(len(transformations)):
            matrix = transformations[cam_num]
            t = self.cameraconfig.get_transform(cam_num)
            t.set_matrix(matrix)
        self.cameraconfig["correspondence"] = correspondence
        return new_pc

    def check_alignment(self, pc : cwipc_pointcloud_wrapper, label : str, analyzer_class = None) -> float:
        if analyzer_class is None:
            assert self.analyzer_class
            analyzer_class = self.analyzer_class
        if True or self.verbose:
            print(f"cwipc_register: Use analyzer class {analyzer_class.__name__}")
        allResults : List[AnalysisResults] = []
        start_time = time.time()
        for cam_index in range(self.cameraconfig.camera_count()):
            targettile = 1 << cam_index
            othertile = 255 - targettile
            analyzer = analyzer_class()
            analyzer.set_source_pointcloud(pc, targettile)
            analyzer.set_reference_pointcloud(pc, othertile)
            analyzer.set_correspondence_measure('mode')
            analyzer.run()
            results = analyzer.get_results()
            allResults.append(results)
        stop_time = time.time()
        print(f"cwipc_register: analyzer ran for {stop_time-start_time:.3f} seconds")

        if self.show_plot:
            plotter = cwipc.registration.plot.Plotter(title="Results")
            plotter.set_results(allResults)
            plotter.plot(show=True)
        correspondences = []
        for result in allResults:
            correspondences.append(result.minCorrespondence)
        return max(correspondences)      

    def _capture_some_frames(self, capturer : cwipc_activesource_abstract) -> None:
        # Capture some frames (so we know get_config() will have obtained all parameters).
        # Get initial cameraconfig and save it
        gotframes = 0
        while gotframes < 1:
            ok = capturer.available(True)
            if ok:
                pc = capturer.get()
                if pc != None:
                    if self.verbose:
                        print(f"cwipc_register: dropped pc with {pc.count()} points")
                    pc= None

                    gotframes += 1
        if self.verbose:
            print(f"cwipc_register: captured {gotframes} frames to ensure config stability")

if __name__ == '__main__':
    main()
    
