"""
Defines the base class for Qt-based frontend windows.

Using Qt with Python presents an issue in which Python exceptions are swallowed.
The workaround to the problem is to use a custom exception handler as follows:
    1. Back up the reference to the default system exception hook
    2. Set a custom exception hook that simply forwards to the system exception hook
    3. Restore the exception hook after the Qt application is finished

"""
import pathlib
import sys

from PySide6.QtWidgets import QApplication, QMainWindow, QWidget
from PySide6.QtUiTools import QUiLoader
from typing import Optional


class MainWindow(QMainWindow):
    """
    MainWindow is the base class for all Qt-based frontends.

    Its main purpose is to set up the Qt application and hide away the workaround for the system exception hook problem.
    Note that as long as an instance of this class exists, the system hook is re-routed.
    """

    def __init__(self) -> None:
        """
        Constructor.
        """

        # Back up the reference to the default system exception hook
        self._default_exception_hook = sys.excepthook

        # Set a custom exception hook that simply forwards to the system exception hook
        sys.excepthook = self._custom_exception_hook

        # Create the singleton Qt application
        self._application: QApplication = QApplication(list())

        # Invoke the constructor of the QMainWindow base class which will then use the existing Qt application
        super().__init__()

        # Create an internal UI object
        self._ui_instance: Optional[QWidget] = None

    def __del__(self) -> None:
        """
        Destructor.
        """

        # Restore the exception hook after the Qt application is finished
        # todo: this check is required because something crazy happens to the sys module. that should be fixed!
        if sys:
            sys.excepthook = self._default_exception_hook

    def run(self) -> int:
        """
        Runs the main event loop of the application.

        :return: Returns the exit code of the application (should be passed to sys.exit).
        """

        return self._application.exec()

    """
    Internals.
    """

    def _custom_exception_hook(self, exception_type, value, traceback) -> None:
        """
        Custom exception hook.
        """

        # Print the error and traceback
        print(exception_type, value, traceback)
        # Call the normal exception hook after
        self._default_exception_hook(exception_type, value, traceback)
        # Exit the program
        sys.exit(1)
