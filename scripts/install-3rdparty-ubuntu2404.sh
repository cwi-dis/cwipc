set -x
# Add the key for the librealsense repo
sudo mkdir -p /etc/apt/keyrings
curl -sSf https://librealsense.realsenseai.com/Debian/librealsenseai.asc | gpg --dearmor | sudo tee /etc/apt/keyrings/librealsenseai.gpg > /dev/null > /dev/null
# Add the librealsense repo server
echo "deb [signed-by=/etc/apt/keyrings/librealsenseai.gpg] https://librealsense.realsenseai.com/Debian/apt-repo `lsb_release -cs` main" | sudo tee /etc/apt/sources.list.d/librealsense.list
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
sudo apt-get install -y linux-headers-$(uname -r)
sudo apt-get install -y llibrealsense2-dkms ibrealsense2-utils librealsense2-dev
