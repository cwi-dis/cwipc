from abc import ABC, abstractmethod
from typing import Union, Callable, List, Any
import threading
from ..abstract import *
from ..util import cwipc_pointcloud_wrapper

# 4CC handling doesn't really belong here, but it's convenient.
vrt_fourcc_type = Union[int, bytes, str]
cwipc_quality_description = dict[str, Any]

def VRT_4CC(code : vrt_fourcc_type) -> int:
    """Convert anything reasonable (bytes, string, int) to 4cc integer"""
    if isinstance(code, int):
        return code
    if not isinstance(code, bytes):
        assert isinstance(code, str)
        code = code.encode('ascii')
    assert len(code) == 4
    rv = (code[0]<<24) | (code[1]<<16) | (code[2]<<8) | (code[3])
    return rv

class cwipc_rawsource_abstract(ABC):
    """A source that produces a stream of raw data blocks (as bytes).

    The blocks are intended to be complete logical units (frames, pointclouds, etc).

    Generally the constructor will have the parameters that tell the source where to
    obtain data from. When start() is called the actual reception begins.

    An example would be a network protocol receiver. An example use case would
    be to feed these blocks to a point cloud decoder.
    """

    @abstractmethod
    def set_fourcc(self, fourcc : vrt_fourcc_type) -> None:
        """Set the expected 4CC for this stream. """
        ...

    @abstractmethod
    def get(self) -> Optional[bytes]:
        """Return the next data block"""
        ...

    @abstractmethod
    def available(self, wait : bool=False) -> bool:
        """Return true if a data block is available.

        wait=true signals that the caller is happy to wait a short while for a block to become available.
        """
        ...

    @abstractmethod
    def eof(self) -> bool:
        """Return true if no more data blocks will ever become available."""
        ...

    @abstractmethod
    def statistics(self) -> None:
        """Print statistics."""
        ...

class cwipc_activerawsource_abstract(cwipc_rawsource_abstract):
    """An active source of raw data blocks.
    Subclass of cwipc_rawsource_abstract that adds start and stop methods.
    """
    @abstractmethod
    def start(self) -> bool:
        """Start the source. This will start the receiver or reader."""
        ...

    @abstractmethod
    def stop(self) -> None:
        """Stop the source."""
        ...

# We don't know what describes a stream (and we don't care). But we do know that the
# object used to describe all tiles, all qualities is first indexed by tile number, then by
# quality index.
cwipc_multistream_description = List[List[Any]]

class cwipc_activerawmultisource_abstract(ABC):
    """A source that produces multiple streams of raw data blocks (as bytes).

    It is a container for multiple cwipc_rawsource_abstract.

    An example would be a network protocol receiver for tiled streams.
    """

    @abstractmethod
    def start(self) -> bool:
        ...

    @abstractmethod
    def stop(self) -> None:
        ...

    # xxxjack do we want seek()? eof()?

    @abstractmethod
    def get_tile_count(self) -> int:
        ...

    @abstractmethod
    def get_description(self) -> cwipc_multistream_description:
        """Return a description of all streams (tiles and qualities)"""
        ...
        
    @abstractmethod
    def get_tile_source(self, tileIdx : int) -> cwipc_rawsource_abstract:
        ...

    @abstractmethod
    def select_tile_quality(self, tileIdx : int, qualityIdx : int) -> None:
        ...


cwipc_producer_abstract = threading.Thread

class cwipc_rawsink_abstract(ABC):
    """A sink that consumes a stream of raw data blocks (as bytes).

    The blocks are intended to be complete logical units (frames, pointclouds, etc).

    Generally the constructor will have the parameters that tell the sink where to
    send data to. Then the various set...() methods are called to set stream parameters.
    When start() is called the actual transmission begins.

    An example would be a network protocol sender. An example use case would
    be to feed this object compressed point clouds.
    """
    @abstractmethod
    def start(self) -> None:
        """Start the sink. This will start the transmitter."""
        ...

    @abstractmethod
    def stop(self) -> None:
        """Stop the sink. Will close network connections, etc."""
        ...

    @abstractmethod
    def set_producer(self, producer : cwipc_producer_abstract) -> None:
        """The rawsink will call producer.is_alive() to determine when it should stop transmitting."""
        ...

    @abstractmethod
    def set_fourcc(self, fourcc : vrt_fourcc_type) -> None:
        """Set the 4CC for this stream. This signals the data type to the receiver on the other end."""
        ...
    
    @abstractmethod
    def add_stream(self, tilenum : Optional[int] = None, tiledesc : Optional[cwipc_tileinfo_dict] = None, qualitydesc : Optional[cwipc_quality_description] = None) -> int:
        """Create a new stream. Optionally specify tile number and/or tile description and/or quality description.
        Returns stream number.
        """
        ...

    @abstractmethod
    def feed(self, buffer : Union[bytes, bytearray], stream_index : Optional[int]=None) -> bool:
        """Feed a data block to the transmitter. For some implementations it is possible to specify the stream index."""
        ...

    @abstractmethod
    def statistics(self) -> None:
        """Print statistics."""
        ...


class cwipc_sink_abstract(ABC):
    """A sink for pointclouds. They could be shown on-screen, transmitted over the network, etc.
    The API is similar to cwipc_sink but subtly different: the intention is that this class is implemented
    in Python, and most likely in a multi-threaded way with queues to ensure producer and sink do not run
    in lock-step.
    """
    
    @abstractmethod
    def start(self) -> None:
        """Start the sink (if needed). This will start the sender."""
        ...

    @abstractmethod
    def stop(self) -> None:
        """Stop the sink."""
        ...

    @abstractmethod
    def set_producer(self, producer : cwipc_producer_abstract) -> None:
        """The rawsink will call producer.is_alive() to determine when it should stop transmitting."""
        ...

    @abstractmethod
    def feed(self, pc : cwipc_pointcloud_wrapper) -> None:
        ...
    @abstractmethod
    def statistics(self) -> None:
        """Print statistics."""
        ...


# Type declarations for the factories of the various sources
cwipc_source_factory_abstract = Callable[[], cwipc_source_abstract]
cwipc_activesource_factory_abstract = Callable[[], cwipc_activesource_abstract]
cwipc_activerawsource_factory_abstract = Callable[[], cwipc_activerawsource_abstract]
cwipc_activedecoder_factory_abstract = Callable[[cwipc_activerawsource_abstract], cwipc_activesource_abstract]