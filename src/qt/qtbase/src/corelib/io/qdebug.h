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
        Stream(QIODevice *device) : ts(device), ref(1), type(QtDebugMsg), space(true), message_output(false), flags(0) {}
        Stream(QString *string) : ts(string, QIODevice::WriteOnly), ref(1), type(QtDebugMsg), space(true), message_output(false), flags(0) {}
        Stream(QtMsgType t) : ts(&buffer, QIODevice::WriteOnly), ref(1), type(t), space(true), message_output(true), flags(0) {}
        QTextStream ts;
        QString buffer;
        int ref;
        QtMsgType type;
        bool space;
        bool message_output;
        QMessageLogContext context;

        enum FormatFlag {
            NoQuotes = 0x1
        };

        // ### Qt 6: unify with space, introduce own version member
        bool testFlag(FormatFlag flag) const { return (context.version > 1) ? (flags & flag) : false; }
        void setFlag(FormatFlag flag) { if (context.version > 1) { flags |= flag; } }
        void unsetFlag(FormatFlag flag) { if (context.version > 1) { flags &= ~flag; } }

        // added in 5.4
        int flags;
    } *stream;
public:
    inline QDebug(QIODevice *device) : stream(new Stream(device)) {}
    inline QDebug(QString *string) : stream(new Stream(string)) {}
    inline QDebug(QtMsgType t) : stream(new Stream(t)) {}
    inline QDebug(const QDebug &o):stream(o.stream) { ++stream->ref; }
    inline QDebug &operator=(const QDebug &other);
    ~QDebug();
    inline void swap(QDebug &other) { qSwap(stream, other.stream); }

    QDebug &resetFormat();

    inline QDebug &space() { stream->space = true; stream->ts << ' '; return *this; }
    inline QDebug &nospace() { stream->space = false; return *this; }
    inline QDebug &maybeSpace() { if (stream->space) stream->ts << ' '; return *this; }

    bool autoInsertSpaces() const { return stream->space; }
    void setAutoInsertSpaces(bool b) { stream->space = b; }

    inline QDebug &quote() { stream->unsetFlag(Stream::NoQuotes); return *this; }
    inline QDebug &noquote() { stream->setFlag(Stream::NoQuotes); return *this; }
    inline QDebug &maybeQuote(char c = '"') { if (!(stream->testFlag(Stream::NoQuotes))) stream->ts << c; return *this; }

    inline QDebug &operator<<(QChar t) { maybeQuote('\''); stream->ts << t; maybeQuote('\''); return maybeSpace(); }
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
    inline QDebug &operator<<(const QString & t) { maybeQuote(); stream->ts << t; maybeQuote(); return maybeSpace(); }
    inline QDebug &operator<<(const QStringRef & t) { return operator<<(t.toString()); }
    inline QDebug &operator<<(QLatin1String t) { maybeQuote(); stream->ts << t; maybeQuote(); return maybeSpace(); }
    inline QDebug &operator<<(const QByteArray & t) { maybeQuote(); stream->ts << t; maybeQuote(); return maybeSpace(); }
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
    inline QNoDebug &quote() { return *this; }
    inline QNoDebug &noquote() { return *this; }
    inline QNoDebug &maybeQuote(const char = '"') { return *this; }

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

template <class T>
inline QDebug operator<<(QDebug debug, const QList<T> &list)
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

template <typename T>
inline QDebug operator<<(QDebug debug, const QVector<T> &vec)
{
    const bool oldSetting = debug.autoInsertSpaces();
    debug.nospace() << "QVector";
    debug.setAutoInsertSpaces(oldSetting);
    return operator<<(debug, vec.toList());
}

template <class aKey, class aT>
inline QDebug operator<<(QDebug debug, const QMap<aKey, aT> &map)
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

template <class aKey, class aT>
inline QDebug operator<<(QDebug debug, const QHash<aKey, aT> &hash)
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

template <class T1, class T2>
inline QDebug operator<<(QDebug debug, const QPair<T1, T2> &pair)
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

template <class T>
inline QDebug operator<<(QDebug debug, const QContiguousCache<T> &cache)
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

template <class T>
inline QDebug operator<<(QDebug debug, const QFlags<T> &flags)
{
    QDebugStateSaver saver(debug);
    debug.resetFormat();
    debug.nospace() << "QFlags(" << hex << showbase;
    bool needSeparator = false;
    for (uint i = 0; i < sizeof(T) * 8; ++i) {
        if (flags.testFlag(T(1 << i))) {
            if (needSeparator)
                debug << '|';
            else
                needSeparator = true;
            debug << (typename QFlags<T>::Int(1) << i);
        }
    }
    debug << ')';
    return debug;
}

QT_END_NAMESPACE

#endif // QDEBUG_H
