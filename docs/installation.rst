Installation
=============

The simplest way to install **cwipc** is to use one of the pre‑built
installers.  Installers are provided for macOS, Windows and Ubuntu and bundle
all of the runtime libraries, command‑line tools and language bindings.

.. note::
   After installing, running ``cwipc view --synthetic`` should open a window
   showing a synthetic point cloud stream (a rotating paraboloid with blinking eyes).
   You can also run ``cwipc_check`` to verify the installation and dependencies, it will
   print any warnings or errors it finds.  
   
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

After installation, use the Start menu entries to run **Check cwipc
installation** and optionally **Attempt to fix cwipc installation**. 

The top-level readme file has some more Windows Installation troubleshooting tips.

Ubuntu 24.04
------------

Install the Debian package::

    sudo apt install ./cwipc-<version>.deb

Realsense drivers are not included; inspect
``/usr/share/cwipc/scripts/install-3rdparty-ubuntu2404.sh`` for instructions.

Android
-------

The Android build is API‑only and used primarily from Unity on Quest devices.

Scripted installs and CI
------------------------

For an install without an installer, for example on a lab machine, in a Docker
image or in the CI of another project, use ``get-cwipc.sh`` (bash; on Windows
Git Bash). It gets cwipc onto the machine and sets it up::

    curl -sSfL -o get-cwipc.sh https://raw.githubusercontent.com/cwi-dis/cwipc/<tag or commit>/scripts/get-cwipc.sh
    bash get-cwipc.sh v8.1.0 /opt/cwipc
    source /opt/cwipc/activate

- On Linux and Windows it downloads the ``-built-`` archive of that release for
  this machine and unpacks it into the given directory. ``--archive FILE`` uses a
  local archive instead.
- On macOS it builds cwipc through the Homebrew formula (``--method brew``, the
  default; the directory argument is then ignored), because the macOS ``-built-``
  archives only work as long as Homebrew still has the exact library versions
  they were built against (cwi-dis/cwipc#356). ``--method archive`` forces the
  archive. ``master`` as the version builds the current master branch
  (``brew install --HEAD``).
- ``--deps`` also installs the runtime dependencies (Linux: ``apt``, needs
  ``sudo``). Without it, they must already be there.

Afterwards ``setup-cwipc.sh`` (run by ``get-cwipc.sh``) has created a Python venv
in ``<prefix>/libexec/cwipc/venv`` with the cwipc modules, and written:

- ``<prefix>/activate``: ``source`` it to get ``PATH``, the library path,
  ``CWIPC_LIBRARY_DIR`` and the venv (on Windows also ``activate.ps1`` for
  PowerShell);
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
