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

#ifndef QMIMEGLOBPATTERN_P_H
#define QMIMEGLOBPATTERN_P_H

#include <QtCore/qstringlist.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

struct QMimeGlobMatchResult
{
    QMimeGlobMatchResult()
    : m_weight(0), m_matchingPatternLength(0)
    {}

    void addMatch(const QString &mimeType, int weight, const QString &pattern);

    QStringList m_matchingMimeTypes;
    int m_weight;
    int m_matchingPatternLength;
    QString m_foundSuffix;
};

class QMimeGlobPattern
{
public:
    static const unsigned MaxWeight = 100;
    static const unsigned DefaultWeight = 50;
    static const unsigned MinWeight = 1;

    explicit QMimeGlobPattern(const QString &thePattern, const QString &theMimeType, unsigned theWeight = DefaultWeight, Qt::CaseSensitivity s = Qt::CaseInsensitive) :
        m_pattern(thePattern), m_mimeType(theMimeType), m_weight(theWeight), m_caseSensitivity(s)
    {
        if (s == Qt::CaseInsensitive) {
            m_pattern = m_pattern.toLower();
        }
    }
    ~QMimeGlobPattern() {}

    bool matchFileName(const QString &filename) const;

    inline const QString &pattern() const { return m_pattern; }
    inline unsigned weight() const { return m_weight; }
    inline const QString &mimeType() const { return m_mimeType; }
    inline bool isCaseSensitive() const { return m_caseSensitivity == Qt::CaseSensitive; }

private:
    QString m_pattern;
    QString m_mimeType;
    int m_weight;
    Qt::CaseSensitivity m_caseSensitivity;
};

class QMimeGlobPatternList : public QList<QMimeGlobPattern>
{
public:
    bool hasPattern(const QString &mimeType, const QString &pattern) const
    {
        const_iterator it = begin();
        const const_iterator myend = end();
        for (; it != myend; ++it)
            if ((*it).pattern() == pattern && (*it).mimeType() == mimeType)
                return true;
        return false;
    }

    /*!
        "noglobs" is very rare occurrence, so it's ok if it's slow
     */
    void removeMimeType(const QString &mimeType)
    {
        QMutableListIterator<QMimeGlobPattern> it(*this);
        while (it.hasNext()) {
            if (it.next().mimeType() == mimeType)
                it.remove();
        }
    }

    void match(QMimeGlobMatchResult &result, const QString &fileName) const;
};

/*!
    Result of the globs parsing, as data structures ready for efficient MIME type matching.
    This contains:
    1) a map of fast regular patterns (e.g. *.txt is stored as "txt" in a qhash's key)
    2) a linear list of high-weight globs
    3) a linear list of low-weight globs
 */
class QMimeAllGlobPatterns
{
public:
    typedef QHash<QString, QStringList> PatternsMap; // MIME type -> patterns

    void addGlob(const QMimeGlobPattern &glob);
    void removeMimeType(const QString &mimeType);
    QStringList matchingGlobs(const QString &fileName, QString *foundSuffix) const;
    void clear();

    PatternsMap m_fastPatterns; // example: "doc" -> "application/msword", "text/plain"
    QMimeGlobPatternList m_highWeightGlobs;
    QMimeGlobPatternList m_lowWeightGlobs; // <= 50, including the non-fast 50 patterns
};

QT_END_NAMESPACE

#endif // QMIMEGLOBPATTERN_P_H
