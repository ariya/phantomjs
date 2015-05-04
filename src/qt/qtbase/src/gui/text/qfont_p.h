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

#ifndef QFONT_P_H
#define QFONT_P_H

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

#include "QtGui/qfont.h"
#include "QtCore/qmap.h"
#include "QtCore/qhash.h"
#include "QtCore/qobject.h"
#include "QtCore/qstringlist.h"
#include <QtGui/qfontdatabase.h>
#include "private/qfixed_p.h"

QT_BEGIN_NAMESPACE

// forwards
class QFontCache;
class QFontEngine;

struct QFontDef
{
    inline QFontDef()
        : pointSize(-1.0), pixelSize(-1),
          styleStrategy(QFont::PreferDefault), styleHint(QFont::AnyStyle),
          weight(50), fixedPitch(false), style(QFont::StyleNormal), stretch(100),
          hintingPreference(QFont::PreferDefaultHinting), ignorePitch(true),
          fixedPitchComputed(0), reserved(0)
    {
    }

    QString family;
    QString styleName;

    QStringList fallBackFamilies;

    qreal pointSize;
    qreal pixelSize;

    uint styleStrategy : 16;
    uint styleHint     : 8;

    uint weight     :  7; // 0-99
    uint fixedPitch :  1;
    uint style      :  2;
    uint stretch    : 12; // 0-4000

    uint hintingPreference : 2;
    uint ignorePitch : 1;
    uint fixedPitchComputed : 1; // for Mac OS X only
    int reserved   : 14; // for future extensions

    bool exactMatch(const QFontDef &other) const;
    bool operator==(const QFontDef &other) const
    {
        return pixelSize == other.pixelSize
                    && weight == other.weight
                    && style == other.style
                    && stretch == other.stretch
                    && styleHint == other.styleHint
                    && styleStrategy == other.styleStrategy
                    && ignorePitch == other.ignorePitch && fixedPitch == other.fixedPitch
                    && family == other.family
                    && (styleName.isEmpty() || other.styleName.isEmpty() || styleName == other.styleName)
                    && hintingPreference == other.hintingPreference
                          ;
    }
    inline bool operator<(const QFontDef &other) const
    {
        if (pixelSize != other.pixelSize) return pixelSize < other.pixelSize;
        if (weight != other.weight) return weight < other.weight;
        if (style != other.style) return style < other.style;
        if (stretch != other.stretch) return stretch < other.stretch;
        if (styleHint != other.styleHint) return styleHint < other.styleHint;
        if (styleStrategy != other.styleStrategy) return styleStrategy < other.styleStrategy;
        if (family != other.family) return family < other.family;
        if (!styleName.isEmpty() && !other.styleName.isEmpty() && styleName != other.styleName)
            return styleName < other.styleName;
        if (hintingPreference != other.hintingPreference) return hintingPreference < other.hintingPreference;


        if (ignorePitch != other.ignorePitch) return ignorePitch < other.ignorePitch;
        if (fixedPitch != other.fixedPitch) return fixedPitch < other.fixedPitch;
        return false;
    }
};

inline uint qHash(const QFontDef &fd, uint seed = 0) Q_DECL_NOTHROW
{
    return qHash(qRound64(fd.pixelSize*10000)) // use only 4 fractional digits
        ^  qHash(fd.weight)
        ^  qHash(fd.style)
        ^  qHash(fd.stretch)
        ^  qHash(fd.styleHint)
        ^  qHash(fd.styleStrategy)
        ^  qHash(fd.ignorePitch)
        ^  qHash(fd.fixedPitch)
        ^  qHash(fd.family, seed)
        ^  qHash(fd.styleName)
        ^  qHash(fd.hintingPreference)
        ;
}

class QFontEngineData
{
public:
    QFontEngineData();
    ~QFontEngineData();

    QAtomicInt ref;
    const int fontCacheId;

    QFontEngine *engines[QChar::ScriptCount];

private:
    Q_DISABLE_COPY(QFontEngineData)
};


class Q_GUI_EXPORT QFontPrivate
{
public:

    QFontPrivate();
    QFontPrivate(const QFontPrivate &other);
    ~QFontPrivate();

    QFontEngine *engineForScript(int script) const;
    void alterCharForCapitalization(QChar &c) const;

    QAtomicInt ref;
    QFontDef request;
    mutable QFontEngineData *engineData;
    int dpi;
    int screen;


    uint rawMode    :  1;
    uint underline  :  1;
    uint overline   :  1;
    uint strikeOut  :  1;
    uint kerning    :  1;
    uint capital    :  3;
    bool letterSpacingIsAbsolute : 1;

    QFixed letterSpacing;
    QFixed wordSpacing;

    mutable QFontPrivate *scFont;
    QFont smallCapsFont() const { return QFont(smallCapsFontPrivate()); }
    QFontPrivate *smallCapsFontPrivate() const;

    static QFontPrivate *get(const QFont &font)
    {
        return font.d.data();
    }

    void resolve(uint mask, const QFontPrivate *other);

    static void detachButKeepEngineData(QFont *font);

private:
    QFontPrivate &operator=(const QFontPrivate &) { return *this; }
};


class Q_AUTOTEST_EXPORT QFontCache : public QObject
{
public:
    // note: these static functions work on a per-thread basis
    static QFontCache *instance();
    static void cleanup();

    QFontCache();
    ~QFontCache();

    int id() const { return m_id; }

    void clear();

    struct Key {
        Key() : script(0), multi(0), screen(0) { }
        Key(const QFontDef &d, uchar c, bool m = 0, uchar s = 0)
            : def(d), script(c), multi(m), screen(s) { }

        QFontDef def;
        uchar script;
        uchar multi: 1;
        uchar screen: 7;

        inline bool operator<(const Key &other) const
        {
            if (script != other.script) return script < other.script;
            if (screen != other.screen) return screen < other.screen;
            if (multi != other.multi) return multi < other.multi;
            if (multi && def.fallBackFamilies.size() != other.def.fallBackFamilies.size())
                return def.fallBackFamilies.size() < other.def.fallBackFamilies.size();
            return def < other.def;
        }
        inline bool operator==(const Key &other) const
        {
            return script == other.script
                    && screen == other.screen
                    && multi == other.multi
                    && (!multi || def.fallBackFamilies == other.def.fallBackFamilies)
                    && def == other.def;
        }
    };

    // QFontEngineData cache
    typedef QMap<QFontDef, QFontEngineData*> EngineDataCache;
    EngineDataCache engineDataCache;

    QFontEngineData *findEngineData(const QFontDef &def) const;
    void insertEngineData(const QFontDef &def, QFontEngineData *engineData);

    // QFontEngine cache
    struct Engine {
        Engine() : data(0), timestamp(0), hits(0) { }
        Engine(QFontEngine *d) : data(d), timestamp(0), hits(0) { }

        QFontEngine *data;
        uint timestamp;
        uint hits;
    };

    typedef QMap<Key,Engine> EngineCache;
    EngineCache engineCache;
    QHash<QFontEngine *, int> engineCacheCount;

    QFontEngine *findEngine(const Key &key);

    void updateHitCountAndTimeStamp(Engine &value);
    void insertEngine(const Key &key, QFontEngine *engine, bool insertMulti = false);

private:
    void increaseCost(uint cost);
    void decreaseCost(uint cost);
    void timerEvent(QTimerEvent *event);

    static const uint min_cost;
    uint total_cost, max_cost;
    uint current_timestamp;
    bool fast;
    int timer_id;
    const int m_id;
};

Q_GUI_EXPORT int qt_defaultDpiX();
Q_GUI_EXPORT int qt_defaultDpiY();
Q_GUI_EXPORT int qt_defaultDpi();

QT_END_NAMESPACE

#endif // QFONT_P_H
