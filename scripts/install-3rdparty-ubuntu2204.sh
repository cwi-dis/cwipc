#
# Install realsense2 SDK from their repository
#
set -x
sudo apt-key adv --keyserver keys.gnupg.net --recv-key F6E65AC044F831AC80A06380C8B3A55A6F3EFCDE || sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-key F6E65AC044F831AC80A06380C8B3A55A6F3EFCDE
sudo add-apt-repository -y "deb https://librealsense.intel.com/Debian/apt-repo focal main" -u
#
# Update package list and upgrade packages
sudo apt-get -y update
sudo apt-get -y upgrade
# Install packages we need
sudo apt-get install -y tzdata
sudo apt-get install -y software-properties-common
sudo apt-get install -y git cmake
sudo apt-get install -y git python3-pip
sudo apt-get install -y git python3-venv
sudo apt-get install -y libpcl-dev
sudo apt-get install -y libturbojpeg0-dev
sudo apt-get install -y libusb-1.0 libusb-dev
sudo apt-get install -y libglfw3 libglfw3-dev
sudo apt-get install -y libopencv-dev
sudo apt-get install -y curl
sudo apt-get install -y librealsense2-dkms librealsense2-utils librealsense2-dev
sudo apt-get install -y libsoundio1
#
# Upgrade essential Python packages that are too old on Ubuntu 20.04
#
sudo pip install --upgrade pip setuptools build wheel pillow
#
# Install k4a SDK from their repository.
# Bit of a hack, see https://github.com/microsoft/Azure-Kinect-Sensor-SDK/issues/1263
# The simple solution by @vinesmsuic does not seem to work. This uses the manual 
# solution by @atinfinity
# Also, https://github.com/microsoft/Azure-Kinect-Sensor-SDK/issues/1190 is part of this solution (needed to non-interactively accept EULA)
#
if ! dpkg -s libk4a1.3 > /dev/null; then
	curl -sSL https://packages.microsoft.com/ubuntu/18.04/prod/pool/main/libk/libk4a1.3/libk4a1.3_1.3.0_amd64.deb > /tmp/libk4a1.3_1.3.0_amd64.deb
	echo 'libk4a1.3 libk4a1.3/accepted-eula-hash string 0f5d5c5de396e4fee4c0753a21fee0c1ed726cf0316204edda484f08cb266d76' | sudo debconf-set-selections
	sudo dpkg -i /tmp/libk4a1.3_1.3.0_amd64.deb
fi
if ! dpkg -s libk4a1.3-dev > /dev/null; then
	curl -sSL https://packages.microsoft.com/ubuntu/18.04/prod/pool/main/libk/libk4a1.3-dev/libk4a1.3-dev_1.3.0_amd64.deb > /tmp/libk4a1.3-dev_1.3.0_amd64.deb
	sudo dpkg -i /tmp/libk4a1.3-dev_1.3.0_amd64.deb
fi
if ! dpkg -s libk4abt1.0 > /dev/null; then
	curl -sSL https://packages.microsoft.com/ubuntu/18.04/prod/pool/main/libk/libk4abt1.0/libk4abt1.0_1.0.0_amd64.deb > /tmp/libk4abt1.0_1.0.0_amd64.deb
	echo 'libk4abt1.0	libk4abt1.0/accepted-eula-hash	string	03a13b63730639eeb6626d24fd45cf25131ee8e8e0df3f1b63f552269b176e38' | sudo debconf-set-selections
	sudo dpkg -i /tmp/libk4abt1.0_1.0.0_amd64.deb
fi
if ! dpkg -s libk4abt1.0-dev > /dev/null; then
	curl -sSL https://packages.microsoft.com/ubuntu/18.04/prod/pool/main/libk/libk4abt1.0-dev/libk4abt1.0-dev_1.0.0_amd64.deb > /tmp/libk4abt1.0-dev_1.0.0_amd64.deb
	sudo dpkg -i /tmp/libk4abt1.0-dev_1.0.0_amd64.deb
fi
if ! dpkg -s k4a-tools > /dev/null; then
	curl -sSL https://packages.microsoft.com/ubuntu/18.04/prod/pool/main/k/k4a-tools/k4a-tools_1.3.0_amd64.deb > /tmp/k4a-tools_1.3.0_amd64.deb
	sudo dpkg -i /tmp/k4a-tools_1.3.0_amd64.deb
fi
#
