#!/bin/bash

#
# usage: just run this script (after having run build.sh)
#        and deploy the created tarball to your target machine.
#
# It creates a phantomjs-$version folder and copies the binary,
# example, license etc. together with all shared library dependencies
# to that folder. Furthermore brandelf is used to make the lib
# and binary compatible with older unix/linux machines that don't
# know the new Linux ELF ABI.
#

cd $(dirname $0)

if [[ ! -f ../bin/phantomjs ]]; then
    echo "phantomjs was not built yet, please run build.sh first"
    exit 1
fi

version=$(../bin/phantomjs --version | sed 's/ /-/' | sed 's/[()]//g')
src=..

echo "packaging phantomjs $version"

if [[ $OSTYPE = darwin* ]]; then
    dest="phantomjs-$version-macosx-static"
else
    if [[ ! -f brandelf ]]; then
        echo
        echo "brandelf executable not found in current dir"
        echo -n "compiling it now..."
        g++ brandelf.c -o brandelf || exit 1
        echo "done"
    fi

    dest="phantomjs-$version-linux-$(uname -m)-dynamic"
fi

rm -Rf $dest{.tar.bz2,} &> /dev/null
mkdir -p $dest/bin

if [[ $OSTYPE != darwin* ]]; then
    mkdir -p $dest/lib
fi

echo

echo -n "copying files..."
cp $src/bin/phantomjs $dest/bin
cp -r $src/{ChangeLog,examples,LICENSE.BSD,README.md} $dest/
echo "done"
echo

if [[ $OSTYPE == darwin* ]]; then
    echo -n "compressing binary..."
    [ ! -z upx ] && upx -qqq -9 $dest/bin/phantomjs
    echo "done"
    echo
else
    echo -n "copying shared libs..."
    libld=
    for l in $(ldd $dest/bin/phantomjs | egrep -o "/[^ ]+ "); do
        if [[ "$l" != "" ]]; then
            ll=$(basename $l)
            cp $l $dest/lib/$ll
            # ensure OS ABI compatibility
            ./brandelf -t SVR4 $dest/lib/$ll
            if [[ "$l" == *"ld-linux"* ]]; then
                libld=$ll
            fi
        fi
    done
    echo "done"
    echo
fi

# strip to reduce file size
echo -n "stripping binary and libs..."

if [[ $OSTYPE = darwin* ]]; then
    strip -x $dest/bin/*
else
    strip -s $dest/lib/*  $dest/bin/*
fi

echo "done"
echo

if [[ $OSTYPE != darwin* ]]; then
    echo -n "writing run script..."
    # write run scripts
    mv $dest/bin/phantomjs $dest/bin/phantomjs.bin
    run=$dest/bin/phantomjs
    echo '#!/bin/sh' >> $run
    echo 'path=$(dirname $(dirname $(readlink -f $0)))' >> $run
    echo 'export LD_LIBRARY_PATH=$path/lib' >> $run
    echo 'exec $path/lib/'$libld' $path/bin/phantomjs.bin $@' >> $run
    chmod +x $run
    echo "done"
    echo
fi

echo -n "creating tarball..."
tar -cjf $dest{.tar.bz2,}
echo "done"
echo

echo "you can now deploy $dest or $dest.tar.bz2"
