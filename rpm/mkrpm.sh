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
buildroot="${topdir}/BUILDROOT"
mkdir -p ${topdir}/RPMS ${topdir}/SRPMS ${topdir}/SOURCES ${topdir}/BUILD
mkdir -p ${buildroot} ${builddir}
echo "=> Copying sources..."
( cd .. && tar cf - ./[A-Z]* ./bin ./examples | tar xf - -C ${builddir} )
echo "=> Creating source tarball under ${sourcedir}..."
( cd ${builddir}/.. && zip -r ${sourcedir}/${name}-${version}-source.zip ${name}-${version}
echo "=> Building RPM..."
rpmbuild --define "_topdir ${topdir}" --buildroot ${buildroot} --clean -bb ${name}.spec
RPMS=(`ls ${topdir}/RPMS/*/*.rpm`)
cp ${topdir}/RPMS/*/*.rpm 
rm -fr ${topdir}
for rpm in "${RPMS[@]}"; do
    echo ${TMPDIR:-/tmp}/${rpm##*/}
done
