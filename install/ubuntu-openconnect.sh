#! /usr/bin/env bash
sudo apt install -y \
  build-essential gettext autoconf automake libproxy-dev \
  libxml2-dev libtool vpnc-scripts pkg-config zlib1g-dev \
  libp11-kit-dev libp11-dev libssl-dev

cd $(mktemp -d)
curl https://gitlab.com/openconnect/openconnect/-/archive/v8.08/openconnect-v8.08.zip\
 -o openconnect-v8.08.zip -L
unzip openconnect-v8.08.zip
cd openconnect-v8.08
./autogen.sh
./configure
make && make check
sudo make install && sudo ldconfig
# Verify
openconnect --version
