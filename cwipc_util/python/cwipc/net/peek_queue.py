import queue
import time

Empty = queue.Empty
Full = queue.Full

class PeekQueue[T](queue.Queue[T]):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def dont_get(self, block : bool=True, timeout : float=None):
        '''Does everything Queue.get does except removing and returning the item.

        If optional args 'block' is true and 'timeout' is None (the default),
        block if necessary until an item is available. If 'timeout' is
        a non-negative number, it blocks at most 'timeout' seconds and raises
        the Empty exception if no item was available within that time.
        Otherwise ('block' is false), return an item if one is immediately
        available, else raise the Empty exception ('timeout' is ignored
        in that case).
        '''
        with self.not_empty:
            if not block:
                if not self._qsize():
                    raise Empty
            elif timeout is None:
                while not self._qsize():
                    self.not_empty.wait()
            elif timeout < 0:
                raise ValueError("'timeout' must be a non-negative number")
            else:
                endtime = time.monotonic() + timeout
                while not self._qsize():
                    remaining = endtime - time.monotonic()
                    if remaining <= 0.0:
                        raise Empty
                    self.not_empty.wait(remaining)
