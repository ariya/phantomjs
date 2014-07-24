/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#ifndef QCOLLATOR_H
#define QCOLLATOR_H

#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qlocale.h>

QT_BEGIN_NAMESPACE

class QCollatorPrivate;
class QCollatorSortKeyPrivate;

class Q_CORE_EXPORT QCollatorSortKey
{
    friend class QCollator;
public:
    QCollatorSortKey(const QCollatorSortKey &other);
    ~QCollatorSortKey();
    QCollatorSortKey &operator=(const QCollatorSortKey &other);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QCollatorSortKey &operator=(QCollatorSortKey &&other)
    { swap(other); return *this; }
#endif
    void swap(QCollatorSortKey &other)
    { d.swap(other.d); }

    int compare(const QCollatorSortKey &key) const;

protected:
    QCollatorSortKey(QCollatorSortKeyPrivate*);

    QSharedDataPointer<QCollatorSortKeyPrivate> d;

private:
    QCollatorSortKey();
};

inline bool operator<(const QCollatorSortKey &lhs, const QCollatorSortKey &rhs)
{
    return lhs.compare(rhs) < 0;
}

class Q_CORE_EXPORT QCollator
{
public:
    explicit QCollator(const QLocale &locale = QLocale());
    QCollator(const QCollator &);
    ~QCollator();
    QCollator &operator=(const QCollator &);
#ifdef Q_COMPILER_RVALUE_REFS
    QCollator(QCollator &&other)
        : d(other.d) { other.d = 0; }
    QCollator &operator=(QCollator &&other)
    { swap(other); return *this; }
#endif

    void swap(QCollator &other)
    { qSwap(d, other.d); }

    void setLocale(const QLocale &locale);
    QLocale locale() const;

    Qt::CaseSensitivity caseSensitivity() const;
    void setCaseSensitivity(Qt::CaseSensitivity cs);

    void setNumericMode(bool on);
    bool numericMode() const;

    void setIgnorePunctuation(bool on);
    bool ignorePunctuation() const;

    int compare(const QString &s1, const QString &s2) const;
    int compare(const QStringRef &s1, const QStringRef &s2) const;
    int compare(const QChar *s1, int len1, const QChar *s2, int len2) const;

    bool operator()(const QString &s1, const QString &s2) const
    { return compare(s1, s2) < 0; }

    QCollatorSortKey sortKey(const QString &string) const;

private:
    QCollatorPrivate *d;

    void detach();
};

Q_DECLARE_SHARED(QCollatorSortKey)
Q_DECLARE_SHARED(QCollator)

QT_END_NAMESPACE

#endif // QCOLLATOR_P_H
