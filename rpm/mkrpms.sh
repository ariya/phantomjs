#!/bin/sh
#
# A silly little helper script to build the RPM.
set -e

phantomdir=$(basename $(dirname $(pwd)))
name=${1:?"Usage: build <toolname>"}
name=${name%.spec}
topdir=$(mktemp -d)
version=$(../bin/phantomjs --version | cut -d. -f1-2 )
release=$(../bin/phantomjs --version | cut -d. -f3 )
echo phantomjs ${version}.${release} found
builddir=${TMPDIR:-/tmp}/${name}-${version}
sourcedir="${topdir}/SOURCES"
mkdir -p ${topdir}/RPMS ${topdir}/SRPMS ${topdir}/SOURCES ${topdir}/BUILD ${topdir}/SPECS
mkdir -p ${builddir}
cp ${name}.spec ${topdir}/SPECS/.
echo "=> Copying sources ${sourcedir}"
( cd .. && tar cf - --exclude='rpm' ./[A-Z]* ./bin ./examples | tar xf - -C ${builddir} )
echo "=> Creating source tarball under ${sourcedir}"
( cd ${builddir}/.. && tar zcf ${sourcedir}/${name}-${version}.tar.gz ${name}-${version} )
echo "=> Building RPM"
rpm=$(rpmbuild --define "_topdir ${topdir}" --define "version ${version}" --define "release ${release}" --clean -bb ${name}.spec 2>/dev/null | \
	awk '/\/RPMS\// { print $2; }')
cp ${rpm} ${TMPDIR:-/tmp}/
rm -fr ${topdir}
echo ${TMPDIR:-/tmp}/${rpm##*/}
