/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the utils of the Qt Toolkit.
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

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QHash>
#include <QDataStream>
#include <QSet>

#include "configfile.h"

#if 1
typedef int InputType;

enum SpecialInputType {
    DigitInput,
    SpaceInput,
    Letter
};

#else

enum SpecialInputType {
    NoSpecialInput = 0,
    DigitInput,
    SpaceInput,
    LetterOrNumberInput
};

struct InputType
{
    inline InputType() : val(0), specialInput(NoSpecialInput) {}
    inline InputType(const int &val) : val(val), specialInput(NoSpecialInput) {}

    inline operator int() const { return val; }

    inline bool operator==(const InputType &other) const
    { return val == other.val; }
    inline bool operator!=(const InputType &other) const
    { return val != other.val; }

    int val;
    SpecialInputType specialInput;
};

inline int qHash(const InputType &t) { return qHash(t.val); }

inline QDataStream &operator<<(QDataStream &stream, const InputType &i)
{
    return stream << i;
}

inline QDataStream &operator>>(QDataStream &stream, InputType &i)
{
    return stream >> i;
}

#endif

const InputType Epsilon = -1;

struct Config
{
    inline Config() : caseSensitivity(Qt::CaseSensitive), debug(false), cache(false) {}
    QSet<InputType> maxInputSet;
    Qt::CaseSensitivity caseSensitivity;
    QString className;
    bool debug;
    bool cache;
    QString ruleFile;
    ConfigFile::SectionMap configSections;
};

#endif // GLOBAL_H
