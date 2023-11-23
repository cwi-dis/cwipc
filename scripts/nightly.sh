# Create a new "nightly" release (which isn't really nightly)
#
# Remove the old nightly tag, create a new one and pushit, which will trigger the build Action.
git fetch --recurse-submodules
gh release delete nightly --cleanup-tag --yes
git tag -f nightly origin/master
git push origin nightly
