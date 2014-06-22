/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake application of the Qt Toolkit.
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

#ifndef PROITEMS_H
#define PROITEMS_H

#include "qmake_global.h"

#include <qstring.h>
#include <qvector.h>
#include <qhash.h>

QT_BEGIN_NAMESPACE

class QTextStream;

#ifdef PROPARSER_THREAD_SAFE
typedef QAtomicInt ProItemRefCount;
#else
class ProItemRefCount {
public:
    ProItemRefCount(int cnt = 0) : m_cnt(cnt) {}
    bool ref() { return ++m_cnt != 0; }
    bool deref() { return --m_cnt != 0; }
    ProItemRefCount &operator=(int value) { m_cnt = value; return *this; }
private:
    int m_cnt;
};
#endif

#ifndef QT_BUILD_QMAKE
#  define PROITEM_EXPLICIT explicit
#else
#  define PROITEM_EXPLICIT
#endif

class ProKey;
class ProStringList;
class ProFile;

class ProString {
public:
    ProString();
    ProString(const ProString &other);
    PROITEM_EXPLICIT ProString(const QString &str);
    PROITEM_EXPLICIT ProString(const char *str);
    ProString(const QString &str, int offset, int length);
    void setValue(const QString &str);
    void clear() { m_string.clear(); m_length = 0; }
    ProString &setSource(const ProString &other) { m_file = other.m_file; return *this; }
    ProString &setSource(const ProFile *pro) { m_file = pro; return *this; }
    const ProFile *sourceFile() const { return m_file; }

    ProString &prepend(const ProString &other);
    ProString &append(const ProString &other, bool *pending = 0);
    ProString &append(const QString &other) { return append(ProString(other)); }
    ProString &append(const QLatin1String other);
    ProString &append(const char *other) { return append(QLatin1String(other)); }
    ProString &append(QChar other);
    ProString &append(const ProStringList &other, bool *pending = 0, bool skipEmpty1st = false);
    ProString &operator+=(const ProString &other) { return append(other); }
    ProString &operator+=(const QString &other) { return append(other); }
    ProString &operator+=(const QLatin1String other) { return append(other); }
    ProString &operator+=(const char *other) { return append(other); }
    ProString &operator+=(QChar other) { return append(other); }

    void chop(int n) { Q_ASSERT(n <= m_length); m_length -= n; }
    void chopFront(int n) { Q_ASSERT(n <= m_length); m_offset += n; m_length -= n; }

    bool operator==(const ProString &other) const { return toQStringRef() == other.toQStringRef(); }
    bool operator==(const QString &other) const { return toQStringRef() == other; }
    bool operator==(QLatin1String other) const  { return toQStringRef() == other; }
    bool operator==(const char *other) const { return toQStringRef() == QLatin1String(other); }
    bool operator!=(const ProString &other) const { return !(*this == other); }
    bool operator!=(const QString &other) const { return !(*this == other); }
    bool operator!=(QLatin1String other) const { return !(*this == other); }
    bool operator!=(const char *other) const { return !(*this == other); }
    bool isNull() const { return m_string.isNull(); }
    bool isEmpty() const { return !m_length; }
    int length() const { return m_length; }
    int size() const { return m_length; }
    QChar at(int i) const { Q_ASSERT((uint)i < (uint)m_length); return constData()[i]; }
    const QChar *constData() const { return m_string.constData() + m_offset; }
    ProString mid(int off, int len = -1) const;
    ProString left(int len) const { return mid(0, len); }
    ProString right(int len) const { return mid(qMax(0, size() - len)); }
    ProString trimmed() const;
    int compare(const ProString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().compare(sub.toQStringRef(), cs); }
    int compare(const QString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().compare(sub, cs); }
    int compare(const char *sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().compare(QLatin1String(sub), cs); }
    bool startsWith(const ProString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().startsWith(sub.toQStringRef(), cs); }
    bool startsWith(const QString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().startsWith(sub, cs); }
    bool startsWith(const char *sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().startsWith(QLatin1String(sub), cs); }
    bool startsWith(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().startsWith(c, cs); }
    bool endsWith(const ProString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().endsWith(sub.toQStringRef(), cs); }
    bool endsWith(const QString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().endsWith(sub, cs); }
    bool endsWith(const char *sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().endsWith(QLatin1String(sub), cs); }
    bool endsWith(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().endsWith(c, cs); }
    int indexOf(const QString &s, int from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().indexOf(s, from, cs); }
    int indexOf(const char *s, int from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().indexOf(QLatin1String(s), from, cs); }
    int indexOf(QChar c, int from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().indexOf(c, from, cs); }
    int lastIndexOf(const QString &s, int from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().lastIndexOf(s, from, cs); }
    int lastIndexOf(const char *s, int from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().lastIndexOf(QLatin1String(s), from, cs); }
    int lastIndexOf(QChar c, int from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toQStringRef().lastIndexOf(c, from, cs); }
    bool contains(const QString &s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return indexOf(s, 0, cs) >= 0; }
    bool contains(const char *s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return indexOf(QLatin1String(s), 0, cs) >= 0; }
    bool contains(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return indexOf(c, 0, cs) >= 0; }
    int toInt(bool *ok = 0, int base = 10) const { return toQString().toInt(ok, base); } // XXX optimize
    short toShort(bool *ok = 0, int base = 10) const { return toQString().toShort(ok, base); } // XXX optimize

    static uint hash(const QChar *p, int n);

    ALWAYS_INLINE QStringRef toQStringRef() const { return QStringRef(&m_string, m_offset, m_length); }

    ALWAYS_INLINE ProKey &toKey() { return *(ProKey *)this; }
    ALWAYS_INLINE const ProKey &toKey() const { return *(const ProKey *)this; }

    QString toQString() const;
    QString &toQString(QString &tmp) const;

    QByteArray toLatin1() const { return toQStringRef().toLatin1(); }

private:
    ProString(const ProKey &other);
    ProString &operator=(const ProKey &other);

    enum OmitPreHashing { NoHash };
    ProString(const ProString &other, OmitPreHashing);

    enum DoPreHashing { DoHash };
    ALWAYS_INLINE ProString(const QString &str, DoPreHashing);
    ALWAYS_INLINE ProString(const char *str, DoPreHashing);
    ALWAYS_INLINE ProString(const QString &str, int offset, int length, DoPreHashing);
    ALWAYS_INLINE ProString(const QString &str, int offset, int length, uint hash);

    QString m_string;
    int m_offset, m_length;
    const ProFile *m_file;
    mutable uint m_hash;
    QChar *prepareExtend(int extraLen, int thisTarget, int extraTarget);
    uint updatedHash() const;
    friend uint qHash(const ProString &str);
    friend QString operator+(const ProString &one, const ProString &two);
    friend class ProKey;
};
Q_DECLARE_TYPEINFO(ProString, Q_MOVABLE_TYPE);

class ProKey : public ProString {
public:
    ALWAYS_INLINE ProKey() : ProString() {}
    explicit ProKey(const QString &str);
    PROITEM_EXPLICIT ProKey(const char *str);
    ProKey(const QString &str, int off, int len);
    ProKey(const QString &str, int off, int len, uint hash);
    void setValue(const QString &str);

#ifdef Q_CC_MSVC
    // Workaround strange MSVC behaviour when exporting classes with ProKey members.
    ALWAYS_INLINE ProKey(const ProKey &other) : ProString(other.toString()) {}
    ALWAYS_INLINE ProKey &operator=(const ProKey &other)
    {
        toString() = other.toString();
        return *this;
    }
#endif

    ALWAYS_INLINE ProString &toString() { return *(ProString *)this; }
    ALWAYS_INLINE const ProString &toString() const { return *(const ProString *)this; }

private:
    ProKey(const ProString &other);
};
Q_DECLARE_TYPEINFO(ProKey, Q_MOVABLE_TYPE);

uint qHash(const ProString &str);
QString operator+(const ProString &one, const ProString &two);
inline QString operator+(const ProString &one, const QString &two)
    { return one + ProString(two); }
inline QString operator+(const QString &one, const ProString &two)
    { return ProString(one) + two; }

inline QString operator+(const ProString &one, const char *two)
    { return one + ProString(two); } // XXX optimize
inline QString operator+(const char *one, const ProString &two)
    { return ProString(one) + two; } // XXX optimize

inline QString &operator+=(QString &that, const ProString &other)
    { return that += other.toQStringRef(); }

inline bool operator==(const QString &that, const ProString &other)
    { return other == that; }
inline bool operator!=(const QString &that, const ProString &other)
    { return !(other == that); }

QTextStream &operator<<(QTextStream &t, const ProString &str);

class ProStringList : public QVector<ProString> {
public:
    ProStringList() {}
    ProStringList(const ProString &str) { *this << str; }
    explicit ProStringList(const QStringList &list);
    QStringList toQStringList() const;

    ProStringList &operator<<(const ProString &str)
        { QVector<ProString>::operator<<(str); return *this; }

    int length() const { return size(); }

    QString join(const QString &sep) const;
    QString join(QChar sep) const;

    void removeAll(const ProString &str);
    void removeAll(const char *str);
    void removeAt(int idx) { remove(idx); }
    void removeDuplicates();

    bool contains(const ProString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    bool contains(const QString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
        { return contains(ProString(str), cs); }
    bool contains(const char *str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
};
Q_DECLARE_TYPEINFO(ProStringList, Q_MOVABLE_TYPE);

inline ProStringList operator+(const ProStringList &one, const ProStringList &two)
    { ProStringList ret = one; ret += two; return ret; }

typedef QHash<ProKey, ProStringList> ProValueMap;

// These token definitions affect both ProFileEvaluator and ProWriter
enum ProToken {
    TokTerminator = 0,  // end of stream (possibly not included in length; must be zero)
    TokLine,            // line marker:
                        // - line (1)
    TokAssign,          // variable =
    TokAppend,          // variable +=
    TokAppendUnique,    // variable *=
    TokRemove,          // variable -=
    TokReplace,         // variable ~=
                        // previous literal/expansion is a variable manipulation
                        // - lower bound for expected output length (1)
                        // - value expression + TokValueTerminator
    TokValueTerminator, // assignment value terminator
    TokLiteral,         // literal string (fully dequoted)
                        // - length (1)
                        // - string data (length; unterminated)
    TokHashLiteral,     // literal string with hash (fully dequoted)
                        // - hash (2)
                        // - length (1)
                        // - string data (length; unterminated)
    TokVariable,        // qmake variable expansion
                        // - hash (2)
                        // - name length (1)
                        // - name (name length; unterminated)
    TokProperty,        // qmake property expansion
                        // - hash (2)
                        // - name length (1)
                        // - name (name length; unterminated)
    TokEnvVar,          // environment variable expansion
                        // - name length (1)
                        // - name (name length; unterminated)
    TokFuncName,        // replace function expansion
                        // - hash (2)
                        // - name length (1)
                        // - name (name length; unterminated)
                        // - ((nested expansion + TokArgSeparator)* + nested expansion)?
                        // - TokFuncTerminator
    TokArgSeparator,    // function argument separator
    TokFuncTerminator,  // function argument list terminator
    TokCondition,       // previous literal/expansion is a conditional
    TokTestCall,        // previous literal/expansion is a test function call
                        // - ((nested expansion + TokArgSeparator)* + nested expansion)?
                        // - TokFuncTerminator
    TokReturn,          // previous literal/expansion is a return value
    TokBreak,           // break loop
    TokNext,            // shortcut to next loop iteration
    TokNot,             // '!' operator
    TokAnd,             // ':' operator
    TokOr,              // '|' operator
    TokBranch,          // branch point:
                        // - then block length (2)
                        // - then block + TokTerminator (then block length)
                        // - else block length (2)
                        // - else block + TokTerminator (else block length)
    TokForLoop,         // for loop:
                        // - variable name: hash (2), length (1), chars (length)
                        // - expression: length (2), bytes + TokValueTerminator (length)
                        // - body length (2)
                        // - body + TokTerminator (body length)
    TokTestDef,         // test function definition:
    TokReplaceDef,      // replace function definition:
                        // - function name: hash (2), length (1), chars (length)
                        // - body length (2)
                        // - body + TokTerminator (body length)
    TokMask = 0xff,
    TokQuoted = 0x100,  // The expression is quoted => join expanded stringlist
    TokNewStr = 0x200   // Next stringlist element
};

class QMAKE_EXPORT ProFile
{
public:
    explicit ProFile(const QString &fileName);
    ~ProFile();

    QString fileName() const { return m_fileName; }
    QString directoryName() const { return m_directoryName; }
    const QString &items() const { return m_proitems; }
    QString *itemsRef() { return &m_proitems; }
    const ushort *tokPtr() const { return (const ushort *)m_proitems.constData(); }

    void ref() { m_refCount.ref(); }
    void deref() { if (!m_refCount.deref()) delete this; }

    bool isOk() const { return m_ok; }
    void setOk(bool ok) { m_ok = ok; }

    bool isHostBuild() const { return m_hostBuild; }
    void setHostBuild(bool host_build) { m_hostBuild = host_build; }

private:
    ProItemRefCount m_refCount;
    QString m_proitems;
    QString m_fileName;
    QString m_directoryName;
    bool m_ok;
    bool m_hostBuild;
};

class ProFunctionDef {
public:
    ProFunctionDef(ProFile *pro, int offset) : m_pro(pro), m_offset(offset) { m_pro->ref(); }
    ProFunctionDef(const ProFunctionDef &o) : m_pro(o.m_pro), m_offset(o.m_offset) { m_pro->ref(); }
    ~ProFunctionDef() { m_pro->deref(); }
    ProFunctionDef &operator=(const ProFunctionDef &o)
    {
        if (this != &o) {
            m_pro->deref();
            m_pro = o.m_pro;
            m_pro->ref();
            m_offset = o.m_offset;
        }
        return *this;
    }
    ProFile *pro() const { return m_pro; }
    const ushort *tokPtr() const { return m_pro->tokPtr() + m_offset; }
private:
    ProFile *m_pro;
    int m_offset;
};

Q_DECLARE_TYPEINFO(ProFunctionDef, Q_MOVABLE_TYPE);

struct ProFunctionDefs {
    QHash<ProKey, ProFunctionDef> testFunctions;
    QHash<ProKey, ProFunctionDef> replaceFunctions;
};

QT_END_NAMESPACE

#endif // PROITEMS_H
