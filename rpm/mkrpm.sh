#!/bin/sh
#
# A silly little helper script to build the RPM.
set -e

name=${1:?"Usage: build <toolname>"}
name=${name%.spec}
topdir=$(mktemp -d)
version=$(awk '/define version/ { print $NF }' ${name}.spec)
builddir=${TMPDIR:-/tmp}/${name}-${version}
sourcedir="${topdir}/SOURCES"
buildroot="${topdir}/BUILD/${name}-${version}-root"
mkdir -p ${topdir}/RPMS ${topdir}/SRPMS ${topdir}/SOURCES ${topdir}/BUILD
mkdir -p ${buildroot} ${builddir}
echo "=> Copying sources..."
( cd .. && tar cf - ./[A-Z]* ./bin ./examples | tar xf - -C ${builddir} )
echo "=> Creating source tarball under ${sourcedir}..."
( cd ${builddir}/.. && tar zcf ${sourcedir}/${name}-${version}.tar.gz ${name}-${version} )
echo "=> Building RPM..."
rpm=$(rpmbuild --define "_topdir ${topdir}" --buildroot ${buildroot} --clean -bb ${name}.spec 2>/dev/null | \
	awk '/\/RPMS\// { print $2; }')
cp ${rpm} ${TMPDIR:-/tmp}/
rm -fr ${topdir}
echo ${TMPDIR:-/tmp}/${rpm##*/}
