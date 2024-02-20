import sys
import time
from typing import Optional
import cwipc

MAX_TIME_PER_STEP=5
MAX_ITERATIONS_PER_STEP=100

class TimingTest:

    def __init__(self, testfile : str):
        self.pc : Optional[cwipc.cwipc_wrapper] = None
        self.time_test_readfile(testfile)

    def __del__(self):
        if self.pc:
            self.pc.free()
            self.pc = None

    def _time(self) -> float:
        return time.perf_counter()
    
    def time_test_readfile(self, filename : str) -> None:
        start_time = self._time()
        count = 0
        while True:
            if self.pc != None:
                self.pc.free()
                self.pc = None
            self.pc = cwipc.cwipc_read(filename, 0)
            count += 1
            assert self.pc
            duration = self._time() - start_time
            if duration >= MAX_TIME_PER_STEP:
                break
            if count >= MAX_ITERATIONS_PER_STEP:
                break
        assert self.pc
        print(f"time_test_readfile: {self.pc.count()} points")
        print(f"time_test_readfile: {duration / count:.6f} seconds")

    def time_test_none(self):
        start_time = self._time()
        count = 0
        assert self.pc
        while True:
            pass
            count += 1
            duration = self._time() - start_time
            if duration > MAX_TIME_PER_STEP:
                break
            if count >= MAX_ITERATIONS_PER_STEP:
                break
        assert self.pc
        print(f"time_test_none: {duration / count:.6f} seconds")

    def time_test_get_points(self):
        start_time = self._time()
        count = 0
        assert self.pc
        while True:
            points = None
            points = self.pc.get_points()
            assert len(points) == self.pc.count()
            count += 1
            duration = self._time() - start_time
            if duration > MAX_TIME_PER_STEP:
                break
            if count >= MAX_ITERATIONS_PER_STEP:
                break
        assert self.pc
        print(f"time_test_get_points: {duration / count:.6f} seconds")

    def time_test_get_bytes(self):
        start_time = self._time()
        count = 0
        assert self.pc
        while True:
            bytes = None
            bytes = self.pc.get_bytes()
            assert len(bytes) == self.pc.count()*16
            count += 1
            duration = self._time() - start_time
            if duration > MAX_TIME_PER_STEP:
                break
            if count >= MAX_ITERATIONS_PER_STEP:
                break
        assert self.pc
        print(f"time_test_get_bytes: {duration / count:.6f} seconds")

    def time_test_get_packet(self):
        start_time = self._time()
        count = 0
        assert self.pc
        while True:
            packet = None
            packet = self.pc.get_packet()
            assert len(packet) > self.pc.count()*16
            count += 1
            duration = self._time() - start_time
            if duration > MAX_TIME_PER_STEP:
                break
            if count >= MAX_ITERATIONS_PER_STEP:
                break
        assert self.pc
        print(f"time_test_get_packet: {duration / count:.6f} seconds")

    def time_test_get_points_roundtrip(self):
        start_time = self._time()
        count = 0
        assert self.pc
        while True:
            points = None
            points = self.pc.get_points()
            new_pc = cwipc.cwipc_from_points(points, 0)
            assert new_pc.count() == self.pc.count()
            count += 1
            duration = self._time() - start_time
            if duration > MAX_TIME_PER_STEP:
                break
            if count >= MAX_ITERATIONS_PER_STEP:
                break
        assert self.pc
        print(f"time_test_get_points_roundtrip: {duration / count:.6f} seconds")

    def time_test_get_packet_roundtrip(self):
        start_time = self._time()
        count = 0
        assert self.pc
        while True:
            packet = None
            packet = self.pc.get_packet()
            new_pc = cwipc.cwipc_from_packet(packet)
            assert new_pc.count() == self.pc.count()
            count += 1
            duration = self._time() - start_time
            if duration > MAX_TIME_PER_STEP:
                break
            if count >= MAX_ITERATIONS_PER_STEP:
                break
        assert self.pc
        print(f"time_test_get_packet_roundtrip: {duration / count:.6f} seconds")

    def run(self):
        self.time_test_none()
        self.time_test_get_bytes()
        self.time_test_get_packet()
        self.time_test_get_packet_roundtrip()
        self.time_test_get_points()
        self.time_test_get_points_roundtrip()

def main():
    t = TimingTest(sys.argv[1])
    t.run()

if __name__ == "__main__":
    main()
