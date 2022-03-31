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
apt install zlib1g-dev -y
apt install zlib1g -y
apt install flex -y
apt install python3-pip -y



echo "Cloning Submodules"
git submodule update --init --recursive

echo "installing BoringSSl"
cd $ROOT/submodules/boringssl
BORINGSSL=$PWD
cmake -DBUILD_SHARED_LIBS=1 . && make
cp $ROOT/submodules/boringssl/crypto/libcrypto.so /usr/local/lib/libcrypto.so
cp $ROOT/submodules/boringssl/ssl/libssl.so /usr/local/lib/libssl.so

echo "installing LSQUIC"
cd $ROOT/submodules/lsquic
git submodule init
git submodule update

cmake -DLSQUIC_SHARED_LIB=1 -DBORINGSSL_DIR=$BORINGSSL .
make
make install
ldconfig

echo "installing plugins"
cd $ROOT/src/gst-quic/
mkdir build
meson build
cd build
meson install

echo "Plugins installed, installing wireshark"
cd $ROOT/submodules/wireshark
mkdir build
cd build
cmake -G Ninja ..
ninja
ninja install

echo "building GStreamer"
cd $ROOT/submodules/gst-build
mkdir build
meson build/
ninja -C build/
cd gstreamer
git pull origin
git reset --hard origin
cd ../gst-plugins-good
git pull origin
git reset --hard origin

echo "build Mininet"
cd $ROOT/submodules/mininet
mkdir ../mininet_install_dir
util/install.sh -s ../mininet_install_dir -a


echo "Installing gdown to pull BBB raw video"
pip3 install --upgrade --no-cache gdown

echo "Installing pandas (Used by analysis scripts)"
pip3 install --upgrade --no-cache pandas

if [ ! -f ${ROOT}/src/mininet_tests/BBB_dec.raw ]
then
  echo "Downloading raw Big Buck Bunny Video"
  cd $ROOT/src/mininet_tests
  gdown --id 1x5irCt_B2XdU38uEp-rjXOeF5eNCBDiW
fi