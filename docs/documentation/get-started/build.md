---
layout: post
title: Build
categories: docs docs-get-started
permalink: build.html
---

Unless it is absolutely necessary to build PhantomJS from source, it is highly recommended to download and install the ready-made [binary package]({{ site.url }}/download.html) instead.

## Requirements

### Hardware requirements

* RAM: at least 4 GB
* Disk space: at least 3 GB
* CPU: 1.8 GHz, 4 cores or more

PhantomJS is still a web browser, albeit headless. Thus, building it from source takes a long time (mainly due to thousands of files in the WebKit module). Estimated build time for a 4-core system is 30 minutes. 

### Linux requirements

First, install the development packages of the following tools and libraries: GNU C++ compiler, bison, flex, gperf, Perl, Ruby, SQLite, FreeType, Fontconfig, OpenSSL, and ICU. The actual package names may vary from one distribution to another.

On Debian-based distro (tested on Ubuntu 14.04 and Debian 7.0), run:

```bash
sudo apt-get install build-essential g++ flex bison gperf ruby perl \
  libsqlite3-dev libfontconfig1-dev libicu-dev libfreetype6 libssl-dev \
  libpng-dev libjpeg-dev python libx11-dev libxext-dev
```

**Note**: It is recommend also to install `ttf-mscorefonts-installer` package.

On Fedora-based distro (tested on CentOS 6), run:

```bash
sudo yum -y install gcc gcc-c++ make flex bison gperf ruby \
  openssl-devel freetype-devel fontconfig-devel libicu-devel sqlite-devel \
  libpng-devel libjpeg-devel
```

### Windows requirements

Supported toolchains: `MSVC2012` and `MSVC2013`.

You must have Perl, Python, Ruby, and Git on `PATH`. Also, note that Git comes with it's own version of `perl.exe`. If you have both Git and a separate perl installation in your PATH, please make sure that you separate Perl install's `bin` folder comes before the git's `bin` folder in your PATH.

Please also add the folder `<phantomjs_path>\src\qt\3rdparty\gnuwin32\bin` to your `PATH`, as required tools such as `bison`, `flex`, and `gperf` will not be found otherwise.
Example:

```
SET PATH=%CD%\src\qt\3rdparty\gnuwin32\bin;%PATH%
```

Run the build script from Visual Studio Command Prompt.

**Tip**: Enabling incremental linking will make the linkage process faster.

### OS X requirements

* [Xcode](https://developer.apple.com/xcode/) and the necessary SDK for development (gcc, various tools, libraries, etc)
* OpenSSL via [Homebrew](http://brew.sh/) or via [MacPorts](https://www.macports.org/)

### FreeBSD requirements

Build PhantomJS from the FreeBSD Ports Collection: [svnweb.freebsd.org/ports/head/lang/phantomjs/](https://svnweb.freebsd.org/ports/head/lang/phantomjs/)

## Getting the Code

To obtain the code using [Git](http://git-scm.com/) from the official repository [github.com/ariya/phantomjs](https://github.com/ariya/phantomjs/):

```bash
git clone git://github.com/ariya/phantomjs.git
cd phantomjs
git checkout 2.1.1
git submodule init
git submodule update
```

## Compile and Link

From the code checkout:

```bash
python build.py
```

This will take some time. Once it is completed, the executable will be available under the `bin` subdirectory.

**Tip**: If the compilation process is interrupted, once started again the `build.py` script will continue where left off.
