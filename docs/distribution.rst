Preparing a distribution
========================


These instructions are primarily for our own benefit. Lest we forget.

When creating a new release, go through the following. The first part, the external versions,
is best done early in a release cycle, on a branch with CI: raising them can take a few rounds.

External versions to check at a release
---------------------------------------

Everything external whose version this repository pins, and where. Things that are not
pinned here (Homebrew's packages on macOS, Ubuntu's apt packages) move by themselves; the
CI builds show when that breaks something.

Dependencies of cwipc
^^^^^^^^^^^^^^^^^^^^^

- Python dependencies and Python maximum version need to be updated

	- in ``cwipc_util/python/pyproject.toml`` remove the dependency specifiers,
	- build on all platforms, ensure everything works, possibly lowering the versions of some dependencies, and possibly lowering the maximum Python version
		
    - Especially watch out for ``opencv`` and ``open3d`` which can some times lag 2 Python versions

	- Add the Python package dependency specifiers again for the currently selected versions.
	- Update ``PYTHON_VERSION`` (preferred: installed, built and tested with), ``PYTHON_VERSION_MIN`` (that version or the one before it, accepted if already there) and ``PYTHON_MANAGER_VERSION`` (the Windows Python install manager) in ``scripts/versions.env``. ``CMakeLists.txt`` reads them from there.
	- Ubuntu uses its system Python (3.12 on 24.04): check that it is still within the range.
	- Check ``.github/workflows/build.yml`` for the Python versions used.
- ``nlohmann_json``: the minimum version in ``find_package(nlohmann_json ...)`` in ``CMakeLists.txt``.

- The OrbbecSDK and OrbbecViewer: the version and the per-platform archive names (file names with
  extension, from the release's assets) in ``scripts/versions.env``. Orbbec also recommends a
  camera firmware version per SDK release (in its release notes).

- For Windows and Android, the ``vcpkg`` dependent packages should all be updated to the most recent
  vcpkg **release** (a tag such as ``2026.10.16``, not ``master``: the vcpkg tool and the ports of a
  release belong together). Move the submodule to it and set the baseline in
  ``vcpkg-configuration.json`` to the same commit, on any platform::

    git -C vcpkg fetch --tags origin
    git -C vcpkg tag --sort=-creatordate | head -1          # the most recent release
    git -C vcpkg checkout <tag>
    git -C vcpkg rev-parse HEAD                             # put this in "baseline"

  Set the baseline by hand rather than with ``vcpkg x-update-baseline``: our default registry is a
  ``git`` registry, for which that takes the newest commit of vcpkg's GitHub repository rather than
  that of the submodule (seen in November 2026: with the submodule at ``2026.10.16`` it proposed
  vcpkg's ``master``). To see
  which port versions change, compare ``versions/baseline.json`` in the old and the new commit.
  Then, on Windows, ``scripts\install-deps-windows.ps1 -Build`` bootstraps the new vcpkg tool, and
  ``cmake --preset windows-production`` builds all vcpkg packages (this takes long). CI rebuilds
  them too, for Windows and Android (cwi-dis/cwipc#348).

- Dependencies for the ``.deb`` installer for apt/Ubuntu need to be updated. On the targeted Ubuntu,
  after a build, let ``dpkg-shlibdeps`` compute them (it needs a dummy ``debian/control`` in its
  current directory)::

      B=$PWD/build; T=$(mktemp -d); mkdir $T/debian
      printf "Source: cwipc\n\nPackage: cwipc\nArchitecture: any\n" > $T/debian/control
      (cd $T && dpkg-shlibdeps -O --ignore-missing-info -l$B/lib -l$PWD/3rdparty_installed/orbbecsdk/lib \
          -e $(file $B/lib/libcwipc_*.so $B/bin/cwipc_* | grep ELF | cut -d: -f1))
      rm -rf $T

  Put the result in ``CPACK_DEBIAN_PACKAGE_DEPENDS`` in ``CMakeFiles/CwipcInstallers.cmake``, plus
  ``python3-venv`` (for the postinst), with ``librealsense2`` moved to
  ``CPACK_DEBIAN_PACKAGE_RECOMMENDS`` (it comes from the RealSense apt repository). Ubuntu links with
  ``--as-needed``, so the list is short (of PCL only ``libpcl-common``). Keep the runtime level of
  ``scripts/install-deps-ubuntu2404.sh`` the same.

  ``dpkg-shlibdeps`` only sees cwipc's own binaries, not the pip wheels the postinst installs into
  the venv (open3d, PySide6, opencv-python, ...), which need system libraries too. To find those:
  in a scratch venv, install the dependencies from ``cwipc_util/python/pyproject.toml``, import
  the modules cwipc uses (with ``QT_QPA_PLATFORM=offscreen`` without a display) and list the
  libraries loaded from ``/proc/self/maps``; add ``ldd`` of Qt's X11 and Wayland plugins
  (``PySide6/Qt/plugins/platforms/libqxcb.so``, ``libqwayland*.so`` and
  ``xcbglintegrations/*.so``), which only load with a display. Map the libraries outside the
  venv to packages with ``dpkg -S``, and keep only those not already pulled in by the other
  dependencies (``apt-cache depends --recurse``). That list is the second line of
  ``CPACK_DEBIAN_PACKAGE_DEPENDS``. Don't take everything the wheels could load: PySide6 also
  ships WebEngine, multimedia and SQL drivers cwipc doesn't use.

  CI tests the package on a fresh runner without the build dependencies
  (``test-package-ubuntu2404-intel``, ``.github/scripts/test-deb-package.sh``), so a missing
  dependency shows there; it has no display, so the X11 plugin isn't exercised.

  The ``Recommends: librealsense2 (>= ...)`` in the same file follows the librealsense version of
  the RealSense apt repository.

Build tools
^^^^^^^^^^^

- CMake: ``cmakeMinimumRequired`` in ``CMakePresets.json`` decides what developers need; it must
  not exceed what the Ubuntu we target has in apt (24.04: 3.28; 26.04: 4.2). In CI the Ubuntu job
  pins exactly that version (``cmakeVersion`` of its ``lukka/get-cmake`` step in
  ``.github/workflows/build.yml``), so the minimum is tested; the other jobs use the newest CMake,
  as developers on those platforms get it.
- NSIS: ``nsis-version`` in ``.github/workflows/build.yml``. Check whether the slurped and modified
  ``cpack`` config files for NSIS (in ``CMakeFiles``) need to be updated too.
- Android: the NDK (``ndk-version`` in ``.github/workflows/build.yml``) and
  ``ANDROID_NATIVE_API_LEVEL`` in ``CMakePresets.json``.

CI runners and actions
^^^^^^^^^^^^^^^^^^^^^^

- The GitHub runner images (``runs-on:`` in ``.github/workflows/*.yml``; GitHub announces new
  images and the retirement of old ones). They are pinned on purpose: a ``-latest`` label changes
  under us, and a new image brings new compilers and tools (cwi-dis/cwipc#242). Expect:

  - Windows and Android rebuild all their vcpkg packages once (Windows about an hour).
  - Android on Ubuntu 26.04: mono's NuGet couldn't verify GitHub's certificate (November 2026);
    needs fixing (for example ``cert-sync``) before Android can move.
  - macOS: a newer image means newer Homebrew bottles; check that Intel Mac images are still
    available (``macos-15-intel``).

- The GitHub actions (``uses: ...@vN`` in ``.github/workflows/*.yml`` and
  ``.github/actions/``): GitHub deprecates the Node.js versions older actions run on.
- The Python version of ``actions/setup-python`` (``python-version`` in the workflows): the same as
  ``PYTHON_VERSION`` in ``scripts/versions.env``.

Documentation
^^^^^^^^^^^^^

- ``.readthedocs.yaml`` (the build image and the Python version) and ``docs/requirements.txt``
  (Sphinx and its extensions).

cwipc's own version numbers
---------------------------

- The toplevel ``vcpkg.json`` has a version string.
- ``setup.py`` in ``cwipc_util/python`` and every other package has a default version string that is only used when installing with ``pip install -e`` (because usually it is dynamically determined at build time. For good measure update these default version strings when doing a major release.
- ``CMakeFiles/GetGitVersion.cmake`` has a ``CWIPC_VERSION_UNKNOWN`` that is only used when building from a source distribution. Update it.
- ``CWIPC_API_VERSION`` incremented if there are any API changes (additions only).
- ``CWIPC_API_VERSION_OLD`` incremented if there are API changes that are not backward compatible.

	- Both these need to be changed in ``cwipc_util/include/api.h`` and ``cwipc_util/python/cwipc/util.py``.

- ``CHANGELOG.md`` updated.

Releasing
---------

Version numbers for the release no longer need to be updated manually, but note the exceptions above.

After making all these changes push to github. Ensure the CI/CD build passes (easiest is by running ``./scripts/nightly.sh`` which does a nightly build). This build will take a looooong time, most likely, because the ``vcpkg`` dependencies have been updated and the Windows runner will have to rebuild the world.

The CI build also tests what it built, each on a fresh runner: the Windows installer, the Debian
package, and the Linux and Windows ``-built-`` archives installed with cwipc's own install action
(``.github/actions/install-cwipc``), as downstream CI uses them (cwi-dis/cwipc#352). The release is
only created when those pass. As a second check, once the release exists: bump lldash's
``cwipc_version_tag`` to it and see lldash's CI pass.

After that tag with *v_X_._Y_._Z_*.

If one of the next steps fails just fix the issue and do another micro-release. Has happened to me every single release, I think:-)

Push the tag to github, this will build the release.

After the release is built copy the relevant new section of ``CHANGELOG.md`` to the release notes.

Then update the ``brew`` formula at <https://github.com/cwi-dis/homebrew-cwipc>. Use:

- ``brew edit cwipc`` and change the URL and version (and possibly Python or other dependencies),
- ``brew fetch cwipc`` to get the error about the SHA mismatch, fix the SHA,
-  ``brew install`` to install the new version,
-  then push the changes (easy from within ``vscode``),
-  then ``brew upgrade cwipc`` on another machine to test.

Finally, when you are happy that everything works, edit the release on the github web interface and clear the ``prerelease`` flag.

After the release
-----------------

Things that point at the previous release and need to move to the new one:

- The default ``version`` in ``.github/workflows/test-install-action.yml``.
- Downstream use of the install action (cwi-dis/cwipc_unity, cwi-dis/VR2Gather): from a commit pin
  to the new release tag (cwi-dis/cwipc#354).
- lldash's ``cwipc_version_tag`` (see above).
- The ``brew`` formula (see above).

Continuous integration
----------------------

The project uses GitHub Actions (``.github/workflows/build.yml``) to build,
test and package on every push to ``master``, on pull requests, and on tags.
Tags (``v*``, ``nightly``, ``exp*``) also trigger a release job which uploads
the artifacts to GitHub Releases. The Homebrew formula is updated by hand (see
above).

Building and publishing the documentation
------------------------------------------

Documentation is generated from reStructuredText files in ``docs/`` using Sphinx with
the ReadTheDocs theme.  You will need the packages listed in ``docs/requirements.txt``
(``sphinx``, ``myst-parser``, ``sphinx-rtd-theme``).

To build locally::

    cd docs
    python -m pip install -r requirements.txt
    sphinx-build -b html . _build/html

The HTML output appears in ``docs/_build/html``.  Run the same commands in a CI
job to verify formatting or catch broken links.

On ReadTheDocs the project ``https://cwipc.readthedocs.io`` is configured to use this ``docs/`` folder as the
source. Currently you have to manually trigger a build there. The project page is at ``https://app.readthedocs.org/projects/cwipc/`` and it may be that only Jack can do this at the moment.
