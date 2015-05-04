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


#ifndef QMIMETYPEPARSER_P_H
#define QMIMETYPEPARSER_P_H

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

#include "qmimedatabase_p.h"
#include "qmimeprovider_p.h"

QT_BEGIN_NAMESPACE

class QIODevice;

class QMimeTypeParserBase
{
    Q_DISABLE_COPY(QMimeTypeParserBase)

public:
    QMimeTypeParserBase() {}
    virtual ~QMimeTypeParserBase() {}

    bool parse(QIODevice *dev, const QString &fileName, QString *errorMessage);

protected:
    virtual bool process(const QMimeType &t, QString *errorMessage) = 0;
    virtual bool process(const QMimeGlobPattern &t, QString *errorMessage) = 0;
    virtual void processParent(const QString &child, const QString &parent) = 0;
    virtual void processAlias(const QString &alias, const QString &name) = 0;
    virtual void processMagicMatcher(const QMimeMagicRuleMatcher &matcher) = 0;

private:
    enum ParseState {
        ParseBeginning,
        ParseMimeInfo,
        ParseMimeType,
        ParseComment,
        ParseGenericIcon,
        ParseIcon,
        ParseGlobPattern,
        ParseSubClass,
        ParseAlias,
        ParseMagic,
        ParseMagicMatchRule,
        ParseOtherMimeTypeSubTag,
        ParseError
    };

    static ParseState nextState(ParseState currentState, const QStringRef &startElement);
};


class QMimeTypeParser : public QMimeTypeParserBase
{
public:
    explicit QMimeTypeParser(QMimeXMLProvider &provider) : m_provider(provider) {}

protected:
    inline bool process(const QMimeType &t, QString *)
    { m_provider.addMimeType(t); return true; }

    inline bool process(const QMimeGlobPattern &glob, QString *)
    { m_provider.addGlobPattern(glob); return true; }

    inline void processParent(const QString &child, const QString &parent)
    { m_provider.addParent(child, parent); }

    inline void processAlias(const QString &alias, const QString &name)
    { m_provider.addAlias(alias, name); }

    inline void processMagicMatcher(const QMimeMagicRuleMatcher &matcher)
    { m_provider.addMagicMatcher(matcher); }

private:
    QMimeXMLProvider &m_provider;
};

QT_END_NAMESPACE

#endif // MIMETYPEPARSER_P_H
