---
layout: post
title: Release Preparation
categories: docs docs-contribute
permalink: release-preparation.html
---

## Get the repository ready

Make sure the `ChangeLog` file is up to date.

Update the release number in `src/consts.h` and `test/phantom-spec.js`, e.g. "1.9.0" instead of "1.9.0 (development)".

Create a new release branch. Send an email to the mailing list for the heads up.

Make sure all examples run without problem and run all tests.

When it's a green light (there is no last-minute problem), tag the repo with the release number, e.g.

```bash
git tag -a 1.9.0 -m 'Version 1.9.0'
```

## Web site

The site is hosted using GitHub pages. The contents are in the `gh-pages` branch.

Prepare the release notes, e.g. `release-1.9.html`.

Update the information on the download page (`download.html`) and build instructions (`build.html`).

Update the version number on the main `index.html`.

## Packaging

### Source

Create a source tarball/zip of the latest stable VERSION and place it in the download location.

To generate ZIP:

```bash
git archive --format=zip --prefix=phantomjs-VERSION/ origin/VERSION > phantomjs-VERSION-source.zip
```

To generate TAR.GZ

```bash
git archive --format=tar upstream/VERSION --prefix=phantomjs-VERSION/ | gzip --stdout > phantomjs-VERSION-source.tar.gz
```

### Mac OS X

Make sure [UPX](http://upx.sf.net/) is installed. It is used to compress the executable.

Run the script:

```bash
deploy/build-and-package.sh
```

Upload **both** the package (-macosx.zip) and the symbols file (-macosx-symbols.tar.bz2) from the `deploy` directory.

### Linux

Linux package needs to be created using a virtualized CentoS 5.8 (`glibc 2.5`, lowest common denominator).

Install [Vagrant](http://www.vagrantup.com) and make sure it is running properly.

**Note**: Vagrant will automatically download and install CentOS base box. This may take a lot of bandwidth.

For the 32-bit package, run:

```bash
vagrant up i686
vagrant halt i686
```
For the 64-bit package, run:

```bash
vagrant up x86_64
vagrant halt x86_64
```

Upload **both** the package and the symbols file from the `deploy` directory.


### Windows

Build it based on the [usual instructions](http://phantomjs.org/build.html#windows).

Bundle it for deployment (copy paste following code into the command line window):

```bash
@rem Create deployment directoy and copy files into it
if exist %PHANTOMJSDEPLOYDIR% rd /s /q %PHANTOMJSDEPLOYDIR%
md %PHANTOMJSDEPLOYDIR%
xcopy %PHANTOMJSDIR%\bin\phantomjs.exe %PHANTOMJSDEPLOYDIR%
xcopy %PHANTOMJSDIR%\examples %PHANTOMJSDEPLOYDIR%\examples\
xcopy %PHANTOMJSDIR%\ChangeLog %PHANTOMJSDEPLOYDIR%
xcopy %PHANTOMJSDIR%\LICENSE.BSD %PHANTOMJSDEPLOYDIR%
xcopy %PHANTOMJSDIR%\third-party.txt %PHANTOMJSDEPLOYDIR%
xcopy %PHANTOMJSDIR%\README.md %PHANTOMJSDEPLOYDIR%
```

Pack `phantomjs.exe` with [UPX](http://upx.sf.net/):

```bash
upx %PHANTOMJSDEPLOYDIR%\phantomjs.exe
```

Zip the contents of `%PHANTOMJSDEPLOYDIR%`.
