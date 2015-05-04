#!/bin/bash

#############################################################################
##
## Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is the build configuration utility of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3.0 as published by the Free Software
## Foundation and appearing in the file LICENSE.GPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU General Public License version 3.0 requirements will be
## met: http://www.gnu.org/copyleft/gpl.html.
##
##
## $QT_END_LICENSE$
##
#############################################################################

if [ $# -ne 2 ]; then
    echo "$0: wrong number of arguments for internal tool used by iOS mkspec"
else
    arch_paths=""
    for a in $2; do
        arch_paths="$arch_paths $1/$a"
    done
    for f in $(find $arch_paths -name '*.o'); do
        # Skip object files without the _main symbol
        nm $f 2>/dev/null | grep -q 'T _main$' || continue

        fname=${f#$1/}

        file -b $f | grep -qi 'llvm bit-code' && \
            (cat \
<<EOF >&2
$f:: error: The file '$fname' contains LLVM bitcode, not object code. Automatic main() redirection could not be applied.
note: This is most likely due to the use of link-time optimization (-flto). Please disable LTO, or work around the \
issue by manually renaming your main() function to qtmn():

#ifdef Q_OS_IOS
extern "C" int qtmn(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
EOF
            ) && exit 1

        echo "Found main() in $fname"

        strings -t d - $f | grep '_main\(\.eh\)\?$' | while read match; do
            offset=$(echo $match | cut -d ' ' -f 1)
            symbol=$(echo $match | cut -d ' ' -f 2)

            echo "  Renaming '$symbol' at offset $offset to '${symbol/main/qtmn}'"

            # In-place rename the string (keeping the same length)
            printf '_qtmn' | dd of=$f bs=1 seek=$offset conv=notrunc >/dev/null 2>&1
        done
    done
fi