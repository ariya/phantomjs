#!/bin/sh

PYTHONEXE=$(cygpath -u "${SYSTEMDRIVE}\\Python25\\python.exe")
ZIPNAME="cygwin-downloader.zip"

if [[ ! -f "${PYTHONEXE}" ]]; then
        echo "Couldn't find python.exe at ${PYTHONEXE}" 1>&2
        exit 1
fi

"${PYTHONEXE}" setup.py py2exe || {
        echo "Failed executing setup.py" 1>&2
        exit 1
}

rm -f "${ZIPNAME}"

cd dist

zip -r ../"${ZIPNAME}" * || {
        echo "Failed to create cygwin-downloader" 1>&2
        exit 1
}

cd ..

rm -rf build dist || {
        echo "Failed to cleanup cygwin-downloader and build directories" 1>&2
        exit 1
}
