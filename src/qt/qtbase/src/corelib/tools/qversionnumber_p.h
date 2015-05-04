/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 Keith Gardner <kreios4004@gmail.com>
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

#ifndef QVERSIONNUMBER_H
#define QVERSIONNUMBER_H

#include <QtCore/qnamespace.h>
#include <QtCore/qstring.h>
#include <QtCore/qvector.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qtypeinfo.h>

QT_BEGIN_NAMESPACE

class QVersionNumber;
Q_CORE_EXPORT uint qHash(const QVersionNumber &key, uint seed = 0);

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream& operator<<(QDataStream &out, const QVersionNumber &version);
Q_CORE_EXPORT QDataStream& operator>>(QDataStream &in, QVersionNumber &version);
#endif

class Q_CORE_EXPORT QVersionNumber
{
public:
    inline QVersionNumber() Q_DECL_NOTHROW
        : m_segments()
    {}
    // compiler-generated copy/move ctor/assignment operators are ok

    inline explicit QVersionNumber(const QVector<int> &seg) Q_DECL_NOTHROW
        : m_segments(seg)
    {}
#ifdef Q_COMPILER_RVALUE_REFS
    inline explicit QVersionNumber(QVector<int> &&seg) Q_DECL_NOTHROW
        : m_segments(qMove(seg))
    {}
#endif
#ifdef Q_COMPILER_INITIALIZER_LISTS
    inline QVersionNumber(std::initializer_list<int> args)
        : m_segments(args)
    {}
#endif

    inline explicit QVersionNumber(int maj)
    { m_segments.reserve(1); m_segments << maj; }

    inline explicit QVersionNumber(int maj, int min)
    { m_segments.reserve(2); m_segments << maj << min; }

    inline explicit QVersionNumber(int maj, int min, int mic)
    { m_segments.reserve(3); m_segments << maj << min << mic; }

    inline bool isNull() const Q_DECL_NOTHROW Q_REQUIRED_RESULT
    { return m_segments.isEmpty(); }

    inline bool isNormalized() const Q_DECL_NOTHROW Q_REQUIRED_RESULT
    { return isNull() || m_segments.last() != 0; }

    inline int majorVersion() const Q_DECL_NOTHROW Q_REQUIRED_RESULT
    { return segmentAt(0); }

    inline int minorVersion() const Q_DECL_NOTHROW Q_REQUIRED_RESULT
    { return segmentAt(1); }

    inline int microVersion() const Q_DECL_NOTHROW Q_REQUIRED_RESULT
    { return segmentAt(2); }

#if defined(Q_COMPILER_REF_QUALIFIERS)
#  if defined(Q_CC_GNU)
    // required due to https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61941
#    pragma push_macro("Q_REQUIRED_RESULT")
#    undef Q_REQUIRED_RESULT
#    define Q_REQUIRED_RESULT
#    define Q_REQUIRED_RESULT_pushed
#  endif
    inline QVersionNumber normalized() const & Q_REQUIRED_RESULT
    {
        QVector<int> segs(m_segments);
        return normalizedImpl(segs);
    }

    inline QVersionNumber normalized() && Q_REQUIRED_RESULT
    {
        return normalizedImpl(m_segments);
    }

    inline QVector<int> segments() const & Q_DECL_NOTHROW Q_REQUIRED_RESULT
    { return m_segments; }

    inline QVector<int> segments() && Q_DECL_NOTHROW Q_REQUIRED_RESULT
    { return qMove(m_segments); }

#  ifdef Q_REQUIRED_RESULT_pushed
#    pragma pop_macro("Q_REQUIRED_RESULT")
#  endif
#else
    inline QVersionNumber normalized() const Q_REQUIRED_RESULT
    {
        QVector<int> segs(m_segments);
        return normalizedImpl(segs);
    }

    inline QVector<int> segments() const Q_DECL_NOTHROW Q_REQUIRED_RESULT
    { return m_segments; }
#endif

    inline int segmentAt(int index) const Q_DECL_NOTHROW Q_REQUIRED_RESULT
    { return (m_segments.size() > index) ? m_segments.at(index) : 0; }

    inline int segmentCount() const Q_DECL_NOTHROW Q_REQUIRED_RESULT
    { return m_segments.size(); }

    bool isPrefixOf(const QVersionNumber &other) const Q_DECL_NOTHROW Q_REQUIRED_RESULT;

    static int compare(const QVersionNumber &v1, const QVersionNumber &v2) Q_DECL_NOTHROW Q_REQUIRED_RESULT;

    static Q_DECL_PURE_FUNCTION QVersionNumber commonPrefix(const QVersionNumber &v1, const QVersionNumber &v2) Q_REQUIRED_RESULT;

    QString toString() const Q_REQUIRED_RESULT;
    static Q_DECL_PURE_FUNCTION QVersionNumber fromString(const QString &string, int *suffixIndex = 0) Q_REQUIRED_RESULT;

private:
    static QVersionNumber normalizedImpl(QVector<int> &segs) Q_REQUIRED_RESULT;

#ifndef QT_NO_DATASTREAM
    friend Q_CORE_EXPORT QDataStream& operator>>(QDataStream &in, QVersionNumber &version);
#endif
    friend Q_CORE_EXPORT uint qHash(const QVersionNumber &key, uint seed);

    QVector<int> m_segments;
};

Q_DECLARE_TYPEINFO(QVersionNumber, Q_MOVABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QVersionNumber &version);
#endif

Q_REQUIRED_RESULT inline bool operator> (const QVersionNumber &lhs, const QVersionNumber &rhs) Q_DECL_NOTHROW
{ return QVersionNumber::compare(lhs, rhs) > 0; }

Q_REQUIRED_RESULT inline bool operator>=(const QVersionNumber &lhs, const QVersionNumber &rhs) Q_DECL_NOTHROW
{ return QVersionNumber::compare(lhs, rhs) >= 0; }

Q_REQUIRED_RESULT inline bool operator< (const QVersionNumber &lhs, const QVersionNumber &rhs) Q_DECL_NOTHROW
{ return QVersionNumber::compare(lhs, rhs) < 0; }

Q_REQUIRED_RESULT inline bool operator<=(const QVersionNumber &lhs, const QVersionNumber &rhs) Q_DECL_NOTHROW
{ return QVersionNumber::compare(lhs, rhs) <= 0; }

Q_REQUIRED_RESULT inline bool operator==(const QVersionNumber &lhs, const QVersionNumber &rhs) Q_DECL_NOTHROW
{ return QVersionNumber::compare(lhs, rhs) == 0; }

Q_REQUIRED_RESULT inline bool operator!=(const QVersionNumber &lhs, const QVersionNumber &rhs) Q_DECL_NOTHROW
{ return QVersionNumber::compare(lhs, rhs) != 0; }

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QVersionNumber)

#endif //QVERSIONNUMBER_H
