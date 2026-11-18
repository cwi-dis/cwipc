Installation
=============

The simplest way to install **cwipc** is to use one of the pre‑built
installers.  Installers are provided for macOS, Windows and Ubuntu and bundle
all of the runtime libraries, command‑line tools and language bindings.

.. note::
   After installing, running ``cwipc view --synthetic`` should open a window
   showing a synthetic point cloud stream (a rotating paraboloid with blinking eyes).
   You can also run ``cwipc_check`` to verify the installation and dependencies, it will
   print any warnings or errors it finds. It checks whether cwipc works *from the
   terminal you run it in*; if not, it tells you whether the installation itself is
   broken (``cwipc_check install`` may fix that) or whether your environment leads
   elsewhere: ``bin`` not on ``PATH`` (open a new terminal after installing), or
   another cwipc installation found first. A camera module whose SDK isn't installed (for
   example ``librealsense2``, which the Debian package only recommends) is reported as not
   available; that is not an error.
   
macOS
-----

Use Homebrew::

    brew install cwi-dis/cwipc/cwipc

Use the full name ``cwi-dis/cwipc/cwipc``: since Homebrew 6, formulae from
third-party taps are ignored unless they are trusted, and installing by the full
name taps ``cwi-dis/cwipc`` and trusts only the ``cwipc`` formula (see
`Tap Trust <https://docs.brew.sh/Tap-Trust>`_). If you prefer the short name
``brew install cwipc``, first run ``brew trust --formula cwi-dis/cwipc/cwipc``.


Windows
-------

Download the ``.exe`` installer from the `releases page
<https://github.com/cwi-dis/cwipc/releases/latest>`_ and execute it.  If the
installer refuses to run, install the **Microsoft VC++ Redistributable
(x64)** first.

The installer uses a Python 3.12 that is already on the machine. If there is none,
it installs one of its own inside the cwipc installation, with the Python install
manager (which it first downloads from python.org if needed). A Python that was
installed for one user only works for that user alone: if cwipc was set up with
one, ``cwipc_check`` tells other users so (`#312
<https://github.com/cwi-dis/cwipc/issues/312>`_).

After installation, use the Start menu entries to run **Check cwipc
installation** and optionally **Attempt to fix cwipc installation**. 

The top-level readme file has some more Windows Installation troubleshooting tips.

Ubuntu 24.04
------------

Install the Debian package::

    sudo apt install ./cwipc-<version>.deb

This also creates the Python venv in ``/usr/libexec/cwipc/venv`` (downloading
the Python dependencies from PyPI) and puts the ``cwipc`` commands in
``/usr/bin``. If that part fails, for example without network access, the
native libraries are still installed; run ``sudo cwipc_check install`` later.

RealSense support needs ``librealsense2`` from the RealSense apt repository,
which is not in Ubuntu. To add the repository and install it, together with the
other runtime dependencies::

    /usr/libexec/cwipc/scripts/install-deps-ubuntu2404.sh

The package also installs the udev rules that let non-root users use Orbbec
cameras over USB (``/usr/lib/udev/rules.d/99-obsensor-libusb.rules``, from the
OrbbecSDK). Cameras over Ethernet don't need them. When cwipc runs in a container,
the rules must be on the *host*, which owns the USB devices: copy that file to the
host's ``/etc/udev/rules.d/`` and run ``sudo udevadm control --reload-rules`` and
``sudo udevadm trigger`` there. For an unpacked ``-built-`` archive or a source
tree, ``install-deps-ubuntu2404.sh`` installs the rules into ``/etc/udev/rules.d``.

Android
-------

The Android build is API‑only and used primarily from Unity on Quest devices.

.. _own-python:

Using cwipc from your own Python
--------------------------------

Every cwipc installation has its own Python, with the cwipc modules: the ``cwipc``
command uses it, and ``cwipc python`` runs it. To use cwipc from *your own* Python
instead (a virtual environment of your project, a notebook kernel, your IDE's
interpreter), install the cwipc modules there with ``cwipc pipinstall``::

    source myproject/venv/bin/activate      # or: & myproject\venv\Scripts\Activate.ps1
    cwipc pipinstall

This installs the modules of the cwipc that the ``cwipc`` command belongs to into the
``python`` on your ``PATH`` (or the Python given with ``--python PYTHON``), with their
dependencies (numpy, open3d, ...), and *binds* that Python to that cwipc: a file
``cwipc.pth`` in its ``site-packages`` sets ``CWIPC_LIBRARY_DIR`` to the directory with
that cwipc's native libraries, every time that Python starts. That Python must be at least
``PYTHON_VERSION_MIN`` (``scripts/versions.env``; 3.12 for this release). cwipc itself is
built and tested with ``PYTHON_VERSION`` (also 3.12); newer versions work as long as all
dependencies have them, which pip checks: open3d supports up to Python 3.14, except on Intel
Macs, where it stops at 3.12.

- It only installs into a virtual environment (a venv or a conda environment), so a
  Python used for other things isn't bound to cwipc by accident. ``--force`` installs
  into any Python, also one older than cwipc needs; a Python marked as externally managed (Homebrew's, Ubuntu's ``/usr/bin/python3``) still refuses.
- To bind it to another cwipc (a newer installation, or a build of your own), run
  ``cwipc pipinstall`` of that cwipc.
- ``cwipc pipinstall --uninstall`` removes the cwipc modules and the binding again.
- In a source tree, ``cwipc pipinstall --editable`` installs the modules editable, so
  changes to the Python sources take effect without reinstalling.
- Setting ``CWIPC_LIBRARY_DIR`` yourself overrides the binding.

If the Python modules and the native libraries they load come from different cwipc
installations or builds, importing cwipc warns about it, naming both versions and the
library it loaded. In a source tree that also happens after a CMake configure without a
build.

Scripted installs and CI
------------------------

For an install without an installer, for example on a lab machine, in a Docker
image or in the CI of another project, use ``get-cwipc.sh`` (bash; on Windows
Git Bash). It gets cwipc onto the machine and sets it up::

    curl -sSfL -o get-cwipc.sh https://raw.githubusercontent.com/cwi-dis/cwipc/<tag or commit>/scripts/get-cwipc.sh
    bash get-cwipc.sh v8.1.0 /opt/cwipc
    source /opt/cwipc/libexec/cwipc/activate

- On Linux and Windows it downloads the ``-built-`` archive of that release for
  this machine and unpacks it into the given directory. ``--archive FILE`` uses a
  local archive instead.
- On macOS it builds cwipc through the Homebrew formula (``--method brew``, the
  default; the directory argument is then ignored). Releases from 8.3.0 on have no
  macOS ``-built-`` archives: they only worked as long as Homebrew still had the
  exact library versions they were built against (cwi-dis/cwipc#356).
  ``--method archive --archive FILE`` still works with an archive built on the same
  machine, or earlier in the same CI run. ``master`` as the version builds the current master branch
  (``brew install --HEAD``).
- ``--deps`` also installs the runtime dependencies (Linux: ``apt``, needs
  ``sudo``). Without it, they must already be there.

Afterwards ``setup-cwipc.sh`` (run by ``get-cwipc.sh``) has created a Python venv
in ``<prefix>/libexec/cwipc/venv`` with the cwipc modules, and written:

- ``<prefix>/libexec/cwipc/activate``: ``source`` it to get ``PATH``, the
  library path, ``CWIPC_LIBRARY_DIR`` and the venv (on Windows also
  ``activate.ps1`` for PowerShell);
- ``<prefix>/libexec/cwipc/environment``: the same settings in a simple
  machine-readable form, one per line: ``path DIR`` (prepend to ``PATH``),
  ``env NAME=VALUE`` and ``libpath NAME DIR`` (prepend to the library path
  variable ``NAME``).

``setup-cwipc.sh --help`` and ``get-cwipc.sh --help`` list all options.

**Older releases:** run from a cwipc checkout, ``get-cwipc.sh`` uses the
``setup-cwipc.sh`` next to it when an archive doesn't contain one, so it also
works for releases from before 8.3.0, such as v8.1.0. Fetched on its own (as
above) it needs the setup script inside the archive, so 8.3.0 or later.

GitHub Actions
~~~~~~~~~~~~~~

In a GitHub Actions workflow, use the install action. It runs ``get-cwipc.sh``
and makes the environment available to the following steps of the job::

    - uses: cwi-dis/cwipc/.github/actions/install-cwipc@<tag or commit>
      id: cwipc
      with:
        version: v8.1.0
    - run: cwipc view --synthetic --count 3 --nodisplay

Pin the action to a tag or commit (the ``@...`` part, which version of the
*action* is used), independent of ``version`` (which *release of cwipc* is
installed). Inputs: ``version`` (required), ``method``, ``archive``, ``prefix``,
``deps`` (default ``true``), ``python-modules`` (default ``true``) and
``python``; outputs: ``prefix`` and ``cwipc-version``. See
``.github/actions/install-cwipc/action.yml``. On Windows, set up Python 3.12
first (``actions/setup-python``); on macOS the install takes 10–20 minutes,
because Homebrew builds cwipc from source.

From source
-----------

If you require a custom build or want to contribute, follow the instructions in
:doc:`build-from-source`.
