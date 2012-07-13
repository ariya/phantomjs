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

if [[ "$1" = "--bundle-libs" ]]; then
    bundle_libs=1
else
    bundle_libs=0
fi

version=$(../bin/phantomjs --version | sed 's/ /-/' | sed 's/[()]//g')
src=..

echo "packaging phantomjs $version"

if [[ $OSTYPE = darwin* ]]; then
    dest="phantomjs-$version-macosx-static"
else
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

phantomjs=$dest/bin/phantomjs

if [[ $OSTYPE != darwin* ]]; then
    if [[ "$bundle_libs" = "1" ]]; then
        if [[ ! -f brandelf ]]; then
            echo
            echo "brandelf executable not found in current dir"
            echo -n "compiling it now..."
            g++ brandelf.c -o brandelf || exit 1
            echo "done"
        fi

        libs=$(ldd $phantomjs | egrep -o "/[^ ]+ ")
    else
        libs=$(ldd $phantomjs | egrep "libQt" | egrep -o "/[^ ]+ ")
    fi

    echo -n "copying shared libs..."
    libld=
    for l in $libs; do
        ll=$(basename $l)
        cp $l $dest/lib/$ll

        if [[ "$bundle_libs" = "1" ]]; then
            # ensure OS ABI compatibility
            ./brandelf -t SVR4 $dest/lib/$ll
            if [[ "$l" == *"ld-linux"* ]]; then
                libld=$ll
            fi
        fi
    done
    echo "done"
    echo

    if [[ "$bundle_libs" = "1" ]]; then
        echo -n "writing run script..."
        mv $phantomjs $phantomjs.bin
        phantomjs=$phantomjs.bin
        run=$dest/bin/phantomjs
        echo '#!/bin/sh' >> $run
        echo 'path=$(dirname $(dirname $(readlink -f $0)))' >> $run
        echo 'export LD_LIBRARY_PATH=$path/lib' >> $run
        echo 'exec $path/lib/'$libld' $phantomjs $@' >> $run
        chmod +x $run
        echo "done"
        echo
    fi
fi

echo -n "stripping binary and libs..."
if [[ $OSTYPE = darwin* ]]; then
    strip -x $phantomjs
else
    strip -s $dest/lib/* $phantomjs
fi
echo "done"
echo

if [[ $OSTYPE == darwin* ]]; then
    echo -n "compressing binary..."
    if [[ ! -z upx ]]; then
        upx -qqq -9 $phantomjs
        echo "done"
    else
        echo "upx not found"
    fi
    echo
fi

echo -n "creating archive..."
if [[ $OSTYPE = darwin* ]]; then
    zip -r $dest.zip $dest
else
    tar -cjf $dest{.tar.bz2,}
fi
echo "done"
echo
