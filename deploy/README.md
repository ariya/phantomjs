Packaging PhantomJS
===================

This directory contains various scripts to assist with making PhantomJS
packages.

Packaging for Linux
-------------------

Linux building/packaging is best done in a container to ensure
isolation.  We use [Docker](https://www.docker.com/) to automate the
process. Please see the [Docker documentation](https://docs.docker.com/)
for instructions on installing Docker. For OS X or Windows host,
please use [Docker Toolbox](https://www.docker.com/docker-toolbox).

Once you have Docker installed, run these commands from the top level
of the PhantomJS source repository:

```bash
 $ git clean -xfd .
 $ docker run -v $PWD:/src debian:wheezy /src/deploy/docker-build.sh
```

For the 32-bit version:

```bash
 $ git clean -xfd .
 $ docker run -v $PWD:/src tubia/debian:wheezy /src/deploy/docker-build.sh
```

The built binary will be extracted out of the container and copied to
the current directory.


Packaging for OS X
------------------

Run `deploy/build-and-package.sh`. That's it.

However, if you have previously built the sources in release mode, you
should clean your tree to make sure all the debugging symbols gets
compiled:

  $ make clean && cd src/qt && make clean && cd ../..
