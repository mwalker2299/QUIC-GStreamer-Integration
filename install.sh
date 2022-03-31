#!/bin/bash

ROOT=$PWD

echo "Installing necessary packages via APT"
apt install meson -y
apt install ninja-build -y
apt install qt5-default -y
apt install qtmultimedia5-dev -y
apt install qttools5-dev -y
apt install qttools5-dev-tools -y
apt install openvswitch-switch -y
apt install openvswitch-testcontroller -y
apt install net-tools -y
apt install libc6-dev -y
apt install libc-ares-dev -y
apt install libconfig-dev -y
apt install libgcrypt20-dev -y
apt install libgstreamer1.0-dev -y
apt install libgstreamer-plugins-bad1.0-dev -y
apt install libgstreamer-plugins-base1.0-dev -y
apt install libnet1 -y
apt install libnet1-dev -y
apt install libpcap-dev -y
apt install libpulse-dev -y
apt install libqt5multimedia5 -y
apt install libqt5svg5-dev -y
apt install libsnmp-dev -y
apt install cmake -y
apt install autoconf -y
apt install automake -y
apt install autotools-dev -y
apt install bison -y
apt install golang -y
apt install gstreamer1.0-doc -y
apt install gstreamer1.0-plugins-bad -y
apt install gstreamer1.0-qt5 -y
apt install care -y



echo "Cloning Submodules"
git submodule update --init --recursive

echo "Building BoringSSl"
cd boringssl
BORINGSSL=$PWD
cmake -DBUILD_SHARED_LIBS=1 . && make

echo "Building LSQUIC"
cd ../lsquic
git submodule init
git submodule update

cmake -DLSQUIC_SHARED_LIB=1 -DBORINGSSL_DIR=$BORINGSSL .
make

echo "installing plugins"
cd ../src/gst-quic/
mkdir build
cd build
meson install

echo "Plugins installed, building wireshark"
cd $ROOT
cd wireshark
mkdir build
cd build
cmake -G Ninja ..
ninja



