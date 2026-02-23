Preparing a distribution
========================


These instructions are primarily for our own benefit. Lest we forget.

When creating a new release, ensure the following have been done:

- Python dependencies and Python maximum version need to be updated

	- in ``cwipc_util/python/pyproject.toml`` remove the dependency specifiers,
	- build on all platforms, ensure everything works, possibly lowering the versions of some dependencies, and possibly lowering the maximum Python version
		
    - Especially watch out for ``opencv`` and ``open3d`` which can some times lag 2 Python versions

	- Add the Python package dependency specifiers again for the currently selected versions.
	- Update the Python version range in the toplevel ``CMakeLists.txt``.
	- Update ``scripts/install-3rdparty-windows.ps1`` with the best Python version.
	- Update ``scripts/install-3rdparty-macos.sh`` with the best Python version.
	- Check the Ubuntu install-3rdparty scripts for which Python they install.
	- Check ``.github/workflows/build.yml`` for the Python versions used.
- Check whether ``nlohman_json`` can be updated (``CMakeLists.txt``)
- Check whether ``nsis`` can be updated (``.github/workflows/build.yml``)
- Dependencies for the ``.deb`` installer for apt/Ubuntu need to be updated. There may be better ways to do this, but this works

  - On the targeted Ubuntu, check out and edit ``CMakeFiles/CwipcInstallers.cmake``
  - Comment out the definitions for ``CPACK_DEBIAN_PACKAGE_DEPENDS`` and ``CPACK_DEBIAN_PACKAGE_RECOMMENDS``.
  - Un-comment-out ``CPACK_DEBIAN_PACKAGE_SHLIBDEPS YES``.
  - Build, run cpack with::
    
      cpack --config build/CPackConfig.cmake -D CPACK_DEBIAN_FILE_NAME="cwipc_test_ubuntu2404.deb"
  
  - Extract the resulting debian package with ``ar x``.
  - Unpack the ``control.tar.gz`` file.
  - Inspect the dependencies that cpack auto-generated.
  - Fix the dependencies and recommendations based on what cpack found.

- ``scripts/install-3rdparty-windows.ps1`` should be updated to download the most recent compatible packages. Go through each of the packages, determine the current version. Uninstall old versions from your build machine. Run the powershell script to test it installs the new packages. Do the build, to ensure it works with the new packages. Test the build to ensure it runs with the new packages.
  
  - **Note**: the only package that is important here nowadays is Python, because the other other two left here, ``k4a`` and ``k4abt``, are no longer maintained.

- For Windows and Android, the ``vcpkg`` dependent packages should all be updated to the most recent version::

    cd .\vcpkg
    git pull
    .\bootstrap-vcpkg.bat
    cd ..
    .\vcpkg\vcpkg x-update-baseline
    .\vcpkg\vcpkg.exe install
    git commit -a -m "Vcpkg packages updated to most recent version"

- The toplevel ``vcpkg.json`` has a version string.
- ``setup.py`` in ``cwipc_util/python`` and every other package has a default version string that is only used when installing with ``pip install -e`` (because usually it is dynamically determined at build time. For good measure update these default version strings when doing a major release.
- ``CWIPC_API_VERSION`` incremented if there are any API changes (additions only).
- ``CWIPC_API_VERSION_OLD`` incremented if there are API changes that are not backward compatible.

	- Both these need to be changed in ``api.h`` and ``cwipc/util.py``.

- ``CHANGELOG.md`` updated.

Version numbers for the release no longer need to be updated manually, but note the exceptions above.

After making all these changes push to github. Ensure the CI/CD build passes (easiest is by running ``./scripts/nightly.sh`` which does a nightly build). This build will take a looooong time, most likely, because the ``vcpkg`` dependencies have been updated and the Windows runner will have to rebuild the world.

After that tag all submodules and the main module with *v_X_._Y_._Z_*.

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
