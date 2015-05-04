/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QLOCALE_TOOLS_P_H
#define QLOCALE_TOOLS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include "qlocale_p.h"
#include "qstring.h"

#if !defined(QT_QLOCALE_NEEDS_VOLATILE)
#  if defined(Q_CC_GNU)
#    if  __GNUC__ == 4
#      define QT_QLOCALE_NEEDS_VOLATILE
#    elif defined(Q_OS_WIN)
#      define QT_QLOCALE_NEEDS_VOLATILE
#    endif
#  endif
#endif

#if defined(QT_QLOCALE_NEEDS_VOLATILE)
#   define NEEDS_VOLATILE volatile
#else
#   define NEEDS_VOLATILE
#endif

QT_BEGIN_NAMESPACE

QString qulltoa(qulonglong l, int base, const QChar _zero);
QString qlltoa(qlonglong l, int base, const QChar zero);

enum PrecisionMode {
    PMDecimalDigits =             0x01,
    PMSignificantDigits =   0x02,
    PMChopTrailingZeros =   0x03
};

QString &decimalForm(QChar zero, QChar decimal, QChar group,
                     QString &digits, int decpt, uint precision,
                     PrecisionMode pm,
                     bool always_show_decpt,
                     bool thousands_group);
QString &exponentForm(QChar zero, QChar decimal, QChar exponential,
                      QChar group, QChar plus, QChar minus,
                      QString &digits, int decpt, uint precision,
                      PrecisionMode pm,
                      bool always_show_decpt);

inline bool isZero(double d)
{
    uchar *ch = (uchar *)&d;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return !(ch[0] & 0x7F || ch[1] || ch[2] || ch[3] || ch[4] || ch[5] || ch[6] || ch[7]);
    } else {
        return !(ch[7] & 0x7F || ch[6] || ch[5] || ch[4] || ch[3] || ch[2] || ch[1] || ch[0]);
    }
}

Q_CORE_EXPORT char *qdtoa(double d, int mode, int ndigits, int *decpt,
                          int *sign, char **rve, char **digits_str);
Q_CORE_EXPORT double qstrtod(const char *s00, char const **se, bool *ok);
qlonglong qstrtoll(const char *nptr, const char **endptr, int base, bool *ok);
qulonglong qstrtoull(const char *nptr, const char **endptr, int base, bool *ok);

QT_END_NAMESPACE

#endif
