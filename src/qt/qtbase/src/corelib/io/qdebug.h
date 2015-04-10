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

#ifndef QDEBUG_H
#define QDEBUG_H

#include <QtCore/qalgorithms.h>
#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qpair.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qstring.h>
#include <QtCore/qvector.h>
#include <QtCore/qset.h>
#include <QtCore/qcontiguouscache.h>

QT_BEGIN_NAMESPACE


class Q_CORE_EXPORT QDebug
{
    friend class QMessageLogger;
    friend class QDebugStateSaverPrivate;
    struct Stream {
        Stream(QIODevice *device) : ts(device), ref(1), type(QtDebugMsg), space(true), message_output(false) {}
        Stream(QString *string) : ts(string, QIODevice::WriteOnly), ref(1), type(QtDebugMsg), space(true), message_output(false) {}
        Stream(QtMsgType t) : ts(&buffer, QIODevice::WriteOnly), ref(1), type(t), space(true), message_output(true) {}
        QTextStream ts;
        QString buffer;
        int ref;
        QtMsgType type;
        bool space;
        bool message_output;
        QMessageLogContext context;
    } *stream;
public:
    inline QDebug(QIODevice *device) : stream(new Stream(device)) {}
    inline QDebug(QString *string) : stream(new Stream(string)) {}
    inline QDebug(QtMsgType t) : stream(new Stream(t)) {}
    inline QDebug(const QDebug &o):stream(o.stream) { ++stream->ref; }
    inline QDebug &operator=(const QDebug &other);
    inline ~QDebug() {
        if (!--stream->ref) {
            if (stream->space && stream->buffer.endsWith(QLatin1Char(' ')))
                stream->buffer.chop(1);
            if (stream->message_output) {
                QT_TRY {
                    qt_message_output(stream->type,
                                      stream->context,
                                      stream->buffer);
                } QT_CATCH(std::bad_alloc&) { /* We're out of memory - give up. */ }
            }
            delete stream;
        }
    }
    inline void swap(QDebug &other) { qSwap(stream, other.stream); }

    inline QDebug &space() { stream->space = true; stream->ts << ' '; return *this; }
    inline QDebug &nospace() { stream->space = false; return *this; }
    inline QDebug &maybeSpace() { if (stream->space) stream->ts << ' '; return *this; }

    bool autoInsertSpaces() const { return stream->space; }
    void setAutoInsertSpaces(bool b) { stream->space = b; }

    inline QDebug &operator<<(QChar t) { stream->ts << '\'' << t << '\''; return maybeSpace(); }
    inline QDebug &operator<<(bool t) { stream->ts << (t ? "true" : "false"); return maybeSpace(); }
    inline QDebug &operator<<(char t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(signed short t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(unsigned short t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(signed int t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(unsigned int t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(signed long t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(unsigned long t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(qint64 t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(quint64 t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(float t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(double t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(const char* t) { stream->ts << QString::fromUtf8(t); return maybeSpace(); }
    inline QDebug &operator<<(const QString & t) { stream->ts << '\"' << t  << '\"'; return maybeSpace(); }
    inline QDebug &operator<<(const QStringRef & t) { return operator<<(t.toString()); }
    inline QDebug &operator<<(QLatin1String t) { stream->ts << '\"'  << t << '\"'; return maybeSpace(); }
    inline QDebug &operator<<(const QByteArray & t) { stream->ts  << '\"' << t << '\"'; return maybeSpace(); }
    inline QDebug &operator<<(const void * t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(QTextStreamFunction f) {
        stream->ts << f;
        return *this;
    }

    inline QDebug &operator<<(QTextStreamManipulator m)
    { stream->ts << m; return *this; }
};

Q_DECLARE_SHARED(QDebug)

class QDebugStateSaverPrivate;
class Q_CORE_EXPORT QDebugStateSaver
{
public:
    QDebugStateSaver(QDebug &dbg);
    ~QDebugStateSaver();
private:
    Q_DISABLE_COPY(QDebugStateSaver)
    QScopedPointer<QDebugStateSaverPrivate> d;
};

class QNoDebug
{
public:
    inline QNoDebug &operator<<(QTextStreamFunction) { return *this; }
    inline QNoDebug &operator<<(QTextStreamManipulator) { return *this; }
    inline QNoDebug &space() { return *this; }
    inline QNoDebug &nospace() { return *this; }
    inline QNoDebug &maybeSpace() { return *this; }

    template<typename T>
    inline QNoDebug &operator<<(const T &) { return *this; }
};

inline QDebug &QDebug::operator=(const QDebug &other)
{
    if (this != &other) {
        QDebug copy(other);
        qSwap(stream, copy.stream);
    }
    return *this;
}

#if defined(FORCE_UREF)
template <class T>
inline QDebug &operator<<(QDebug debug, const QList<T> &list)
#else
template <class T>
inline QDebug operator<<(QDebug debug, const QList<T> &list)
#endif
{
    const bool oldSetting = debug.autoInsertSpaces();
    debug.nospace() << '(';
    for (typename QList<T>::size_type i = 0; i < list.count(); ++i) {
        if (i)
            debug << ", ";
        debug << list.at(i);
    }
    debug << ')';
    debug.setAutoInsertSpaces(oldSetting);
    return debug.maybeSpace();
}

#if defined(FORCE_UREF)
template <typename T>
inline QDebug &operator<<(QDebug debug, const QVector<T> &vec)
#else
template <typename T>
inline QDebug operator<<(QDebug debug, const QVector<T> &vec)
#endif
{
    const bool oldSetting = debug.autoInsertSpaces();
    debug.nospace() << "QVector";
    debug.setAutoInsertSpaces(oldSetting);
    return operator<<(debug, vec.toList());
}

#if defined(FORCE_UREF)
template <class aKey, class aT>
inline QDebug &operator<<(QDebug debug, const QMap<aKey, aT> &map)
#else
template <class aKey, class aT>
inline QDebug operator<<(QDebug debug, const QMap<aKey, aT> &map)
#endif
{
    const bool oldSetting = debug.autoInsertSpaces();
    debug.nospace() << "QMap(";
    for (typename QMap<aKey, aT>::const_iterator it = map.constBegin();
         it != map.constEnd(); ++it) {
        debug << '(' << it.key() << ", " << it.value() << ')';
    }
    debug << ')';
    debug.setAutoInsertSpaces(oldSetting);
    return debug.maybeSpace();
}

#if defined(FORCE_UREF)
template <class aKey, class aT>
inline QDebug &operator<<(QDebug debug, const QHash<aKey, aT> &hash)
#else
template <class aKey, class aT>
inline QDebug operator<<(QDebug debug, const QHash<aKey, aT> &hash)
#endif
{
    const bool oldSetting = debug.autoInsertSpaces();
    debug.nospace() << "QHash(";
    for (typename QHash<aKey, aT>::const_iterator it = hash.constBegin();
            it != hash.constEnd(); ++it)
        debug << '(' << it.key() << ", " << it.value() << ')';
    debug << ')';
    debug.setAutoInsertSpaces(oldSetting);
    return debug.maybeSpace();
}

#if defined(FORCE_UREF)
template <class T1, class T2>
inline QDebug &operator<<(QDebug debug, const QPair<T1, T2> &pair)
#else
template <class T1, class T2>
inline QDebug operator<<(QDebug debug, const QPair<T1, T2> &pair)
#endif
{
    const bool oldSetting = debug.autoInsertSpaces();
    debug.nospace() << "QPair(" << pair.first << ',' << pair.second << ')';
    debug.setAutoInsertSpaces(oldSetting);
    return debug.maybeSpace();
}

template <typename T>
inline QDebug operator<<(QDebug debug, const QSet<T> &set)
{
    const bool oldSetting = debug.autoInsertSpaces();
    debug.nospace() << "QSet";
    debug.setAutoInsertSpaces(oldSetting);
    return operator<<(debug, set.toList());
}

#if defined(FORCE_UREF)
template <class T>
inline QDebug &operator<<(QDebug debug, const QContiguousCache<T> &cache)
#else
template <class T>
inline QDebug operator<<(QDebug debug, const QContiguousCache<T> &cache)
#endif
{
    const bool oldSetting = debug.autoInsertSpaces();
    debug.nospace() << "QContiguousCache(";
    for (int i = cache.firstIndex(); i <= cache.lastIndex(); ++i) {
        debug << cache[i];
        if (i != cache.lastIndex())
            debug << ", ";
    }
    debug << ')';
    debug.setAutoInsertSpaces(oldSetting);
    return debug.maybeSpace();
}

#if defined(FORCE_UREF)
template <class T>
inline QDebug &operator<<(QDebug debug, const QFlags<T> &flags)
#else
template <class T>
inline QDebug operator<<(QDebug debug, const QFlags<T> &flags)
#endif
{
    const bool oldSetting = debug.autoInsertSpaces();
    debug.nospace() << "QFlags(";
    bool needSeparator = false;
    for (uint i = 0; i < sizeof(T) * 8; ++i) {
        if (flags.testFlag(T(1 << i))) {
            if (needSeparator)
                debug.nospace() << '|';
            else
                needSeparator = true;
            debug.nospace() << "0x" << QByteArray::number(typename QFlags<T>::Int(1) << i, 16).constData();
        }
    }
    debug << ')';
    debug.setAutoInsertSpaces(oldSetting);
    return debug.maybeSpace();
}

QT_END_NAMESPACE

#endif // QDEBUG_H
