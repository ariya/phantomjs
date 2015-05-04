/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QRAWFONTPRIVATE_P_H
#define QRAWFONTPRIVATE_P_H

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

#include "qrawfont.h"

#include "qfontengine_p.h"
#include <QtCore/qthread.h>
#include <QtCore/qthreadstorage.h>

#if !defined(QT_NO_RAWFONT)

QT_BEGIN_NAMESPACE

namespace { class CustomFontFileLoader; }
class Q_GUI_EXPORT QRawFontPrivate
{
public:
    QRawFontPrivate()
        : fontEngine(0)
        , hintingPreference(QFont::PreferDefaultHinting)
        , thread(0)
    {}

    QRawFontPrivate(const QRawFontPrivate &other)
        : fontEngine(other.fontEngine)
        , hintingPreference(other.hintingPreference)
        , thread(other.thread)
    {
#ifndef QT_NO_DEBUG
        Q_ASSERT(fontEngine == 0 || thread == QThread::currentThread());
#endif
        if (fontEngine != 0)
            fontEngine->ref.ref();
    }

    ~QRawFontPrivate()
    {
#ifndef QT_NO_DEBUG
        Q_ASSERT(ref.load() == 0);
#endif
        cleanUp();
    }

    inline void cleanUp()
    {
        setFontEngine(0);
        hintingPreference = QFont::PreferDefaultHinting;
    }

    inline bool isValid() const
    {
#ifndef QT_NO_DEBUG
        Q_ASSERT(fontEngine == 0 || thread == QThread::currentThread());
#endif
        return fontEngine != 0;
    }

    inline void setFontEngine(QFontEngine *engine)
    {
#ifndef QT_NO_DEBUG
        Q_ASSERT(fontEngine == 0 || thread == QThread::currentThread());
#endif
        if (fontEngine == engine)
            return;

        if (fontEngine != 0) {
            if (!fontEngine->ref.deref())
                delete fontEngine;
#ifndef QT_NO_DEBUG
            thread = 0;
#endif
        }

        fontEngine = engine;

        if (fontEngine != 0) {
            fontEngine->ref.ref();
#ifndef QT_NO_DEBUG
            thread = QThread::currentThread();
            Q_ASSERT(thread);
#endif
        }
    }

    void loadFromData(const QByteArray &fontData,
                              qreal pixelSize,
                              QFont::HintingPreference hintingPreference);

    static QRawFontPrivate *get(const QRawFont &font) { return font.d.data(); }

    QFontEngine *fontEngine;
    QFont::HintingPreference hintingPreference;
    QAtomicInt ref;

private:
    QThread *thread;
};

QT_END_NAMESPACE

#endif // QT_NO_RAWFONT

#endif // QRAWFONTPRIVATE_P_H
