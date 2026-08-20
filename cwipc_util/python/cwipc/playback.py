import cwipc
import time
import os
import json
from typing import List, Optional, Any, Iterable, Union
from .abstract import cwipc_activesource_abstract

class _Filesource(cwipc_activesource_abstract):
    def __init__(self, filenames : Union[str, List[str]], tileInfo : Optional[List[dict[Any,Any]]]=None, loop : bool=False, fps : Optional[int]=None, retimestamp : bool=False):
        if not tileInfo:
            tileInfo = [{
                "cameraName" : "None",
                "cameraMask" : 0,
                "normal" : {"x":0, "y":0, "z":0}
            }]
        self.tileInfo = tileInfo
        self.filenames = list(filenames)
        self.loop = loop
        self.single_file_mode = self.loop and len(self.filenames) == 1
        self.single_file_mode_pc : Optional[cwipc.cwipc_pointcloud_wrapper] = None
        self.delta_t = 0
        self.retimestamp = retimestamp
        self.earliest_return = time.time()
        if fps:
            self.delta_t = 1/fps
        
    def free(self) -> None:
        self.stop()
        
    def reload_config(self, config: str | bytes | None) -> None:
        return

    def reload_config_fast(self, config : str | bytes) -> None:
        return
    
    def get_config(self) -> Optional[str]:
        return None
    
    def start(self) -> bool:
        return True
    
    def stop(self) -> None:
        self.filenames = []
        if self.single_file_mode_pc:
            self.single_file_mode_pc = None
            
    def seek(self, timestamp : int) -> bool:
        return False
    
    def eof(self) -> bool:
        if self.single_file_mode_pc != None:
            return False
        return not self.filenames
    
    def available(self, wait : bool=False) -> bool:
        if self.single_file_mode_pc:
            return True
        return not not self.filenames
        
    def get(self) -> Optional[cwipc.cwipc_pointcloud_wrapper]:
        if not self.filenames:
            if self.single_file_mode_pc:
                return self.single_file_mode_pc.clone()
            return None
        fn = self.filenames.pop(0)
        if self.loop: self.filenames.append(fn)
        rv = self._get(fn)
        if self.single_file_mode and rv:
            self.single_file_mode_pc = rv.clone()
        if time.time() < self.earliest_return:
            time.sleep(self.earliest_return - time.time())
        self.earliest_return = time.time() + self.delta_t
        if self.retimestamp:
            timestamp = int(time.time()*1000)
            rv._set_timestamp(timestamp) # type: ignore
        return rv
        
    def _get(self, fn : str) -> Optional[cwipc.cwipc_pointcloud_wrapper]:
        numbers = ''.join(x for x in fn if x.isdigit())
        if numbers == '':
            numbers = 0
        timestamp = int(numbers)
        rv = cwipc.cwipc_read(fn, timestamp)
        return rv     
        
    def maxtile(self) -> int:
        return len(self.tileInfo)
        
    def get_tileinfo_dict(self, i : int) -> dict[Any,Any]:
        return self.tileInfo[i]
    
    def request_metadata(self, name: str) -> None:
        raise cwipc.CwipcError("Not supported for _Filesource")
    
    def is_metadata_requested(self, name: str) -> bool:
        return False
    
    def auxiliary_operation(self, op: str, inbuf: bytes, outbuf: bytearray) -> bool:
        return False   
    
    def statistics(self) -> None:
        pass
        
class _DumpFilesource(_Filesource):
    def _get(self, fn : str) -> Optional[cwipc.cwipc_pointcloud_wrapper]:
        rv = cwipc.cwipc_read_debugdump(fn)
        return rv
        
class _CompressedFilesource(_Filesource):
    def __init__(self, *args : Any, **kwargs : Any):
        _Filesource.__init__(self, *args, **kwargs)
        from .codec import cwipc_new_decoder
        self.decoder = cwipc_new_decoder()
        
    def _get(self, fn : str) -> Optional[cwipc.cwipc_pointcloud_wrapper]:
        with open(fn, 'rb') as fp:
            data : bytes = fp.read()
        self.decoder.feed(data)
        return self.decoder.get()
    
def cwipc_playback(dir_or_files : Union[str, List[str]], ext : str='.ply', loop : bool=False, fps : Optional[int]=None, inpoint : Optional[int]=None, outpoint : Optional[int]=None, retimestamp : bool=False) -> cwipc_activesource_abstract:
    """Return cwipc_source-like object that reads .ply or .cwipcdump files from a directory or list of filenames"""
    tileInfo = None
    filenames : Iterable[str]
    if isinstance(dir_or_files, str):
        filenames = filter(lambda fn : fn.lower().endswith(ext), os.listdir(dir_or_files))
        if not filenames:
            raise cwipc.CwipcError(f"No {ext} files in {dir_or_files}")
        filenames = sorted(filenames)
        filenames = list(filenames)
                
        #remove files based on inpoint and/or outpoint
        if inpoint:
            filenames = [fn for fn in filenames if not int(''.join(x for x in fn if x.isdigit())) < inpoint]
            
        if outpoint:
            filenames = [fn for fn in filenames if not int(''.join(x for x in fn if x.isdigit())) > outpoint]
        
        tileInfo_fn = os.path.join(dir_or_files, "tileconfig.json")
        if os.path.exists(tileInfo_fn):
            with open(tileInfo_fn) as fp:
                ti = json.load(fp)
                tileInfo = ti.get('tileInfo')

        filenames = map(lambda fn: os.path.join(dir_or_files, fn), filenames) # type: ignore
        filenames = list(filenames)
        dir_or_files = filenames
    if ext == '.ply':
        return _Filesource(dir_or_files, tileInfo=tileInfo, loop=loop, fps=fps, retimestamp=retimestamp)
    elif ext == '.cwipcdump':
        return _DumpFilesource(dir_or_files, tileInfo=tileInfo, loop=loop, fps=fps, retimestamp=retimestamp)
    elif ext == '.cwicpc':
        return _CompressedFilesource(dir_or_files, tileInfo=tileInfo, loop=loop, fps=fps, retimestamp=retimestamp)
    else:
        raise cwipc.CwipcError(f'Unknown playback filetype {ext}')
        
