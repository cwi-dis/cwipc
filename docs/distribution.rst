Preparing a distribution
========================


These instructions are primarily for our own benefit. Lest we forget.

When creating a new release, ensure the following have been done:

- Python dependencies and Python maximum version need to be updated

	- in ``cwipc_util/python/pyproject.toml`` remove the dependency specifiers,
	- build on all platforms, ensure everything works, possibly lowering the versions of some dependencies, and possibly lowering the maximum Python version
		
    - Especially watch out for ``opencv`` and ``open3d`` which can some times lag 2 Python versions

	- Add the Python package dependency specifiers again for the currently selected versions.
	- Update ``PYTHON_VERSION`` (preferred: installed, built and tested with), ``PYTHON_VERSION_MIN`` (that version or the one before it, accepted if already there) and ``PYTHON_WINDOWS_INSTALLER_VERSION`` in ``scripts/versions.env``. ``CMakeLists.txt`` reads them from there.
	- Ubuntu uses its system Python (3.12 on 24.04): check that it is still within the range.
	- Check ``.github/workflows/build.yml`` for the Python versions used.
- Check whether ``nlohman_json`` can be updated (``CMakeLists.txt``)
- Check whether ``nsis`` can be updated (``.github/workflows/build.yml``)
- Check whether the slurped and modified ``cpack`` config files for ``nsis`` (in ``CMakeFiles``) need to be updated.
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

- The OrbbecSDK version and its per-platform archive names are in ``scripts/versions.env``.

- For Windows and Android, the ``vcpkg`` dependent packages should all be updated to the most recent version::

    cd .\vcpkg
    git checkout master
    git pull
    .\bootstrap-vcpkg.bat
    cd ..
    .\vcpkg\vcpkg x-update-baseline
    .\vcpkg\vcpkg.exe install
    git commit -a -m "Vcpkg packages updated to most recent version"

- The toplevel ``vcpkg.json`` has a version string.
- ``setup.py`` in ``cwipc_util/python`` and every other package has a default version string that is only used when installing with ``pip install -e`` (because usually it is dynamically determined at build time. For good measure update these default version strings when doing a major release.
- ``CMakeFiles/GetGitVersion.cmake`` has a ``CWIPC_VERSION_UNKNOWN`` that is only used when building from a source distribution. Update it.
- ``CWIPC_API_VERSION`` incremented if there are any API changes (additions only).
- ``CWIPC_API_VERSION_OLD`` incremented if there are API changes that are not backward compatible.

	- Both these need to be changed in ``cwipc_util/include/api.h`` and ``cwipc_util/python/cwipc/util.py``.

- ``CHANGELOG.md`` updated.

Version numbers for the release no longer need to be updated manually, but note the exceptions above.

After making all these changes push to github. Ensure the CI/CD build passes (easiest is by running ``./scripts/nightly.sh`` which does a nightly build). This build will take a looooong time, most likely, because the ``vcpkg`` dependencies have been updated and the Windows runner will have to rebuild the world.

After that tag with *v_X_._Y_._Z_*.

If one of the next steps fails just fix the issue and do another micro-release. Has happened to me every single release, I think:-)

Push the tag to github, this will build the release.

After the release is built copy the relevant new section of ``CHANGELOG.md`` to the release notes.

After that, update the ``brew`` formula at <https://github.com/cwi-dis/homebrew-cwipc>. Use:

- ``brew edit cwipc`` and change the URL and version (and possibly Python or other dependencies),
- ``brew fetch cwipc`` to get the error about the SHA mismatch, fix the SHA,
-  ``brew install`` to install the new version,
-  then push the changes (easy from within ``vscode``),
-  then ``brew upgrade cwipc`` on another machine to test.

Finally, when you are happy that everything works, edit the release on the github web interface and clear the ``prerelease`` flag.

Continuous integration
----------------------

The project uses GitHub Actions (``.github/workflows/build.yml``) to build and
package on every push.  Tags matching ``v*`` trigger a release job which uploads
artifacts to GitHub Releases and updates the Homebrew formula.

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
