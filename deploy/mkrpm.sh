#!/bin/sh
#
# A silly little helper script to build the RPM.
set -e

startdir=`pwd`
name=phantomjs
topdir=$(mktemp -d)
version=$(awk '/define version/ { print $NF }' ${name}.spec)
builddir=${TMPDIR:-/tmp}/${name}-${version}
sourcedir="${topdir}/SOURCES"
buildroot="${topdir}/BUILD/${name}-${version}-root"
mkdir -p ${topdir}/RPMS ${topdir}/SRPMS ${topdir}/SOURCES ${topdir}/BUILD
mkdir -p ${buildroot} ${builddir}
echo "=> Copying sources..."
( cd .. && tar cf - . | tar xf - -C ${builddir} )
echo "=> Creating source tarball under ${sourcedir}..."
( cd ${builddir}/.. && tar zcf ${sourcedir}/${name}-${version}.tar.gz ${name}-${version} )
echo "=> Building RPM..."
yum-builddep -y phantomjs.spec
rpmbuild --define "_topdir ${topdir}" --buildroot ${buildroot} --clean -bb ${name}.spec
find ${topdir}/RPMS/ -name "*.rpm" -exec mv {} ${startdir} \;
rm -fr ${topdir}