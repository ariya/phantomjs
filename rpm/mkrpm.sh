#!/bin/sh
#
# A silly little helper script to build the RPM.
set -e

help="Usage: $0 <RPM spec file name> <final RPM destination absolute path>"
name=${1:?${help}}
name=${name%.spec}
destdir=${2:?${help}}
topdir=$(mktemp -d)
version=$(awk '/define version/ { print $NF }' ${name}.spec)
builddir=${TMPDIR:-/tmp}/${name}-${version}
sourcedir="${topdir}/SOURCES"
buildroot="${topdir}/BUILDROOT"
mkdir -p ${topdir}/RPMS ${topdir}/SRPMS ${topdir}/SOURCES ${topdir}/BUILD
mkdir -p ${buildroot} ${builddir} ${destdir}
echo "=> Copying sources..."
( cd .. && tar cf - ./[A-Z]* ./examples | tar xf - -C ${builddir} )
echo "=> Creating source tarball under ${sourcedir}..."
( cd ${builddir}/.. && zip -r ${sourcedir}/${name}-${version}-source.zip ${name}-${version} )
echo "=> Building RPM..."
rpmbuild --define "_topdir ${topdir}" --buildroot ${buildroot} --clean -bb ${name}.spec
RPMS=(`ls ${topdir}/RPMS/*/*.rpm`)
cp ${topdir}/RPMS/*/*.rpm ${destdir}/
rm -fr ${topdir} ${builddir}
for rpm in "${RPMS[@]}"; do
    echo ${TMPDIR:-/tmp}/${rpm##*/}
done
