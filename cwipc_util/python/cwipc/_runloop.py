"""
Wait for whatever happens first among the participants of a run (cwi-dis/cwipc#333).

A participant is a child process, a thread, or an object with is_alive(). Each one's
ending (normal or not) goes to one event queue; the runloop takes endings from it in the
order they happen, so nothing assumes in which order things end. Used by `cwipc parallel`
for child processes and by the scripts for their threads.

Terms (see the design in #333): a *participant* ends with an *ending*: who, a status
(0 is normal, anything else a failure) and a reason. The *grace period* is how long the
others get to end by themselves after the first ending, before they are stopped.
"""
import queue
import subprocess
import sys
import threading
import time
import traceback
from typing import Any, Callable, Dict, List, NamedTuple, Optional

__all__ = ["Ending", "Failure", "Runloop"]

# How often participants that can only be polled (is_alive()) are checked.
POLL_INTERVAL = 0.2


class Failure(Exception):
    """An expected failure (a refused connection, for example): a participant's thread that
    raises this ends with a failure and the message, without a traceback."""
    pass


class Ending(NamedTuple):
    who: str
    status: int     # 0: normal end; anything else: failure
    reason: str


class _Polled(NamedTuple):
    is_alive: Callable[[], bool]
    failure: Callable[[], Optional[str]]


class Runloop:
    """Collects the endings of the participants of a run, in the order they happen."""

    def __init__(self, name: str, verbose: bool = False):
        self.name = name
        self.verbose = verbose
        self._queue: "queue.Queue[Ending]" = queue.Queue()
        self._lock = threading.Lock()
        self._stoppers: Dict[str, Callable[[], None]] = {}
        self._polled: Dict[str, _Polled] = {}
        self._ended: List[str] = []
        self.endings: List[Ending] = []

    def _log(self, msg: str) -> None:
        if self.verbose:
            print(f"{self.name}: {msg}", file=sys.stderr, flush=True)

    def post(self, who: str, status: int, reason: str) -> None:
        """Report that participant `who` ended. Safe to call from any thread."""
        self._queue.put(Ending(who, status, reason))

    def add_process(self, who: str, proc: "subprocess.Popen[Any]") -> None:
        """A child process: its exit is an ending, with its exit status."""
        def wait() -> None:
            sts = proc.wait()
            self.post(who, sts, f"exited with status {sts}")
        def stop() -> None:
            if proc.poll() is None:
                proc.kill()
        self._add(who, stop)
        threading.Thread(target=wait, name=f"{self.name}.wait.{who}", daemon=True).start()

    def add_thread(self, who: str, target: Callable[[], Any], stop: Optional[Callable[[], None]] = None, daemon: bool = False) -> threading.Thread:
        """A thread running target(): returning is a normal ending, an exception a failure.
        Returns the thread, not yet started."""
        def run() -> None:
            try:
                target()
            except Failure as e:
                self.post(who, 1, str(e))
            except BaseException as e:
                traceback.print_exc()
                self.post(who, 1, f"{type(e).__name__}: {e}")
            else:
                self.post(who, 0, "ended")
        self._add(who, stop)
        return threading.Thread(target=run, name=f"{self.name}.{who}", daemon=daemon)

    def add_polled(self, who: str, is_alive: Callable[[], bool], stop: Optional[Callable[[], None]] = None, failure: Optional[Callable[[], Optional[str]]] = None) -> None:
        """A participant that can only be asked whether it is still alive (the threads in
        cwipc.net, for example). It has ended when it is no longer alive, or as soon as
        failure() returns a reason (a sink whose connection is gone may still look alive);
        in the latter case the ending is a failure."""
        self._add(who, stop)
        with self._lock:
            self._polled[who] = _Polled(is_alive, failure or (lambda: None))

    def add_participant(self, who: str, stop: Optional[Callable[[], None]] = None) -> None:
        """Any other participant (code running on the main thread, for example); the caller
        reports its ending with post()."""
        self._add(who, stop)

    def _add(self, who: str, stop: Optional[Callable[[], None]]) -> None:
        with self._lock:
            assert who not in self._stoppers, f"duplicate participant {who}"
            self._stoppers[who] = stop or (lambda: None)

    def running(self) -> List[str]:
        """The participants that have not ended (as far as the runloop knows)."""
        with self._lock:
            return [w for w in self._stoppers if w not in self._ended]

    def next_ending(self, timeout: Optional[float] = None) -> Optional[Ending]:
        """Wait for the next ending; None if there is none within timeout (seconds)."""
        deadline = None if timeout is None else time.monotonic() + timeout
        while True:
            self._check_polled()
            wait = POLL_INTERVAL
            if deadline is not None:
                wait = min(wait, deadline - time.monotonic())
                if wait <= 0:
                    return None
            try:
                ending = self._queue.get(timeout=wait)
            except queue.Empty:
                continue
            with self._lock:
                if ending.who in self._ended:
                    continue
                self._ended.append(ending.who)
            self.endings.append(ending)
            self._log(f"{ending.who}: {ending.reason}")
            return ending

    def _check_polled(self) -> None:
        with self._lock:
            polled = [(w, p) for w, p in self._polled.items() if w not in self._ended]
        for who, p in polled:
            reason = p.failure()
            if reason or not p.is_alive():
                with self._lock:
                    self._polled.pop(who, None)
                if reason:
                    self.post(who, 1, reason)
                else:
                    self.post(who, 0, "ended")

    def wait_for_rest(self, grace: float) -> List[str]:
        """Give the participants that are still running `grace` seconds to end by themselves.
        Returns those that didn't."""
        deadline = time.monotonic() + grace
        while self.running():
            if self.next_ending(timeout=max(0.0, deadline - time.monotonic())) is None:
                break
        return self.running()

    def stop(self, who: str) -> None:
        with self._lock:
            stop = self._stoppers[who]
        try:
            stop()
        except Exception:
            traceback.print_exc()

    def stop_all(self) -> None:
        for who in self.running():
            self.stop(who)

    def first_failure(self) -> Optional[Ending]:
        for e in self.endings:
            if e.status != 0:
                return e
        return None
