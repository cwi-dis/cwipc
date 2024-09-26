#
# Install realsense2 SDK from their repository
#
set -x
# Add the key
sudo mkdir -p /etc/apt/keyrings
curl -sSf https://librealsense.intel.com/Debian/librealsense.pgp | sudo tee /etc/apt/keyrings/librealsense.pgp > /dev/null
# Add the repo server
echo "deb [signed-by=/etc/apt/keyrings/librealsense.pgp] https://librealsense.intel.com/Debian/apt-repo `lsb_release -cs` main" | \
sudo tee /etc/apt/sources.list.d/librealsense.list
#
# Update package list and upgrade packages
sudo apt-get -y update
sudo apt-get -y upgrade
# We need Python 3.11, for now
sudo apt-add-repository -y ppa:deadsnakes/ppa
sudo apt-get install -y python3.11 python3.11-dev python3.11-pip python3.11-venv
# Install packages we need
sudo apt-get install -y tzdata
sudo apt-get install -y software-properties-common
sudo apt-get install -y git cmake
sudo apt-get install -y libpcl-dev
sudo apt-get install -y libturbojpeg0-dev
sudo apt-get install -y libusb-1.0 libusb-dev
sudo apt-get install -y libglfw3 libglfw3-dev
sudo apt-get install -y libopencv-dev
sudo apt-get install -y curl
sudo apt-get install -y librealsense2-dkms librealsense2-utils librealsense2-dev
# K4A doesn't work on 24.04.
