set -x
brew install pkg-config || brew upgrade pkg-config
brew install cmake || brew upgrade cmake
brew install pcl || brew upgrade pcl
brew install python@3.12 || brew upgrade python@3.12
brew link python@3.12
brew install pkg-config || brew upgrade pkg-config
brew install homebrew/core/glfw3 || brew upgrade homebrew/core/glfw3
brew install librealsense || brew upgrade librealsense
brew install jpeg-turbo || brew upgrade jpeg-turbo
brew link --force jpeg-turbo

# Install Python delendencies
python3.12 -m pip install --upgrade pip setuptools build wheel

# Install Python dependencies for Deployment scripts (unsure they are still needed)
python3.12 -m pip install requests requests_toolbelt mechanize pillow numpy open3d
