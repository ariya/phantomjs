#!/usr/bin/env bash

set -e

SOURCE_PATH=/src
BUILD_PATH=$HOME/build

# In case the old package URL is still being used
sed -i 's/http\.debian\.net/httpredir\.debian\.org/g' /etc/apt/sources.list

echo "Installing packages for development tools..." && sleep 1
apt-get -y update
apt-get install -y build-essential git flex bison gperf python ruby git libfontconfig1-dev
echo

echo "Preparing to download Debian source package..."
echo "deb-src http://httpredir.debian.org/debian wheezy main" >> /etc/apt/sources.list
apt-get -y update
echo

OPENSSL_TARGET='linux-x86_64'
if [ `getconf LONG_BIT` -eq 32 ]; then
    OPENSSL_TARGET='linux-generic32'
fi
echo "Recompiling OpenSSL for ${OPENSSL_TARGET}..." && sleep 1
apt-get source openssl
cd openssl-1.0.1e
OPENSSL_FLAGS='no-idea no-mdc2 no-rc5 no-zlib enable-tlsext no-ssl2 no-ssl3 no-ssl3-method enable-rfc3779 enable-cms'
./Configure --prefix=/usr --openssldir=/etc/ssl --libdir=lib ${OPENSSL_FLAGS} ${OPENSSL_TARGET}
make depend && make && make install
cd ..
echo

echo "Building the static version of ICU library..." && sleep 1
apt-get source icu
cd icu-4.8.1.1/source
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
