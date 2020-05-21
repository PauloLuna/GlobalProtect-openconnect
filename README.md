# GlobalProtect-openconnect
A GlobalProtect VPN client (GUI) for Linux based on Openconnect and built with Qt5, supports SAML auth mode, inspired by [gp-saml-gui](https://github.com/dlenski/gp-saml-gui).




|Initial Window      |Gateways in dropdown list after login     |VPN Connected       |Install VPN certificates (Ubuntu and Fedora)
|----------|----------|----------|-----------|
| ![Start](imgs/img3.png)  |![Login](imgs/img4.png)|![Connect](imgs/img5.png)|![Connect](imgs/img7.png)|


## Prerequisites

- Openconnect v8.x
- Qt5, qt5-webengine, qt5-websockets

### Ubuntu
To install openconnect 8.08
```sh
bash <(curl -s https://raw.githubusercontent.com/inloco/GlobalProtect-openconnect/master/install/ubuntu-openconnect.sh)
```
To install client
```sh
bash <(curl -s https://raw.githubusercontent.com/inloco/GlobalProtect-openconnect/master/install/ubuntu.sh)
```
### OpenSUSE
Install the Qt dependencies

```sh
sudo zypper install libqt5-qtbase-devel libqt5-qtwebsockets-devel libqt5-qtwebengine-devel
```

### Fedora
To install openconnect 8.08
```sh
bash <(curl -s https://raw.githubusercontent.com/inloco/GlobalProtect-openconnect/master/install/fedora-openconnect.sh)
```
To install client
```sh
bash <(curl -s https://raw.githubusercontent.com/inloco/GlobalProtect-openconnect/master/install/fedora.sh)
```
## Install

### Install from AUR (Arch/Manjaro)

Install [globalprotect-openconnect](https://aur.archlinux.org/packages/globalprotect-openconnect/).

### Build from source code

```sh
git clone https://github.com/yuezk/GlobalProtect-openconnect.git
cd GlobalProtect-openconnect
git submodule update --init

# qmake or qmake-qt5
qmake CONFIG+=release
make
sudo make install
```
Open `GlobalProtect VPN` in the application dashboard.

## [License](./LICENSE)
GPLv3
