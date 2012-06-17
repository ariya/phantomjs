/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMETAOBJECT_P_H
#define QMETAOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of moc.  This header file may change from version to version without notice,
// or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qobjectdefs.h>

QT_BEGIN_NAMESPACE

enum PropertyFlags  {
    Invalid = 0x00000000,
    Readable = 0x00000001,
    Writable = 0x00000002,
    Resettable = 0x00000004,
    EnumOrFlag = 0x00000008,
    StdCppSet = 0x00000100,
//     Override = 0x00000200,
    Constant = 0x00000400,
    Final = 0x00000800,
    Designable = 0x00001000,
    ResolveDesignable = 0x00002000,
    Scriptable = 0x00004000,
    ResolveScriptable = 0x00008000,
    Stored = 0x00010000,
    ResolveStored = 0x00020000,
    Editable = 0x00040000,
    ResolveEditable = 0x00080000,
    User = 0x00100000,
    ResolveUser = 0x00200000,
    Notify = 0x00400000,
    Revisioned = 0x00800000
};

enum MethodFlags  {
    AccessPrivate = 0x00,
    AccessProtected = 0x01,
    AccessPublic = 0x02,
    AccessMask = 0x03, //mask

    MethodMethod = 0x00,
    MethodSignal = 0x04,
    MethodSlot = 0x08,
    MethodConstructor = 0x0c,
    MethodTypeMask = 0x0c,

    MethodCompatibility = 0x10,
    MethodCloned = 0x20,
    MethodScriptable = 0x40,
    MethodRevisioned = 0x80
};

enum MetaObjectFlags {
    DynamicMetaObject = 0x01
};

class QMutex;

struct QMetaObjectPrivate
{
    int revision;
    int className;
    int classInfoCount, classInfoData;
    int methodCount, methodData;
    int propertyCount, propertyData;
    int enumeratorCount, enumeratorData;
    int constructorCount, constructorData; //since revision 2
    int flags; //since revision 3
    int signalCount; //since revision 4
    // revision 5 introduces changes in normalized signatures, no new members
    // revision 6 added qt_static_metacall as a member of each Q_OBJECT and inside QMetaObject itself

    static inline const QMetaObjectPrivate *get(const QMetaObject *metaobject)
    { return reinterpret_cast<const QMetaObjectPrivate*>(metaobject->d.data); }

    static int indexOfSignalRelative(const QMetaObject **baseObject,
                                     const char* name,
                                     bool normalizeStringData);
    static int indexOfSlotRelative(const QMetaObject **m,
                           const char *slot,
                           bool normalizeStringData);
    static int originalClone(const QMetaObject *obj, int local_method_index);

#ifndef QT_NO_QOBJECT
    //defined in qobject.cpp
    enum DisconnectType { DisconnectAll, DisconnectOne };
    static void memberIndexes(const QObject *obj, const QMetaMethod &member,
                              int *signalIndex, int *methodIndex);
    static bool connect(const QObject *sender, int signal_index,
                        const QObject *receiver, int method_index_relative,
                        const QMetaObject *rmeta = 0,
                        int type = 0, int *types = 0);
    static bool disconnect(const QObject *sender, int signal_index,
                           const QObject *receiver, int method_index,
                           DisconnectType = DisconnectAll);
    static inline bool disconnectHelper(QObjectPrivate::Connection *c,
                                        const QObject *receiver, int method_index,
                                        QMutex *senderMutex, DisconnectType);
#endif
};

#ifndef UTILS_H
// mirrored in moc's utils.h
static inline bool is_ident_char(char s)
{
    return ((s >= 'a' && s <= 'z')
            || (s >= 'A' && s <= 'Z')
            || (s >= '0' && s <= '9')
            || s == '_'
       );
}

static inline bool is_space(char s)
{
    return (s == ' ' || s == '\t');
}
#endif

// This code is shared with moc.cpp
static QByteArray normalizeTypeInternal(const char *t, const char *e, bool fixScope = false, bool adjustConst = true)
{
    int len = e - t;
    /*
      Convert 'char const *' into 'const char *'. Start at index 1,
      not 0, because 'const char *' is already OK.
    */
    QByteArray constbuf;
    for (int i = 1; i < len; i++) {
        if ( t[i] == 'c'
             && strncmp(t + i + 1, "onst", 4) == 0
             && (i + 5 >= len || !is_ident_char(t[i + 5]))
             && !is_ident_char(t[i-1])
             ) {
            constbuf = QByteArray(t, len);
            if (is_space(t[i-1]))
                constbuf.remove(i-1, 6);
            else
                constbuf.remove(i, 5);
            constbuf.prepend("const ");
            t = constbuf.data();
            e = constbuf.data() + constbuf.length();
            break;
        }
        /*
          We musn't convert 'char * const *' into 'const char **'
          and we must beware of 'Bar<const Bla>'.
        */
        if (t[i] == '&' || t[i] == '*' ||t[i] == '<')
            break;
    }
    if (adjustConst && e > t + 6 && strncmp("const ", t, 6) == 0) {
        if (*(e-1) == '&') { // treat const reference as value
            t += 6;
            --e;
        } else if (is_ident_char(*(e-1)) || *(e-1) == '>') { // treat const value as value
            t += 6;
        }
    }
    QByteArray result;
    result.reserve(len);

#if 1
    // consume initial 'const '
    if (strncmp("const ", t, 6) == 0) {
        t+= 6;
        result += "const ";
    }
#endif

    // some type substitutions for 'unsigned x'
    if (strncmp("unsigned", t, 8) == 0) {
        // make sure "unsigned" is an isolated word before making substitutions
        if (!t[8] || !is_ident_char(t[8])) {
            if (strncmp(" int", t+8, 4) == 0) {
                t += 8+4;
                result += "uint";
            } else if (strncmp(" long", t+8, 5) == 0) {
                if ((strlen(t + 8 + 5) < 4 || strncmp(t + 8 + 5, " int", 4) != 0) // preserve '[unsigned] long int'
                    && (strlen(t + 8 + 5) < 5 || strncmp(t + 8 + 5, " long", 5) != 0) // preserve '[unsigned] long long'
                   ) {
                    t += 8+5;
                    result += "ulong";
                }
            } else if (strncmp(" short", t+8, 6) != 0  // preserve unsigned short
                && strncmp(" char", t+8, 5) != 0) {    // preserve unsigned char
                //  treat rest (unsigned) as uint
                t += 8;
                result += "uint";
            }
        }
    } else {
        // discard 'struct', 'class', and 'enum'; they are optional
        // and we don't want them in the normalized signature
        struct {
            const char *keyword;
            int len;
        } optional[] = {
            { "struct ", 7 },
            { "class ", 6 },
            { "enum ", 5 },
            { 0, 0 }
        };
        int i = 0;
        do {
            if (strncmp(optional[i].keyword, t, optional[i].len) == 0) {
                t += optional[i].len;
                break;
            }
        } while (optional[++i].keyword != 0);
    }

    bool star = false;
    while (t != e) {
        char c = *t++;
        if (fixScope && c == ':' && *t == ':' ) {
            ++t;
            c = *t++;
            int i = result.size() - 1;
            while (i >= 0 && is_ident_char(result.at(i)))
                --i;
            result.resize(i + 1);
        }
        star = star || c == '*';
        result += c;
        if (c == '<') {
            //template recursion
            const char* tt = t;
            int templdepth = 1;
            while (t != e) {
                c = *t++;
                if (c == '<')
                    ++templdepth;
                if (c == '>')
                    --templdepth;
                if (templdepth == 0 || (templdepth == 1 && c == ',')) {
                    result += normalizeTypeInternal(tt, t-1, fixScope, false);
                    result += c;
                    if (templdepth == 0) {
                        if (*t == '>')
                            result += ' '; // avoid >>
                        break;
                    }
                    tt = t;
                }
            }
        }

        // cv qualifers can appear after the type as well
        if (!is_ident_char(c) && t != e && (e - t >= 5 && strncmp("const", t, 5) == 0)
            && (e - t == 5 || !is_ident_char(t[5]))) {
            t += 5;
            while (t != e && is_space(*t))
                ++t;
            if (adjustConst && t != e && *t == '&') {
                // treat const ref as value
                ++t;
            } else if (adjustConst && !star) {
                // treat const as value
            } else if (!star) {
                // move const to the front (but not if const comes after a *)
                result.prepend("const ");
            } else {
                // keep const after a *
                result += "const";
            }
        }
    }

    return result;
}


QT_END_NAMESPACE

#endif

