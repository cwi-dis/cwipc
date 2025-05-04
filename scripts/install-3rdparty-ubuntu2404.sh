set -x
# Add the key for the librealsense repo
sudo mkdir -p /etc/apt/keyrings
curl -sSf https://librealsense.intel.com/Debian/librealsense.pgp | sudo tee /etc/apt/keyrings/librealsense.pgp > /dev/null
# Add the librealsense repo server
echo "deb [signed-by=/etc/apt/keyrings/librealsense.pgp] https://librealsense.intel.com/Debian/apt-repo `lsb_release -cs` main" | \
sudo tee /etc/apt/sources.list.d/librealsense.list
#
# Update package list and upgrade packages
#
sudo apt-get -y update
sudo apt-get -y upgrade
# Install packages we need
# We install some -dev packages because this will install the correct
# version of the underlying dynamic library for the current distribution.
sudo apt-get install -y git python3-pip
sudo apt-get install -y git python3-venv
sudo apt-get install -y tzdata
sudo apt-get install -y software-properties-common
sudo apt-get install -y git cmake
sudo apt-get install -y libpcl-dev
sudo apt-get install -y libturbojpeg0-dev
sudo apt-get install -y libusb-1.0 libusb-dev
sudo apt-get install -y libglfw3 libglfw3-dev
sudo apt-get install -y libopencv-dev
sudo apt-get install -y curl
# debian packages not yet released for librealsense
# sudo apt-get install -y librealsense2-dkms librealsense2-utils librealsense2-dev
# K4A doesn't work on 24.04.
