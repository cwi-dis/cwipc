set -x
brew install pkg-config || brew upgrade pkg-config
brew install cmake || brew upgrade cmake
brew install libomp || brew upgrade libomp
brew install pcl || brew upgrade pcl
brew install python@3.12 || brew upgrade python@3.12
brew link python@3.12
brew install pkg-config || brew upgrade pkg-config
brew install homebrew/core/glfw3 || brew upgrade homebrew/core/glfw3
brew install librealsense || brew upgrade librealsense
brew install opencv || brew upgrade opencv
brew install jpeg-turbo || brew upgrade jpeg-turbo
brew link --force jpeg-turbo
