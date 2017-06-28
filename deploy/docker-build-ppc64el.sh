#!/usr/bin/env bash

#
# sudo yum install -y docker
# sudo systemctl start docker.service
#
# OLDDIR=$(pwd)
# git clone https://github.com/ariya/phantomjs.git
# # see https://github.com/ariya/phantomjs/issues/14585
# git checkout 2.1
# pushd phantomjs/
# cp ${OLDDIR}/docker-build-ppc64el.sh deploy/
# sudo docker run -v $PWD:/src -it --privileged --name phantomjs_ppc64el docker.io/ppc64le/debian:jessie /src/deploy/docker-build-ppc64el.sh
# sudo docker cp phantomjs_ppc64el:/src/phantomjs ${HOME}/bin/
# sudo docker rm phantomjs_ppc64el
# popd
#

set -e

SOURCE_PATH=/src
BUILD_PATH=$HOME/build

echo "Installing packages for development tools..." && sleep 1
apt-get -y update
apt-get install -y build-essential g++-4.8 gcc-4.8 git flex bison gperf python ruby git libfontconfig1-dev
echo

USE_GCC_49=/bin/false
USE_GCC_48=/bin/true

if ${USE_GCC_48}
then
    echo "Setting gcc version 4.8 to have priority against the auto installed 4.9"
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.9 10
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 20
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 10
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 20
    # gcc-4.8 does not support -fstack-protector-strong
    export DEB_CFLAGS_SET=$(dpkg-buildflags --get CFLAGS | sed 's/-fstack-protector-strong//')
fi

echo "Preparing to download Debian source package..."
echo "deb-src http://deb.debian.org/debian oldstable main" >> /etc/apt/sources.list
apt-get -y update
echo

OPENSSL_TARGET='debian-ppc64el'
if [ `getconf LONG_BIT` -eq 32 ]; then
    OPENSSL_TARGET='linux-generic32'
fi
echo "Recompiling OpenSSL for ${OPENSSL_TARGET}..." && sleep 1
apt-get source openssl

cd openssl-1.0.1t/
OPENSSL_FLAGS='no-idea no-mdc2 no-rc5 no-zlib enable-tlsext no-ssl2 no-ssl3 no-ssl3-method enable-rfc3779 enable-cms'
./Configure --prefix=/usr --openssldir=/etc/ssl --libdir=lib ${OPENSSL_FLAGS} ${OPENSSL_TARGET}
make depend && make && make install
cd ..
echo

echo "Building the static version of ICU library..." && sleep 1
apt-get source icu
cd icu-52.1/source/
./configure --prefix=/usr --enable-static --disable-shared
make && make install
cd ..
echo

echo "Recreating the build directory $BUILD_PATH..."
rm -rf $BUILD_PATH && mkdir -p $BUILD_PATH
echo

echo "Transferring the source: $SOURCE_PATH -> $BUILD_PATH. Please wait..."
cd $BUILD_PATH && cp -rp $SOURCE_PATH . && cd src
echo

echo "Compiling PhantomJS..." && sleep 1
python build.py --confirm --release --qt-config="-no-pkg-config" --git-clean-qtbase --git-clean-qtwebkit
echo

echo "Stripping the executable..." && sleep 1
ls -l bin/phantomjs
strip bin/phantomjs
echo "Copying the executable..." && sleep 1
ls -l bin/phantomjs
cp bin/phantomjs $SOURCE_PATH
echo

echo "Finished."
