# vim:expandtab:shiftwidth=4:softtabstop=4

COMPILE_JOBS=4
DEPLOY_DIR=`dirname $0`
STATIC=true

QT_VERSION=4.8.0
QT_FOLDER=$DEPLOY_DIR/Qt-$QT_VERSION
QT_TARBALL=$DEPLOY_DIR/qt-everywhere-opensource-src-$QT_VERSION.tar.gz
QT_URL=http://get.qt.nokia.com/qt/source/qt-everywhere-opensource-src-$QT_VERSION.tar.gz # Tip: change this to local/shared mirror

QT_CFG=''
QT_CFG+=' -opensource'          # Use the open-source license
QT_CFG+=' -confirm-license'     # Silently acknowledge the license confirmation
QT_CFG+=' -v'                   # Makes it easier to see what header dependencies are missing

QT_CFG+=' -release'             # Build only for release (no debugging support)
QT_CFG+=' -fast'                # Accelerate Makefiles generation
QT_CFG+=' -nomake demos'        # Don't build with the demos
QT_CFG+=' -nomake docs'         # Don't generate the documentatio
QT_CFG+=' -nomake examples'     # Don't build any examples
QT_CFG+=' -nomake translations' # Ignore the translations
QT_CFG+=' -nomake tools'        # Don't built the tools

QT_CFG+=' -no-exceptions'       # Don't use C++ exception
QT_CFG+=' -no-stl'              # No need for STL compatibility

# Irrelevant Qt features
QT_CFG+=' -no-libmng'
QT_CFG+=' -no-libtiff'

# Unnecessary Qt modules
QT_CFG+=' -no-declarative'
QT_CFG+=' -no-multimedia'
QT_CFG+=' -no-opengl'
QT_CFG+=' -no-openvg'
QT_CFG+=' -no-phonon'
QT_CFG+=' -no-qt3support'
QT_CFG+=' -no-script'
QT_CFG+=' -no-scripttools'
QT_CFG+=' -no-svg'
QT_CFG+=' -no-xmlpatterns'

# Sets the default graphics system to the raster engine
QT_CFG+=' -graphicssystem raster'

# Unix
QT_CFG+=' -no-dbus'             # Disable D-Bus feature
QT_CFG+=' -no-glib'             # No need for Glib integration
QT_CFG+=' -no-gstreamer'        # Turn off GStreamer support
QT_CFG+=' -no-gtkstyle'         # Disable theming integration with Gtk+
QT_CFG+=' -no-cups'             # Disable CUPs support
QT_CFG+=' -no-sm'
QT_CFG+=' -no-xinerama'
QT_CFG+=' -no-xkb'

# Use the bundled libraries, vs system-installed
QT_CFG+=' -qt-libjpeg'
QT_CFG+=' -qt-libpng'

# Explicitly compile with SSL support, so build will fail if headers are missing
QT_CFG+=' -openssl'

# Useless styles
QT_CFG+=' -D QT_NO_STYLESHEET'
QT_CFG+=' -D QT_NO_STYLE_CDE'
QT_CFG+=' -D QT_NO_STYLE_CLEANLOOKS'
QT_CFG+=' -D QT_NO_STYLE_MOTIF'
QT_CFG+=' -D QT_NO_STYLE_PLASTIQUE'

until [ -z "$1" ]; do
    case $1 in
        "--headless")
            QT_HEADLESS=1
            shift;;
        "--qt-4.8")
            QT_VERSION=4.8.0
            shift;;
        "--qt-4.7")
            QT_VERSION=4.7.4
            shift;;
        "--qt")
            shift

            if [ "$1" = '4.7' ]; then
                QT_VERSION=4.7.4
            else
                QT_VERSION=4.8.0
            fi

            shift;;
        "--qt-config")
            shift
            QT_CFG+=" $1"
            shift;;
        "--jobs")
            shift
            COMPILE_JOBS=$1
            shift;;
        "--help")
            echo "Usage: $0 [--headless] [--qt VERSION] [--qt-config CONFIG] [--jobs NUM]"
            echo
            echo "  --headless                  Build Qt in qpa mode so that no X server is required (Qt 4.8 only)"
            echo "  --qt VERSION                VERSION is 4.8 (default) or 4.7"
            echo "  --qt-config CONFIG          Specify extra config options to be used when configuring Qt"
            echo "  --jobs NUM                  How many parallel compile jobs to use. Defaults to 4."
            exit 0
            ;;
        *)
            echo "Unrecognised option: $1"
            exit 1;;
    esac
done

echo "Using Qt $QT_VERSION"

download_qt() {
    if [ ! -f $QT_TARBALL ]
    then
        echo "Downloading Qt $QT_VERSION from Nokia. Please wait..."
        if ! curl -C - -o $QT_TARBALL -S $QT_URL
        then
            echo
            echo "Fatal error: fail to download from $QT_URL !"
            exit 1
        fi
    fi
}

extract_qt() {
    [ -d $QT_FOLDER ] && rm -rf $QT_FOLDER
    echo "Extracting Qt $QT_VERSION source tarball..."
    echo
    tar -C $DEPLOY_DIR -xzf $QT_TARBALL
    mv $DEPLOY_DIR/qt-everywhere-opensource-src-$QT_VERSION $QT_FOLDER
}

apply_patches() {
  for p in $(ls $@); do
      echo
      echo "applying patch: $p"
      patch -p1 < $p
  done
}

patch_qt() {
    pushd $QT_FOLDER
    echo "Patching Qt"

    if [ $STATIC = 'true' ] ; then
        QT_CFG+=' -static'
    fi

    if [ $QT_VERSION = 4.8.0 ] ; then
        apply_patches ../qt-patches/4.8/*.patch

        # Build in lighthose mode for an x-less build
        if [[ "$QT_HEADLESS" == "1" && "$DISABLE_HEADLESS" != "1" ]] ; then
            echo "Building 4.8 in qpa headless mode"
            QT_CFG+=' -qpa '
        fi
    fi

    apply_patches ../qt-patches/all/*.patch

    # Tests don't build well with -static, but we don't need to build them
    rm -rf src/3rdparty/webkit/Source/WebKit/qt/tests

    popd
}

build_qt() {
    pushd $QT_FOLDER
    echo "Building Qt $QT_VERSION. Please wait..."
    echo
    echo "./configure -prefix $PWD $QT_CFG"
    ./configure -prefix $PWD $QT_CFG
    make -j$COMPILE_JOBS
    popd

    if [ $STATIC = 'true' ] ; then
        # Extra step to ensure the static libraries are found
        cp -rp $QT_FOLDER/src/3rdparty/webkit/Source/JavaScriptCore/release/* $QT_FOLDER/lib
        cp -rp $QT_FOLDER/src/3rdparty/webkit/Source/WebCore/release/* $QT_FOLDER/lib
    fi
}

build_phantomjs() {
    echo "Building PhantomJS. Please wait..."
    echo

    pushd $DEPLOY_DIR/..
    [ -f Makefile ] && make distclean
    deploy/Qt-$QT_VERSION/bin/qmake
    make -j$COMPILE_JOBS
    popd
}

compress_phantomjs() {
    echo "Compressing PhantomJS executable..."
    echo

    pushd $DEPLOY_DIR/..
    strip bin/phantomjs
    if [ `command -v upx` ]; then
        upx -9 bin/phantomjs
    else
        echo "You don't have UPX. Consider installing it to reduce the executable size."
    fi
    popd
}

# Memey meme is memey :)
BUILD_ALL_THE_THINGS() {
    download_qt
    extract_qt
    patch_qt
    build_qt
    build_phantomjs

    if [ $STATIC = 'true' ]; then
        compress_phantomjs
    fi
}
