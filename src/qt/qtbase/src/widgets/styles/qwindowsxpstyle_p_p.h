/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QWINDOWSXPSTYLE_P_P_H
#define QWINDOWSXPSTYLE_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include "qwindowsxpstyle_p.h"
#include "qwindowsstyle_p_p.h"
#include <qmap.h>
#include <qt_windows.h>

// Note, these tests are duplicated in qwizard_win.cpp.
#ifdef Q_CC_GNU
#  include <w32api.h>
#  if (__W32API_MAJOR_VERSION >= 3 || (__W32API_MAJOR_VERSION == 2 && __W32API_MINOR_VERSION >= 5))
#    ifdef _WIN32_WINNT
#      undef _WIN32_WINNT
#    endif
#    define _WIN32_WINNT 0x0501
#    include <commctrl.h>
#  endif
#endif

#include <uxtheme.h>

#if WINVER >= 0x0600
#include <vssym32.h>
#else
#include <tmschema.h>
#endif

#include <limits.h>

QT_BEGIN_NAMESPACE

// Older Platform SDKs do not have the extended DrawThemeBackgroundEx
// function. We add the needed parts here, and use the extended
// function dynamically, if available in uxtheme.dll. Else, we revert
// back to using the DrawThemeBackground function.
#ifndef DTBG_OMITBORDER
#  ifndef DTBG_CLIPRECT
#   define DTBG_CLIPRECT        0x00000001
#  endif
#  ifndef DTBG_DRAWSOLID
#   define DTBG_DRAWSOLID       0x00000002
#  endif
#  ifndef DTBG_OMITBORDER
#   define DTBG_OMITBORDER      0x00000004
#  endif
#  ifndef DTBG_OMITCONTENT
#   define DTBG_OMITCONTENT     0x00000008
#  endif
#  ifndef DTBG_COMPUTINGREGION
#   define DTBG_COMPUTINGREGION 0x00000010
#  endif
#  ifndef DTBG_MIRRORDC
#   define DTBG_MIRRORDC        0x00000020
#  endif
    typedef struct _DTBGOPTS
    {
        DWORD dwSize;
        DWORD dwFlags;
        RECT rcClip;
    } DTBGOPTS, *PDTBGOPTS;
#endif // _DTBGOPTS

// Undefined for some compile environments
#ifndef TMT_TEXTCOLOR
#  define TMT_TEXTCOLOR 3803
#endif
#ifndef TMT_BORDERCOLORHINT
#  define TMT_BORDERCOLORHINT 3822
#endif
#ifndef TMT_BORDERSIZE
#  define TMT_BORDERSIZE 2403
#endif
#ifndef TMT_BORDERONLY
#  define TMT_BORDERONLY 2203
#endif
#ifndef TMT_TRANSPARENTCOLOR
#  define TMT_TRANSPARENTCOLOR 3809
#endif
#ifndef TMT_CAPTIONMARGINS
#  define TMT_CAPTIONMARGINS 3603
#endif
#ifndef TMT_CONTENTMARGINS
#  define TMT_CONTENTMARGINS 3602
#endif
#ifndef TMT_SIZINGMARGINS
#  define TMT_SIZINGMARGINS 3601
#endif
#ifndef TMT_GLYPHTYPE
#  define TMT_GLYPHTYPE 4012
#endif
#ifndef TMT_BGTYPE
#  define TMT_BGTYPE 4001
#endif
#ifndef TMT_TEXTSHADOWTYPE
#    define TMT_TEXTSHADOWTYPE 4010
#endif
#ifndef TMT_BORDERCOLOR
#    define TMT_BORDERCOLOR 3801
#endif
#ifndef BT_IMAGEFILE
#  define BT_IMAGEFILE 0
#endif
#ifndef BT_BORDERFILL
#  define BT_BORDERFILL 1
#endif
#ifndef BT_NONE
#  define BT_NONE 2
#endif
#ifndef TMT_FILLCOLOR
#  define TMT_FILLCOLOR 3802
#endif
#ifndef TMT_PROGRESSCHUNKSIZE
#  define TMT_PROGRESSCHUNKSIZE 2411
#endif

// TMT_TEXTSHADOWCOLOR is wrongly defined in mingw
#if TMT_TEXTSHADOWCOLOR != 3818
#undef TMT_TEXTSHADOWCOLOR
#define TMT_TEXTSHADOWCOLOR 3818
#endif
#ifndef TST_NONE
#  define TST_NONE 0
#endif

#ifndef GT_NONE
#  define GT_NONE 0
#endif
#ifndef GT_IMAGEGLYPH
#  define GT_IMAGEGLYPH 1
#endif

// These defines are missing from the tmschema, but still exist as
// states for their parts
#ifndef MINBS_INACTIVE
#define MINBS_INACTIVE 5
#endif
#ifndef MAXBS_INACTIVE
#define MAXBS_INACTIVE 5
#endif
#ifndef RBS_INACTIVE
#define RBS_INACTIVE 5
#endif
#ifndef HBS_INACTIVE
#define HBS_INACTIVE 5
#endif
#ifndef CBS_INACTIVE
#define CBS_INACTIVE 5
#endif

// Uncomment define below to build debug assisting code, and output
// #define DEBUG_XP_STYLE

#if !defined(QT_NO_STYLE_WINDOWSXP)

// Declarations -----------------------------------------------------------------------------------
class XPThemeData
{
public:
    explicit XPThemeData(const QWidget *w = 0, QPainter *p = 0, int themeIn = -1,
                         int part = 0, int state = 0, const QRect &r = QRect())
        : widget(w), painter(p), theme(themeIn), htheme(0), partId(part), stateId(state),
          mirrorHorizontally(false), mirrorVertically(false), noBorder(false),
          noContent(false), rotate(0), rect(r)
    {}

    HRGN mask(QWidget *widget);
    HTHEME handle();

    static RECT toRECT(const QRect &qr);
    bool isValid();

    QSize size();
    QMargins margins(const QRect &rect, int propId = TMT_CONTENTMARGINS);
    QMargins margins(int propId = TMT_CONTENTMARGINS);

    static QSize themeSize(const QWidget *w = 0, QPainter *p = 0, int themeIn = -1, int part = 0, int state = 0);
    static QMargins themeMargins(const QRect &rect, const QWidget *w = 0, QPainter *p = 0, int themeIn = -1,
                                 int part = 0, int state = 0, int propId = TMT_CONTENTMARGINS);
    static QMargins themeMargins(const QWidget *w = 0, QPainter *p = 0, int themeIn = -1,
                                 int part = 0, int state = 0, int propId = TMT_CONTENTMARGINS);

    const QWidget *widget;
    QPainter *painter;

    int theme;
    HTHEME htheme;
    int partId;
    int stateId;

    uint mirrorHorizontally : 1;
    uint mirrorVertically : 1;
    uint noBorder : 1;
    uint noContent : 1;
    uint rotate;
    QRect rect;
};

struct ThemeMapKey {
    int theme;
    int partId;
    int stateId;
    bool noBorder;
    bool noContent;

    ThemeMapKey() : partId(-1), stateId(-1) {}
    ThemeMapKey(const XPThemeData &data)
        : theme(data.theme), partId(data.partId), stateId(data.stateId),
        noBorder(data.noBorder), noContent(data.noContent) {}

};

inline uint qHash(const ThemeMapKey &key)
{ return key.theme ^ key.partId ^ key.stateId; }

inline bool operator==(const ThemeMapKey &k1, const ThemeMapKey &k2)
{
    return k1.theme == k2.theme
           && k1.partId == k2.partId
           && k1.stateId == k2.stateId;
}

enum AlphaChannelType {
    UnknownAlpha = -1,          // Alpha of part & state not yet known
    NoAlpha,                    // Totally opaque, no need to touch alpha (RGB)
    MaskAlpha,                  // Alpha channel must be fixed            (ARGB)
    RealAlpha                   // Proper alpha values from Windows       (ARGB_Premultiplied)
};

struct ThemeMapData {
    AlphaChannelType alphaType; // Which type of alpha on part & state

    bool dataValid         : 1; // Only used to detect if hash value is ok
    bool partIsTransparent : 1;
    bool hasAlphaChannel   : 1; // True =  part & state has real Alpha
    bool wasAlphaSwapped   : 1; // True =  alpha channel needs to be swapped
    bool hadInvalidAlpha   : 1; // True =  alpha channel contained invalid alpha values

    ThemeMapData() : dataValid(false), partIsTransparent(false),
                     hasAlphaChannel(false), wasAlphaSwapped(false), hadInvalidAlpha(false) {}
};

struct QWindowsUxThemeLib {
    typedef bool (WINAPI *PtrIsAppThemed)();
    typedef bool (WINAPI *PtrIsThemeActive)();
    typedef HTHEME (WINAPI *PtrOpenThemeData)(HWND hwnd, LPCWSTR pszClassList);
    typedef HRESULT (WINAPI *PtrCloseThemeData)(HTHEME hTheme);
    typedef HRESULT (WINAPI *PtrDrawThemeBackground)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, OPTIONAL const RECT *pClipRect);
    typedef HRESULT (WINAPI *PtrDrawThemeBackgroundEx)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, OPTIONAL const DTBGOPTS *pOptions);
    typedef HRESULT (WINAPI *PtrGetCurrentThemeName)(OUT LPWSTR pszThemeFileName, int cchMaxNameChars, OUT OPTIONAL LPWSTR pszColorBuff, int cchMaxColorChars, OUT OPTIONAL LPWSTR pszSizeBuff, int cchMaxSizeChars);
    typedef HRESULT (WINAPI *PtrGetThemeDocumentationProperty)(LPCWSTR pszThemeName, LPCWSTR pszPropertyName, OUT LPWSTR pszValueBuff, int cchMaxValChars);
    typedef HRESULT (WINAPI *PtrGetThemeBool)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT BOOL *pfVal);
    typedef HRESULT (WINAPI *PtrGetThemeColor)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT COLORREF *pColor);
    typedef HRESULT (WINAPI *PtrGetThemeEnumValue)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT int *piVal);
    typedef HRESULT (WINAPI *PtrGetThemeFilename)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT LPWSTR pszThemeFileName, int cchMaxBuffChars);
    typedef HRESULT (WINAPI *PtrGetThemeFont)(HTHEME hTheme, OPTIONAL HDC hdc, int iPartId, int iStateId, int iPropId, OUT LOGFONT *pFont);
    typedef HRESULT (WINAPI *PtrGetThemeInt)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT int *piVal);
    typedef HRESULT (WINAPI *PtrGetThemeIntList)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT INTLIST *pIntList);
    typedef HRESULT (WINAPI *PtrGetThemeMargins)(HTHEME hTheme, OPTIONAL HDC hdc, int iPartId, int iStateId, int iPropId, OPTIONAL RECT *prc, OUT MARGINS *pMargins);
    typedef HRESULT (WINAPI *PtrGetThemeMetric)(HTHEME hTheme, OPTIONAL HDC hdc, int iPartId, int iStateId, int iPropId, OUT int *piVal);
    typedef HRESULT (WINAPI *PtrGetThemePartSize)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, OPTIONAL RECT *prc, enum THEMESIZE eSize, OUT SIZE *psz);
    typedef HRESULT (WINAPI *PtrGetThemePosition)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT POINT *pPoint);
    typedef HRESULT (WINAPI *PtrGetThemePropertyOrigin)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT enum PROPERTYORIGIN *pOrigin);
    typedef HRESULT (WINAPI *PtrGetThemeRect)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT RECT *pRect);
    typedef HRESULT (WINAPI *PtrGetThemeString)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, OUT LPWSTR pszBuff, int cchMaxBuffChars);
    typedef HRESULT (WINAPI *PtrGetThemeBackgroundRegion)(HTHEME hTheme, OPTIONAL HDC hdc, int iPartId, int iStateId, const RECT *pRect, OUT HRGN *pRegion);
    typedef BOOL (WINAPI *PtrIsThemeBackgroundPartiallyTransparent)(HTHEME hTheme, int iPartId, int iStateId);
    typedef HRESULT (WINAPI *PtrSetWindowTheme)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
    typedef HRESULT (WINAPI *PtrGetThemeTransitionDuration)(HTHEME hTheme, int iPartId, int iStateFromId, int iStateToId, int iPropId, int *pDuration);

    static bool resolveSymbols();

    static PtrIsAppThemed pIsAppThemed;
    static PtrIsThemeActive pIsThemeActive;
    static PtrOpenThemeData pOpenThemeData;
    static PtrCloseThemeData pCloseThemeData;
    static PtrDrawThemeBackground pDrawThemeBackground;
    static PtrDrawThemeBackgroundEx pDrawThemeBackgroundEx;
    static PtrGetCurrentThemeName pGetCurrentThemeName;
    static PtrGetThemeBool pGetThemeBool;
    static PtrGetThemeColor pGetThemeColor;
    static PtrGetThemeEnumValue pGetThemeEnumValue;
    static PtrGetThemeFilename pGetThemeFilename;
    static PtrGetThemeFont pGetThemeFont;
    static PtrGetThemeInt pGetThemeInt;
    static PtrGetThemeIntList pGetThemeIntList;
    static PtrGetThemeMargins pGetThemeMargins;
    static PtrGetThemeMetric pGetThemeMetric;
    static PtrGetThemePartSize pGetThemePartSize;
    static PtrGetThemePosition pGetThemePosition;
    static PtrGetThemePropertyOrigin pGetThemePropertyOrigin;
    static PtrGetThemeRect pGetThemeRect;
    static PtrGetThemeString pGetThemeString;
    static PtrGetThemeBackgroundRegion pGetThemeBackgroundRegion;
    static PtrGetThemeDocumentationProperty pGetThemeDocumentationProperty;
    static PtrIsThemeBackgroundPartiallyTransparent pIsThemeBackgroundPartiallyTransparent;
    static PtrSetWindowTheme pSetWindowTheme;
    static PtrGetThemeTransitionDuration pGetThemeTransitionDuration; // Windows Vista onwards.
};

class QWindowsXPStylePrivate : public QWindowsStylePrivate, public QWindowsUxThemeLib
{
    Q_DECLARE_PUBLIC(QWindowsXPStyle)
public:
    enum Theme {
        ButtonTheme,
        ComboboxTheme,
        EditTheme,
        HeaderTheme,
        ListViewTheme,
        MenuTheme,
        ProgressTheme,
        RebarTheme,
        ScrollBarTheme,
        SpinTheme,
        TabTheme,
        TaskDialogTheme,
        ToolBarTheme,
        ToolTipTheme,
        TrackBarTheme,
        TreeViewTheme,
        WindowTheme,
        StatusTheme,
        NThemes
    };

    QWindowsXPStylePrivate()
        : QWindowsStylePrivate(), hasInitColors(false), bufferDC(0), bufferBitmap(0), nullBitmap(0),
          bufferPixels(0), bufferW(0), bufferH(0)
    { init(); }

    ~QWindowsXPStylePrivate()
    { cleanup(); }

    static int pixelMetricFromSystemDp(QStyle::PixelMetric pm, const QStyleOption *option = 0, const QWidget *widget = 0);
    static int fixedPixelMetric(QStyle::PixelMetric pm, const QStyleOption *option = 0, const QWidget *widget = 0);

    static HWND winId(const QWidget *widget);

    void init(bool force = false);
    void cleanup(bool force = false);
    void cleanupHandleMap();
    const QPixmap *tabBody(QWidget *widget);

    HBITMAP buffer(int w = 0, int h = 0);
    HDC bufferHDC()
    { return bufferDC;}

    static bool resolveSymbols();
    static bool useXP(bool update = false);
    static QRect scrollBarGripperBounds(QStyle::State flags, const QWidget *widget, XPThemeData *theme);

    bool isTransparent(XPThemeData &themeData);
    QRegion region(XPThemeData &themeData);

    void setTransparency(QWidget *widget, XPThemeData &themeData);
    void drawBackground(XPThemeData &themeData);
    void drawBackgroundThruNativeBuffer(XPThemeData &themeData);
    void drawBackgroundDirectly(XPThemeData &themeData);

    bool hasAlphaChannel(const QRect &rect);
    bool fixAlphaChannel(const QRect &rect);
    bool swapAlphaChannel(const QRect &rect, bool allPixels = false);

    QRgb groupBoxTextColor;
    QRgb groupBoxTextColorDisabled;
    QRgb sliderTickColor;
    bool hasInitColors;

    static HTHEME createTheme(int theme, HWND hwnd);
    static QString themeName(int theme);
    static inline bool hasTheme(int theme) { return theme >= 0 && theme < NThemes && m_themes[theme]; }
    static bool isItemViewDelegateLineEdit(const QWidget *widget);

    QIcon dockFloat, dockClose;

private:
#ifdef DEBUG_XP_STYLE
    void dumpNativeDIB(int w, int h);
    void showProperties(XPThemeData &themeData);
#endif

    static QBasicAtomicInt ref;
    static bool use_xp;
    static QPixmap *tabbody;

    QHash<ThemeMapKey, ThemeMapData> alphaCache;
    HDC bufferDC;
    HBITMAP bufferBitmap;
    HBITMAP nullBitmap;
    uchar *bufferPixels;
    int bufferW, bufferH;

    static HTHEME m_themes[NThemes];
};

inline QSize XPThemeData::size()
{
    QSize result(0, 0);
    if (isValid()) {
        SIZE size;
        if (SUCCEEDED(QWindowsXPStylePrivate::pGetThemePartSize(handle(), 0, partId, stateId, 0, TS_TRUE, &size)))
            result = QSize(size.cx, size.cy);
    }
    return result;
}

inline QMargins XPThemeData::margins(const QRect &qRect, int propId)
{
    QMargins result(0, 0, 0 ,0);
    if (isValid()) {
        MARGINS margins;
        RECT rect = XPThemeData::toRECT(qRect);
        if (SUCCEEDED(QWindowsXPStylePrivate::pGetThemeMargins(handle(), 0, partId, stateId, propId, &rect, &margins)))
            result = QMargins(margins.cxLeftWidth, margins.cyTopHeight, margins.cxRightWidth, margins.cyBottomHeight);
    }
    return result;
}

inline QMargins XPThemeData::margins(int propId)
{
    QMargins result(0, 0, 0 ,0);
    if (isValid()) {
        MARGINS margins;
        if (SUCCEEDED(QWindowsXPStylePrivate::pGetThemeMargins(handle(), 0, partId, stateId, propId, NULL, &margins)))
            result = QMargins(margins.cxLeftWidth, margins.cyTopHeight, margins.cxRightWidth, margins.cyBottomHeight);
    }
    return result;
}

inline QSize XPThemeData::themeSize(const QWidget *w, QPainter *p, int themeIn, int part, int state)
{
    XPThemeData theme(w, p, themeIn, part, state);
    return theme.size();
}

inline QMargins XPThemeData::themeMargins(const QRect &rect, const QWidget *w, QPainter *p, int themeIn,
                                          int part, int state, int propId)
{
    XPThemeData theme(w, p, themeIn, part, state);
    return theme.margins(rect, propId);
}

inline QMargins XPThemeData::themeMargins(const QWidget *w, QPainter *p, int themeIn,
                                          int part, int state, int propId)
{
    XPThemeData theme(w, p, themeIn, part, state);
    return theme.margins(propId);
}

#endif // QT_NO_STYLE_WINDOWS

QT_END_NAMESPACE

#endif //QWINDOWSXPSTYLE_P_P_H
