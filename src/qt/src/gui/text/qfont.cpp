/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qfont.h"
#include "qdebug.h"
#include "qpaintdevice.h"
#include "qfontdatabase.h"
#include "qfontmetrics.h"
#include "qfontinfo.h"
#include "qpainter.h"
#include "qhash.h"
#include "qdatastream.h"
#include "qapplication.h"
#include "qstringlist.h"

#include "qthread.h"
#include "qthreadstorage.h"

#include <private/qunicodetables_p.h>
#include "qfont_p.h"
#include <private/qfontengine_p.h>
#include <private/qpainter_p.h>
#include <private/qtextengine_p.h>
#include <limits.h>

#ifdef Q_WS_X11
#include "qx11info_x11.h"
#include <private/qt_x11_p.h>
#endif
#ifdef Q_WS_QWS
#include "qscreen_qws.h"
#if !defined(QT_NO_QWS_QPF2)
#include <qfile.h>
#include "qfontengine_qpf_p.h"
#endif
#endif
#ifdef Q_OS_SYMBIAN
#include <private/qt_s60_p.h>
#endif
#ifdef Q_WS_QPA
#include <QtGui/qplatformscreen_qpa.h>
#include <QtGui/private/qapplication_p.h>
#endif

#include <QMutexLocker>

// #define QFONTCACHE_DEBUG
#ifdef QFONTCACHE_DEBUG
#  define FC_DEBUG qDebug
#else
#  define FC_DEBUG if (false) qDebug
#endif

QT_BEGIN_NAMESPACE

#ifdef Q_WS_WIN
extern HDC shared_dc();
#endif

#ifdef Q_WS_X11
extern const QX11Info *qt_x11Info(const QPaintDevice *pd);
#endif

bool QFontDef::exactMatch(const QFontDef &other) const
{
    /*
      QFontDef comparison is more complicated than just simple
      per-member comparisons.

      When comparing point/pixel sizes, either point or pixelsize
      could be -1.  in This case we have to compare the non negative
      size value.

      This test will fail if the point-sizes differ by 1/2 point or
      more or they do not round to the same value.  We have to do this
      since our API still uses 'int' point-sizes in the API, but store
      deci-point-sizes internally.

      To compare the family members, we need to parse the font names
      and compare the family/foundry strings separately.  This allows
      us to compare e.g. "Helvetica" and "Helvetica [Adobe]" with
      positive results.
    */
    if (pixelSize != -1 && other.pixelSize != -1) {
        if (pixelSize != other.pixelSize)
            return false;
    } else if (pointSize != -1 && other.pointSize != -1) {
        if (pointSize != other.pointSize)
            return false;
    } else {
        return false;
    }

    if (!ignorePitch && !other.ignorePitch && fixedPitch != other.fixedPitch)
        return false;

    if (stretch != 0 && other.stretch != 0 && stretch != other.stretch)
        return false;

    QString this_family, this_foundry, other_family, other_foundry;
    QFontDatabase::parseFontName(family, this_foundry, this_family);
    QFontDatabase::parseFontName(other.family, other_foundry, other_family);

    this_family = QFontDatabase::resolveFontFamilyAlias(this_family);
    other_family = QFontDatabase::resolveFontFamilyAlias(other_family);

    return (styleHint     == other.styleHint
            && styleStrategy == other.styleStrategy
            && weight        == other.weight
            && style        == other.style
            && this_family   == other_family
            && (styleName.isEmpty() || other.styleName.isEmpty() || styleName == other.styleName)
            && (this_foundry.isEmpty()
                || other_foundry.isEmpty()
                || this_foundry == other_foundry)
#ifdef Q_WS_X11
            && addStyle == other.addStyle
#endif // Q_WS_X11
       );
}

extern bool qt_is_gui_used;

Q_GUI_EXPORT int qt_defaultDpiX()
{
    if (!qt_is_gui_used)
        return 75;

    int dpi;
#ifdef Q_WS_X11
    dpi = QX11Info::appDpiX();
#elif defined(Q_WS_WIN)
    dpi = GetDeviceCaps(shared_dc(),LOGPIXELSX);
#elif defined(Q_WS_MAC)
    extern float qt_mac_defaultDpi_x(); //qpaintdevice_mac.cpp
    dpi = qt_mac_defaultDpi_x();
#elif defined(Q_WS_QWS)
    if (!qt_screen)
        return 72;
    QScreen *screen = qt_screen;
    const QList<QScreen*> subScreens = qt_screen->subScreens();
    if (!subScreens.isEmpty())
        screen = subScreens.at(0);
    dpi = qRound(screen->width() / (screen->physicalWidth() / qreal(25.4)));
#elif defined(Q_WS_QPA)
    QPlatformIntegration *pi = QApplicationPrivate::platformIntegration();
    if (pi) {
        QPlatformScreen *screen = QApplicationPrivate::platformIntegration()->screens().at(0);
        const QSize screenSize = screen->geometry().size();
        const QSize physicalSize = screen->physicalSize();
        dpi = qRound(screenSize.width() / (physicalSize.width() / qreal(25.4)));
    } else {
        //PI has not been initialised, or it is being initialised. Give a default dpi
        dpi = 100;
    }
#elif defined(Q_OS_SYMBIAN)
    dpi = S60->defaultDpiX;
#endif // Q_WS_X11

    return dpi;
}

Q_GUI_EXPORT int qt_defaultDpiY()
{
    if (!qt_is_gui_used)
        return 75;

    int dpi;
#ifdef Q_WS_X11
    dpi = QX11Info::appDpiY();
#elif defined(Q_WS_WIN)
    dpi = GetDeviceCaps(shared_dc(),LOGPIXELSY);
#elif defined(Q_WS_MAC)
    extern float qt_mac_defaultDpi_y(); //qpaintdevice_mac.cpp
    dpi = qt_mac_defaultDpi_y();
#elif defined(Q_WS_QWS)
    if (!qt_screen)
        return 72;
    QScreen *screen = qt_screen;
    const QList<QScreen*> subScreens = qt_screen->subScreens();
    if (!subScreens.isEmpty())
        screen = subScreens.at(0);
    dpi = qRound(screen->height() / (screen->physicalHeight() / qreal(25.4)));
#elif defined(Q_WS_QPA)
    QPlatformIntegration *pi = QApplicationPrivate::platformIntegration();
    if (pi) {
        QPlatformScreen *screen = QApplicationPrivate::platformIntegration()->screens().at(0);
        const QSize screenSize = screen->geometry().size();
        const QSize physicalSize = screen->physicalSize();
        dpi = qRound(screenSize.height() / (physicalSize.height() / qreal(25.4)));
    } else {
        //PI has not been initialised, or it is being initialised. Give a default dpi
        dpi = 100;
    }
#elif defined(Q_OS_SYMBIAN)
    dpi = S60->defaultDpiY;
#endif // Q_WS_X11

    return dpi;
}

Q_GUI_EXPORT int qt_defaultDpi()
{
    return qt_defaultDpiY();
}

QFontPrivate::QFontPrivate()
    : engineData(0), dpi(qt_defaultDpi()), screen(0),
      rawMode(false), underline(false), overline(false), strikeOut(false), kerning(true),
      capital(0), letterSpacingIsAbsolute(false), scFont(0)
{
#ifdef Q_WS_X11
    if (QX11Info::display())
        screen = QX11Info::appScreen();
    else
        screen = 0;
#endif
#ifdef Q_WS_WIN
    hdc = 0;
#endif
}

QFontPrivate::QFontPrivate(const QFontPrivate &other)
    : request(other.request), engineData(0), dpi(other.dpi), screen(other.screen),
      rawMode(other.rawMode), underline(other.underline), overline(other.overline),
      strikeOut(other.strikeOut), kerning(other.kerning),
      capital(other.capital), letterSpacingIsAbsolute(other.letterSpacingIsAbsolute),
      letterSpacing(other.letterSpacing), wordSpacing(other.wordSpacing),
      scFont(other.scFont)
{
#ifdef Q_WS_WIN
    hdc = other.hdc;
#endif
    if (scFont && scFont != this)
        scFont->ref.ref();
}

QFontPrivate::~QFontPrivate()
{
    if (engineData)
        engineData->ref.deref();
    engineData = 0;
    if (scFont && scFont != this)
        scFont->ref.deref();
    scFont = 0;
}

extern QMutex *qt_fontdatabase_mutex();

#if !defined(Q_WS_MAC)
#define QT_FONT_ENGINE_FROM_DATA(data, script) data->engines[script]
#else
#define QT_FONT_ENGINE_FROM_DATA(data, script) data->engine
#endif

QFontEngine *QFontPrivate::engineForScript(int script) const
{
    QMutexLocker locker(qt_fontdatabase_mutex());
    if (script >= QUnicodeTables::Inherited)
        script = QUnicodeTables::Common;
    if (engineData && engineData->fontCache != QFontCache::instance()) {
        // throw out engineData that came from a different thread
        engineData->ref.deref();
        engineData = 0;
    }
    if (!engineData || !QT_FONT_ENGINE_FROM_DATA(engineData, script))
        QFontDatabase::load(this, script);
    return QT_FONT_ENGINE_FROM_DATA(engineData, script);
}

void QFontPrivate::alterCharForCapitalization(QChar &c) const {
    switch (capital) {
    case QFont::AllUppercase:
    case QFont::SmallCaps:
        c = c.toUpper();
        break;
    case QFont::AllLowercase:
        c = c.toLower();
        break;
    case QFont::MixedCase:
        break;
    }
}

QFontPrivate *QFontPrivate::smallCapsFontPrivate() const
{
    if (scFont)
        return scFont;
    QFont font(const_cast<QFontPrivate *>(this));
    qreal pointSize = font.pointSizeF();
    if (pointSize > 0)
        font.setPointSizeF(pointSize * .7);
    else
        font.setPixelSize((font.pixelSize() * 7 + 5) / 10);
    scFont = font.d.data();
    if (scFont != this)
        scFont->ref.ref();
    return scFont;
}


void QFontPrivate::resolve(uint mask, const QFontPrivate *other)
{
    Q_ASSERT(other != 0);

    dpi = other->dpi;

    if ((mask & QFont::AllPropertiesResolved) == QFont::AllPropertiesResolved) return;

    // assign the unset-bits with the set-bits of the other font def
    if (! (mask & QFont::FamilyResolved))
        request.family = other->request.family;

    if (! (mask & QFont::StyleNameResolved))
        request.styleName = other->request.styleName;

    if (! (mask & QFont::SizeResolved)) {
        request.pointSize = other->request.pointSize;
        request.pixelSize = other->request.pixelSize;
    }

    if (! (mask & QFont::StyleHintResolved))
        request.styleHint = other->request.styleHint;

    if (! (mask & QFont::StyleStrategyResolved))
        request.styleStrategy = other->request.styleStrategy;

    if (! (mask & QFont::WeightResolved))
        request.weight = other->request.weight;

    if (! (mask & QFont::StyleResolved))
        request.style = other->request.style;

    if (! (mask & QFont::FixedPitchResolved))
        request.fixedPitch = other->request.fixedPitch;

    if (! (mask & QFont::StretchResolved))
        request.stretch = other->request.stretch;

    if (! (mask & QFont::HintingPreferenceResolved))
        request.hintingPreference = other->request.hintingPreference;

    if (! (mask & QFont::UnderlineResolved))
        underline = other->underline;

    if (! (mask & QFont::OverlineResolved))
        overline = other->overline;

    if (! (mask & QFont::StrikeOutResolved))
        strikeOut = other->strikeOut;

    if (! (mask & QFont::KerningResolved))
        kerning = other->kerning;

    if (! (mask & QFont::LetterSpacingResolved)) {
        letterSpacing = other->letterSpacing;
        letterSpacingIsAbsolute = other->letterSpacingIsAbsolute;
    }
    if (! (mask & QFont::WordSpacingResolved))
        wordSpacing = other->wordSpacing;
    if (! (mask & QFont::CapitalizationResolved))
        capital = other->capital;
}




QFontEngineData::QFontEngineData()
    : ref(1), fontCache(QFontCache::instance())
{
#if !defined(Q_WS_MAC)
    memset(engines, 0, QUnicodeTables::ScriptCount * sizeof(QFontEngine *));
#else
    engine = 0;
#endif
}

QFontEngineData::~QFontEngineData()
{
#if !defined(Q_WS_MAC)
    for (int i = 0; i < QUnicodeTables::ScriptCount; ++i) {
        if (engines[i])
            engines[i]->ref.deref();
        engines[i] = 0;
    }
#else
    if (engine)
        engine->ref.deref();
    engine = 0;
#endif // Q_WS_X11 || Q_WS_WIN || Q_WS_MAC
}




/*!
    \class QFont
    \reentrant

    \brief The QFont class specifies a font used for drawing text.

    \ingroup painting
    \ingroup appearance
    \ingroup shared
    \ingroup richtext-processing


    When you create a QFont object you specify various attributes that
    you want the font to have. Qt will use the font with the specified
    attributes, or if no matching font exists, Qt will use the closest
    matching installed font. The attributes of the font that is
    actually used are retrievable from a QFontInfo object. If the
    window system provides an exact match exactMatch() returns true.
    Use QFontMetrics to get measurements, e.g. the pixel length of a
    string using QFontMetrics::width().

    Note that a QApplication instance must exist before a QFont can be
    used. You can set the application's default font with
    QApplication::setFont().

    If a chosen font does not include all the characters that
    need to be displayed, QFont will try to find the characters in the
    nearest equivalent fonts. When a QPainter draws a character from a
    font the QFont will report whether or not it has the character; if
    it does not, QPainter will draw an unfilled square.

    Create QFonts like this:

    \snippet doc/src/snippets/code/src_gui_text_qfont.cpp 0

    The attributes set in the constructor can also be set later, e.g.
    setFamily(), setPointSize(), setPointSizeFloat(), setWeight() and
    setItalic(). The remaining attributes must be set after
    contstruction, e.g. setBold(), setUnderline(), setOverline(),
    setStrikeOut() and setFixedPitch(). QFontInfo objects should be
    created \e after the font's attributes have been set. A QFontInfo
    object will not change, even if you change the font's
    attributes. The corresponding "get" functions, e.g. family(),
    pointSize(), etc., return the values that were set, even though
    the values used may differ. The actual values are available from a
    QFontInfo object.

    If the requested font family is unavailable you can influence the
    \link #fontmatching font matching algorithm\endlink by choosing a
    particular \l{QFont::StyleHint} and \l{QFont::StyleStrategy} with
    setStyleHint(). The default family (corresponding to the current
    style hint) is returned by defaultFamily().

    The font-matching algorithm has a lastResortFamily() and
    lastResortFont() in cases where a suitable match cannot be found.
    You can provide substitutions for font family names using
    insertSubstitution() and insertSubstitutions(). Substitutions can
    be removed with removeSubstitution(). Use substitute() to retrieve
    a family's first substitute, or the family name itself if it has
    no substitutes. Use substitutes() to retrieve a list of a family's
    substitutes (which may be empty).

    Every QFont has a key() which you can use, for example, as the key
    in a cache or dictionary. If you want to store a user's font
    preferences you could use QSettings, writing the font information
    with toString() and reading it back with fromString(). The
    operator<<() and operator>>() functions are also available, but
    they work on a data stream.

    It is possible to set the height of characters shown on the screen
    to a specified number of pixels with setPixelSize(); however using
    setPointSize() has a similar effect and provides device
    independence.

    In X11 you can set a font using its system
    specific name with setRawName().

    Loading fonts can be expensive, especially on X11. QFont contains
    extensive optimizations to make the copying of QFont objects fast,
    and to cache the results of the slow window system functions it
    depends upon.

    \target fontmatching
    The font matching algorithm works as follows:
    \list 1
    \o The specified font family is searched for.
    \o If not found, the styleHint() is used to select a replacement
       family.
    \o Each replacement font family is searched for.
    \o If none of these are found or there was no styleHint(), "helvetica"
       will be searched for.
    \o If "helvetica" isn't found Qt will try the lastResortFamily().
    \o If the lastResortFamily() isn't found Qt will try the
       lastResortFont() which will always return a name of some kind.
    \endlist

    Note that the actual font matching algorithm varies from platform to platform.

    In Windows a request for the "Courier" font is automatically changed to
    "Courier New", an improved version of Courier that allows for smooth scaling.
    The older "Courier" bitmap font can be selected by setting the PreferBitmap
    style strategy (see setStyleStrategy()).

    Once a font is found, the remaining attributes are matched in order of
    priority:
    \list 1
    \o fixedPitch()
    \o pointSize() (see below)
    \o weight()
    \o style()
    \endlist

    If you have a font which matches on family, even if none of the
    other attributes match, this font will be chosen in preference to
    a font which doesn't match on family but which does match on the
    other attributes. This is because font family is the dominant
    search criteria.

    The point size is defined to match if it is within 20% of the
    requested point size. When several fonts match and are only
    distinguished by point size, the font with the closest point size
    to the one requested will be chosen.

    The actual family, font size, weight and other font attributes
    used for drawing text will depend on what's available for the
    chosen family under the window system. A QFontInfo object can be
    used to determine the actual values used for drawing the text.

    Examples:

    \snippet doc/src/snippets/code/src_gui_text_qfont.cpp 1
    If you had both an Adobe and a Cronyx Helvetica, you might get
    either.

    \snippet doc/src/snippets/code/src_gui_text_qfont.cpp 2

    You can specify the foundry you want in the family name. The font f
    in the above example will be set to "Helvetica
    [Cronyx]".

    To determine the attributes of the font actually used in the window
    system, use a QFontInfo object, e.g.

    \snippet doc/src/snippets/code/src_gui_text_qfont.cpp 3

    To find out font metrics use a QFontMetrics object, e.g.

    \snippet doc/src/snippets/code/src_gui_text_qfont.cpp 4

    For more general information on fonts, see the
    \link http://nwalsh.com/comp.fonts/FAQ/ comp.fonts FAQ.\endlink
    Information on encodings can be found from
    \link http://czyborra.com/ Roman Czyborra's\endlink page.

    \sa QFontComboBox, QFontMetrics, QFontInfo, QFontDatabase, {Character Map Example}
*/

/*!
    \internal
    \enum QFont::ResolveProperties

    This enum describes the properties of a QFont that can be set on a font
    individually and then considered resolved.

    \value FamilyResolved
    \value SizeResolved
    \value StyleHintResolved
    \value StyleStrategyResolved
    \value WeightResolved
    \value StyleResolved
    \value UnderlineResolved
    \value OverlineResolved
    \value StrikeOutResolved
    \value FixedPitchResolved
    \value StretchResolved
    \value KerningResolved
    \value CapitalizationResolved
    \value LetterSpacingResolved
    \value WordSpacingResolved
    \value CompletelyResolved
*/

/*!
    \enum QFont::Style

    This enum describes the different styles of glyphs that are used to
    display text.

    \value StyleNormal  Normal glyphs used in unstyled text.
    \value StyleItalic  Italic glyphs that are specifically designed for
                        the purpose of representing italicized text.
    \value StyleOblique Glyphs with an italic appearance that are typically
                        based on the unstyled glyphs, but are not fine-tuned
                        for the purpose of representing italicized text.

    \sa Weight
*/

/*!
    \fn Qt::HANDLE QFont::handle() const

    Returns the window system handle to the font, for low-level
    access. Using this function is \e not portable.
*/

/*!
    \fn FT_Face QFont::freetypeFace() const

    Returns the handle to the primary FreeType face of the font. If font merging is not disabled a
    QFont can contain several physical fonts.

    Returns 0 if the font does not contain a FreeType face.

    \note This function is only available on platforms that provide the FreeType library;
    i.e., X11 and some Embedded Linux platforms.
*/

/*!
    \fn QString QFont::rawName() const

    Returns the name of the font within the underlying window system.

    On X11, this function will return an empty string if Qt is built with
    FontConfig support; otherwise the XLFD (X Logical Font Description) is
    returned.

    Using the return value of this function is usually \e not \e
    portable.

    \sa setRawName()
*/

/*!
    \fn void QFont::setRawName(const QString &name)

    Sets a font by its system specific name. The function is
    particularly useful under X, where system font settings (for
    example X resources) are usually available in XLFD (X Logical Font
    Description) form only. You can pass an XLFD as \a name to this
    function.

    A font set with setRawName() is still a full-featured QFont. It can
    be queried (for example with italic()) or modified (for example with
    setItalic()) and is therefore also suitable for rendering rich text.

    If Qt's internal font database cannot resolve the raw name, the
    font becomes a raw font with \a name as its family.

    Note that the present implementation does not handle wildcards in
    XLFDs well, and that font aliases (file \c fonts.alias in the font
    directory on X11) are not supported.

    \sa rawName(), setRawMode(), setFamily()
*/

/*!
    \fn QString QFont::lastResortFamily() const

    Returns the "last resort" font family name.

    The current implementation tries a wide variety of common fonts,
    returning the first one it finds. Is is possible that no family is
    found in which case an empty string is returned.

    \sa lastResortFont()
*/

/*!
    \fn QString QFont::defaultFamily() const

    Returns the family name that corresponds to the current style
    hint.

    \sa StyleHint styleHint() setStyleHint()
*/

/*!
    \fn QString QFont::lastResortFont() const

    Returns a "last resort" font name for the font matching algorithm.
    This is used if the last resort family is not available. It will
    always return a name, if necessary returning something like
    "fixed" or "system".

    The current implementation tries a wide variety of common fonts,
    returning the first one it finds. The implementation may change
    at any time, but this function will always return a string
    containing something.

    It is theoretically possible that there really isn't a
    lastResortFont() in which case Qt will abort with an error
    message. We have not been able to identify a case where this
    happens. Please \link bughowto.html report it as a bug\endlink if
    it does, preferably with a list of the fonts you have installed.

    \sa lastResortFamily() rawName()
*/

/*!
  Constructs a font from \a font for use on the paint device \a pd.
*/
QFont::QFont(const QFont &font, QPaintDevice *pd)
    : resolve_mask(font.resolve_mask)
{
    Q_ASSERT(pd != 0);
    int dpi = pd->logicalDpiY();
#ifdef Q_WS_X11
    const QX11Info *info = qt_x11Info(pd);
    int screen = info ? info->screen() : 0;
#else
    const int screen = 0;
#endif
    if (font.d->dpi != dpi || font.d->screen != screen ) {
        d = new QFontPrivate(*font.d);
        d->dpi = dpi;
        d->screen = screen;
    } else {
        d = font.d.data();
    }
#ifdef Q_WS_WIN
    if (pd->devType() == QInternal::Printer && pd->getDC())
        d->hdc = pd->getDC();
#endif
}

/*!
  \internal
*/
QFont::QFont(QFontPrivate *data)
    : d(data), resolve_mask(QFont::AllPropertiesResolved)
{
}

/*! \internal
    Detaches the font object from common font data.
*/
void QFont::detach()
{
    if (d->ref == 1) {
        if (d->engineData)
            d->engineData->ref.deref();
        d->engineData = 0;
        if (d->scFont && d->scFont != d.data())
            d->scFont->ref.deref();
        d->scFont = 0;
        return;
    }

    d.detach();
}

/*!
    Constructs a font object that uses the application's default font.

    \sa QApplication::setFont(), QApplication::font()
*/
QFont::QFont()
    : d(QApplication::font().d.data()), resolve_mask(0)
{
}

/*!
    Constructs a font object with the specified \a family, \a
    pointSize, \a weight and \a italic settings.

    If \a pointSize is zero or negative, the point size of the font
    is set to a system-dependent default value. Generally, this is
    12 points, except on Symbian where it is 7 points.

    The \a family name may optionally also include a foundry name,
    e.g. "Helvetica [Cronyx]". If the \a family is
    available from more than one foundry and the foundry isn't
    specified, an arbitrary foundry is chosen. If the family isn't
    available a family will be set using the \l{QFont}{font matching}
    algorithm.

    \sa Weight, setFamily(), setPointSize(), setWeight(), setItalic(),
    setStyleHint() QApplication::font()
*/
QFont::QFont(const QString &family, int pointSize, int weight, bool italic)
    : d(new QFontPrivate()), resolve_mask(QFont::FamilyResolved)
{
    if (pointSize <= 0) {
#ifdef Q_OS_SYMBIAN
        pointSize = 7;
#else
        pointSize = 12;
#endif
    } else {
        resolve_mask |= QFont::SizeResolved;
    }

    if (weight < 0) {
        weight = Normal;
    } else {
        resolve_mask |= QFont::WeightResolved | QFont::StyleResolved;
    }

    if (italic)
        resolve_mask |= QFont::StyleResolved;

    d->request.family = family;
    d->request.pointSize = qreal(pointSize);
    d->request.pixelSize = -1;
    d->request.weight = weight;
    d->request.style = italic ? QFont::StyleItalic : QFont::StyleNormal;
}

/*!
    Constructs a font that is a copy of \a font.
*/
QFont::QFont(const QFont &font)
    : d(font.d.data()), resolve_mask(font.resolve_mask)
{
}

/*!
    Destroys the font object and frees all allocated resources.
*/
QFont::~QFont()
{
}

/*!
    Assigns \a font to this font and returns a reference to it.
*/
QFont &QFont::operator=(const QFont &font)
{
    d = font.d.data();
    resolve_mask = font.resolve_mask;
    return *this;
}

/*!
    Returns the requested font family name, i.e. the name set in the
    constructor or the last setFont() call.

    \sa setFamily() substitutes() substitute()
*/
QString QFont::family() const
{
    return d->request.family;
}

/*!
    Sets the family name of the font. The name is case insensitive and
    may include a foundry name.

    The \a family name may optionally also include a foundry name,
    e.g. "Helvetica [Cronyx]". If the \a family is
    available from more than one foundry and the foundry isn't
    specified, an arbitrary foundry is chosen. If the family isn't
    available a family will be set using the \l{QFont}{font matching}
    algorithm.

    \sa family(), setStyleHint(), QFontInfo
*/
void QFont::setFamily(const QString &family)
{
    detach();

    d->request.family = family;
#if defined(Q_WS_X11)
    d->request.addStyle.clear();
#endif // Q_WS_X11

    resolve_mask |= QFont::FamilyResolved;
}

/*!
    \since 4.8

    Returns the requested font style name, it will be used to match the
    font with irregular styles (that can't be normalized in other style
    properties). It depends on system font support, thus only works for
    Mac OS X and X11 so far. On Windows irregular styles will be added
    as separate font families so there is no need for this.

    \sa setFamily() setStyle()
*/
QString QFont::styleName() const
{
    return d->request.styleName;
}

/*!
    \since 4.8

    Sets the style name of the font to the given \a styleName.
    When set, other style properties like style() and weight() will be ignored
    for font matching.

    \sa styleName()
*/
void QFont::setStyleName(const QString &styleName)
{
    detach();

    d->request.styleName = styleName;
    resolve_mask |= QFont::StyleNameResolved;
}

/*!
    Returns the point size of the font. Returns -1 if the font size
    was specified in pixels.

    \sa setPointSize() pointSizeF()
*/
int QFont::pointSize() const
{
    return qRound(d->request.pointSize);
}

/*!
    \since 4.8

    \enum QFont::HintingPreference

    This enum describes the different levels of hinting that can be applied
    to glyphs to improve legibility on displays where it might be warranted
    by the density of pixels.

    \value PreferDefaultHinting Use the default hinting level for the target platform.
    \value PreferNoHinting If possible, render text without hinting the outlines
           of the glyphs. The text layout will be typographically accurate and
           scalable, using the same metrics as are used e.g. when printing.
    \value PreferVerticalHinting If possible, render text with no horizontal hinting,
           but align glyphs to the pixel grid in the vertical direction. The text will appear
           crisper on displays where the density is too low to give an accurate rendering
           of the glyphs. But since the horizontal metrics of the glyphs are unhinted, the text's
           layout will be scalable to higher density devices (such as printers) without impacting
           details such as line breaks.
    \value PreferFullHinting If possible, render text with hinting in both horizontal and
           vertical directions. The text will be altered to optimize legibility on the target
           device, but since the metrics will depend on the target size of the text, the positions
           of glyphs, line breaks, and other typographical detail will not scale, meaning that a
           text layout may look different on devices with different pixel densities.

    Please note that this enum only describes a preference, as the full range of hinting levels
    are not supported on all of Qt's supported platforms. The following table details the effect
    of a given hinting preference on a selected set of target platforms.

    \table
    \header
    \o
    \o PreferDefaultHinting
    \o PreferNoHinting
    \o PreferVerticalHinting
    \o PreferFullHinting
    \row
    \o Windows Vista (w/o Platform Update) and earlier
    \o Full hinting
    \o Full hinting
    \o Full hinting
    \o Full hinting
    \row
    \o Windows 7 and Windows Vista (w/Platform Update) and DirectWrite enabled in Qt
    \o Full hinting
    \o Vertical hinting
    \o Vertical hinting
    \o Full hinting
    \row
    \o FreeType
    \o Operating System setting
    \o No hinting
    \o Vertical hinting (light)
    \o Full hinting
    \row
    \o Cocoa on Mac OS X
    \o No hinting
    \o No hinting
    \o No hinting
    \o No hinting
    \endtable

    \note Please be aware that altering the hinting preference on Windows is available through
    the DirectWrite font engine. This is available on Windows Vista after installing the platform
    update, and on Windows 7. In order to use this extension, configure Qt using -directwrite.
    The target application will then depend on the availability of DirectWrite on the target
    system.

*/

/*!
    \since 4.8

    Set the preference for the hinting level of the glyphs to \a hintingPreference. This is a hint
    to the underlying font rendering system to use a certain level of hinting, and has varying
    support across platforms. See the table in the documentation for QFont::HintingPreference for
    more details.

    The default hinting preference is QFont::PreferDefaultHinting.
*/
void QFont::setHintingPreference(HintingPreference hintingPreference)
{
    detach();

    d->request.hintingPreference = hintingPreference;

    resolve_mask |= QFont::HintingPreferenceResolved;
}

/*!
    \since 4.8

    Returns the currently preferred hinting level for glyphs rendered with this font.
*/
QFont::HintingPreference QFont::hintingPreference() const
{
    return QFont::HintingPreference(d->request.hintingPreference);
}

/*!
    Sets the point size to \a pointSize. The point size must be
    greater than zero.

    \sa pointSize() setPointSizeF()
*/
void QFont::setPointSize(int pointSize)
{
    if (pointSize <= 0) {
        qWarning("QFont::setPointSize: Point size <= 0 (%d), must be greater than 0", pointSize);
        return;
    }

    detach();

    d->request.pointSize = qreal(pointSize);
    d->request.pixelSize = -1;

    resolve_mask |= QFont::SizeResolved;
}

/*!
    Sets the point size to \a pointSize. The point size must be
    greater than zero. The requested precision may not be achieved on
    all platforms.

    \sa pointSizeF() setPointSize() setPixelSize()
*/
void QFont::setPointSizeF(qreal pointSize)
{
    if (pointSize <= 0) {
        qWarning("QFont::setPointSizeF: Point size <= 0 (%f), must be greater than 0", pointSize);
        return;
    }

    detach();

    d->request.pointSize = pointSize;
    d->request.pixelSize = -1;

    resolve_mask |= QFont::SizeResolved;
}

/*!
    Returns the point size of the font. Returns -1 if the font size was
    specified in pixels.

    \sa pointSize() setPointSizeF() pixelSize() QFontInfo::pointSize() QFontInfo::pixelSize()
*/
qreal QFont::pointSizeF() const
{
    return d->request.pointSize;
}

/*!
    Sets the font size to \a pixelSize pixels.

    Using this function makes the font device dependent. Use
    setPointSize() or setPointSizeF() to set the size of the font
    in a device independent manner.

    \sa pixelSize()
*/
void QFont::setPixelSize(int pixelSize)
{
    if (pixelSize <= 0) {
        qWarning("QFont::setPixelSize: Pixel size <= 0 (%d)", pixelSize);
        return;
    }

    detach();

    d->request.pixelSize = pixelSize;
    d->request.pointSize = -1;

    resolve_mask |= QFont::SizeResolved;
}

/*!
    Returns the pixel size of the font if it was set with
    setPixelSize(). Returns -1 if the size was set with setPointSize()
    or setPointSizeF().

    \sa setPixelSize() pointSize() QFontInfo::pointSize() QFontInfo::pixelSize()
*/
int QFont::pixelSize() const
{
    return d->request.pixelSize;
}

#ifdef QT3_SUPPORT
/*! \obsolete

  Sets the logical pixel height of font characters when shown on
  the screen to \a pixelSize.
*/
void QFont::setPixelSizeFloat(qreal pixelSize)
{
    setPixelSize((int)pixelSize);
}
#endif

/*!
  \fn bool QFont::italic() const

    Returns true if the style() of the font is not QFont::StyleNormal

    \sa setItalic() style()
*/

/*!
  \fn void QFont::setItalic(bool enable)

  Sets the style() of the font to QFont::StyleItalic if \a enable is true;
  otherwise the style is set to QFont::StyleNormal.

  \sa italic() QFontInfo
*/

/*!
    Returns the style of the font.

    \sa setStyle()
*/
QFont::Style QFont::style() const
{
    return (QFont::Style)d->request.style;
}


/*!
  Sets the style of the font to \a style.

  \sa italic(), QFontInfo
*/
void QFont::setStyle(Style style)
{
    detach();

    d->request.style = style;
    resolve_mask |= QFont::StyleResolved;
}

/*!
    Returns the weight of the font which is one of the enumerated
    values from \l{QFont::Weight}.

    \sa setWeight(), Weight, QFontInfo
*/
int QFont::weight() const
{
    return d->request.weight;
}

/*!
    \enum QFont::Weight

    Qt uses a weighting scale from 0 to 99 similar to, but not the
    same as, the scales used in Windows or CSS. A weight of 0 is
    ultralight, whilst 99 will be an extremely black.

    This enum contains the predefined font weights:

    \value Light 25
    \value Normal 50
    \value DemiBold 63
    \value Bold 75
    \value Black 87
*/

/*!
    Sets the weight the font to \a weight, which should be a value
    from the \l QFont::Weight enumeration.

    \sa weight(), QFontInfo
*/
void QFont::setWeight(int weight)
{
    Q_ASSERT_X(weight >= 0 && weight <= 99, "QFont::setWeight", "Weight must be between 0 and 99");

    detach();

    d->request.weight = weight;
    resolve_mask |= QFont::WeightResolved;
}

/*!
    \fn bool QFont::bold() const

    Returns true if weight() is a value greater than \link Weight
    QFont::Normal \endlink; otherwise returns false.

    \sa weight(), setBold(), QFontInfo::bold()
*/

/*!
    \fn void QFont::setBold(bool enable)

    If \a enable is true sets the font's weight to \link Weight
    QFont::Bold \endlink; otherwise sets the weight to \link Weight
    QFont::Normal\endlink.

    For finer boldness control use setWeight().

    \sa bold(), setWeight()
*/

/*!
    Returns true if underline has been set; otherwise returns false.

    \sa setUnderline()
*/
bool QFont::underline() const
{
    return d->underline;
}

/*!
    If \a enable is true, sets underline on; otherwise sets underline
    off.

    \sa underline(), QFontInfo
*/
void QFont::setUnderline(bool enable)
{
    detach();

    d->underline = enable;
    resolve_mask |= QFont::UnderlineResolved;
}

/*!
    Returns true if overline has been set; otherwise returns false.

    \sa setOverline()
*/
bool QFont::overline() const
{
    return d->overline;
}

/*!
  If \a enable is true, sets overline on; otherwise sets overline off.

  \sa overline(), QFontInfo
*/
void QFont::setOverline(bool enable)
{
    detach();

    d->overline = enable;
    resolve_mask |= QFont::OverlineResolved;
}

/*!
    Returns true if strikeout has been set; otherwise returns false.

    \sa setStrikeOut()
*/
bool QFont::strikeOut() const
{
    return d->strikeOut;
}

/*!
    If \a enable is true, sets strikeout on; otherwise sets strikeout
    off.

    \sa strikeOut(), QFontInfo
*/
void QFont::setStrikeOut(bool enable)
{
    detach();

    d->strikeOut = enable;
    resolve_mask |= QFont::StrikeOutResolved;
}

/*!
    Returns true if fixed pitch has been set; otherwise returns false.

    \sa setFixedPitch(), QFontInfo::fixedPitch()
*/
bool QFont::fixedPitch() const
{
    return d->request.fixedPitch;
}

/*!
    If \a enable is true, sets fixed pitch on; otherwise sets fixed
    pitch off.

    \sa fixedPitch(), QFontInfo
*/
void QFont::setFixedPitch(bool enable)
{
    detach();

    d->request.fixedPitch = enable;
    d->request.ignorePitch = false;
    resolve_mask |= QFont::FixedPitchResolved;
}

/*!
  Returns true if kerning should be used when drawing text with this font.

  \sa setKerning()
*/
bool QFont::kerning() const
{
    return d->kerning;
}

/*!
    Enables kerning for this font if \a enable is true; otherwise
    disables it. By default, kerning is enabled.

    When kerning is enabled, glyph metrics do not add up anymore,
    even for Latin text. In other words, the assumption that
    width('a') + width('b') is equal to width("ab") is not
    neccesairly true.

    \sa kerning(), QFontMetrics
*/
void QFont::setKerning(bool enable)
{
    detach();
    d->kerning = enable;
    resolve_mask |= QFont::KerningResolved;
}

/*!
    Returns the StyleStrategy.

    The style strategy affects the \l{QFont}{font matching} algorithm.
    See \l QFont::StyleStrategy for the list of available strategies.

    \sa setStyleHint() QFont::StyleHint
*/
QFont::StyleStrategy QFont::styleStrategy() const
{
    return (StyleStrategy) d->request.styleStrategy;
}

/*!
    Returns the StyleHint.

    The style hint affects the \l{QFont}{font matching} algorithm.
    See \l QFont::StyleHint for the list of available hints.

    \sa setStyleHint(), QFont::StyleStrategy QFontInfo::styleHint()
*/
QFont::StyleHint QFont::styleHint() const
{
    return (StyleHint) d->request.styleHint;
}

/*!
    \enum QFont::StyleHint

    Style hints are used by the \l{QFont}{font matching} algorithm to
    find an appropriate default family if a selected font family is
    not available.

    \value AnyStyle leaves the font matching algorithm to choose the
           family. This is the default.

    \value SansSerif the font matcher prefer sans serif fonts.
    \value Helvetica is a synonym for \c SansSerif.

    \value Serif the font matcher prefers serif fonts.
    \value Times is a synonym for \c Serif.

    \value TypeWriter the font matcher prefers fixed pitch fonts.
    \value Courier a synonym for \c TypeWriter.

    \value OldEnglish the font matcher prefers decorative fonts.
    \value Decorative is a synonym for \c OldEnglish.

    \value Monospace the font matcher prefers fonts that map to the
    CSS generic font-family 'monospace'.

    \value Fantasy the font matcher prefers fonts that map to the
    CSS generic font-family 'fantasy'.

    \value Cursive the font matcher prefers fonts that map to the
    CSS generic font-family 'cursive'.

    \value System the font matcher prefers system fonts.
*/

/*!
    \enum QFont::StyleStrategy

    The style strategy tells the \l{QFont}{font matching} algorithm
    what type of fonts should be used to find an appropriate default
    family.

    The following strategies are available:

    \value PreferDefault the default style strategy. It does not prefer
           any type of font.
    \value PreferBitmap prefers bitmap fonts (as opposed to outline
           fonts).
    \value PreferDevice prefers device fonts.
    \value PreferOutline prefers outline fonts (as opposed to bitmap fonts).
    \value ForceOutline forces the use of outline fonts.
    \value NoAntialias don't antialias the fonts.
    \value PreferAntialias antialias if possible.
    \value OpenGLCompatible forces the use of OpenGL compatible
           fonts.
    \value NoFontMerging If the font selected for a certain writing system
           does not contain a character requested to draw, then Qt automatically chooses a similar
           looking font that contains the character. The NoFontMerging flag disables this feature.
           Please note that enabling this flag will not prevent Qt from automatically picking a
           suitable font when the selected font does not support the writing system of the text.

    Any of these may be OR-ed with one of these flags:

    \value PreferMatch prefer an exact match. The font matcher will try to
           use the exact font size that has been specified.
    \value PreferQuality prefer the best quality font. The font matcher
           will use the nearest standard point size that the font
           supports.
    \value ForceIntegerMetrics forces the use of integer values in font engines that support fractional
           font metrics.
*/

/*!
    Sets the style hint and strategy to \a hint and \a strategy,
    respectively.

    If these aren't set explicitly the style hint will default to
    \c AnyStyle and the style strategy to \c PreferDefault.

    Qt does not support style hints on X11 since this information
    is not provided by the window system.

    \sa StyleHint, styleHint(), StyleStrategy, styleStrategy(), QFontInfo
*/
void QFont::setStyleHint(StyleHint hint, StyleStrategy strategy)
{
    detach();

    if ((resolve_mask & (QFont::StyleHintResolved | QFont::StyleStrategyResolved)) &&
         (StyleHint) d->request.styleHint == hint &&
         (StyleStrategy) d->request.styleStrategy == strategy)
        return;

    d->request.styleHint = hint;
    d->request.styleStrategy = strategy;
    resolve_mask |= QFont::StyleHintResolved;
    resolve_mask |= QFont::StyleStrategyResolved;

#if defined(Q_WS_X11)
    d->request.addStyle.clear();
#endif // Q_WS_X11
}

/*!
    Sets the style strategy for the font to \a s.

    \sa QFont::StyleStrategy
*/
void QFont::setStyleStrategy(StyleStrategy s)
{
    detach();

    if ((resolve_mask & QFont::StyleStrategyResolved) &&
         s == (StyleStrategy)d->request.styleStrategy)
        return;

    d->request.styleStrategy = s;
    resolve_mask |= QFont::StyleStrategyResolved;
}


/*!
    \enum QFont::Stretch

    Predefined stretch values that follow the CSS naming convention. The higher
    the value, the more stretched the text is.

    \value UltraCondensed 50
    \value ExtraCondensed 62
    \value Condensed 75
    \value SemiCondensed 87
    \value Unstretched 100
    \value SemiExpanded 112
    \value Expanded 125
    \value ExtraExpanded 150
    \value UltraExpanded 200

    \sa setStretch() stretch()
*/

/*!
    Returns the stretch factor for the font.

    \sa setStretch()
 */
int QFont::stretch() const
{
    return d->request.stretch;
}

/*!
    Sets the stretch factor for the font.

    The stretch factor changes the width of all characters in the font
    by \a factor percent.  For example, setting \a factor to 150
    results in all characters in the font being 1.5 times (ie. 150%)
    wider.  The default stretch factor is 100.  The minimum stretch
    factor is 1, and the maximum stretch factor is 4000.

    The stretch factor is only applied to outline fonts.  The stretch
    factor is ignored for bitmap fonts.

    NOTE: QFont cannot stretch XLFD fonts.  When loading XLFD fonts on
    X11, the stretch factor is matched against a predefined set of
    values for the SETWIDTH_NAME field of the XLFD.

    \sa stretch() QFont::Stretch
*/
void QFont::setStretch(int factor)
{
    if (factor < 1 || factor > 4000) {
        qWarning("QFont::setStretch: Parameter '%d' out of range", factor);
        return;
    }

    if ((resolve_mask & QFont::StretchResolved) &&
         d->request.stretch == (uint)factor)
        return;

    detach();

    d->request.stretch = (uint)factor;
    resolve_mask |= QFont::StretchResolved;
}

/*!
    \enum QFont::SpacingType
    \since 4.4

    \value PercentageSpacing  A value of 100 will keep the spacing unchanged; a value of 200 will enlarge the
                                                   spacing after a character by the width of the character itself.
    \value AbsoluteSpacing      A positive value increases the letter spacing by the corresponding pixels; a negative
                                                   value decreases the spacing.
*/

/*!
    \since 4.4
    Returns the letter spacing for the font.

    \sa setLetterSpacing(), letterSpacingType(), setWordSpacing()
 */
qreal QFont::letterSpacing() const
{
    return d->letterSpacing.toReal();
}

/*!
    \since 4.4
    Sets the letter spacing for the font to \a spacing and the type
    of spacing to \a type.

    Letter spacing changes the default spacing between individual
    letters in the font.  The spacing between the letters can be
    made smaller as well as larger.

    \sa letterSpacing(), letterSpacingType(), setWordSpacing()
*/
void QFont::setLetterSpacing(SpacingType type, qreal spacing)
{
    const QFixed newSpacing = QFixed::fromReal(spacing);
    const bool absoluteSpacing = type == AbsoluteSpacing;
    if ((resolve_mask & QFont::LetterSpacingResolved) &&
        d->letterSpacingIsAbsolute == absoluteSpacing &&
        d->letterSpacing == newSpacing)
        return;

    detach();

    d->letterSpacing = newSpacing;
    d->letterSpacingIsAbsolute = absoluteSpacing;
    resolve_mask |= QFont::LetterSpacingResolved;
}

/*!
    \since 4.4
    Returns the spacing type used for letter spacing.

    \sa letterSpacing(), setLetterSpacing(), setWordSpacing()
*/
QFont::SpacingType QFont::letterSpacingType() const
{
    return d->letterSpacingIsAbsolute ? AbsoluteSpacing : PercentageSpacing;
}

/*!
    \since 4.4
    Returns the word spacing for the font.

    \sa setWordSpacing(), setLetterSpacing()
 */
qreal QFont::wordSpacing() const
{
    return d->wordSpacing.toReal();
}

/*!
    \since 4.4
    Sets the word spacing for the font to \a spacing.

    Word spacing changes the default spacing between individual
    words. A positive value increases the word spacing
    by a corresponding amount of pixels, while a negative value
    decreases the inter-word spacing accordingly.

    Word spacing will not apply to writing systems, where indiviaul
    words are not separated by white space.

    \sa wordSpacing(), setLetterSpacing()
*/
void QFont::setWordSpacing(qreal spacing)
{
    const QFixed newSpacing = QFixed::fromReal(spacing);
    if ((resolve_mask & QFont::WordSpacingResolved) &&
        d->wordSpacing == newSpacing)
        return;

    detach();

    d->wordSpacing = newSpacing;
    resolve_mask |= QFont::WordSpacingResolved;
}

/*!
    \enum QFont::Capitalization
    \since 4.4

    Rendering option for text this font applies to.


    \value MixedCase    This is the normal text rendering option where no capitalization change is applied.
    \value AllUppercase This alters the text to be rendered in all uppercase type.
    \value AllLowercase This alters the text to be rendered in all lowercase type.
    \value SmallCaps    This alters the text to be rendered in small-caps type.
    \value Capitalize   This alters the text to be rendered with the first character of each word as an uppercase character.
*/

/*!
    \since 4.4
    Sets the capitalization of the text in this font to \a caps.

    A font's capitalization makes the text appear in the selected capitalization mode.

    \sa capitalization()
*/
void QFont::setCapitalization(Capitalization caps)
{
    if ((resolve_mask & QFont::CapitalizationResolved) &&
        capitalization() == caps)
        return;

    detach();

    d->capital = caps;
    resolve_mask |= QFont::CapitalizationResolved;
}

/*!
    \since 4.4
    Returns the current capitalization type of the font.

    \sa setCapitalization()
*/
QFont::Capitalization QFont::capitalization() const
{
    return static_cast<QFont::Capitalization> (d->capital);
}


/*!
    If \a enable is true, turns raw mode on; otherwise turns raw mode
    off. This function only has an effect under X11.

    If raw mode is enabled, Qt will search for an X font with a
    complete font name matching the family name, ignoring all other
    values set for the QFont. If the font name matches several fonts,
    Qt will use the first font returned by X. QFontInfo \e cannot be
    used to fetch information about a QFont using raw mode (it will
    return the values set in the QFont for all parameters, including
    the family name).

    \warning Do not use raw mode unless you really, really need it! In
    most (if not all) cases, setRawName() is a much better choice.

    \sa rawMode(), setRawName()
*/
void QFont::setRawMode(bool enable)
{
    detach();

    if ((bool) d->rawMode == enable) return;

    d->rawMode = enable;
}

/*!
    Returns true if a window system font exactly matching the settings
    of this font is available.

    \sa QFontInfo
*/
bool QFont::exactMatch() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    return (d->rawMode
            ? engine->type() != QFontEngine::Box
            : d->request.exactMatch(engine->fontDef));
}

/*!
    Returns true if this font is equal to \a f; otherwise returns
    false.

    Two QFonts are considered equal if their font attributes are
    equal. If rawMode() is enabled for both fonts, only the family
    fields are compared.

    \sa operator!=() isCopyOf()
*/
bool QFont::operator==(const QFont &f) const
{
    return (f.d == d
            || (f.d->request   == d->request
                && f.d->request.pointSize == d->request.pointSize
                && f.d->underline == d->underline
                && f.d->overline  == d->overline
                && f.d->strikeOut == d->strikeOut
                && f.d->kerning == d->kerning
                && f.d->capital == d->capital
                && f.d->letterSpacingIsAbsolute == d->letterSpacingIsAbsolute
                && f.d->letterSpacing == d->letterSpacing
                && f.d->wordSpacing == d->wordSpacing
            ));
}


/*!
    Provides an arbitrary comparison of this font and font \a f.
    All that is guaranteed is that the operator returns false if both
    fonts are equal and that (f1 \< f2) == !(f2 \< f1) if the fonts
    are not equal.

    This function is useful in some circumstances, for example if you
    want to use QFont objects as keys in a QMap.

    \sa operator==() operator!=() isCopyOf()
*/
bool QFont::operator<(const QFont &f) const
{
    if (f.d == d) return false;
    // the < operator for fontdefs ignores point sizes.
    QFontDef &r1 = f.d->request;
    QFontDef &r2 = d->request;
    if (r1.pointSize != r2.pointSize) return r1.pointSize < r2.pointSize;
    if (r1.pixelSize != r2.pixelSize) return r1.pixelSize < r2.pixelSize;
    if (r1.weight != r2.weight) return r1.weight < r2.weight;
    if (r1.style != r2.style) return r1.style < r2.style;
    if (r1.stretch != r2.stretch) return r1.stretch < r2.stretch;
    if (r1.styleHint != r2.styleHint) return r1.styleHint < r2.styleHint;
    if (r1.styleStrategy != r2.styleStrategy) return r1.styleStrategy < r2.styleStrategy;
    if (r1.family != r2.family) return r1.family < r2.family;
#ifdef Q_WS_X11
    if (r1.addStyle != r2.addStyle) return r1.addStyle < r2.addStyle;
#endif // Q_WS_X11
    if (f.d->capital != d->capital) return f.d->capital < d->capital;

    if (f.d->letterSpacingIsAbsolute != d->letterSpacingIsAbsolute) return f.d->letterSpacingIsAbsolute < d->letterSpacingIsAbsolute;
    if (f.d->letterSpacing != d->letterSpacing) return f.d->letterSpacing < d->letterSpacing;
    if (f.d->wordSpacing != d->wordSpacing) return f.d->wordSpacing < d->wordSpacing;

    int f1attrs = (f.d->underline << 3) + (f.d->overline << 2) + (f.d->strikeOut<<1) + f.d->kerning;
    int f2attrs = (d->underline << 3) + (d->overline << 2) + (d->strikeOut<<1) + d->kerning;
    return f1attrs < f2attrs;
}


/*!
    Returns true if this font is different from \a f; otherwise
    returns false.

    Two QFonts are considered to be different if their font attributes
    are different. If rawMode() is enabled for both fonts, only the
    family fields are compared.

    \sa operator==()
*/
bool QFont::operator!=(const QFont &f) const
{
    return !(operator==(f));
}

/*!
   Returns the font as a QVariant
*/
QFont::operator QVariant() const
{
    return QVariant(QVariant::Font, this);
}

/*!
    Returns true if this font and \a f are copies of each other, i.e.
    one of them was created as a copy of the other and neither has
    been modified since. This is much stricter than equality.

    \sa operator=() operator==()
*/
bool QFont::isCopyOf(const QFont & f) const
{
    return d == f.d;
}

/*!
    Returns true if raw mode is used for font name matching; otherwise
    returns false.

    \sa setRawMode() rawName()
*/
bool QFont::rawMode() const
{
    return d->rawMode;
}

/*!
    Returns a new QFont that has attributes copied from \a other that
    have not been previously set on this font.
*/
QFont QFont::resolve(const QFont &other) const
{
    if (*this == other
        && (resolve_mask == other.resolve_mask || resolve_mask == 0)
        && d->dpi == other.d->dpi) {
        QFont o = other;
        o.resolve_mask = resolve_mask;
        return o;
    }

    QFont font(*this);
    font.detach();
    font.d->resolve(resolve_mask, other.d.data());

    return font;
}

/*!
    \fn uint QFont::resolve() const
    \internal
*/

/*!
    \fn void QFont::resolve(uint mask)
    \internal
*/

#ifdef QT3_SUPPORT

/*! \obsolete

  Please use QApplication::font() instead.
*/
QFont QFont::defaultFont()
{
    return QApplication::font();
}

/*! \obsolete

  Please use QApplication::setFont() instead.
*/
void QFont::setDefaultFont(const QFont &f)
{
    QApplication::setFont(f);
}

/*!
    \fn qreal QFont::pointSizeFloat() const
    \compat

    Use pointSizeF() instead.
*/

/*!
    \fn void QFont::setPointSizeFloat(qreal size)
    \compat

    Use setPointSizeF() instead.
*/
#endif




/*****************************************************************************
  QFont substitution management
 *****************************************************************************/

typedef QHash<QString, QStringList> QFontSubst;
Q_GLOBAL_STATIC(QFontSubst, globalFontSubst)

// create substitution dict
static void initFontSubst()
{
    // default substitutions
    static const char * const initTbl[] = {

#if defined(Q_WS_X11)
        "arial",        "helvetica",
        "times new roman", "times",
        "courier new",  "courier",
        "sans serif",   "helvetica",
#elif defined(Q_WS_WIN)
        "times",        "times new roman",
        "courier",      "courier new",
        "helvetica",    "arial",
        "sans serif",   "arial",
#endif

        0,              0
    };

    QFontSubst *fontSubst = globalFontSubst();
    Q_ASSERT(fontSubst != 0);
    if (!fontSubst->isEmpty())
        return;
#if defined(Q_WS_X11) && !defined(QT_NO_FONTCONFIG)
    if (X11->has_fontconfig)
        return;
#endif

    for (int i=0; initTbl[i] != 0; i += 2) {
        QStringList &list = (*fontSubst)[QString::fromLatin1(initTbl[i])];
        list.append(QString::fromLatin1(initTbl[i+1]));
    }
}

/*!
    Returns the first family name to be used whenever \a familyName is
    specified. The lookup is case insensitive.

    If there is no substitution for \a familyName, \a familyName is
    returned.

    To obtain a list of substitutions use substitutes().

    \sa setFamily() insertSubstitutions() insertSubstitution() removeSubstitution()
*/
QString QFont::substitute(const QString &familyName)
{
    initFontSubst();

    QFontSubst *fontSubst = globalFontSubst();
    Q_ASSERT(fontSubst != 0);
    QFontSubst::ConstIterator it = fontSubst->constFind(familyName.toLower());
    if (it != fontSubst->constEnd() && !(*it).isEmpty())
        return (*it).first();

    return familyName;
}


/*!
    Returns a list of family names to be used whenever \a familyName
    is specified. The lookup is case insensitive.

    If there is no substitution for \a familyName, an empty list is
    returned.

    \sa substitute() insertSubstitutions() insertSubstitution() removeSubstitution()
 */
QStringList QFont::substitutes(const QString &familyName)
{
    initFontSubst();

    QFontSubst *fontSubst = globalFontSubst();
    Q_ASSERT(fontSubst != 0);
    return fontSubst->value(familyName.toLower(), QStringList());
}


/*!
    Inserts \a substituteName into the substitution
    table for the family \a familyName.

    \sa insertSubstitutions() removeSubstitution() substitutions() substitute() substitutes()
*/
void QFont::insertSubstitution(const QString &familyName,
                               const QString &substituteName)
{
    initFontSubst();

    QFontSubst *fontSubst = globalFontSubst();
    Q_ASSERT(fontSubst != 0);
    QStringList &list = (*fontSubst)[familyName.toLower()];
    QString s = substituteName.toLower();
    if (!list.contains(s))
        list.append(s);
}


/*!
    Inserts the list of families \a substituteNames into the
    substitution list for \a familyName.

    \sa insertSubstitution(), removeSubstitution(), substitutions(), substitute()
*/
void QFont::insertSubstitutions(const QString &familyName,
                                const QStringList &substituteNames)
{
    initFontSubst();

    QFontSubst *fontSubst = globalFontSubst();
    Q_ASSERT(fontSubst != 0);
    QStringList &list = (*fontSubst)[familyName.toLower()];
    QStringList::ConstIterator it = substituteNames.constBegin();
    while (it != substituteNames.constEnd()) {
        QString s = (*it).toLower();
        if (!list.contains(s))
            list.append(s);
        it++;
    }
}

/*! \fn void QFont::initialize()
  \internal

  Internal function that initializes the font system.  The font cache
  and font dict do not alloc the keys. The key is a QString which is
  shared between QFontPrivate and QXFontName.
*/

/*! \fn void QFont::cleanup()
  \internal

  Internal function that cleans up the font system.
*/

// ### mark: should be called removeSubstitutions()
/*!
    Removes all the substitutions for \a familyName.

    \sa insertSubstitutions(), insertSubstitution(), substitutions(), substitute()
*/
void QFont::removeSubstitution(const QString &familyName)
{ // ### function name should be removeSubstitutions() or
  // ### removeSubstitutionList()
    initFontSubst();

    QFontSubst *fontSubst = globalFontSubst();
    Q_ASSERT(fontSubst != 0);
    fontSubst->remove(familyName.toLower());
}


/*!
    Returns a sorted list of substituted family names.

    \sa insertSubstitution(), removeSubstitution(), substitute()
*/
QStringList QFont::substitutions()
{
    initFontSubst();

    QFontSubst *fontSubst = globalFontSubst();
    Q_ASSERT(fontSubst != 0);
    QStringList ret;
    QFontSubst::ConstIterator it = fontSubst->constBegin();

    while (it != fontSubst->constEnd()) {
        ret.append(it.key());
        ++it;
    }

    ret.sort();
    return ret;
}


/*  \internal
    Internal function. Converts boolean font settings to an unsigned
    8-bit number. Used for serialization etc.
*/
static quint8 get_font_bits(int version, const QFontPrivate *f)
{
    Q_ASSERT(f != 0);
    quint8 bits = 0;
    if (f->request.style)
        bits |= 0x01;
    if (f->underline)
        bits |= 0x02;
    if (f->overline)
        bits |= 0x40;
    if (f->strikeOut)
        bits |= 0x04;
    if (f->request.fixedPitch)
        bits |= 0x08;
    // if (f.hintSetByUser)
    // bits |= 0x10;
    if (f->rawMode)
        bits |= 0x20;
    if (version >= QDataStream::Qt_4_0) {
        if (f->kerning)
            bits |= 0x10;
    }
    if (f->request.style == QFont::StyleOblique)
        bits |= 0x80;
    return bits;
}

static quint8 get_extended_font_bits(const QFontPrivate *f)
{
    Q_ASSERT(f != 0);
    quint8 bits = 0;
    if (f->request.ignorePitch)
        bits |= 0x01;
    if (f->letterSpacingIsAbsolute)
        bits |= 0x02;
    return bits;
}

#ifndef QT_NO_DATASTREAM

/*  \internal
    Internal function. Sets boolean font settings from an unsigned
    8-bit number. Used for serialization etc.
*/
static void set_font_bits(int version, quint8 bits, QFontPrivate *f)
{
    Q_ASSERT(f != 0);
    f->request.style         = (bits & 0x01) != 0 ? QFont::StyleItalic : QFont::StyleNormal;
    f->underline             = (bits & 0x02) != 0;
    f->overline              = (bits & 0x40) != 0;
    f->strikeOut             = (bits & 0x04) != 0;
    f->request.fixedPitch    = (bits & 0x08) != 0;
    // f->hintSetByUser      = (bits & 0x10) != 0;
    f->rawMode               = (bits & 0x20) != 0;
    if (version >= QDataStream::Qt_4_0)
        f->kerning               = (bits & 0x10) != 0;
    if ((bits & 0x80) != 0)
        f->request.style         = QFont::StyleOblique;
}

static void set_extended_font_bits(quint8 bits, QFontPrivate *f)
{
    Q_ASSERT(f != 0);
    f->request.ignorePitch = (bits & 0x01) != 0;
    f->letterSpacingIsAbsolute = (bits & 0x02) != 0;
}
#endif


/*!
    Returns the font's key, a textual representation of a font. It is
    typically used as the key for a cache or dictionary of fonts.

    \sa QMap
*/
QString QFont::key() const
{
    return toString();
}

/*!
    Returns a description of the font. The description is a
    comma-separated list of the attributes, perfectly suited for use
    in QSettings.

    \sa fromString()
 */
QString QFont::toString() const
{
    const QChar comma(QLatin1Char(','));
    return family() + comma +
        QString::number(     pointSizeF()) + comma +
        QString::number(      pixelSize()) + comma +
        QString::number((int) styleHint()) + comma +
        QString::number(         weight()) + comma +
        QString::number((int)     style()) + comma +
        QString::number((int) underline()) + comma +
        QString::number((int) strikeOut()) + comma +
        QString::number((int)fixedPitch()) + comma +
        QString::number((int)   rawMode());
}


/*!
    Sets this font to match the description \a descrip. The description
    is a comma-separated list of the font attributes, as returned by
    toString().

    \sa toString()
 */
bool QFont::fromString(const QString &descrip)
{
    QStringList l(descrip.split(QLatin1Char(',')));

    int count = l.count();
    if (!count || (count > 2 && count < 9) || count > 11) {
        qWarning("QFont::fromString: Invalid description '%s'",
                 descrip.isEmpty() ? "(empty)" : descrip.toLatin1().data());
        return false;
    }

    setFamily(l[0]);
    if (count > 1 && l[1].toDouble() > 0.0)
        setPointSizeF(l[1].toDouble());
    if (count == 9) {
        setStyleHint((StyleHint) l[2].toInt());
        setWeight(qMax(qMin(99, l[3].toInt()), 0));
        setItalic(l[4].toInt());
        setUnderline(l[5].toInt());
        setStrikeOut(l[6].toInt());
        setFixedPitch(l[7].toInt());
        setRawMode(l[8].toInt());
    } else if (count == 10) {
        if (l[2].toInt() > 0)
            setPixelSize(l[2].toInt());
        setStyleHint((StyleHint) l[3].toInt());
        setWeight(qMax(qMin(99, l[4].toInt()), 0));
        setStyle((QFont::Style)l[5].toInt());
        setUnderline(l[6].toInt());
        setStrikeOut(l[7].toInt());
        setFixedPitch(l[8].toInt());
        setRawMode(l[9].toInt());
    }
    if (count >= 9 && !d->request.fixedPitch) // assume 'false' fixedPitch equals default
        d->request.ignorePitch = true;

    return true;
}

#if !defined(Q_WS_QWS)
/*! \internal

  Internal function that dumps font cache statistics.
*/
void QFont::cacheStatistics()
{


}
#endif // !Q_WS_QWS



/*****************************************************************************
  QFont stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM

/*!
    \relates QFont

    Writes the font \a font to the data stream \a s. (toString()
    writes to a text stream.)

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/
QDataStream &operator<<(QDataStream &s, const QFont &font)
{
    if (s.version() == 1) {
        s << font.d->request.family.toLatin1();
    } else {
        s << font.d->request.family;
    }

    if (s.version() >= QDataStream::Qt_4_0) {
        // 4.0
        double pointSize = font.d->request.pointSize;
        qint32 pixelSize = font.d->request.pixelSize;
        s << pointSize;
        s << pixelSize;
    } else if (s.version() <= 3) {
        qint16 pointSize = (qint16) (font.d->request.pointSize * 10);
        if (pointSize < 0) {
#ifdef Q_WS_X11
            pointSize = (qint16)(font.d->request.pixelSize*720/QX11Info::appDpiY());
#else
            pointSize = (qint16)QFontInfo(font).pointSize() * 10;
#endif
        }
        s << pointSize;
    } else {
        s << (qint16) (font.d->request.pointSize * 10);
        s << (qint16) font.d->request.pixelSize;
    }

    s << (quint8) font.d->request.styleHint;
    if (s.version() >= QDataStream::Qt_3_1)
        s << (quint8) font.d->request.styleStrategy;
    s << (quint8) 0
      << (quint8) font.d->request.weight
      << get_font_bits(s.version(), font.d.data());
    if (s.version() >= QDataStream::Qt_4_3)
        s << (quint16)font.d->request.stretch;
    if (s.version() >= QDataStream::Qt_4_4)
        s << get_extended_font_bits(font.d.data());
    if (s.version() >= QDataStream::Qt_4_5) {
        s << font.d->letterSpacing.value();
        s << font.d->wordSpacing.value();
    }
    return s;
}


/*!
    \relates QFont

    Reads the font \a font from the data stream \a s. (fromString()
    reads from a text stream.)

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/
QDataStream &operator>>(QDataStream &s, QFont &font)
{
    font.d = new QFontPrivate;
    font.resolve_mask = QFont::AllPropertiesResolved;

    quint8 styleHint, styleStrategy = QFont::PreferDefault, charSet, weight, bits;

    if (s.version() == 1) {
        QByteArray fam;
        s >> fam;
        font.d->request.family = QString::fromLatin1(fam);
    } else {
        s >> font.d->request.family;
    }

    if (s.version() >= QDataStream::Qt_4_0) {
        // 4.0
        double pointSize;
        qint32 pixelSize;
        s >> pointSize;
        s >> pixelSize;
        font.d->request.pointSize = qreal(pointSize);
        font.d->request.pixelSize = pixelSize;
    } else {
        qint16 pointSize, pixelSize = -1;
        s >> pointSize;
        if (s.version() >= 4)
            s >> pixelSize;
        font.d->request.pointSize = qreal(pointSize / 10.);
        font.d->request.pixelSize = pixelSize;
    }
    s >> styleHint;
    if (s.version() >= QDataStream::Qt_3_1)
        s >> styleStrategy;

    s >> charSet;
    s >> weight;
    s >> bits;

    font.d->request.styleHint = styleHint;
    font.d->request.styleStrategy = styleStrategy;
    font.d->request.weight = weight;

    set_font_bits(s.version(), bits, font.d.data());

    if (s.version() >= QDataStream::Qt_4_3) {
        quint16 stretch;
        s >> stretch;
        font.d->request.stretch = stretch;
    }

    if (s.version() >= QDataStream::Qt_4_4) {
        quint8 extendedBits;
        s >> extendedBits;
        set_extended_font_bits(extendedBits, font.d.data());
    }
    if (s.version() >= QDataStream::Qt_4_5) {
        int value;
        s >> value;
        font.d->letterSpacing.setValue(value);
        s >> value;
        font.d->wordSpacing.setValue(value);
    }

    return s;
}

#endif // QT_NO_DATASTREAM


/*****************************************************************************
  QFontInfo member functions
 *****************************************************************************/

/*!
    \class QFontInfo
    \reentrant

    \brief The QFontInfo class provides general information about fonts.

    \ingroup appearance
    \ingroup shared

    The QFontInfo class provides the same access functions as QFont,
    e.g. family(), pointSize(), italic(), weight(), fixedPitch(),
    styleHint() etc. But whilst the QFont access functions return the
    values that were set, a QFontInfo object returns the values that
    apply to the font that will actually be used to draw the text.

    For example, when the program asks for a 25pt Courier font on a
    machine that has a non-scalable 24pt Courier font, QFont will
    (normally) use the 24pt Courier for rendering. In this case,
    QFont::pointSize() returns 25 and QFontInfo::pointSize() returns
    24.

    There are three ways to create a QFontInfo object.
    \list 1
    \o Calling the QFontInfo constructor with a QFont creates a font
    info object for a screen-compatible font, i.e. the font cannot be
    a printer font. If the font is changed later, the font
    info object is \e not updated.

    (Note: If you use a printer font the values returned may be
    inaccurate. Printer fonts are not always accessible so the nearest
    screen font is used if a printer font is supplied.)

    \o QWidget::fontInfo() returns the font info for a widget's font.
    This is equivalent to calling QFontInfo(widget->font()). If the
    widget's font is changed later, the font info object is \e not
    updated.

    \o QPainter::fontInfo() returns the font info for a painter's
    current font. If the painter's font is changed later, the font
    info object is \e not updated.
    \endlist

    \sa QFont QFontMetrics QFontDatabase
*/

/*!
    Constructs a font info object for \a font.

    The font must be screen-compatible, i.e. a font you use when
    drawing text in \link QWidget widgets\endlink or \link QPixmap
    pixmaps\endlink, not QPicture or QPrinter.

    The font info object holds the information for the font that is
    passed in the constructor at the time it is created, and is not
    updated if the font's attributes are changed later.

    Use QPainter::fontInfo() to get the font info when painting.
    This will give correct results also when painting on paint device
    that is not screen-compatible.
*/
QFontInfo::QFontInfo(const QFont &font)
    : d(font.d.data())
{
}

/*!
    Constructs a copy of \a fi.
*/
QFontInfo::QFontInfo(const QFontInfo &fi)
    : d(fi.d.data())
{
}

/*!
    Destroys the font info object.
*/
QFontInfo::~QFontInfo()
{
}

/*!
    Assigns the font info in \a fi.
*/
QFontInfo &QFontInfo::operator=(const QFontInfo &fi)
{
    d = fi.d.data();
    return *this;
}

/*!
    Returns the family name of the matched window system font.

    \sa QFont::family()
*/
QString QFontInfo::family() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    return engine->fontDef.family;
}

/*!
    \since 4.8

    Returns the style name of the matched window system font on
    system that supports it.

    \sa QFont::styleName()
*/
QString QFontInfo::styleName() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    return engine->fontDef.styleName;
}

/*!
    Returns the point size of the matched window system font.

    \sa pointSizeF() QFont::pointSize()
*/
int QFontInfo::pointSize() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    return qRound(engine->fontDef.pointSize);
}

/*!
    Returns the point size of the matched window system font.

    \sa QFont::pointSizeF()
*/
qreal QFontInfo::pointSizeF() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    return engine->fontDef.pointSize;
}

/*!
    Returns the pixel size of the matched window system font.

    \sa QFont::pointSize()
*/
int QFontInfo::pixelSize() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    return engine->fontDef.pixelSize;
}

/*!
    Returns the italic value of the matched window system font.

    \sa QFont::italic()
*/
bool QFontInfo::italic() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    return engine->fontDef.style != QFont::StyleNormal;
}

/*!
    Returns the style value of the matched window system font.

    \sa QFont::style()
*/
QFont::Style QFontInfo::style() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    return (QFont::Style)engine->fontDef.style;
}

/*!
    Returns the weight of the matched window system font.

    \sa QFont::weight(), bold()
*/
int QFontInfo::weight() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    return engine->fontDef.weight;

}

/*!
    \fn bool QFontInfo::bold() const

    Returns true if weight() would return a value greater than
    QFont::Normal; otherwise returns false.

    \sa weight(), QFont::bold()
*/

/*!
    Returns the underline value of the matched window system font.

  \sa QFont::underline()

  \internal

  Here we read the underline flag directly from the QFont.
  This is OK for X11 and for Windows because we always get what we want.
*/
bool QFontInfo::underline() const
{
    return d->underline;
}

/*!
    Returns the overline value of the matched window system font.

    \sa QFont::overline()

    \internal

    Here we read the overline flag directly from the QFont.
    This is OK for X11 and for Windows because we always get what we want.
*/
bool QFontInfo::overline() const
{
    return d->overline;
}

/*!
    Returns the strikeout value of the matched window system font.

  \sa QFont::strikeOut()

  \internal Here we read the strikeOut flag directly from the QFont.
  This is OK for X11 and for Windows because we always get what we want.
*/
bool QFontInfo::strikeOut() const
{
    return d->strikeOut;
}

/*!
    Returns the fixed pitch value of the matched window system font.

    \sa QFont::fixedPitch()
*/
bool QFontInfo::fixedPitch() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
#ifdef Q_OS_MAC
    if (!engine->fontDef.fixedPitchComputed) {
        QChar ch[2] = { QLatin1Char('i'), QLatin1Char('m') };
        QGlyphLayoutArray<2> g;
        int l = 2;
        engine->stringToCMap(ch, 2, &g, &l, 0);
        engine->fontDef.fixedPitch = g.advances_x[0] == g.advances_x[1];
        engine->fontDef.fixedPitchComputed = true;
    }
#endif
    return engine->fontDef.fixedPitch;
}

/*!
    Returns the style of the matched window system font.

    Currently only returns the style hint set in QFont.

    \sa QFont::styleHint() QFont::StyleHint
*/
QFont::StyleHint QFontInfo::styleHint() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    return (QFont::StyleHint) engine->fontDef.styleHint;
}

/*!
    Returns true if the font is a raw mode font; otherwise returns
    false.

    If it is a raw mode font, all other functions in QFontInfo will
    return the same values set in the QFont, regardless of the font
    actually used.

    \sa QFont::rawMode()
*/
bool QFontInfo::rawMode() const
{
    return d->rawMode;
}

/*!
    Returns true if the matched window system font is exactly the same
    as the one specified by the font; otherwise returns false.

    \sa QFont::exactMatch()
*/
bool QFontInfo::exactMatch() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    return (d->rawMode
            ? engine->type() != QFontEngine::Box
            : d->request.exactMatch(engine->fontDef));
}




// **********************************************************************
// QFontCache
// **********************************************************************

#ifdef QFONTCACHE_DEBUG
// fast timeouts for debugging
static const int fast_timeout =   1000;  // 1s
static const int slow_timeout =   5000;  // 5s
#else
static const int fast_timeout =  10000; // 10s
static const int slow_timeout = 300000; //  5m
#endif // QFONTCACHE_DEBUG

const uint QFontCache::min_cost = 4*1024; // 4mb

#ifdef QT_NO_THREAD
Q_GLOBAL_STATIC(QFontCache, theFontCache)

QFontCache *QFontCache::instance()
{
    return theFontCache();
}

void QFontCache::cleanup()
{
}
#else
Q_GLOBAL_STATIC(QThreadStorage<QFontCache *>, theFontCache)

QFontCache *QFontCache::instance()
{
    QFontCache *&fontCache = theFontCache()->localData();
    if (!fontCache)
        fontCache = new QFontCache;
    return fontCache;
}

void QFontCache::cleanup()
{
    QThreadStorage<QFontCache *> *cache = 0;
    QT_TRY {
        cache = theFontCache();
    } QT_CATCH (const std::bad_alloc &) {
        // no cache - just ignore
    }
    if (cache && cache->hasLocalData())
        cache->setLocalData(0);
}
#endif // QT_NO_THREAD

QFontCache::QFontCache()
    : QObject(), total_cost(0), max_cost(min_cost),
      current_timestamp(0), fast(false), timer_id(-1)
{
}

QFontCache::~QFontCache()
{
    clear();
    {
        EngineDataCache::ConstIterator it = engineDataCache.constBegin(),
                                 end = engineDataCache.constEnd();
        while (it != end) {
            if (it.value()->ref == 0)
                delete it.value();
            else
                FC_DEBUG("QFontCache::~QFontCache: engineData %p still has refcount %d",
                         it.value(), int(it.value()->ref));
            ++it;
        }
    }
    EngineCache::ConstIterator it = engineCache.constBegin(),
                         end = engineCache.constEnd();
    while (it != end) {
        if (--it.value().data->cache_count == 0) {
            if (it.value().data->ref == 0) {
                FC_DEBUG("QFontCache::~QFontCache: deleting engine %p key=(%d / %g %g %d %d %d)",
                         it.value().data, it.key().script, it.key().def.pointSize,
                         it.key().def.pixelSize, it.key().def.weight, it.key().def.style,
                         it.key().def.fixedPitch);

                delete it.value().data;
            } else {
                FC_DEBUG("QFontCache::~QFontCache: engine = %p still has refcount %d",
                         it.value().data, int(it.value().data->ref));
            }
        }
        ++it;
    }
}

void QFontCache::clear()
{
    {
        EngineDataCache::Iterator it = engineDataCache.begin(),
                                 end = engineDataCache.end();
        while (it != end) {
            QFontEngineData *data = it.value();
#if !defined(Q_WS_MAC)
            for (int i = 0; i < QUnicodeTables::ScriptCount; ++i) {
                if (data->engines[i]) {
                    data->engines[i]->ref.deref();
                    data->engines[i] = 0;
                }
            }
#else
            if (data->engine) {
                data->engine->ref.deref();
                data->engine = 0;
            }
#endif
            ++it;
        }
    }

    for (EngineCache::Iterator it = engineCache.begin(), end = engineCache.end();
         it != end; ++it) {
        if (it->data->ref == 0) {
            delete it->data;
            it->data = 0;
        }
    }

    for (EngineCache::Iterator it = engineCache.begin(), end = engineCache.end();
         it != end; ++it) {
        if (it->data && it->data->ref == 0) {
            delete it->data;
            it->data = 0;
        }
    }

    engineCache.clear();
}

#if defined(Q_WS_QWS) && !defined(QT_NO_QWS_QPF2)
void QFontCache::removeEngineForFont(const QByteArray &_fontName)
{

    /* This could be optimized but the code becomes much more complex if we want to handle multi
     * font engines and it is probably not worth it. Therefore we just clear the entire font cache.
     */
    Q_UNUSED(_fontName);
    clear();
}
#endif

QFontEngineData *QFontCache::findEngineData(const Key &key) const
{
    EngineDataCache::ConstIterator it = engineDataCache.find(key),
                                  end = engineDataCache.end();
    if (it == end) return 0;

    // found
    return it.value();
}

void QFontCache::insertEngineData(const Key &key, QFontEngineData *engineData)
{
    FC_DEBUG("QFontCache: inserting new engine data %p", engineData);

    engineDataCache.insert(key, engineData);
    increaseCost(sizeof(QFontEngineData));
}

QFontEngine *QFontCache::findEngine(const Key &key)
{
    EngineCache::Iterator it = engineCache.find(key),
                         end = engineCache.end();
    if (it == end) return 0;

    // found... update the hitcount and timestamp
    it.value().hits++;
    it.value().timestamp = ++current_timestamp;

    FC_DEBUG("QFontCache: found font engine\n"
             "  %p: timestamp %4u hits %3u ref %2d/%2d, type '%s'",
             it.value().data, it.value().timestamp, it.value().hits,
             int(it.value().data->ref), it.value().data->cache_count,
             it.value().data->name());

    return it.value().data;
}

void QFontCache::insertEngine(const Key &key, QFontEngine *engine)
{
    FC_DEBUG("QFontCache: inserting new engine %p", engine);

    Engine data(engine);
    data.timestamp = ++current_timestamp;

    engineCache.insert(key, data);

    // only increase the cost if this is the first time we insert the engine
    if (engine->cache_count == 0)
        increaseCost(engine->cache_cost);

    ++engine->cache_count;
}

void QFontCache::increaseCost(uint cost)
{
    cost = (cost + 512) / 1024; // store cost in kb
    cost = cost > 0 ? cost : 1;
    total_cost += cost;

    FC_DEBUG("  COST: increased %u kb, total_cost %u kb, max_cost %u kb",
            cost, total_cost, max_cost);

    if (total_cost > max_cost) {
        max_cost = total_cost;

        if (timer_id == -1 || ! fast) {
            FC_DEBUG("  TIMER: starting fast timer (%d ms)", fast_timeout);

            if (timer_id != -1) killTimer(timer_id);
            timer_id = startTimer(fast_timeout);
            fast = true;
        }
    }
}

void QFontCache::decreaseCost(uint cost)
{
    cost = (cost + 512) / 1024; // cost is stored in kb
    cost = cost > 0 ? cost : 1;
    Q_ASSERT(cost <= total_cost);
    total_cost -= cost;

    FC_DEBUG("  COST: decreased %u kb, total_cost %u kb, max_cost %u kb",
            cost, total_cost, max_cost);
}

#if defined(Q_WS_WIN) || defined (Q_WS_QWS)
void QFontCache::cleanupPrinterFonts()
{
    FC_DEBUG("QFontCache::cleanupPrinterFonts");

    {
        FC_DEBUG("  CLEAN engine data:");

        // clean out all unused engine data
        EngineDataCache::Iterator it = engineDataCache.begin(),
                                 end = engineDataCache.end();
        while (it != end) {
            if (it.key().screen == 0) {
                ++it;
                continue;
            }

            if(it.value()->ref != 0) {
                for(int i = 0; i < QUnicodeTables::ScriptCount; ++i) {
                    if(it.value()->engines[i]) {
                        it.value()->engines[i]->ref.deref();
                        it.value()->engines[i] = 0;
                    }
                }
                ++it;
            } else {

                EngineDataCache::Iterator rem = it++;

                decreaseCost(sizeof(QFontEngineData));

                FC_DEBUG("    %p", rem.value());

                delete rem.value();
                engineDataCache.erase(rem);
            }
        }
    }

    EngineCache::Iterator it = engineCache.begin(),
                         end = engineCache.end();
    while(it != end) {
        if (it.value().data->ref != 0 || it.key().screen == 0) {
            ++it;
            continue;
        }

        FC_DEBUG("    %p: timestamp %4u hits %2u ref %2d/%2d, type '%s'",
                 it.value().data, it.value().timestamp, it.value().hits,
                 int(it.value().data->ref), it.value().data->cache_count,
                 it.value().data->name());

        if (--it.value().data->cache_count == 0) {
            FC_DEBUG("    DELETE: last occurrence in cache");

            decreaseCost(it.value().data->cache_cost);
            delete it.value().data;
        }

        engineCache.erase(it++);
    }
}
#endif

void QFontCache::timerEvent(QTimerEvent *)
{
    FC_DEBUG("QFontCache::timerEvent: performing cache maintenance (timestamp %u)",
              current_timestamp);

    if (total_cost <= max_cost && max_cost <= min_cost) {
        FC_DEBUG("  cache redused sufficiently, stopping timer");

        killTimer(timer_id);
        timer_id = -1;
        fast = false;

        return;
    }

    // go through the cache and count up everything in use
    uint in_use_cost = 0;

    {
        FC_DEBUG("  SWEEP engine data:");

        // make sure the cost of each engine data is at least 1kb
        const uint engine_data_cost =
            sizeof(QFontEngineData) > 1024 ? sizeof(QFontEngineData) : 1024;

        EngineDataCache::ConstIterator it = engineDataCache.constBegin(),
                                      end = engineDataCache.constEnd();
        for (; it != end; ++it) {
#ifdef QFONTCACHE_DEBUG
            FC_DEBUG("    %p: ref %2d", it.value(), int(it.value()->ref));

#  if defined(Q_WS_X11) || defined(Q_WS_WIN)
            // print out all engines
            for (int i = 0; i < QUnicodeTables::ScriptCount; ++i) {
                if (! it.value()->engines[i])
                    continue;
                FC_DEBUG("      contains %p", it.value()->engines[i]);
            }
#  endif // Q_WS_X11 || Q_WS_WIN
#endif // QFONTCACHE_DEBUG

            if (it.value()->ref != 0)
                in_use_cost += engine_data_cost;
        }
    }

    {
        FC_DEBUG("  SWEEP engine:");

        EngineCache::ConstIterator it = engineCache.constBegin(),
                                  end = engineCache.constEnd();
        for (; it != end; ++it) {
            FC_DEBUG("    %p: timestamp %4u hits %2u ref %2d/%2d, cost %u bytes",
                     it.value().data, it.value().timestamp, it.value().hits,
                     int(it.value().data->ref), it.value().data->cache_count,
                     it.value().data->cache_cost);

            if (it.value().data->ref != 0)
                in_use_cost += it.value().data->cache_cost / it.value().data->cache_count;
        }

        // attempt to make up for rounding errors
        in_use_cost += engineCache.size();
    }

    in_use_cost = (in_use_cost + 512) / 1024; // cost is stored in kb

    /*
      calculate the new maximum cost for the cache

      NOTE: in_use_cost is *not* correct due to rounding errors in the
      above algorithm.  instead of worrying about getting the
      calculation correct, we are more interested in speed, and use
      in_use_cost as a floor for new_max_cost
    */
    uint new_max_cost = qMax(qMax(max_cost / 2, in_use_cost), min_cost);

    FC_DEBUG("  after sweep, in use %u kb, total %u kb, max %u kb, new max %u kb",
              in_use_cost, total_cost, max_cost, new_max_cost);

    if (new_max_cost == max_cost) {
        if (fast) {
            FC_DEBUG("  cannot shrink cache, slowing timer");

            killTimer(timer_id);
            timer_id = startTimer(slow_timeout);
            fast = false;
        }

        return;
    } else if (! fast) {
        FC_DEBUG("  dropping into passing gear");

        killTimer(timer_id);
        timer_id = startTimer(fast_timeout);
        fast = true;
    }

    max_cost = new_max_cost;

    {
        FC_DEBUG("  CLEAN engine data:");

        // clean out all unused engine data
        EngineDataCache::Iterator it = engineDataCache.begin(),
                                 end = engineDataCache.end();
        while (it != end) {
            if (it.value()->ref != 0) {
                ++it;
                continue;
            }

            EngineDataCache::Iterator rem = it++;

            decreaseCost(sizeof(QFontEngineData));

            FC_DEBUG("    %p", rem.value());

            delete rem.value();
            engineDataCache.erase(rem);
        }
    }

    // clean out the engine cache just enough to get below our new max cost
    uint current_cost;
    do {
        current_cost = total_cost;

        EngineCache::Iterator it = engineCache.begin(),
                             end = engineCache.end();
        // determine the oldest and least popular of the unused engines
        uint oldest = ~0u;
        uint least_popular = ~0u;

        for (; it != end; ++it) {
            if (it.value().data->ref != 0)
                continue;

            if (it.value().timestamp < oldest &&
                 it.value().hits <= least_popular) {
                oldest = it.value().timestamp;
                least_popular = it.value().hits;
            }
        }

        FC_DEBUG("    oldest %u least popular %u", oldest, least_popular);

        for (it = engineCache.begin(); it != end; ++it) {
            if (it.value().data->ref == 0 &&
                 it.value().timestamp == oldest &&
                 it.value().hits == least_popular)
                break;
        }

        if (it != end) {
            FC_DEBUG("    %p: timestamp %4u hits %2u ref %2d/%2d, type '%s'",
                     it.value().data, it.value().timestamp, it.value().hits,
                     int(it.value().data->ref), it.value().data->cache_count,
                     it.value().data->name());

            if (--it.value().data->cache_count == 0) {
                FC_DEBUG("    DELETE: last occurrence in cache");

                decreaseCost(it.value().data->cache_cost);
                delete it.value().data;
            } else {
                /*
                  this particular font engine is in the cache multiple
                  times...  set current_cost to zero, so that we can
                  keep looping to get rid of all occurrences
                */
                current_cost = 0;
            }

            engineCache.erase(it);
        }
    } while (current_cost != total_cost && total_cost > max_cost);
}


#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug stream, const QFont &font)
{
    return stream << "QFont(" << font.toString() << ')';
}
#endif

QT_END_NAMESPACE
