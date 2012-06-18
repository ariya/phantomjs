Packaging PhantomJS
===================

This directory contains various scripts to assist with making PhantomJS
packages.

Packaging for Linux
-------------------

Linux building/packaging is best done in a virtual machine to ensure
isolation and clean state. This is also necessary to build for different
architectures.

We use [Vagrant](http://vagrantup.com/) to help with this. Please see
the [Vagrant
documentation](http://vagrantup.com/v1/docs/getting-started/index.html)
for instructions on how to install VirtualBox and Vagrant.

Once you have Vagrant installed, building should be as simple as
running:

    $ export PHANTOMJS_VERSION=1.6.0 # change as necessary
    $ vagrant up $ARCH

Where $ARCH is either `i686` or `x86_64`.

This runs the `provision_vm.sh` script, which installs the necessary
dependencies, checks out a fresh copy of the PhantomJS repository,
switches to the relevant tag, builds and packages the software and the
associated debugging symbols tarball, and copies the tarballs out of the
VM onto your host machine.

If it runs successfully, you will see the tarballs in this directory,
ready for upload.

If there are any problems, you can re-run the script with:

    $ vagrant provision $ARCH

Or SSH into the VM:

    $ vagrant ssh $ARCH

Once you're done, you can destroy the VM with:

    $ vagrant destroy $ARCH

If you need to build a new version, you should destroy the VM and start
again to ensure a clean state. (Or SSH in and do a git clean.)

Packaging for OS X
------------------

Run `deploy/build-and-package.sh`. That's it.

However, if you have previously built the sources in release mode, you
should clean your tree to make sure all the debugging symbols gets
compiled:

    $ make clean && cd src/qt && make clean && cd ../..
