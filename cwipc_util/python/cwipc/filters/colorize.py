import time
from typing import Union, List, Tuple, Optional, Dict, Sequence, Any
from .abstract import cwipc_abstract_filter
from ..util import cwipc_pointcloud_wrapper, cwipc_point_array, cwipc_from_points

ColorTuple = Tuple[float, float, float]

class ColorMap:
    def __init__(self, initializer : Optional[Dict[int, ColorTuple]] = None):
        self._map : List[Optional[ColorTuple]] = [None]*256
        if initializer:
            for k, v in initializer.items():
                self._map[k] = v

    def add_mapping(self, tilenum: int, color : ColorTuple):
        self._map[tilenum] = color

    def map(self, tilenum : int):
        return self._map[tilenum]
    
_colorMapTiles = ColorMap()
_colorMapTiles.add_mapping(1, (1, 0, 0))
_colorMapTiles.add_mapping(2, (0, 1, 0))
_colorMapTiles.add_mapping(4, (0, 0, 1))
_colorMapTiles.add_mapping(8, (0.5, 0.5, 0))
_colorMapTiles.add_mapping(16, (0, 0.5, 0.5))
_colorMapTiles.add_mapping(32, (0.5, 0, 0.5))
_colorMapTiles.add_mapping(64, (0.2, 0.2, 0.2))
_colorMapTiles.add_mapping(128, (0.7, 0.7, 0.7))

_colorForBitCount=[
    (0.2,0.2,0.2),
    (1,1,1),
    (1,0,0),
    (0,1,0),
    (0,0,1),
    (0.5, 0.5, 0),
    (0, 0.5, 0.5),
    (0.5, 0, 0.5),
    (0, 0, 0)
]

def _countBitsToColor(src : int) -> ColorTuple:
    """Return number of bits set in argument and return corresponding color"""
    bitCount = bin(src).count('1')
    return _colorForBitCount[bitCount]

_colorMapContributingCameras = ColorMap()
for i in range(255):
    _colorMapContributingCameras.add_mapping(i, _countBitsToColor(i))

_namedColorMaps=dict(
    camera=_colorMapTiles,
    contributions=_colorMapContributingCameras
)

class ColorizeFilter(cwipc_abstract_filter):
    """
    colorize - Change the color of points in a pointcloud, based on the tile number or mask.
        Arguments:
            weight: 1.0 means completely replace original color, 0.0 changes nothing
            colormap: a 3-float-tuple for a uniform color, otherwise a colorize.ColorMap or the name of one:
                      camera: Each tile number gets a different color
                      contributions: the color depends on the number of bits set in the tilenumber
    """
    filtername = "colorize"

    def __init__(self, weight : float, colormap : Any):
        if colormap in _namedColorMaps:
            self.colorMap = _namedColorMaps[colormap]
        elif type(colormap) == type(()):
            self.colorMap = ColorMap()
            for i in range(256):
                self.colorMap.add_mapping(i, colormap)  # type: ignore
        else:
            # We hope for the best...
            self.colorMap = ColorMap(colormap)

        self.count = 0
        self.weight = weight
        self.times = []
        self.original_pointcounts = []
        self.keep_source = False
        
    def set_keep_source(self) -> None:
        """Set the filter to keep the source point cloud instead of freeing it after processing.
        If the filter returns the same point cloud as it received as an argument it will never be freed."""
        self.keep_source = True
        
    def filter(self, pc : cwipc_pointcloud_wrapper) -> cwipc_pointcloud_wrapper:
        self.count += 1
        t1_d = time.time()
        self.original_pointcounts.append(pc.count())
        mapped_pc = self._mapcolor(pc)
        pc = mapped_pc
        t2_d = time.time()
        self.times.append(t2_d-t1_d)
        return pc
    
    def _mapcolor(self, pc : cwipc_pointcloud_wrapper) -> cwipc_pointcloud_wrapper:
        """xxxjack this method should be rewritten using numpy"""
        points = pc.get_points()
        ts = pc.timestamp()
        cellsize = pc.cellsize()
        for i in range(len(points)):
            p = points[i]
            newColor = self.colorMap.map(p.tile)
            if newColor != None:
                oldColor = (p.r/255.0, p.g/255.0, p.b/255.0)
                new_r = newColor[0] * self.weight + oldColor[0] * (1-self.weight)
                new_g = newColor[1] * self.weight + oldColor[1] * (1-self.weight)
                new_b = newColor[2] * self.weight + oldColor[2] * (1-self.weight)
                points[i].r = int(new_r*255)
                points[i].g = int(new_g*255)
                points[i].b = int(new_b*255)
        
        new_pc = cwipc_from_points(points, ts)
        new_pc._set_cellsize(cellsize)
        return new_pc
    
    def statistics(self) -> None:
        if self.times:
            self.print1stat('duration', self.times)
        if self.original_pointcounts:
            self.print1stat('original_pointcount', self.original_pointcounts, True)
      
    def print1stat(self, name : str, values : Union[List[int], List[float]], isInt : bool=False) -> None:
        count = len(values)
        if count == 0:
            print(f'{self.filtername}: {name}: count=0')
            return
        minValue = min(values)
        maxValue = max(values)
        avgValue = sum(values) / count
        if isInt:
            fmtstring = '{}: {}: count={}, average={:.3f}, min={:d}, max={:d}'
        else:
            fmtstring = '{}: {}: count={}, average={:.3f}, min={:.3f}, max={:.3f}'
        print(fmtstring.format(self.filtername, name, count, avgValue, minValue, maxValue))

CustomFilter=ColorizeFilter
