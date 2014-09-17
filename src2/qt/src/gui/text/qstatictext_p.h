/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef QSTATICTEXT_P_H
#define QSTATICTEXT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include "qstatictext.h"

#include <private/qtextureglyphcache_p.h>
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class QStaticTextUserData
{
public:
    enum Type {
        NoUserData,
        OpenGLUserData
    };

    QStaticTextUserData(Type t) : type(t) { ref = 0; }
    virtual ~QStaticTextUserData() {}

    QAtomicInt ref;
    Type type;
};

class Q_GUI_EXPORT QStaticTextItem
{
public:    
    QStaticTextItem() : chars(0), numChars(0), useBackendOptimizations(false),
                        userDataNeedsUpdate(0), m_fontEngine(0), m_userData(0) {}

    QStaticTextItem(const QStaticTextItem &other)
    {
        operator=(other);
    }

    void operator=(const QStaticTextItem &other)
    {
        glyphPositions = other.glyphPositions;
        glyphs = other.glyphs;
        chars = other.chars;
        numGlyphs = other.numGlyphs;
        numChars = other.numChars;
        font = other.font;
        color = other.color;
        useBackendOptimizations = other.useBackendOptimizations;
        userDataNeedsUpdate = other.userDataNeedsUpdate;

        m_fontEngine = 0;
        m_userData = 0;
        setUserData(other.userData());
        setFontEngine(other.fontEngine());
    }

    ~QStaticTextItem();

    void setUserData(QStaticTextUserData *newUserData)
    {
        if (m_userData == newUserData)
            return;

        if (m_userData != 0 && !m_userData->ref.deref())
            delete m_userData;

        m_userData = newUserData;
        if (m_userData != 0)
            m_userData->ref.ref();
    }
    QStaticTextUserData *userData() const { return m_userData; }

    void setFontEngine(QFontEngine *fe);
    QFontEngine *fontEngine() const { return m_fontEngine; }

    union {
        QFixedPoint *glyphPositions;             // 8 bytes per glyph
        int positionOffset;
    };
    union {
        glyph_t *glyphs;                         // 4 bytes per glyph
        int glyphOffset;
    };
    union {
        QChar *chars;                            // 2 bytes per glyph
        int charOffset;
    };
                                                 // =================
                                                 // 14 bytes per glyph

                                                 // 12 bytes for pointers
    int numGlyphs;                               // 4 bytes per item
    int numChars;                                // 4 bytes per item
    QFont font;                                  // 8 bytes per item
    QColor color;                                // 10 bytes per item
    char useBackendOptimizations : 1;            // 1 byte per item
    char userDataNeedsUpdate : 1;                //
                                                 // ================
                                                 // 51 bytes per item

private: // Needs special handling in setters, so private to avoid abuse
    QFontEngine *m_fontEngine;                     // 4 bytes per item
    QStaticTextUserData *m_userData;               // 8 bytes per item

};

class QStaticText;
class Q_AUTOTEST_EXPORT QStaticTextPrivate
{
public:
    QStaticTextPrivate();
    QStaticTextPrivate(const QStaticTextPrivate &other);
    ~QStaticTextPrivate();

    void init();
    void paintText(const QPointF &pos, QPainter *p);

    void invalidate()
    {
        needsRelayout = true;
    }

    QAtomicInt ref;                      // 4 bytes per text

    QString text;                        // 4 bytes per text
    QFont font;                          // 8 bytes per text
    qreal textWidth;                     // 8 bytes per text
    QSizeF actualSize;                   // 16 bytes per text
    QPointF position;                    // 16 bytes per text

    QTransform matrix;                   // 80 bytes per text
    QStaticTextItem *items;              // 4 bytes per text
    int itemCount;                       // 4 bytes per text

    glyph_t *glyphPool;                  // 4 bytes per text
    QFixedPoint *positionPool;           // 4 bytes per text
    QChar *charPool;                     // 4 bytes per text

    QTextOption textOption;              // 28 bytes per text

    unsigned char needsRelayout            : 1; // 1 byte per text
    unsigned char useBackendOptimizations  : 1;
    unsigned char textFormat               : 2;
    unsigned char untransformedCoordinates : 1;
                                         // ================
                                         // 195 bytes per text

    static QStaticTextPrivate *get(const QStaticText *q);
};

QT_END_NAMESPACE

#endif // QSTATICTEXT_P_H
