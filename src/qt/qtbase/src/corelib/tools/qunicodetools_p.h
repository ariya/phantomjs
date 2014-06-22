/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QUNICODETOOLS_P_H
#define QUNICODETOOLS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qchar.h>

QT_BEGIN_NAMESPACE

struct QCharAttributes
{
    uchar graphemeBoundary : 1;
    uchar wordBreak        : 1;
    uchar sentenceBoundary : 1;
    uchar lineBreak        : 1;
    uchar whiteSpace       : 1;
    uchar wordStart        : 1;
    uchar wordEnd          : 1;
    uchar mandatoryBreak   : 1;
};
Q_DECLARE_TYPEINFO(QCharAttributes, Q_PRIMITIVE_TYPE);

namespace QUnicodeTools {

// ### temporary
struct ScriptItem
{
    int position;
    int script;
};

enum CharAttributeOption {
    GraphemeBreaks = 0x01,
    WordBreaks = 0x02,
    SentenceBreaks = 0x04,
    LineBreaks = 0x08,
    WhiteSpaces = 0x10,
    DefaultOptionsCompat = GraphemeBreaks | LineBreaks | WhiteSpaces, // ### remove

    DontClearAttributes = 0x1000
};
Q_DECLARE_FLAGS(CharAttributeOptions, CharAttributeOption)

// attributes buffer has to have a length of string length + 1
Q_CORE_EXPORT void initCharAttributes(const ushort *string, int length,
                                      const ScriptItem *items, int numItems,
                                      QCharAttributes *attributes, CharAttributeOptions options = DefaultOptionsCompat);


Q_CORE_EXPORT void initScripts(const ushort *string, int length, uchar *scripts);

} // namespace QUnicodeTools

QT_END_NAMESPACE

#endif // QUNICODETOOLS_P_H
