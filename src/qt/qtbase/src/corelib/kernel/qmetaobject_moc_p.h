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

#if !defined(QMETAOBJECT_P_H) && !defined(UTILS_H)
# error "Include qmetaobject_p.h (or moc's utils.h) before including this file."
#endif

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

QT_BEGIN_NAMESPACE

// This function is shared with moc.cpp. This file should be included where needed.
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
          We mustn't convert 'char * const *' into 'const char **'
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
            int scopeDepth = 0;
            while (t != e) {
                c = *t++;
                if (c == '{' || c == '(' || c == '[')
                    ++scopeDepth;
                if (c == '}' || c == ')' || c == ']')
                    --scopeDepth;
                if (scopeDepth == 0) {
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
