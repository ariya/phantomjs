/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QNUMERIC_P_H
#define QNUMERIC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qglobal.h"

QT_BEGIN_NAMESPACE

#if !defined(Q_CC_MIPS)

static const union { unsigned char c[8]; double d; } qt_be_inf_bytes = { { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 } };
static const union { unsigned char c[8]; double d; } qt_le_inf_bytes = { { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f } };
static const union { unsigned char c[8]; double d; } qt_armfpa_inf_bytes = { { 0, 0, 0xf0, 0x7f, 0, 0, 0, 0 } };
static inline double qt_inf()
{
#ifdef QT_ARMFPA
    return qt_armfpa_inf_bytes.d;
#else
    return (QSysInfo::ByteOrder == QSysInfo::BigEndian
            ? qt_be_inf_bytes.d
            : qt_le_inf_bytes.d);
#endif
}

// Signaling NAN
static const union { unsigned char c[8]; double d; } qt_be_snan_bytes = { { 0x7f, 0xf8, 0, 0, 0, 0, 0, 0 } };
static const union { unsigned char c[8]; double d; } qt_le_snan_bytes = { { 0, 0, 0, 0, 0, 0, 0xf8, 0x7f } };
static const union { unsigned char c[8]; double d; } qt_armfpa_snan_bytes = { { 0, 0, 0xf8, 0x7f, 0, 0, 0, 0 } };
static inline double qt_snan()
{
#ifdef QT_ARMFPA
    return qt_armfpa_snan_bytes.d;
#else
    return (QSysInfo::ByteOrder == QSysInfo::BigEndian
            ? qt_be_snan_bytes.d
            : qt_le_snan_bytes.d);
#endif
}

// Quiet NAN
static const union { unsigned char c[8]; double d; } qt_be_qnan_bytes = { { 0xff, 0xf8, 0, 0, 0, 0, 0, 0 } };
static const union { unsigned char c[8]; double d; } qt_le_qnan_bytes = { { 0, 0, 0, 0, 0, 0, 0xf8, 0xff } };
static const union { unsigned char c[8]; double d; } qt_armfpa_qnan_bytes = { { 0, 0, 0xf8, 0xff, 0, 0, 0, 0 } };
static inline double qt_qnan()
{
#ifdef QT_ARMFPA
    return qt_armfpa_qnan_bytes.d;
#else
    return (QSysInfo::ByteOrder == QSysInfo::BigEndian
            ? qt_be_qnan_bytes.d
            : qt_le_qnan_bytes.d);
#endif
}

#else // Q_CC_MIPS

static const unsigned char qt_be_inf_bytes[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
static const unsigned char qt_le_inf_bytes[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
static const unsigned char qt_armfpa_inf_bytes[] = { 0, 0, 0xf0, 0x7f, 0, 0, 0, 0 };
static inline double qt_inf()
{
    const unsigned char *bytes;
#ifdef QT_ARMFPA
    bytes = qt_armfpa_inf_bytes;
#else
    bytes = (QSysInfo::ByteOrder == QSysInfo::BigEndian
             ? qt_be_inf_bytes
             : qt_le_inf_bytes);
#endif

    union { unsigned char c[8]; double d; } returnValue;
    qMemCopy(returnValue.c, bytes, sizeof(returnValue.c));
    return returnValue.d;
}

// Signaling NAN
static const unsigned char qt_be_snan_bytes[] = { 0x7f, 0xf8, 0, 0, 0, 0, 0, 0 };
static const unsigned char qt_le_snan_bytes[] = { 0, 0, 0, 0, 0, 0, 0xf8, 0x7f };
static const unsigned char qt_armfpa_snan_bytes[] = { 0, 0, 0xf8, 0x7f, 0, 0, 0, 0 };
static inline double qt_snan()
{
    const unsigned char *bytes;
#ifdef QT_ARMFPA
    bytes = qt_armfpa_snan_bytes;
#else
    bytes = (QSysInfo::ByteOrder == QSysInfo::BigEndian
             ? qt_be_snan_bytes
             : qt_le_snan_bytes);
#endif

    union { unsigned char c[8]; double d; } returnValue;
    qMemCopy(returnValue.c, bytes, sizeof(returnValue.c));
    return returnValue.d;
}

// Quiet NAN
static const unsigned char qt_be_qnan_bytes[] = { 0xff, 0xf8, 0, 0, 0, 0, 0, 0 };
static const unsigned char qt_le_qnan_bytes[] = { 0, 0, 0, 0, 0, 0, 0xf8, 0xff };
static const unsigned char qt_armfpa_qnan_bytes[] = { 0, 0, 0xf8, 0xff, 0, 0, 0, 0 };
static inline double qt_qnan()
{
    const unsigned char *bytes;
#ifdef QT_ARMFPA
    bytes = qt_armfpa_qnan_bytes;
#else
    bytes = (QSysInfo::ByteOrder == QSysInfo::BigEndian
             ? qt_be_qnan_bytes
             : qt_le_qnan_bytes);
#endif

    union { unsigned char c[8]; double d; } returnValue;
    qMemCopy(returnValue.c, bytes, sizeof(returnValue.c));
    return returnValue.d;
}

#endif // Q_CC_MIPS

static inline bool qt_is_inf(double d)
{
    uchar *ch = (uchar *)&d;
#ifdef QT_ARMFPA
    return (ch[3] & 0x7f) == 0x7f && ch[2] == 0xf0;
#else
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return (ch[0] & 0x7f) == 0x7f && ch[1] == 0xf0;
    } else {
        return (ch[7] & 0x7f) == 0x7f && ch[6] == 0xf0;
    }
#endif
}

static inline bool qt_is_nan(double d)
{
    uchar *ch = (uchar *)&d;
#ifdef QT_ARMFPA
    return (ch[3] & 0x7f) == 0x7f && ch[2] > 0xf0;
#else
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return (ch[0] & 0x7f) == 0x7f && ch[1] > 0xf0;
    } else {
        return (ch[7] & 0x7f) == 0x7f && ch[6] > 0xf0;
    }
#endif
}

static inline bool qt_is_finite(double d)
{
    uchar *ch = (uchar *)&d;
#ifdef QT_ARMFPA
    return (ch[3] & 0x7f) != 0x7f || (ch[2] & 0xf0) != 0xf0;
#else
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return (ch[0] & 0x7f) != 0x7f || (ch[1] & 0xf0) != 0xf0;
    } else {
        return (ch[7] & 0x7f) != 0x7f || (ch[6] & 0xf0) != 0xf0;
    }
#endif
}

static inline bool qt_is_inf(float d)
{
    uchar *ch = (uchar *)&d;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return (ch[0] & 0x7f) == 0x7f && ch[1] == 0x80;
    } else {
        return (ch[3] & 0x7f) == 0x7f && ch[2] == 0x80;
    }
}

static inline bool qt_is_nan(float d)
{
    uchar *ch = (uchar *)&d;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return (ch[0] & 0x7f) == 0x7f && ch[1] > 0x80;
    } else {
        return (ch[3] & 0x7f) == 0x7f && ch[2] > 0x80;
    }
}

static inline bool qt_is_finite(float d)
{
    uchar *ch = (uchar *)&d;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        return (ch[0] & 0x7f) != 0x7f || (ch[1] & 0x80) != 0x80;
    } else {
        return (ch[3] & 0x7f) != 0x7f || (ch[2] & 0x80) != 0x80;
    }
}

QT_END_NAMESPACE

#endif // QNUMERIC_P_H
