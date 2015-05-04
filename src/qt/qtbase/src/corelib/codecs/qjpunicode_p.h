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

// Most of the code here was originally written by Serika Kurusugawa
// a.k.a. Junji Takagi, and is included in Qt with the author's permission,
// and the grateful thanks of the Qt team.

/*
 * Copyright (C) 1999 Serika Kurusugawa, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef QJPUNICODE_P_H
#define QJPUNICODE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_BIG_CODECS

class QJpUnicodeConv {
public:
    virtual ~QJpUnicodeConv() {}
    enum Rules {
        // "ASCII" is ANSI X.3.4-1986, a.k.a. US-ASCII here.
        Default                        = 0x0000,

        Unicode                        = 0x0001,
        Unicode_JISX0201                = 0x0001,
        Unicode_ASCII                 = 0x0002,
        JISX0221_JISX0201         = 0x0003,
        JISX0221_ASCII                = 0x0004,
        Sun_JDK117                     = 0x0005,
        Microsoft_CP932                = 0x0006,

        NEC_VDC                = 0x0100,                // NEC Vender Defined Char
        UDC                        = 0x0200,                // User Defined Char
        IBM_VDC                = 0x0400                // IBM Vender Defined Char
    };
    static QJpUnicodeConv *newConverter(int rule);

    virtual uint asciiToUnicode(uint h, uint l) const;
    /*virtual*/ uint jisx0201ToUnicode(uint h, uint l) const;
    virtual uint jisx0201LatinToUnicode(uint h, uint l) const;
    /*virtual*/ uint jisx0201KanaToUnicode(uint h, uint l) const;
    virtual uint jisx0208ToUnicode(uint h, uint l) const;
    virtual uint jisx0212ToUnicode(uint h, uint l) const;

    uint asciiToUnicode(uint ascii) const {
        return asciiToUnicode((ascii & 0xff00) >> 8, (ascii & 0x00ff));
    }
    uint jisx0201ToUnicode(uint jis) const {
        return jisx0201ToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
    }
    uint jisx0201LatinToUnicode(uint jis) const {
        return jisx0201LatinToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
    }
    uint jisx0201KanaToUnicode(uint jis) const {
        return jisx0201KanaToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
    }
    uint jisx0208ToUnicode(uint jis) const {
        return jisx0208ToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
    }
    uint jisx0212ToUnicode(uint jis) const {
        return jisx0212ToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
    }

    virtual uint unicodeToAscii(uint h, uint l) const;
    /*virtual*/ uint unicodeToJisx0201(uint h, uint l) const;
    virtual uint unicodeToJisx0201Latin(uint h, uint l) const;
    /*virtual*/ uint unicodeToJisx0201Kana(uint h, uint l) const;
    virtual uint unicodeToJisx0208(uint h, uint l) const;
    virtual uint unicodeToJisx0212(uint h, uint l) const;

    uint unicodeToAscii(uint unicode) const {
        return unicodeToAscii((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }
    uint unicodeToJisx0201(uint unicode) const {
        return unicodeToJisx0201((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }
    uint unicodeToJisx0201Latin(uint unicode) const {
        return unicodeToJisx0201Latin((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }
    uint unicodeToJisx0201Kana(uint unicode) const {
        return unicodeToJisx0201Kana((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }
    uint unicodeToJisx0208(uint unicode) const {
        return unicodeToJisx0208((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }
    uint unicodeToJisx0212(uint unicode) const {
        return unicodeToJisx0212((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }

    uint sjisToUnicode(uint h, uint l) const;
    uint unicodeToSjis(uint h, uint l) const;
    uint sjisibmvdcToUnicode(uint h, uint l) const;
    uint unicodeToSjisibmvdc(uint h, uint l) const;
    uint cp932ToUnicode(uint h, uint l) const;
    uint unicodeToCp932(uint h, uint l) const;

    uint sjisToUnicode(uint sjis) const {
        return sjisToUnicode((sjis & 0xff00) >> 8, (sjis & 0x00ff));
    }
    uint unicodeToSjis(uint unicode) const {
        return unicodeToSjis((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }

protected:
    explicit QJpUnicodeConv(int r) : rule(r) {}

private:
    int rule;
};

#endif // QT_NO_BIG_CODECS

QT_END_NAMESPACE

#endif // QJPUNICODE_P_H
