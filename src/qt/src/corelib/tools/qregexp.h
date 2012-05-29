/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QREGEXP_H
#define QREGEXP_H

#ifndef QT_NO_REGEXP

#include <QtCore/qstring.h>
#ifdef QT3_SUPPORT
#include <new>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

struct QRegExpPrivate;
class QStringList;

class Q_CORE_EXPORT QRegExp
{
public:
    enum PatternSyntax {
        RegExp,
        Wildcard,
        FixedString,
        RegExp2,
        WildcardUnix,
        W3CXmlSchema11 };
    enum CaretMode { CaretAtZero, CaretAtOffset, CaretWontMatch };

    QRegExp();
    explicit QRegExp(const QString &pattern, Qt::CaseSensitivity cs = Qt::CaseSensitive,
		     PatternSyntax syntax = RegExp);
    QRegExp(const QRegExp &rx);
    ~QRegExp();
    QRegExp &operator=(const QRegExp &rx);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QRegExp &operator=(QRegExp &&other)
    { qSwap(priv,other.priv); return *this; }
#endif
    inline void swap(QRegExp &other) { qSwap(priv, other.priv); }

    bool operator==(const QRegExp &rx) const;
    inline bool operator!=(const QRegExp &rx) const { return !operator==(rx); }

    bool isEmpty() const;
    bool isValid() const;
    QString pattern() const;
    void setPattern(const QString &pattern);
    Qt::CaseSensitivity caseSensitivity() const;
    void setCaseSensitivity(Qt::CaseSensitivity cs);
#ifdef QT3_SUPPORT
    inline QT3_SUPPORT bool caseSensitive() const { return caseSensitivity() == Qt::CaseSensitive; }
    inline QT3_SUPPORT void setCaseSensitive(bool sensitive)
    { setCaseSensitivity(sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive); }
#endif
    PatternSyntax patternSyntax() const;
    void setPatternSyntax(PatternSyntax syntax);
#ifdef QT3_SUPPORT
    inline QT3_SUPPORT bool wildcard() const { return patternSyntax() == Wildcard; }
    inline QT3_SUPPORT void setWildcard(bool aWildcard)
    { setPatternSyntax(aWildcard ? Wildcard : RegExp); }
#endif

    bool isMinimal() const;
    void setMinimal(bool minimal);
#ifdef QT3_SUPPORT
    inline QT3_SUPPORT bool minimal() const { return isMinimal(); }
#endif

    bool exactMatch(const QString &str) const;

    int indexIn(const QString &str, int offset = 0, CaretMode caretMode = CaretAtZero) const;
    int lastIndexIn(const QString &str, int offset = -1, CaretMode caretMode = CaretAtZero) const;
#ifdef QT3_SUPPORT
    inline QT3_SUPPORT int search(const QString &str, int from = 0,
                                CaretMode caretMode = CaretAtZero) const
    { return indexIn(str, from, caretMode); }
    inline QT3_SUPPORT int searchRev(const QString &str, int from = -1,
                                   CaretMode caretMode = CaretAtZero) const
    { return lastIndexIn(str, from, caretMode); }
#endif
    int matchedLength() const;
#ifndef QT_NO_REGEXP_CAPTURE
#ifdef QT_DEPRECATED
    QT_DEPRECATED int numCaptures() const;
#endif
    int captureCount() const;
    QStringList capturedTexts() const;
    QStringList capturedTexts();
    QString cap(int nth = 0) const;
    QString cap(int nth = 0);
    int pos(int nth = 0) const;
    int pos(int nth = 0);
    QString errorString() const;
    QString errorString();
#endif

    static QString escape(const QString &str);

#ifdef QT3_SUPPORT
    inline QT3_SUPPORT_CONSTRUCTOR QRegExp(const QString &aPattern, bool cs, bool aWildcard = false)
    {
        new (this)
            QRegExp(aPattern, cs ? Qt::CaseSensitive : Qt::CaseInsensitive,
                    aWildcard ? Wildcard : RegExp);
    }
#endif

private:
    QRegExpPrivate *priv;
};

Q_DECLARE_TYPEINFO(QRegExp, Q_MOVABLE_TYPE);

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &out, const QRegExp &regExp);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &in, QRegExp &regExp);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_REGEXP

#endif // QREGEXP_H
