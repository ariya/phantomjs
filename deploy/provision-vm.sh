#!/usr/bin/env bash

export PATH=$HOME/git/bin:$PATH

if type apt-get >/dev/null 2>&1; then
    apt-get update
    apt-get install -y build-essential git-core libssl-dev libfontconfig1-dev gdb binutils-gold
fi

if type yum >/dev/null 2>&1; then
    yum -y update
    yum -y install gcc gcc-c++ make openssl-devel freetype-devel fontconfig-devel
    if type git >/dev/null 2>&1; then
        echo "Git is already available."
    else
        yum -y install cpio expat-devel gettext-devel zlib-devel
        echo "Downloading and building git..."
        rm -rf git-*
        wget -nv https://git-core.googlecode.com/files/git-1.8.0.3.tar.gz
        tar -xzvf git-1.8.0.3.tar.gz
        cd git-1.8.0.3
        ./configure --prefix=$HOME/git && make -j2 && make install
        cd ..
        sleep 3
    fi
fi

if [[ ! -d phantomjs ]]; then
    git clone git://github.com/ariya/phantomjs.git
fi

cd phantomjs
git fetch origin
git reset --hard
git checkout $1

cp /vagrant/build-and-package.sh deploy/
cp /vagrant/package.sh deploy/

deploy/build-and-package.sh

cp deploy/*.tar.bz2 /vagrant
