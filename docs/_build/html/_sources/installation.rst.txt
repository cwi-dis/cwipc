Installation
=============

The simplest way to install **cwipc** is to use one of the pre‑built
installers.  Installers are provided for macOS, Windows and Ubuntu and bundle
all of the runtime libraries, command‑line tools and language bindings.

.. note::
   After installing, run ``cwipc_view --synthetic`` to verify the installation. A
   rotating synthetic point cloud should appear.  You can also run
   ``cwipc_check`` which exercises the various parts of the installation.

macOS
-----

Use Homebrew::

    brew tap cwi-dis/cwipc
    brew install cwipc

Verify with ``cwipc_view --version`` and (if necessary) run
``cwipc_pymodules_install.sh`` to repair the Python packages.

Windows
-------

Download the ``.exe`` installer from the `releases page
<https://github.com/cwi-dis/cwipc/releases/latest>`_ and execute it.  If the
installer refuses to run, install the **Microsoft VC++ Redistributable
(x64)** first.

After installation, use the Start menu entries to run **Check cwipc
installation** and optionally **Attempt to fix cwipc installation**.  See the
installation section of the top‑level README for troubleshooting tips.

Ubuntu 24.04
------------

Install the Debian package::

    sudo apt install ./cwipc-<version>.deb

Realsense drivers are not included; inspect
``/usr/share/cwipc/scripts/install-3rdparty-ubuntu2404.sh`` for instructions.

Android
-------

The Android build is API‑only and used primarily from Unity on Quest devices.

From source
-----------

If you require a custom build or want to contribute, follow the instructions in
:doc:`build-from-source`.
