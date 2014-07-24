/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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
#include "qwindowsxpstyle_p.h"
#include "qwindowsxpstyle_p_p.h"

#if !defined(QT_NO_STYLE_WINDOWSXP) || defined(QT_PLUGIN)

#include <private/qobject_p.h>
#include <private/qpaintengine_raster_p.h>
#include <private/qapplication_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <private/qstylehelper_p.h>
#include <private/qwidget_p.h>
#include <private/qsystemlibrary_p.h>
#include <qpainter.h>
#include <qpaintengine.h>
#include <qwidget.h>
#include <qbackingstore.h>
#include <qapplication.h>
#include <qpixmapcache.h>

#include <qdesktopwidget.h>
#include <qtoolbutton.h>
#include <qtabbar.h>
#include <qcombobox.h>
#include <qscrollbar.h>
#include <qheaderview.h>
#include <qspinbox.h>
#include <qlistview.h>
#include <qstackedwidget.h>
#include <qpushbutton.h>
#include <qtoolbar.h>
#include <qlabel.h>
#include <qvarlengtharray.h>
#include <qdebug.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

// Runtime resolved theme engine function calls
typedef bool (WINAPI *PtrIsAppThemed)();
typedef bool (WINAPI *PtrIsThemeActive)();
typedef HRESULT (WINAPI *PtrGetThemePartSize)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, OPTIONAL RECT *prc, enum THEMESIZE eSize, OUT SIZE *psz);
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

static PtrIsAppThemed pIsAppThemed = 0;
static PtrIsThemeActive pIsThemeActive = 0;
static PtrOpenThemeData pOpenThemeData = 0;
static PtrCloseThemeData pCloseThemeData = 0;
static PtrDrawThemeBackground pDrawThemeBackground = 0;
static PtrDrawThemeBackgroundEx pDrawThemeBackgroundEx = 0;
static PtrGetCurrentThemeName pGetCurrentThemeName = 0;
static PtrGetThemeBool pGetThemeBool = 0;
static PtrGetThemeColor pGetThemeColor = 0;
static PtrGetThemeEnumValue pGetThemeEnumValue = 0;
static PtrGetThemeFilename pGetThemeFilename = 0;
static PtrGetThemeFont pGetThemeFont = 0;
static PtrGetThemeInt pGetThemeInt = 0;
static PtrGetThemeIntList pGetThemeIntList = 0;
static PtrGetThemeMargins pGetThemeMargins = 0;
static PtrGetThemeMetric pGetThemeMetric = 0;
static PtrGetThemePartSize pGetThemePartSize = 0;
static PtrGetThemePosition pGetThemePosition = 0;
static PtrGetThemePropertyOrigin pGetThemePropertyOrigin = 0;
static PtrGetThemeRect pGetThemeRect = 0;
static PtrGetThemeString pGetThemeString = 0;
static PtrGetThemeBackgroundRegion pGetThemeBackgroundRegion = 0;
static PtrGetThemeDocumentationProperty pGetThemeDocumentationProperty = 0;
static PtrIsThemeBackgroundPartiallyTransparent pIsThemeBackgroundPartiallyTransparent = 0;

// General const values
static const int windowsItemFrame        =  2; // menu item frame width
static const int windowsItemHMargin      =  3; // menu item hor text margin
static const int windowsItemVMargin      =  0; // menu item ver text margin
static const int windowsArrowHMargin     =  6; // arrow horizontal margin
static const int windowsRightBorder      = 12; // right border on windows

// External function calls
extern Q_WIDGETS_EXPORT HDC qt_win_display_dc();
extern QRegion qt_region_from_HRGN(HRGN rgn);

// Theme names matching the QWindowsXPStylePrivate::Theme enumeration.
static const wchar_t *themeNames[QWindowsXPStylePrivate::NThemes] =
{
    L"BUTTON",   L"COMBOBOX",   L"EDIT",    L"HEADER",    L"LISTVIEW",
    L"MENU",     L"PROGRESS",   L"REBAR",   L"SCROLLBAR", L"SPIN",
    L"TAB",      L"TASKDIALOG", L"TOOLBAR", L"TOOLTIP",   L"TRACKBAR",
    L"TREEVIEW", L"WINDOW",     L"STATUS"
};

static inline QBackingStore *backingStoreForWidget(const QWidget *widget)
{
    if (QBackingStore *backingStore = widget->backingStore())
        return backingStore;
    if (const QWidget *topLevel = widget->nativeParentWidget())
        if (QBackingStore *topLevelBackingStore = topLevel->backingStore())
            return topLevelBackingStore;
    return 0;
}

static inline HDC hdcForWidgetBackingStore(const QWidget *widget)
{
    if (QBackingStore *backingStore = backingStoreForWidget(widget)) {
        QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
        return static_cast<HDC>(nativeInterface->nativeResourceForBackingStore(QByteArrayLiteral("getDC"), backingStore));
    }
    return 0;
}

// Theme data helper ------------------------------------------------------------------------------
/* \internal
    Returns \c true if the themedata is valid for use.
*/
bool XPThemeData::isValid()
{
    return QWindowsXPStylePrivate::useXP() && theme >= 0 && handle();
}


/* \internal
    Returns the theme engine handle to the specific class.
    If the handle hasn't been opened before, it opens the data, and
    adds it to a static map, for caching.
*/
HTHEME XPThemeData::handle()
{
    if (!QWindowsXPStylePrivate::useXP())
        return 0;

    if (!htheme)
        htheme = QWindowsXPStylePrivate::createTheme(theme, QWindowsXPStylePrivate::winId(widget));
    return htheme;
}

/* \internal
    Converts a QRect to the native RECT structure.
*/
RECT XPThemeData::toRECT(const QRect &qr)
{
    RECT r;
    r.left = qr.x();
    r.right = qr.x() + qr.width();
    r.top = qr.y();
    r.bottom = qr.y() + qr.height();
    return r;
}

/* \internal
    Returns the native region of a part, if the part is considered
    transparent. The region is scaled to the parts size (rect).
*/
HRGN XPThemeData::mask(QWidget *widget)
{
    if (!pIsThemeBackgroundPartiallyTransparent(handle(), partId, stateId))
        return 0;

    HRGN hrgn;
    HDC dc = 0;
    if (widget)
        dc = hdcForWidgetBackingStore(widget);
    RECT nativeRect = toRECT(rect);
    pGetThemeBackgroundRegion(handle(), dc, partId, stateId, &nativeRect, &hrgn);
    return hrgn;
}

// QWindowsXPStylePrivate -------------------------------------------------------------------------
// Static initializations
QPixmap *QWindowsXPStylePrivate::tabbody = 0;
HTHEME QWindowsXPStylePrivate::m_themes[NThemes];
bool QWindowsXPStylePrivate::use_xp = false;
QBasicAtomicInt QWindowsXPStylePrivate::ref = Q_BASIC_ATOMIC_INITIALIZER(-1); // -1 based refcounting

static void qt_add_rect(HRGN &winRegion, QRect r)
{
    HRGN rgn = CreateRectRgn(r.left(), r.top(), r.x() + r.width(), r.y() + r.height());
    if (rgn) {
        HRGN dest = CreateRectRgn(0,0,0,0);
        int result = CombineRgn(dest, winRegion, rgn, RGN_OR);
        if (result) {
            DeleteObject(winRegion);
            winRegion = dest;
        }
        DeleteObject(rgn);
    }
}

static HRGN qt_hrgn_from_qregion(const QRegion &region)
{
    HRGN hRegion = CreateRectRgn(0,0,0,0);
    if (region.rectCount() == 1) {
        qt_add_rect(hRegion, region.boundingRect());
        return hRegion;
    }
    foreach (const QRect &rect, region.rects())
        qt_add_rect(hRegion, rect);
    return hRegion;
}

/* \internal
    Checks if the theme engine can/should be used, or if we should
    fall back to Windows style.
*/
bool QWindowsXPStylePrivate::useXP(bool update)
{
    if (!update)
        return use_xp;
    return (use_xp = resolveSymbols() && pIsThemeActive()
            && (pIsAppThemed() || !QApplication::instance()));
}

/* \internal
    Handles refcounting, and queries the theme engine for usage.
*/
void QWindowsXPStylePrivate::init(bool force)
{
    if (ref.ref() && !force)
        return;
    if (!force) // -1 based atomic refcounting
        ref.ref();

    useXP(true);
    std::fill(m_themes, m_themes + NThemes, HTHEME(0));
}

/* \internal
    Cleans up all static data.
*/
void QWindowsXPStylePrivate::cleanup(bool force)
{
    if(bufferBitmap) {
        if (bufferDC && nullBitmap)
            SelectObject(bufferDC, nullBitmap);
        DeleteObject(bufferBitmap);
        bufferBitmap = 0;
    }

    if(bufferDC)
        DeleteDC(bufferDC);
    bufferDC = 0;

    if (ref.deref() && !force)
        return;
    if (!force)  // -1 based atomic refcounting
        ref.deref();

    use_xp = false;
    cleanupHandleMap();
    delete tabbody;
    tabbody = 0;
}

/* \internal
    Closes all open theme data handles to ensure that we don't leak
    resources, and that we don't refere to old handles when for
    example the user changes the theme style.
*/
void QWindowsXPStylePrivate::cleanupHandleMap()
{
    for (int i = 0; i < NThemes; ++i)
        if (m_themes[i]) {
            pCloseThemeData(m_themes[i]);
            m_themes[i] = 0;
        }
}

HTHEME QWindowsXPStylePrivate::createTheme(int theme, HWND hwnd)
{
    if (theme < 0 || theme >= NThemes || !hwnd) {
        qWarning("%s: Invalid parameters #%d, %p", Q_FUNC_INFO, theme, hwnd);
        return 0;
    }
    if (!m_themes[theme]) {
        const wchar_t *name = themeNames[theme];
        m_themes[theme] = pOpenThemeData(hwnd, name);
        if (!m_themes[theme])
            qErrnoWarning("%s: OpenThemeData() failed for theme %d (%s).",
                          Q_FUNC_INFO, theme, qPrintable(themeName(theme)));
    }
    return m_themes[theme];
}

QString QWindowsXPStylePrivate::themeName(int theme)
{
    return theme >= 0 && theme < NThemes ?
           QString::fromWCharArray(themeNames[theme]) :
           QString();
}

bool QWindowsXPStylePrivate::isItemViewDelegateLineEdit(const QWidget *widget)
{
    if (!widget)
        return false;
    const QWidget *parent1 = widget->parentWidget();
    // Exlude dialogs or other toplevels parented on item views.
    if (!parent1 || parent1->isWindow())
        return false;
    const QWidget *parent2 = parent1->parentWidget();
    return parent2 && widget->inherits("QLineEdit")
        && parent2->inherits("QAbstractItemView");
}

/*! \internal
    This function will always return a valid window handle, and might
    create a limbo widget to do so.
    We often need a window handle to for example open theme data, so
    this function ensures that we get one.
*/
HWND QWindowsXPStylePrivate::winId(const QWidget *widget)
{
    if (widget)
        if (const HWND hwnd = QApplicationPrivate::getHWNDForWidget(const_cast<QWidget *>(widget)))
            return hwnd;

    // Find top level with native window (there might be dialogs that do not have one).
    foreach (const QWidget *toplevel, QApplication::topLevelWidgets())
        if (toplevel->windowHandle() && toplevel->windowHandle()->handle())
            if (const HWND topLevelHwnd = QApplicationPrivate::getHWNDForWidget(toplevel))
                return topLevelHwnd;

    if (QDesktopWidget *desktop = qApp->desktop())
        if (const HWND desktopHwnd = QApplicationPrivate::getHWNDForWidget(desktop))
            return desktopHwnd;

    Q_ASSERT(false);

    return 0;
}

/*! \internal
    Returns the pointer to a tab widgets body pixmap, scaled to the
    height of the screen. This way the theme engine doesn't need to
    scale the body for every time we ask for it. (Speed optimization)
*/
const QPixmap *QWindowsXPStylePrivate::tabBody(QWidget *)
{
    if (!tabbody) {
        SIZE sz;
        XPThemeData theme(0, 0, QWindowsXPStylePrivate::TabTheme, TABP_BODY);
        pGetThemePartSize(theme.handle(), qt_win_display_dc(), TABP_BODY, 0, 0, TS_TRUE, &sz);

        tabbody = new QPixmap(sz.cx, QApplication::desktop()->screenGeometry().height());
        QPainter painter(tabbody);
        theme.rect = QRect(0, 0, sz.cx, sz.cy);
        drawBackground(theme);
        // We fill with the last line of the themedata, that
        // way we don't get a tiled pixmap inside big tabs
        QPixmap temp(sz.cx, 1);
        painter.drawPixmap(0, 0, temp, 0, sz.cy-1, -1, -1);
        painter.drawTiledPixmap(0, sz.cy, sz.cx, tabbody->height()-sz.cy, temp);
    }
    return tabbody;
}

/*! \internal
    Returns \c true if all the necessary theme engine symbols were
    resolved.
*/
bool QWindowsXPStylePrivate::resolveSymbols()
{
    static bool tried = false;
    if (!tried) {
        tried = true;
        QSystemLibrary themeLib(QLatin1String("uxtheme"));
        pIsAppThemed = (PtrIsAppThemed)themeLib.resolve("IsAppThemed");
        if (pIsAppThemed) {
            pIsThemeActive          = (PtrIsThemeActive         )themeLib.resolve("IsThemeActive");
            pGetThemePartSize       = (PtrGetThemePartSize      )themeLib.resolve("GetThemePartSize");
            pOpenThemeData          = (PtrOpenThemeData         )themeLib.resolve("OpenThemeData");
            pCloseThemeData         = (PtrCloseThemeData        )themeLib.resolve("CloseThemeData");
            pDrawThemeBackground    = (PtrDrawThemeBackground   )themeLib.resolve("DrawThemeBackground");
            pDrawThemeBackgroundEx  = (PtrDrawThemeBackgroundEx )themeLib.resolve("DrawThemeBackgroundEx");
            pGetCurrentThemeName    = (PtrGetCurrentThemeName   )themeLib.resolve("GetCurrentThemeName");
            pGetThemeBool           = (PtrGetThemeBool          )themeLib.resolve("GetThemeBool");
            pGetThemeColor          = (PtrGetThemeColor         )themeLib.resolve("GetThemeColor");
            pGetThemeEnumValue      = (PtrGetThemeEnumValue     )themeLib.resolve("GetThemeEnumValue");
            pGetThemeFilename       = (PtrGetThemeFilename      )themeLib.resolve("GetThemeFilename");
            pGetThemeFont           = (PtrGetThemeFont          )themeLib.resolve("GetThemeFont");
            pGetThemeInt            = (PtrGetThemeInt           )themeLib.resolve("GetThemeInt");
            pGetThemeIntList        = (PtrGetThemeIntList       )themeLib.resolve("GetThemeIntList");
            pGetThemeMargins        = (PtrGetThemeMargins       )themeLib.resolve("GetThemeMargins");
            pGetThemeMetric         = (PtrGetThemeMetric        )themeLib.resolve("GetThemeMetric");
            pGetThemePartSize       = (PtrGetThemePartSize      )themeLib.resolve("GetThemePartSize");
            pGetThemePosition       = (PtrGetThemePosition      )themeLib.resolve("GetThemePosition");
            pGetThemePropertyOrigin = (PtrGetThemePropertyOrigin)themeLib.resolve("GetThemePropertyOrigin");
            pGetThemeRect           = (PtrGetThemeRect          )themeLib.resolve("GetThemeRect");
            pGetThemeString         = (PtrGetThemeString        )themeLib.resolve("GetThemeString");
            pGetThemeBackgroundRegion              = (PtrGetThemeBackgroundRegion             )themeLib.resolve("GetThemeBackgroundRegion");
            pGetThemeDocumentationProperty         = (PtrGetThemeDocumentationProperty        )themeLib.resolve("GetThemeDocumentationProperty");
            pIsThemeBackgroundPartiallyTransparent = (PtrIsThemeBackgroundPartiallyTransparent)themeLib.resolve("IsThemeBackgroundPartiallyTransparent");
        }
    }

    return pIsAppThemed != 0;
}

/*! \internal
    Returns a native buffer (DIB section) of at least the size of
    ( \a x , \a y ). The buffer has a 32 bit depth, to not lose
    the alpha values on proper alpha-pixmaps.
*/
HBITMAP QWindowsXPStylePrivate::buffer(int w, int h)
{
    // If we already have a HBITMAP which is of adequate size, just return that
    if (bufferBitmap) {
        if (bufferW >= w && bufferH >= h)
            return bufferBitmap;
        // Not big enough, discard the old one
        if (bufferDC && nullBitmap)
            SelectObject(bufferDC, nullBitmap);
        DeleteObject(bufferBitmap);
        bufferBitmap = 0;
    }

    w = qMax(bufferW, w);
    h = qMax(bufferH, h);

    if (!bufferDC)
        bufferDC = CreateCompatibleDC(qt_win_display_dc());

    // Define the header
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    // Create the pixmap
    bufferPixels = 0;
    bufferBitmap = CreateDIBSection(bufferDC, &bmi, DIB_RGB_COLORS, (void **) &bufferPixels, 0, 0);
    GdiFlush();
    nullBitmap = (HBITMAP)SelectObject(bufferDC, bufferBitmap);

    if (!bufferBitmap) {
        qErrnoWarning("QWindowsXPStylePrivate::buffer(w,h), failed to create dibsection");
        bufferW = 0;
        bufferH = 0;
        return 0;
    }
    if (!bufferPixels) {
        qErrnoWarning("QWindowsXPStylePrivate::buffer(w,h), did not allocate pixel data");
        bufferW = 0;
        bufferH = 0;
        return 0;
    }
    bufferW = w;
    bufferH = h;
#ifdef DEBUG_XP_STYLE
    qDebug("Creating new dib section (%d, %d)", w, h);
#endif
    return bufferBitmap;
}

/*! \internal
    Returns \c true if the part contains any transparency at all. This does
    not indicate what kind of transparency we're dealing with. It can be
        - Alpha transparency
        - Masked transparency
*/
bool QWindowsXPStylePrivate::isTransparent(XPThemeData &themeData)
{
    return pIsThemeBackgroundPartiallyTransparent(themeData.handle(), themeData.partId,
                                                  themeData.stateId);
}


/*! \internal
    Returns a QRegion of the region of the part
*/
QRegion QWindowsXPStylePrivate::region(XPThemeData &themeData)
{
    HRGN hRgn = 0;
    RECT rect = themeData.toRECT(themeData.rect);
    if (!SUCCEEDED(pGetThemeBackgroundRegion(themeData.handle(), bufferHDC(), themeData.partId,
                                             themeData.stateId, &rect, &hRgn)))
        return QRegion();

    HRGN dest = CreateRectRgn(0, 0, 0, 0);
    const bool success = CombineRgn(dest, hRgn, 0, RGN_COPY) != ERROR;

    QRegion region;

    if (success) {
        int numBytes = GetRegionData(dest, 0, 0);
        if (numBytes == 0)
            return QRegion();

        char *buf = new char[numBytes];
        if (buf == 0)
            return QRegion();

        RGNDATA *rd = reinterpret_cast<RGNDATA*>(buf);
        if (GetRegionData(dest, numBytes, rd) == 0) {
            delete [] buf;
            return QRegion();
        }

        RECT *r = reinterpret_cast<RECT*>(rd->Buffer);
        for (uint i = 0; i < rd->rdh.nCount; ++i) {
            QRect rect;
            rect.setCoords(r->left, r->top, r->right - 1, r->bottom - 1);
            ++r;
            region |= rect;
        }

        delete [] buf;
    }

    DeleteObject(hRgn);
    DeleteObject(dest);

    return region;
}

/*! \internal
    Sets the parts region on a window.
*/
void QWindowsXPStylePrivate::setTransparency(QWidget *widget, XPThemeData &themeData)
{
    HRGN hrgn = themeData.mask(widget);
    if (hrgn && widget)
        SetWindowRgn(winId(widget), hrgn, true);
}

/*! \internal
    Returns \c true if the native doublebuffer contains pixels with
    varying alpha value.
*/
bool QWindowsXPStylePrivate::hasAlphaChannel(const QRect &rect)
{
    const int startX = rect.left();
    const int startY = rect.top();
    const int w = rect.width();
    const int h = rect.height();

    int firstAlpha = -1;
    for (int y = startY; y < h/2; ++y) {
        DWORD *buffer = (DWORD*)bufferPixels + (y * bufferW);
        for (int x = startX; x < w; ++x, ++buffer) {
            int alpha = (*buffer) >> 24;
            if (firstAlpha == -1)
                firstAlpha = alpha;
            else if (alpha != firstAlpha)
                return true;
        }
    }
    return false;
}

/*! \internal
    When the theme engine paints both a true alpha pixmap and a glyph
    into our buffer, the glyph might not contain a proper alpha value.
    The rule of thumb for premultiplied pixmaps is that the color
    values of a pixel can never be higher than the alpha values, so
    we use this to our advantage here, and fix all instances where
    this occures.
*/
bool QWindowsXPStylePrivate::fixAlphaChannel(const QRect &rect)
{
    const int startX = rect.left();
    const int startY = rect.top();
    const int w = rect.width();
    const int h = rect.height();
    bool hasFixedAlphaValue = false;

    for (int y = startY; y < h; ++y) {
        DWORD *buffer = (DWORD*)bufferPixels + (y * bufferW);
        for (int x = startX; x < w; ++x, ++buffer) {
            uint pixel = *buffer;
            int alpha = qAlpha(pixel);
            if (qRed(pixel) > alpha || qGreen(pixel) > alpha || qBlue(pixel) > alpha) {
                *buffer |= 0xff000000;
                hasFixedAlphaValue = true;
            }
        }
    }
    return hasFixedAlphaValue;
}

/*! \internal
    Swaps the alpha values on certain pixels:
        0xFF?????? -> 0x00??????
        0x00?????? -> 0xFF??????
    Used to determin the mask of a non-alpha transparent pixmap in
    the native doublebuffer, and swap the alphas so we may paint
    the image as a Premultiplied QImage with drawImage(), and obtain
    the mask transparency.
*/
bool QWindowsXPStylePrivate::swapAlphaChannel(const QRect &rect, bool allPixels)
{
    const int startX = rect.left();
    const int startY = rect.top();
    const int w = rect.width();
    const int h = rect.height();
    bool valueChange = false;

    // Flip the alphas, so that 255-alpha pixels are 0, and 0-alpha are 255.
    for (int y = startY; y < h; ++y) {
        DWORD *buffer = (DWORD*)bufferPixels + (y * bufferW);
        for (int x = startX; x < w; ++x, ++buffer) {
            if (allPixels) {
                *buffer |= 0xFF000000;
                continue;
            }
            unsigned int alphaValue = (*buffer) & 0xFF000000;
            if (alphaValue == 0xFF000000) {
                *buffer = 0;
                valueChange = true;
            } else if (alphaValue == 0) {
                *buffer |= 0xFF000000;
                valueChange = true;
            }
        }
    }
    return valueChange;
}

/*! \internal
    Main theme drawing function.
    Determines the correct lowlevel drawing method depending on several
    factors.
        Use drawBackgroundThruNativeBuffer() if:
            - Painter does not have an HDC
            - Theme part is flipped (mirrored horizontally)
        else use drawBackgroundDirectly().
*/
void QWindowsXPStylePrivate::drawBackground(XPThemeData &themeData)
{
    if (themeData.rect.isEmpty())
        return;

    QPainter *painter = themeData.painter;
    Q_ASSERT_X(painter != 0, "QWindowsXPStylePrivate::drawBackground()", "Trying to draw a theme part without a painter");
    if (!painter || !painter->isActive())
        return;

    painter->save();

    bool complexXForm = painter->deviceTransform().type() > QTransform::TxTranslate;

    // Access paintDevice via engine since the painter may
    // return the clip device which can still be a widget device in case of grabWidget().

    bool translucentToplevel = false;
    const QPaintDevice *paintDevice = painter->device();
    if (paintDevice->devType() == QInternal::Widget) {
        const QWidget *window = static_cast<const QWidget *>(paintDevice)->window();
        translucentToplevel = window->testAttribute(Qt::WA_TranslucentBackground);
    }

    bool canDrawDirectly = false;
    if (themeData.widget && painter->opacity() == 1.0 && !themeData.rotate
        && !complexXForm && !themeData.mirrorVertically
        && (!themeData.mirrorHorizontally || pDrawThemeBackgroundEx)
        && !translucentToplevel) {
        // Draw on backing store DC only for real widgets or backing store images.
        const QPaintDevice *enginePaintDevice = painter->paintEngine()->paintDevice();
        switch (enginePaintDevice->devType()) {
        case QInternal::Widget:
            canDrawDirectly = true;
            break;
        case QInternal::Image:
            // Ensure the backing store has received as resize and is initialized.
            if (QBackingStore *bs = backingStoreForWidget(themeData.widget))
                if (bs->size().isValid() && bs->paintDevice() == enginePaintDevice)
                    canDrawDirectly = true;
        }
    }

    const HDC dc = canDrawDirectly ? hdcForWidgetBackingStore(themeData.widget) : HDC(0);
    if (dc) {
        drawBackgroundDirectly(themeData);
    } else {
        drawBackgroundThruNativeBuffer(themeData);
    }

    painter->restore();
}

/*! \internal
    This function draws the theme parts directly to the paintengines HDC.
    Do not use this if you need to perform other transformations on the
    resulting data.
*/
void QWindowsXPStylePrivate::drawBackgroundDirectly(XPThemeData &themeData)
{
    QPainter *painter = themeData.painter;
    HDC dc = 0;
    if (themeData.widget)
        dc = hdcForWidgetBackingStore(themeData.widget);

    QPoint redirectionDelta(int(painter->deviceMatrix().dx()),
                            int(painter->deviceMatrix().dy()));
    QRect area = themeData.rect.translated(redirectionDelta);

    QRegion sysRgn = painter->paintEngine()->systemClip();
    if (sysRgn.isEmpty())
        sysRgn = area;
    else
        sysRgn &= area;
    if (painter->hasClipping())
        sysRgn &= painter->clipRegion().translated(redirectionDelta);
    HRGN hrgn = qt_hrgn_from_qregion(sysRgn);
    SelectClipRgn(dc, hrgn);

#ifdef DEBUG_XP_STYLE
        printf("---[ DIRECT PAINTING ]------------------> Name(%-10s) Part(%d) State(%d)\n",
               qPrintable(themeData.name), themeData.partId, themeData.stateId);
        showProperties(themeData);
#endif

    RECT drawRECT = themeData.toRECT(area);
    DTBGOPTS drawOptions;
    drawOptions.dwSize = sizeof(drawOptions);
    drawOptions.rcClip = themeData.toRECT(sysRgn.boundingRect());
    drawOptions.dwFlags = DTBG_CLIPRECT
                          | (themeData.noBorder ? DTBG_OMITBORDER : 0)
                          | (themeData.noContent ? DTBG_OMITCONTENT : 0)
                          | (themeData.mirrorHorizontally ? DTBG_MIRRORDC : 0);

    if (pDrawThemeBackgroundEx != 0) {
        pDrawThemeBackgroundEx(themeData.handle(), dc, themeData.partId, themeData.stateId, &(drawRECT), &drawOptions);
    } else {
        // We are running on a system where the uxtheme.dll does not have
        // the DrawThemeBackgroundEx function, so we need to clip away
        // borders or contents manually. All flips and mirrors uses the
        // fallback implementation

        int borderSize = 0;
        PROPERTYORIGIN origin = PO_NOTFOUND;
        pGetThemePropertyOrigin(themeData.handle(), themeData.partId, themeData.stateId, TMT_BORDERSIZE, &origin);
        pGetThemeInt(themeData.handle(), themeData.partId, themeData.stateId, TMT_BORDERSIZE, &borderSize);

        // Clip away border region
        QRegion extraClip = sysRgn;
        if ((origin == PO_CLASS || origin == PO_PART || origin == PO_STATE) && borderSize > 0) {
            if (themeData.noBorder) {
                // extraClip &= area is already done
                drawRECT = themeData.toRECT(area.adjusted(-borderSize, -borderSize, borderSize, borderSize));
            }

            // Clip away content region
            if (themeData.noContent) {
                QRegion content = area.adjusted(borderSize, borderSize, -borderSize, -borderSize);
                extraClip ^= content;
            }

            // Set the clip region, if used..
            if (themeData.noBorder || themeData.noContent) {
                DeleteObject(hrgn);
                hrgn = qt_hrgn_from_qregion(extraClip);
                SelectClipRgn(dc, hrgn);
            }
        }

        pDrawThemeBackground(themeData.handle(), dc, themeData.partId, themeData.stateId, &(drawRECT), &(drawOptions.rcClip));
    }
    SelectClipRgn(dc, 0);
    DeleteObject(hrgn);
}

/*! \internal
    This function uses a secondary Native doublebuffer for painting parts.
    It should only be used when the painteengine doesn't provide a proper
    HDC for direct painting (e.g. when doing a grabWidget(), painting to
    other pixmaps etc), or when special transformations are needed (e.g.
    flips (horizonal mirroring only, vertical are handled by the theme
    engine).
*/
void QWindowsXPStylePrivate::drawBackgroundThruNativeBuffer(XPThemeData &themeData)
{
    QPainter *painter = themeData.painter;
    QRect rect = themeData.rect;

    if ((themeData.rotate + 90) % 180 == 0) { // Catch 90,270,etc.. degree flips.
        rect = QRect(0, 0, rect.height(), rect.width());
    }
    rect.moveTo(0,0);
    int partId = themeData.partId;
    int stateId = themeData.stateId;
    int w = rect.width();
    int h = rect.height();

    // Values initialized later, either from cached values, or from function calls
    AlphaChannelType alphaType = UnknownAlpha;
    bool stateHasData = true; // We assume so;
    bool hasAlpha = false;
    bool partIsTransparent;
    bool potentialInvalidAlpha;

    QString pixmapCacheKey = QStringLiteral("$qt_xp_");
    pixmapCacheKey.append(themeName(themeData.theme));
    pixmapCacheKey.append(QLatin1Char('p'));
    pixmapCacheKey.append(QString::number(partId));
    pixmapCacheKey.append(QLatin1Char('s'));
    pixmapCacheKey.append(QString::number(stateId));
    pixmapCacheKey.append(QLatin1Char('s'));
    pixmapCacheKey.append(themeData.noBorder ? QLatin1Char('0') : QLatin1Char('1'));
    pixmapCacheKey.append(QLatin1Char('b'));
    pixmapCacheKey.append(themeData.noContent ? QLatin1Char('0') : QLatin1Char('1'));
    pixmapCacheKey.append(QString::number(w));
    pixmapCacheKey.append(QLatin1Char('w'));
    pixmapCacheKey.append(QString::number(h));
    pixmapCacheKey.append(QLatin1Char('h'));

    QPixmap cachedPixmap;
    ThemeMapKey key(themeData);
    ThemeMapData data = alphaCache.value(key);

    bool haveCachedPixmap = false;
    bool isCached = data.dataValid;
    if (isCached) {
        partIsTransparent = data.partIsTransparent;
        hasAlpha = data.hasAlphaChannel;
        alphaType = data.alphaType;
        potentialInvalidAlpha = data.hadInvalidAlpha;

        haveCachedPixmap = QPixmapCache::find(pixmapCacheKey, cachedPixmap);

#ifdef DEBUG_XP_STYLE
        char buf[25];
        ::sprintf(buf, "+ Pixmap(%3d, %3d) ]", w, h);
        printf("---[ CACHED %s--------> Name(%-10s) Part(%d) State(%d)\n",
               haveCachedPixmap ? buf : "]-------------------",
               qPrintable(themeData.name), themeData.partId, themeData.stateId);
#endif
    } else {
        // Not cached, so get values from Theme Engine
        BOOL tmt_borderonly = false;
        COLORREF tmt_transparentcolor = 0x0;
        PROPERTYORIGIN proporigin = PO_NOTFOUND;
        pGetThemeBool(themeData.handle(), themeData.partId, themeData.stateId, TMT_BORDERONLY, &tmt_borderonly);
        pGetThemeColor(themeData.handle(), themeData.partId, themeData.stateId, TMT_TRANSPARENTCOLOR, &tmt_transparentcolor);
        pGetThemePropertyOrigin(themeData.handle(), themeData.partId, themeData.stateId, TMT_CAPTIONMARGINS, &proporigin);

        partIsTransparent = isTransparent(themeData);

        potentialInvalidAlpha = false;
        pGetThemePropertyOrigin(themeData.handle(), themeData.partId, themeData.stateId, TMT_GLYPHTYPE, &proporigin);
        if (proporigin == PO_PART || proporigin == PO_STATE) {
            int tmt_glyphtype = GT_NONE;
            pGetThemeEnumValue(themeData.handle(), themeData.partId, themeData.stateId, TMT_GLYPHTYPE, &tmt_glyphtype);
            potentialInvalidAlpha = partIsTransparent && tmt_glyphtype == GT_IMAGEGLYPH;
        }

#ifdef DEBUG_XP_STYLE
        printf("---[ NOT CACHED ]-----------------------> Name(%-10s) Part(%d) State(%d)\n",
               qPrintable(themeData.name), themeData.partId, themeData.stateId);
        printf("-->partIsTransparen      = %d\n", partIsTransparent);
        printf("-->potentialInvalidAlpha = %d\n", potentialInvalidAlpha);
        showProperties(themeData);
#endif
    }
    bool wasAlphaSwapped = false;
    bool wasAlphaFixed = false;

    // OLD PSDK Workaround ------------------------------------------------------------------------
    // See if we need extra clipping for the older PSDK, which does
    // not have a DrawThemeBackgroundEx function for DTGB_OMITBORDER
    // and DTGB_OMITCONTENT
    bool addBorderContentClipping = false;
    QRegion extraClip;
    QRect area = rect;
    if (themeData.noBorder || themeData.noContent) {
        extraClip = area;
        // We are running on a system where the uxtheme.dll does not have
        // the DrawThemeBackgroundEx function, so we need to clip away
        // borders or contents manually.

        int borderSize = 0;
        PROPERTYORIGIN origin = PO_NOTFOUND;
        pGetThemePropertyOrigin(themeData.handle(), themeData.partId, themeData.stateId, TMT_BORDERSIZE, &origin);
        pGetThemeInt(themeData.handle(), themeData.partId, themeData.stateId, TMT_BORDERSIZE, &borderSize);

        // Clip away border region
        if ((origin == PO_CLASS || origin == PO_PART || origin == PO_STATE) && borderSize > 0) {
            if (themeData.noBorder) {
                extraClip &= area;
                area = area.adjusted(-borderSize, -borderSize, borderSize, borderSize);
            }

            // Clip away content region
            if (themeData.noContent) {
                QRegion content = area.adjusted(borderSize, borderSize, -borderSize, -borderSize);
                extraClip ^= content;
            }
        }
        addBorderContentClipping = (themeData.noBorder | themeData.noContent);
    }

    QImage img;
    if (!haveCachedPixmap) { // If the pixmap is not cached, generate it! -------------------------
        buffer(w, h); // Ensure a buffer of at least (w, h) in size
        HDC dc = bufferHDC();

        // Clear the buffer
        if (alphaType != NoAlpha) {
            // Consider have separate "memset" function for small chunks for more speedup
            memset(bufferPixels, 0x00, bufferW * h * 4);
        }

        // Difference between area and rect
        int dx = area.x() - rect.x();
        int dy = area.y() - rect.y();
        int dr = area.right() - rect.right();
        int db = area.bottom() - rect.bottom();

        // Adjust so painting rect starts from Origo
        rect.moveTo(0,0);
        area.moveTo(dx,dy);
        DTBGOPTS drawOptions;
        drawOptions.dwSize = sizeof(drawOptions);
        drawOptions.rcClip = themeData.toRECT(rect);
        drawOptions.dwFlags = DTBG_CLIPRECT
                            | (themeData.noBorder ? DTBG_OMITBORDER : 0)
                            | (themeData.noContent ? DTBG_OMITCONTENT : 0);

        // Drawing the part into the backing store
        if (pDrawThemeBackgroundEx != 0) {
            RECT rect(themeData.toRECT(area));
            pDrawThemeBackgroundEx(themeData.handle(), dc, themeData.partId, themeData.stateId, &rect, &drawOptions);
        } else {
            // Set the clip region, if used..
            if (addBorderContentClipping) {
                HRGN hrgn = qt_hrgn_from_qregion(extraClip);
                SelectClipRgn(dc, hrgn);
                // Compensate for the noBorder area difference (noContent has the same area)
                drawOptions.rcClip = themeData.toRECT(rect.adjusted(dx, dy, dr, db));
                DeleteObject(hrgn);
            }

            pDrawThemeBackground(themeData.handle(), dc, themeData.partId, themeData.stateId, &(drawOptions.rcClip), 0);

            if (addBorderContentClipping)
                SelectClipRgn(dc, 0);
        }

        // If not cached, analyze the buffer data to figure
        // out alpha type, and if it contains data
        if (!isCached) {
            // SHORTCUT: If the part's state has no data, cache it for NOOP later
            if (!stateHasData) {
                memset(&data, 0, sizeof(data));
                data.dataValid = true;
                alphaCache.insert(key, data);
                return;
            }
            hasAlpha = hasAlphaChannel(rect);
            if (!hasAlpha && partIsTransparent)
                potentialInvalidAlpha = true;
#if defined(DEBUG_XP_STYLE) && 1
            dumpNativeDIB(w, h);
#endif
        }

        // Fix alpha values, if needed
        if (potentialInvalidAlpha)
            wasAlphaFixed = fixAlphaChannel(rect);

        QImage::Format format;
        if ((partIsTransparent && !wasAlphaSwapped) || (!partIsTransparent && hasAlpha)) {
            format = QImage::Format_ARGB32_Premultiplied;
            alphaType = RealAlpha;
        } else if (wasAlphaSwapped) {
            format = QImage::Format_ARGB32_Premultiplied;
            alphaType = MaskAlpha;
        } else {
            format = QImage::Format_RGB32;
            // The image data we got from the theme engine does not have any transparency,
            // thus the alpha channel is set to 0.
            // However, Format_RGB32 requires the alpha part to be set to 0xff, thus
            // we must flip it from 0x00 to 0xff
            swapAlphaChannel(rect, true);
            alphaType = NoAlpha;
        }
#if defined(DEBUG_XP_STYLE) && 1
        printf("Image format is: %s\n", alphaType == RealAlpha ? "Real Alpha" : alphaType == MaskAlpha ? "Masked Alpha" : "No Alpha");
#endif
        img = QImage(bufferPixels, bufferW, bufferH, format);
    }

    // Blitting backing store
    bool useRegion = partIsTransparent && !hasAlpha && !wasAlphaSwapped;

    QRegion newRegion;
    QRegion oldRegion;
    if (useRegion) {
        newRegion = region(themeData);
        oldRegion = painter->clipRegion();
        painter->setClipRegion(newRegion);
#if defined(DEBUG_XP_STYLE) && 0
        printf("Using region:\n");
        QVector<QRect> rects = newRegion.rects();
        for (int i = 0; i < rects.count(); ++i) {
            const QRect &r = rects.at(i);
            printf("    (%d, %d, %d, %d)\n", r.x(), r.y(), r.right(), r.bottom());
        }
#endif
    }

    if (addBorderContentClipping)
        painter->setClipRegion(extraClip, Qt::IntersectClip);

    if (!themeData.mirrorHorizontally && !themeData.mirrorVertically && !themeData.rotate) {
        if (!haveCachedPixmap)
            painter->drawImage(themeData.rect, img, rect);
        else
            painter->drawPixmap(themeData.rect, cachedPixmap);
    } else {
        // This is _slow_!
        // Make a copy containing only the necessary data, and mirror
        // on all wanted axes. Then draw the copy.
        // If cached, the normal pixmap is cached, instead of caching
        // all possible orientations for each part and state.
        QImage imgCopy;
        if (!haveCachedPixmap)
            imgCopy = img.copy(rect);
        else
            imgCopy = cachedPixmap.toImage();

        if (themeData.rotate) {
            QMatrix rotMatrix;
            rotMatrix.rotate(themeData.rotate);
            imgCopy = imgCopy.transformed(rotMatrix);
        }
        if (themeData.mirrorHorizontally || themeData.mirrorVertically) {
            imgCopy = imgCopy.mirrored(themeData.mirrorHorizontally, themeData.mirrorVertically);
        }
        painter->drawImage(themeData.rect,
                           imgCopy);
    }

    if (useRegion || addBorderContentClipping) {
        if (oldRegion.isEmpty())
            painter->setClipping(false);
        else
            painter->setClipRegion(oldRegion);
    }

    // Cache the pixmap to avoid expensive swapAlphaChannel() calls
    if (!haveCachedPixmap && w && h) {
        QPixmap pix = QPixmap::fromImage(img).copy(rect);
        QPixmapCache::insert(pixmapCacheKey, pix);
#ifdef DEBUG_XP_STYLE
        printf("+++Adding pixmap to cache, size(%d, %d), wasAlphaSwapped(%d), wasAlphaFixed(%d), name(%s)\n",
               w, h, wasAlphaSwapped, wasAlphaFixed, qPrintable(pixmapCacheKey));
#endif
    }

    // Add to theme part cache
    if (!isCached) {
        memset(&data, 0, sizeof(data));
        data.dataValid = true;
        data.partIsTransparent = partIsTransparent;
        data.alphaType = alphaType;
        data.hasAlphaChannel = hasAlpha;
        data.wasAlphaSwapped = wasAlphaSwapped;
        data.hadInvalidAlpha = wasAlphaFixed;
        alphaCache.insert(key, data);
    }
}


// ------------------------------------------------------------------------------------------------

/*!
    \class QWindowsXPStyle
    \brief The QWindowsXPStyle class provides a Microsoft Windows XP-like look and feel.

    \ingroup appearance
    \inmodule QtWidgets
    \internal

    \warning This style is only available on the Windows XP platform
    because it makes use of Windows XP's style engine.

    Most of the functions are documented in the base classes
    QWindowsStyle, QCommonStyle, and QStyle, but the
    QWindowsXPStyle overloads of drawComplexControl(), drawControl(),
    drawControlMask(), drawPrimitive(), proxy()->subControlRect(), and
    sizeFromContents(), are documented here.

    \image qwindowsxpstyle.png
    \sa QMacStyle, QWindowsStyle, QFusionStyle
*/

/*!
    Constructs a QWindowsStyle
*/
QWindowsXPStyle::QWindowsXPStyle()
    : QWindowsStyle(*new QWindowsXPStylePrivate)
{
}

/*!
    Destroys the style.
*/
QWindowsXPStyle::~QWindowsXPStyle()
{
}

/*! \reimp */
void QWindowsXPStyle::unpolish(QApplication *app)
{
    QWindowsStyle::unpolish(app);
}

/*! \reimp */
void QWindowsXPStyle::polish(QApplication *app)
{
    QWindowsStyle::polish(app);
    if (!QWindowsXPStylePrivate::useXP())
        return;
}

/*! \reimp */
void QWindowsXPStyle::polish(QWidget *widget)
{
    QWindowsStyle::polish(widget);
    if (!QWindowsXPStylePrivate::useXP())
        return;

    if (qobject_cast<QAbstractButton*>(widget)
        || qobject_cast<QToolButton*>(widget)
        || qobject_cast<QTabBar*>(widget)
#ifndef QT_NO_COMBOBOX
        || qobject_cast<QComboBox*>(widget)
#endif // QT_NO_COMBOBOX
        || qobject_cast<QScrollBar*>(widget)
        || qobject_cast<QSlider*>(widget)
        || qobject_cast<QHeaderView*>(widget)
#ifndef QT_NO_SPINBOX
        || qobject_cast<QAbstractSpinBox*>(widget)
        || qobject_cast<QSpinBox*>(widget)
#endif // QT_NO_SPINBOX
        ) {
        widget->setAttribute(Qt::WA_Hover);
    }

#ifndef QT_NO_RUBBERBAND
    if (qobject_cast<QRubberBand*>(widget)) {
        widget->setWindowOpacity(0.6);
    }
#endif
    if (qobject_cast<QStackedWidget*>(widget) &&
               qobject_cast<QTabWidget*>(widget->parent()))
        widget->parentWidget()->setAttribute(Qt::WA_ContentsPropagated);

    Q_D(QWindowsXPStyle);
    if (!d->hasInitColors) {
        // Get text color for group box labels
        COLORREF cref;
        XPThemeData theme(0, 0, QWindowsXPStylePrivate::ButtonTheme, 0, 0);
        pGetThemeColor(theme.handle(), BP_GROUPBOX, GBS_NORMAL, TMT_TEXTCOLOR, &cref);
        d->groupBoxTextColor = qRgb(GetRValue(cref), GetGValue(cref), GetBValue(cref));
        pGetThemeColor(theme.handle(), BP_GROUPBOX, GBS_DISABLED, TMT_TEXTCOLOR, &cref);
        d->groupBoxTextColorDisabled = qRgb(GetRValue(cref), GetGValue(cref), GetBValue(cref));
        // Where does this color come from?
        //pGetThemeColor(theme.handle(), TKP_TICS, TSS_NORMAL, TMT_COLOR, &cref);
        d->sliderTickColor = qRgb(165, 162, 148);
        d->hasInitColors = true;
    }
}

/*! \reimp */
void QWindowsXPStyle::polish(QPalette &pal)
{
    QWindowsStyle::polish(pal);
    pal.setBrush(QPalette::AlternateBase, pal.base().color().darker(110));
}

/*! \reimp */
void QWindowsXPStyle::unpolish(QWidget *widget)
{
#ifndef QT_NO_RUBBERBAND
    if (qobject_cast<QRubberBand*>(widget)) {
        widget->setWindowOpacity(1.0);
    }
#endif
    Q_D(QWindowsXPStyle);
    // Unpolish of widgets is the first thing that
    // happens when a theme changes, or the theme
    // engine is turned off. So we detect it here.
    bool oldState = QWindowsXPStylePrivate::useXP();
    bool newState = QWindowsXPStylePrivate::useXP(true);
    if ((oldState != newState) && newState) {
        d->cleanup(true);
        d->init(true);
    } else {
        // Cleanup handle map, if just changing style,
        // or turning it on. In both cases the values
        // already in the map might be old (other style).
        d->cleanupHandleMap();
    }
    if (qobject_cast<QAbstractButton*>(widget)
        || qobject_cast<QToolButton*>(widget)
        || qobject_cast<QTabBar*>(widget)
#ifndef QT_NO_COMBOBOX
        || qobject_cast<QComboBox*>(widget)
#endif // QT_NO_COMBOBOX
        || qobject_cast<QScrollBar*>(widget)
        || qobject_cast<QSlider*>(widget)
        || qobject_cast<QHeaderView*>(widget)
#ifndef QT_NO_SPINBOX
        || qobject_cast<QAbstractSpinBox*>(widget)
        || qobject_cast<QSpinBox*>(widget)
#endif // QT_NO_SPINBOX
        ) {
        widget->setAttribute(Qt::WA_Hover, false);
    }
    QWindowsStyle::unpolish(widget);
}

/*! \reimp */
QRect QWindowsXPStyle::subElementRect(SubElement sr, const QStyleOption *option, const QWidget *widget) const
{
    if (!QWindowsXPStylePrivate::useXP()) {
        return QWindowsStyle::subElementRect(sr, option, widget);
    }

    QRect rect(option->rect);
    switch(sr) {
    case SE_DockWidgetCloseButton:
    case SE_DockWidgetFloatButton:
        rect = QWindowsStyle::subElementRect(sr, option, widget);
        return rect.translated(0, 1);
    break;
    case SE_TabWidgetTabContents:
        if (qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option))
        {
            rect = QWindowsStyle::subElementRect(sr, option, widget);
            if (sr == SE_TabWidgetTabContents) {
                if (const QTabWidget *tabWidget = qobject_cast<const QTabWidget *>(widget)) {
                    if (tabWidget->documentMode())
                        break;
                }

                rect.adjust(0, 0, -2, -2);
            }
        }
        break;
    case SE_TabWidgetTabBar: {
        rect = QWindowsStyle::subElementRect(sr, option, widget);
        const QStyleOptionTabWidgetFrame *twfOption =
            qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option);
        if (twfOption && twfOption->direction == Qt::RightToLeft
            && (twfOption->shape == QTabBar::RoundedNorth
                || twfOption->shape == QTabBar::RoundedSouth))
        {
            QStyleOptionTab otherOption;
            otherOption.shape = (twfOption->shape == QTabBar::RoundedNorth
                                ? QTabBar::RoundedEast : QTabBar::RoundedSouth);
            int overlap = proxy()->pixelMetric(PM_TabBarBaseOverlap, &otherOption, widget);
            int borderThickness = proxy()->pixelMetric(PM_DefaultFrameWidth, option, widget);
            rect.adjust(-overlap + borderThickness, 0, -overlap + borderThickness, 0);
        }
        break;}

    case SE_PushButtonContents:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            MARGINS borderSize;
            if (widget) {
                XPThemeData buttontheme(widget, 0, QWindowsXPStylePrivate::ButtonTheme);
                HTHEME theme = buttontheme.handle();
                if (theme) {
                    int stateId;
                    if (!(option->state & State_Enabled))
                        stateId = PBS_DISABLED;
                    else if (option->state & State_Sunken)
                        stateId = PBS_PRESSED;
                    else if (option->state & State_MouseOver)
                        stateId = PBS_HOT;
                    else if (btn->features & QStyleOptionButton::DefaultButton)
                        stateId = PBS_DEFAULTED;
                    else
                        stateId = PBS_NORMAL;

                    int border = proxy()->pixelMetric(PM_DefaultFrameWidth, btn, widget);
                    rect = option->rect.adjusted(border, border, -border, -border);

                    int result = pGetThemeMargins(theme,
                                                  NULL,
                                                  BP_PUSHBUTTON,
                                                  stateId,
                                                  TMT_CONTENTMARGINS,
                                                  NULL,
                                                  &borderSize);

                    if (result == S_OK) {
                        rect.adjust(borderSize.cxLeftWidth, borderSize.cyTopHeight,
                                    -borderSize.cxRightWidth, -borderSize.cyBottomHeight);
                        rect = visualRect(option->direction, option->rect, rect);
                    }
                }
            }
        }
        break;
    case SE_ProgressBarContents:
        rect = QCommonStyle::subElementRect(SE_ProgressBarGroove, option, widget);
        if (option->state & QStyle::State_Horizontal)
            rect.adjust(4, 3, -4, -3);
        else
            rect.adjust(3, 2, -3, -2);
        break;
    default:
        rect = QWindowsStyle::subElementRect(sr, option, widget);
    }
    return rect;
}

/*!
    \reimp
*/
void QWindowsXPStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *option, QPainter *p,
                                    const QWidget *widget) const
{
    QWindowsXPStylePrivate *d = const_cast<QWindowsXPStylePrivate*>(d_func());

    if (!QWindowsXPStylePrivate::useXP()) {
        QWindowsStyle::drawPrimitive(pe, option, p, widget);
        return;
    }

    int themeNumber = -1;
    int partId = 0;
    int stateId = 0;
    QRect rect = option->rect;
    State flags = option->state;
    bool hMirrored = false;
    bool vMirrored = false;
    bool noBorder = false;
    bool noContent = false;
    int  rotate = 0;

    switch (pe) {
    case PE_FrameTabBarBase:
        if (const QStyleOptionTabBarBase *tbb
                = qstyleoption_cast<const QStyleOptionTabBarBase *>(option)) {
            p->save();
            switch (tbb->shape) {
            case QTabBar::RoundedNorth:
                p->setPen(QPen(tbb->palette.dark(), 0));
                p->drawLine(tbb->rect.topLeft(), tbb->rect.topRight());
                break;
            case QTabBar::RoundedWest:
                p->setPen(QPen(tbb->palette.dark(), 0));
                p->drawLine(tbb->rect.left(), tbb->rect.top(), tbb->rect.left(), tbb->rect.bottom());
                break;
            case QTabBar::RoundedSouth:
                p->setPen(QPen(tbb->palette.dark(), 0));
                p->drawLine(tbb->rect.left(), tbb->rect.top(),
                            tbb->rect.right(), tbb->rect.top());
                break;
            case QTabBar::RoundedEast:
                p->setPen(QPen(tbb->palette.dark(), 0));
                p->drawLine(tbb->rect.topLeft(), tbb->rect.bottomLeft());
                break;
            case QTabBar::TriangularNorth:
            case QTabBar::TriangularEast:
            case QTabBar::TriangularWest:
            case QTabBar::TriangularSouth:
                p->restore();
                QWindowsStyle::drawPrimitive(pe, option, p, widget);
                return;
            }
            p->restore();
        }
        return;
    case PE_PanelButtonBevel:
        themeNumber = QWindowsXPStylePrivate::ButtonTheme;
        partId = BP_PUSHBUTTON;
        if (!(flags & State_Enabled))
            stateId = PBS_DISABLED;
        else if ((flags & State_Sunken) || (flags & State_On))
            stateId = PBS_PRESSED;
        else if (flags & State_MouseOver)
            stateId = PBS_HOT;
        //else if (flags & State_ButtonDefault)
        //    stateId = PBS_DEFAULTED;
        else
            stateId = PBS_NORMAL;
        break;

    case PE_PanelButtonTool:
        if (widget && widget->inherits("QDockWidgetTitleButton")) {
            if (const QWidget *dw = widget->parentWidget())
                if (dw->isWindow())
                    return;
        }
        themeNumber = QWindowsXPStylePrivate::ToolBarTheme;
        partId = TP_BUTTON;
        if (!(flags & State_Enabled))
            stateId = TS_DISABLED;
        else if (flags & State_Sunken)
            stateId = TS_PRESSED;
        else if (flags & State_MouseOver)
            stateId = flags & State_On ? TS_HOTCHECKED : TS_HOT;
        else if (flags & State_On)
            stateId = TS_CHECKED;
        else if (!(flags & State_AutoRaise))
            stateId = TS_HOT;
        else
            stateId = TS_NORMAL;
        break;

    case PE_IndicatorButtonDropDown:
        themeNumber = QWindowsXPStylePrivate::ToolBarTheme;
        partId = TP_SPLITBUTTONDROPDOWN;
        if (!(flags & State_Enabled))
            stateId = TS_DISABLED;
        else if (flags & State_Sunken)
            stateId = TS_PRESSED;
        else if (flags & State_MouseOver)
            stateId = flags & State_On ? TS_HOTCHECKED : TS_HOT;
        else if (flags & State_On)
            stateId = TS_CHECKED;
        else if (!(flags & State_AutoRaise))
            stateId = TS_HOT;
        else
            stateId = TS_NORMAL;
        if (option->direction == Qt::RightToLeft)
            hMirrored = true;
        break;

    case PE_IndicatorCheckBox:
        themeNumber = QWindowsXPStylePrivate::ButtonTheme;
        partId = BP_CHECKBOX;
        if (!(flags & State_Enabled))
            stateId = CBS_UNCHECKEDDISABLED;
        else if (flags & State_Sunken)
            stateId = CBS_UNCHECKEDPRESSED;
        else if (flags & State_MouseOver)
            stateId = CBS_UNCHECKEDHOT;
        else
            stateId = CBS_UNCHECKEDNORMAL;

        if (flags & State_On)
            stateId += CBS_CHECKEDNORMAL-1;
        else if (flags & State_NoChange)
            stateId += CBS_MIXEDNORMAL-1;

        break;

    case PE_IndicatorRadioButton:
        themeNumber = QWindowsXPStylePrivate::ButtonTheme;
        partId = BP_RADIOBUTTON;
        if (!(flags & State_Enabled))
            stateId = RBS_UNCHECKEDDISABLED;
        else if (flags & State_Sunken)
            stateId = RBS_UNCHECKEDPRESSED;
        else if (flags & State_MouseOver)
            stateId = RBS_UNCHECKEDHOT;
        else
            stateId = RBS_UNCHECKEDNORMAL;

        if (flags & State_On)
            stateId += RBS_CHECKEDNORMAL-1;
        break;

    case PE_IndicatorDockWidgetResizeHandle:
        return;

case PE_Frame:
    {
        if (flags & State_Raised)
            return;
        themeNumber = QWindowsXPStylePrivate::ListViewTheme;
        partId = LVP_LISTGROUP;
        XPThemeData theme(0, 0, themeNumber, partId, 0);

        if (!(flags & State_Enabled))
            stateId = ETS_DISABLED;
        else
            stateId = ETS_NORMAL;
        int fillType;
        if (pGetThemeEnumValue(theme.handle(), partId, stateId, TMT_BGTYPE, &fillType) == S_OK) {
            if (fillType == BT_BORDERFILL) {
                COLORREF bcRef;
                pGetThemeColor(theme.handle(), partId, stateId, TMT_BORDERCOLOR, &bcRef);
                QColor bordercolor(qRgb(GetRValue(bcRef), GetGValue(bcRef), GetBValue(bcRef)));
                QPen oldPen = p->pen();
                // int borderSize = 1;
                // pGetThemeInt(theme.handle(), partId, stateId, TMT_BORDERCOLOR, &borderSize);

                // Inner white border
                p->setPen(QPen(option->palette.base().color(), 1));
                p->drawRect(option->rect.adjusted(1, 1, -2, -2));
                // Outer dark border
                p->setPen(QPen(bordercolor, 1));
                p->drawRect(option->rect.adjusted(0, 0, -1, -1));
                p->setPen(oldPen);
                return;
            } else if (fillType == BT_NONE) {
                return;
            } else {
                break;
            }
        }
    }
    case PE_FrameLineEdit: {
        // we try to check if this lineedit is a delegate on a QAbstractItemView-derived class.
        if (QWindowsXPStylePrivate::isItemViewDelegateLineEdit(widget)) {
            QPen oldPen = p->pen();
            // Inner white border
            p->setPen(QPen(option->palette.base().color(), 1));
            p->drawRect(option->rect.adjusted(1, 1, -2, -2));
            // Outer dark border
            p->setPen(QPen(option->palette.shadow().color(), 1));
            p->drawRect(option->rect.adjusted(0, 0, -1, -1));
            p->setPen(oldPen);
            return;
        } else if (qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            themeNumber = QWindowsXPStylePrivate::EditTheme;
            partId = EP_EDITTEXT;
            noContent = true;
            if (!(flags & State_Enabled))
                stateId = ETS_DISABLED;
            else
                stateId = ETS_NORMAL;
        }
        break;
    }

    case PE_PanelLineEdit:
        if (const QStyleOptionFrame *panel = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            themeNumber = QWindowsXPStylePrivate::EditTheme;
            partId = EP_EDITTEXT;
            noBorder = true;
            QBrush bg;
            bool usePalette = false;
            bool isEnabled = flags & State_Enabled;
            uint resolve_mask = panel->palette.resolve();

#ifndef QT_NO_SPINBOX
            // Since spin box includes a line edit we need to resolve the palette mask also from
            // the parent, as while the color is always correct on the palette supplied by panel,
            // the mask can still be empty. If either mask specifies custom base color, use that.
            if (widget) {
                if (QAbstractSpinBox *spinbox = qobject_cast<QAbstractSpinBox*>(widget->parentWidget()))
                    resolve_mask |= spinbox->palette().resolve();
            }
#endif // QT_NO_SPINBOX
            if (resolve_mask & (1 << QPalette::Base)) {
                // Base color is set for this widget, so use it
                bg = panel->palette.brush(QPalette::Base);
                usePalette = true;
            }

            stateId = isEnabled ? ETS_NORMAL : ETS_DISABLED;

            if (usePalette) {
                p->fillRect(panel->rect, bg);
            } else {
                XPThemeData theme(0, p, themeNumber, partId, stateId, rect);
                if (!theme.isValid()) {
                    QWindowsStyle::drawPrimitive(pe, option, p, widget);
                    return;
                }
                int bgType;
                pGetThemeEnumValue( theme.handle(),
                                    partId,
                                    stateId,
                                    TMT_BGTYPE,
                                    &bgType);
                if( bgType == BT_IMAGEFILE ) {
                    theme.mirrorHorizontally = hMirrored;
                    theme.mirrorVertically = vMirrored;
                    theme.noBorder = noBorder;
                    theme.noContent = noContent;
                    theme.rotate = rotate;
                    d->drawBackground(theme);
                } else {
                    QBrush fillColor = option->palette.brush(QPalette::Base);

                    if (!isEnabled) {
                        PROPERTYORIGIN origin = PO_NOTFOUND;
                        pGetThemePropertyOrigin(theme.handle(), theme.partId, theme.stateId, TMT_FILLCOLOR, &origin);
                        // Use only if the fill property comes from our part
                        if ((origin == PO_PART || origin == PO_STATE)) {
                            COLORREF bgRef;
                            pGetThemeColor(theme.handle(), partId, stateId, TMT_FILLCOLOR, &bgRef);
                            fillColor = QBrush(qRgb(GetRValue(bgRef), GetGValue(bgRef), GetBValue(bgRef)));
                        }
                    }
                    p->fillRect(option->rect, fillColor);
                }
            }

            if (panel->lineWidth > 0)
                proxy()->drawPrimitive(PE_FrameLineEdit, panel, p, widget);
            return;
        }
        break;

    case PE_FrameTabWidget:
        if (const QStyleOptionTabWidgetFrame *tab = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option))
        {
            themeNumber = QWindowsXPStylePrivate::TabTheme;
            partId = TABP_PANE;

            if (widget) {
                bool useGradient = true;
                const int maxlength = 256;
                wchar_t themeFileName[maxlength];
                wchar_t themeColor[maxlength];
                // Due to a a scaling issue with the XP Silver theme, tab gradients are not used with it
                if (pGetCurrentThemeName(themeFileName, maxlength, themeColor, maxlength, NULL, 0) == S_OK) {
                    wchar_t *offset = 0;
                    if ((offset = wcsrchr(themeFileName, QChar(QLatin1Char('\\')).unicode())) != NULL) {
                        offset++;
                        if (!lstrcmp(offset, L"Luna.msstyles") && !lstrcmp(offset, L"Metallic")) {
                            useGradient = false;
                        }
                    }
                }
                // This should work, but currently there's an error in the ::drawBackgroundDirectly()
                // code, when using the HDC directly..
                if (useGradient) {
                    QStyleOptionTabWidgetFrameV2 frameOpt = *tab;
                    frameOpt.rect = widget->rect();
                    QRect contentsRect = subElementRect(SE_TabWidgetTabContents, &frameOpt, widget);
                    QRegion reg = option->rect;
                    reg -= contentsRect;
                    p->setClipRegion(reg);
                    XPThemeData theme(widget, p, themeNumber, partId, stateId, rect);
                    theme.mirrorHorizontally = hMirrored;
                    theme.mirrorVertically = vMirrored;
                    d->drawBackground(theme);
                    p->setClipRect(contentsRect);
                    partId = TABP_BODY;
                }
            }
            switch (tab->shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                vMirrored = true;
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                rotate = 90;
                break;
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                rotate = 90;
                hMirrored = true;
                break;
            default:
                break;
            }
        }
        break;

    case PE_FrameMenu:
        p->save();
        p->setPen(option->palette.dark().color());
        p->drawRect(rect.adjusted(0, 0, -1, -1));
        p->restore();
        return;

    case PE_PanelMenuBar:
        break;

 case PE_FrameDockWidget:
     if (const QStyleOptionFrame *frm = qstyleoption_cast<const QStyleOptionFrame *>(option))
        {
            themeNumber = QWindowsXPStylePrivate::WindowTheme;
            if (flags & State_Active)
                stateId = FS_ACTIVE;
            else
                stateId = FS_INACTIVE;

            int fwidth = proxy()->pixelMetric(PM_DockWidgetFrameWidth, frm, widget);

            XPThemeData theme(widget, p, themeNumber, 0, stateId);
            if (!theme.isValid())
                break;
            theme.rect = QRect(frm->rect.x(), frm->rect.y(), frm->rect.x()+fwidth, frm->rect.height()-fwidth);           theme.partId = WP_SMALLFRAMELEFT;
            d->drawBackground(theme);
            theme.rect = QRect(frm->rect.width()-fwidth, frm->rect.y(), fwidth, frm->rect.height()-fwidth);
            theme.partId = WP_SMALLFRAMERIGHT;
            d->drawBackground(theme);
            theme.rect = QRect(frm->rect.x(), frm->rect.bottom()-fwidth+1, frm->rect.width(), fwidth);
            theme.partId = WP_SMALLFRAMEBOTTOM;
            d->drawBackground(theme);
            return;
        }
        break;

    case PE_IndicatorHeaderArrow:
        {
#if 0 // XP theme engine doesn't know about this :(
            name = QWindowsXPStylePrivate::HeaderTheme;
            partId = HP_HEADERSORTARROW;
            if (flags & State_Down)
                stateId = HSAS_SORTEDDOWN;
            else
                stateId = HSAS_SORTEDUP;
#else
            if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
                p->save();
                p->setPen(option->palette.dark().color());
                p->translate(0, option->rect.height()/2 - 4);
                if (header->sortIndicator & QStyleOptionHeader::SortUp) { // invert logic to follow Windows style guide
                    p->drawLine(option->rect.x(), option->rect.y(), option->rect.x()+8, option->rect.y());
                    p->drawLine(option->rect.x()+1, option->rect.y()+1, option->rect.x()+7, option->rect.y()+1);
                    p->drawLine(option->rect.x()+2, option->rect.y()+2, option->rect.x()+6, option->rect.y()+2);
                    p->drawLine(option->rect.x()+3, option->rect.y()+3, option->rect.x()+5, option->rect.y()+3);
                    p->drawPoint(option->rect.x()+4, option->rect.y()+4);
                } else if(header->sortIndicator & QStyleOptionHeader::SortDown) {
                    p->drawLine(option->rect.x(), option->rect.y()+4, option->rect.x()+8, option->rect.y()+4);
                    p->drawLine(option->rect.x()+1, option->rect.y()+3, option->rect.x()+7, option->rect.y()+3);
                    p->drawLine(option->rect.x()+2, option->rect.y()+2, option->rect.x()+6, option->rect.y()+2);
                    p->drawLine(option->rect.x()+3, option->rect.y()+1, option->rect.x()+5, option->rect.y()+1);
                    p->drawPoint(option->rect.x()+4, option->rect.y());
                }
                p->restore();
                return;
            }
#endif
        }
        break;

    case PE_FrameStatusBarItem:
        themeNumber = QWindowsXPStylePrivate::StatusTheme;
        partId = SP_PANE;
        break;

    case PE_FrameGroupBox:
        themeNumber = QWindowsXPStylePrivate::ButtonTheme;
        partId = BP_GROUPBOX;
        if (!(flags & State_Enabled))
            stateId = GBS_DISABLED;
        else
            stateId = GBS_NORMAL;
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            const QStyleOptionFrameV2 *frame2 = qstyleoption_cast<const QStyleOptionFrameV2 *>(option);
            if (frame2->features & QStyleOptionFrameV2::Flat) {
                // Windows XP does not have a theme part for a flat GroupBox, paint it with the windows style
                QRect fr = frame->rect;
                QPoint p1(fr.x(), fr.y() + 1);
                QPoint p2(fr.x() + fr.width(), p1.y() + 1);
                rect = QRect(p1, p2);
                themeNumber = -1;
            }
        }
        break;

    case PE_IndicatorProgressChunk:
        {
        Qt::Orientation orient = Qt::Horizontal;
        bool inverted = false;
        if (const QStyleOptionProgressBarV2 *pb2 = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(option)) {
            orient = pb2->orientation;
            if (pb2->invertedAppearance)
                inverted = true;
        }
        if (orient == Qt::Horizontal) {
            partId = PP_CHUNK;
            rect = QRect(option->rect.x(), option->rect.y(), option->rect.width(), option->rect.height() );
            if (inverted && option->direction == Qt::LeftToRight)
                hMirrored = true;
        } else {
            partId = PP_CHUNKVERT;
            rect = QRect(option->rect.x(), option->rect.y(), option->rect.width(), option->rect.height());
        }
        themeNumber = QWindowsXPStylePrivate::ProgressTheme;
        stateId = 1;
        }
        break;

    case PE_FrameWindow:
        if (const QStyleOptionFrame *frm = qstyleoption_cast<const QStyleOptionFrame *>(option))
        {
            themeNumber = QWindowsXPStylePrivate::WindowTheme;
            if (flags & State_Active)
                stateId = FS_ACTIVE;
            else
                stateId = FS_INACTIVE;

            int fwidth = frm->lineWidth + frm->midLineWidth;

            XPThemeData theme(0, p, themeNumber, 0, stateId);
            if (!theme.isValid())
                break;

            theme.rect = QRect(option->rect.x(), option->rect.y()+fwidth, option->rect.x()+fwidth, option->rect.height()-fwidth);
            theme.partId = WP_FRAMELEFT;
            d->drawBackground(theme);
            theme.rect = QRect(option->rect.width()-fwidth, option->rect.y()+fwidth, fwidth, option->rect.height()-fwidth);
            theme.partId = WP_FRAMERIGHT;
            d->drawBackground(theme);
            theme.rect = QRect(option->rect.x(), option->rect.height()-fwidth, option->rect.width(), fwidth);
            theme.partId = WP_FRAMEBOTTOM;
            d->drawBackground(theme);
            theme.rect = QRect(option->rect.x(), option->rect.y(), option->rect.width(), option->rect.y()+fwidth);
            theme.partId = WP_CAPTION;
            d->drawBackground(theme);
            return;
        }
        break;

    case PE_IndicatorBranch:
        {
            static const int decoration_size = 9;
            int mid_h = option->rect.x() + option->rect.width() / 2;
            int mid_v = option->rect.y() + option->rect.height() / 2;
            int bef_h = mid_h;
            int bef_v = mid_v;
            int aft_h = mid_h;
            int aft_v = mid_v;
            QBrush brush(option->palette.dark().color(), Qt::Dense4Pattern);
            if (option->state & State_Item) {
                if (option->direction == Qt::RightToLeft)
                    p->fillRect(option->rect.left(), mid_v, bef_h - option->rect.left(), 1, brush);
                else
                    p->fillRect(aft_h, mid_v, option->rect.right() - aft_h + 1, 1, brush);
            }
            if (option->state & State_Sibling)
                p->fillRect(mid_h, aft_v, 1, option->rect.bottom() - aft_v + 1, brush);
            if (option->state & (State_Open | State_Children | State_Item | State_Sibling))
                p->fillRect(mid_h, option->rect.y(), 1, bef_v - option->rect.y(), brush);
            if (option->state & State_Children) {
                int delta = decoration_size / 2;
                bef_h -= delta;
                bef_v -= delta;
                aft_h += delta;
                aft_v += delta;
                XPThemeData theme(0, p, QWindowsXPStylePrivate::TreeViewTheme);
                theme.rect = QRect(bef_h, bef_v, decoration_size, decoration_size);
                theme.partId = TVP_GLYPH;
                theme.stateId = flags & QStyle::State_Open ? GLPS_OPENED : GLPS_CLOSED;
                d->drawBackground(theme);
            }
        }
        return;

    case PE_IndicatorToolBarSeparator:
        if (option->rect.height() < 3) {
            // XP style requires a few pixels for the separator
            // to be visible.
            QWindowsStyle::drawPrimitive(pe, option, p, widget);
            return;
        }
        themeNumber = QWindowsXPStylePrivate::ToolBarTheme;
        partId = TP_SEPARATOR;

        if (option->state & State_Horizontal)
            partId = TP_SEPARATOR;
        else
            partId = TP_SEPARATORVERT;

        break;

    case PE_IndicatorToolBarHandle:

        themeNumber = QWindowsXPStylePrivate::RebarTheme;
        partId = RP_GRIPPER;
        if (option->state & State_Horizontal) {
            partId = RP_GRIPPER;
            rect.adjust(0, 0, -2, 0);
        }
        else {
            partId = RP_GRIPPERVERT;
            rect.adjust(0, 0, 0, -2);
        }
        break;

    case PE_IndicatorItemViewItemCheck: {
        QStyleOptionButton button;
        button.QStyleOption::operator=(*option);
        button.state &= ~State_MouseOver;
        proxy()->drawPrimitive(PE_IndicatorCheckBox, &button, p, widget);
        return;
    }

    default:
        break;
    }

    XPThemeData theme(widget, p, themeNumber, partId, stateId, rect);
    if (!theme.isValid()) {
        QWindowsStyle::drawPrimitive(pe, option, p, widget);
        return;
    }
    theme.mirrorHorizontally = hMirrored;
    theme.mirrorVertically = vMirrored;
    theme.noBorder = noBorder;
    theme.noContent = noContent;
    theme.rotate = rotate;
    d->drawBackground(theme);
}

/*!
    \reimp
*/
void QWindowsXPStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *p,
                                  const QWidget *widget) const
{
  QWindowsXPStylePrivate *d = const_cast<QWindowsXPStylePrivate*>(d_func());
    if (!QWindowsXPStylePrivate::useXP()) {
        QWindowsStyle::drawControl(element, option, p, widget);
        return;
    }

    QRect rect(option->rect);
    State flags = option->state;

    int rotate = 0;
    bool hMirrored = false;
    bool vMirrored = false;

    int themeNumber = -1;
    int partId = 0;
    int stateId = 0;
    switch (element) {
    case CE_SizeGrip:
        {
            themeNumber = QWindowsXPStylePrivate::StatusTheme;
            partId = SP_GRIPPER;
            SIZE sz;
            XPThemeData theme(0, p, themeNumber, partId, 0);
            pGetThemePartSize(theme.handle(), 0, partId, 0, 0, TS_TRUE, &sz);
            --sz.cy;
            if (const QStyleOptionSizeGrip *sg = qstyleoption_cast<const QStyleOptionSizeGrip *>(option)) {
                switch (sg->corner) {
                    case Qt::BottomRightCorner:
                        rect = QRect(rect.right() - sz.cx, rect.bottom() - sz.cy, sz.cx, sz.cy);
                        break;
                    case Qt::BottomLeftCorner:
                        rect = QRect(rect.left() + 1, rect.bottom() - sz.cy, sz.cx, sz.cy);
                        hMirrored = true;
                        break;
                    case Qt::TopRightCorner:
                        rect = QRect(rect.right() - sz.cx, rect.top() + 1, sz.cx, sz.cy);
                        vMirrored = true;
                        break;
                    case Qt::TopLeftCorner:
                        rect = QRect(rect.left() + 1, rect.top() + 1, sz.cx, sz.cy);
                        hMirrored = vMirrored = true;
                }
            }
        }
        break;

    case CE_HeaderSection:
        themeNumber = QWindowsXPStylePrivate::HeaderTheme;
        partId = HP_HEADERITEM;
        if (flags & State_Sunken)
            stateId = HIS_PRESSED;
        else if (flags & State_MouseOver)
            stateId = HIS_HOT;
        else
            stateId = HIS_NORMAL;
        break;

    case CE_Splitter:
        p->eraseRect(option->rect);
        return;

    case CE_PushButtonBevel:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option))
        {
            themeNumber = QWindowsXPStylePrivate::ButtonTheme;
            partId = BP_PUSHBUTTON;
            bool justFlat = ((btn->features & QStyleOptionButton::Flat) && !(flags & (State_On|State_Sunken)))
                || ((btn->features & QStyleOptionButton::CommandLinkButton)
                    && !(flags & State_MouseOver)
                    && !(btn->features & QStyleOptionButton::DefaultButton));
            if (!(flags & State_Enabled) && !(btn->features & QStyleOptionButton::Flat))
                stateId = PBS_DISABLED;
            else if (justFlat)
                ;
            else if (flags & (State_Sunken | State_On))
                stateId = PBS_PRESSED;
            else if (flags & State_MouseOver)
                stateId = PBS_HOT;
            else if (btn->features & QStyleOptionButton::DefaultButton)
                stateId = PBS_DEFAULTED;
            else
                stateId = PBS_NORMAL;

            if (!justFlat) {
                XPThemeData theme(widget, p, themeNumber, partId, stateId, rect);
                d->drawBackground(theme);
            }

            if (btn->features & QStyleOptionButton::HasMenu) {
                int mbiw = 0, mbih = 0;
                XPThemeData theme(widget, 0,
                                  QWindowsXPStylePrivate::ToolBarTheme,
                                  TP_SPLITBUTTONDROPDOWN);
                if (theme.isValid()) {
                    SIZE size;
                    pGetThemePartSize(theme.handle(), 0, theme.partId, theme.stateId, 0, TS_TRUE, &size);
                    mbiw = size.cx;
                    mbih = size.cy;
                }

                QRect ir = btn->rect;
                QStyleOptionButton newBtn = *btn;
                newBtn.rect = QRect(ir.right() - mbiw - 1, 1 + (ir.height()/2) - (mbih/2), mbiw, mbih);
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &newBtn, p, widget);
            }
            return;
        }
        break;
    case CE_TabBarTab:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option))
        {
            stateId = tab->state & State_Enabled ? TIS_NORMAL : TIS_DISABLED;
        }
    break;

    case CE_TabBarTabShape:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option))
        {
            themeNumber = QWindowsXPStylePrivate::TabTheme;
            bool isDisabled = !(tab->state & State_Enabled);
            bool hasFocus = tab->state & State_HasFocus;
            bool isHot = tab->state & State_MouseOver;
            bool selected = tab->state & State_Selected;
            bool lastTab = tab->position == QStyleOptionTab::End;
            bool firstTab = tab->position == QStyleOptionTab::Beginning;
            bool onlyOne = tab->position == QStyleOptionTab::OnlyOneTab;
            bool leftAligned = proxy()->styleHint(SH_TabBar_Alignment, tab, widget) == Qt::AlignLeft;
            bool centerAligned = proxy()->styleHint(SH_TabBar_Alignment, tab, widget) == Qt::AlignCenter;
            int borderThickness = proxy()->pixelMetric(PM_DefaultFrameWidth, option, widget);
            int tabOverlap = proxy()->pixelMetric(PM_TabBarTabOverlap, option, widget);

            if (isDisabled)
                stateId = TIS_DISABLED;
            else if (selected)
                stateId = TIS_SELECTED;
            else if (hasFocus)
                stateId = TIS_FOCUSED;
            else if (isHot)
                stateId = TIS_HOT;
            else
                stateId = TIS_NORMAL;

            // Selecting proper part depending on position
            if (firstTab || onlyOne) {
                if (leftAligned) {
                    partId = TABP_TABITEMLEFTEDGE;
                } else if (centerAligned) {
                    partId = TABP_TABITEM;
                } else { // rightAligned
                    partId = TABP_TABITEMRIGHTEDGE;
                }
            } else {
                partId = TABP_TABITEM;
            }

            if (tab->direction == Qt::RightToLeft
                && (tab->shape == QTabBar::RoundedNorth
                    || tab->shape == QTabBar::RoundedSouth)) {
                bool temp = firstTab;
                firstTab = lastTab;
                lastTab = temp;
            }
            bool begin = firstTab || onlyOne;
            bool end = lastTab || onlyOne;
            switch (tab->shape) {
            case QTabBar::RoundedNorth:
                if (selected)
                    rect.adjust(begin ? 0 : -tabOverlap, 0, end ? 0 : tabOverlap, borderThickness);
                else
                    rect.adjust(begin? tabOverlap : 0, tabOverlap, end ? -tabOverlap : 0, 0);
                break;
            case QTabBar::RoundedSouth:
                //vMirrored = true;
                rotate = 180; // Not 100% correct, but works
                if (selected)
                    rect.adjust(begin ? 0 : -tabOverlap , -borderThickness, end ? 0 : tabOverlap, 0);
                else
                    rect.adjust(begin ? tabOverlap : 0, 0, end ? -tabOverlap : 0 , -tabOverlap);
                break;
            case QTabBar::RoundedEast:
                rotate = 90;
                if (selected) {
                    rect.adjust(-borderThickness, begin ? 0 : -tabOverlap, 0, end ? 0 : tabOverlap);
                }else{
                    rect.adjust(0, begin ? tabOverlap : 0, -tabOverlap, end ? -tabOverlap : 0);
                }
                break;
            case QTabBar::RoundedWest:
                hMirrored = true;
                rotate = 90;
                if (selected) {
                    rect.adjust(0, begin ? 0 : -tabOverlap, borderThickness, end ? 0 : tabOverlap);
                }else{
                    rect.adjust(tabOverlap, begin ? tabOverlap : 0, 0, end ? -tabOverlap : 0);
                }
                break;
            default:
                themeNumber = -1; // Do our own painting for triangular
                break;
            }

            if (!selected) {
                switch (tab->shape) {
                case QTabBar::RoundedNorth:
                    rect.adjust(0,0, 0,-1);
                    break;
                case QTabBar::RoundedSouth:
                    rect.adjust(0,1, 0,0);
                    break;
                case QTabBar::RoundedEast:
                    rect.adjust( 1,0, 0,0);
                    break;
                case QTabBar::RoundedWest:
                    rect.adjust(0,0, -1,0);
                    break;
                default:
                    break;
                }
            }
        }
        break;

    case CE_ProgressBarGroove:
        {
        Qt::Orientation orient = Qt::Horizontal;
        if (const QStyleOptionProgressBarV2 *pb2 = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(option))
            orient = pb2->orientation;
        partId = (orient == Qt::Horizontal) ? PP_BAR : PP_BARVERT;
        themeNumber = QWindowsXPStylePrivate::ProgressTheme;
        stateId = 1;
        }
        break;

    case CE_MenuEmptyArea:
    case CE_MenuItem:
        if (const QStyleOptionMenuItem *menuitem = qstyleoption_cast<const QStyleOptionMenuItem *>(option))
        {
            int tab = menuitem->tabWidth;
            bool dis = !(menuitem->state & State_Enabled);
            bool act = menuitem->state & State_Selected;
            bool checkable = menuitem->menuHasCheckableItems;
            bool checked = checkable ? menuitem->checked : false;

            // windows always has a check column, regardless whether we have an icon or not
            int checkcol = qMax(menuitem->maxIconWidth, 12);

            int x, y, w, h;
            rect.getRect(&x, &y, &w, &h);

            QBrush fill = menuitem->palette.brush(act ? QPalette::Highlight : QPalette::Button);
            p->fillRect(rect, fill);

            if (element == CE_MenuEmptyArea)
                break;

            // draw separator -------------------------------------------------
            if (menuitem->menuItemType == QStyleOptionMenuItem::Separator) {
                int yoff = y-1 + h / 2;
                p->setPen(menuitem->palette.dark().color());
                p->drawLine(x, yoff, x+w, yoff);
                ++yoff;
                p->setPen(menuitem->palette.light().color());
                p->drawLine(x, yoff, x+w, yoff);
                return;
            }

            int xpos = x;

            // draw icon ------------------------------------------------------
            if (!menuitem->icon.isNull()) {
                QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
                if (act && !dis)
                    mode = QIcon::Active;
                QPixmap pixmap = checked ?
                                 menuitem->icon.pixmap(proxy()->pixelMetric(PM_SmallIconSize, option, widget), mode, QIcon::On) :
                                 menuitem->icon.pixmap(proxy()->pixelMetric(PM_SmallIconSize, option, widget), mode);
                int pixw = pixmap.width();
                int pixh = pixmap.height();
                QRect iconRect(0, 0, pixw, pixh);
                iconRect.moveCenter(QRect(xpos, y, checkcol, h).center());
                QRect vIconRect = visualRect(option->direction, option->rect, iconRect);
                p->setPen(menuitem->palette.text().color());
                p->setBrush(Qt::NoBrush);
                if (checked)
                    p->drawRect(vIconRect.adjusted(-1, -1, 0, 0));
                p->drawPixmap(vIconRect.topLeft(), pixmap);

            // draw checkmark -------------------------------------------------
            } else if (checked) {
                QStyleOptionMenuItem newMi = *menuitem;
                newMi.state = State_None;
                if (!dis)
                    newMi.state |= State_Enabled;
                if (act)
                    newMi.state |= State_On;

                QRect checkMarkRect = QRect(menuitem->rect.x() + windowsItemFrame,
                                            menuitem->rect.y() + windowsItemFrame,
                                            checkcol - 2 * windowsItemFrame,
                                            menuitem->rect.height() - 2*windowsItemFrame);
                newMi.rect = visualRect(option->direction, option->rect, checkMarkRect);
                proxy()->drawPrimitive(PE_IndicatorMenuCheckMark, &newMi, p, widget);
            }

            QColor textColor = dis ? menuitem->palette.text().color() :
                               act ? menuitem->palette.highlightedText().color() : menuitem->palette.buttonText().color();
            p->setPen(textColor);

            // draw text ------------------------------------------------------
            int xm = windowsItemFrame + checkcol + windowsItemHMargin;
            xpos = menuitem->rect.x() + xm;
            QRect textRect(xpos, y + windowsItemVMargin, w - xm - windowsRightBorder - tab + 1, h - 2 * windowsItemVMargin);
            QRect vTextRect = visualRect(option->direction, option->rect, textRect);
            QString s = menuitem->text;
            if (!s.isEmpty()) {
                p->save();
                int t = s.indexOf(QLatin1Char('\t'));
                int text_flags = Qt::AlignVCenter|Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine | Qt::AlignLeft;
                if (!proxy()->styleHint(SH_UnderlineShortcut, menuitem, widget))
                    text_flags |= Qt::TextHideMnemonic;
                // draw tab text ----------------
                if (t >= 0) {
                    QRect vShortcutRect = visualRect(option->direction, option->rect, QRect(textRect.topRight(), menuitem->rect.bottomRight()));
                    if (dis && !act && proxy()->styleHint(SH_EtchDisabledText, option, widget)) {
                        p->setPen(menuitem->palette.light().color());
                        p->drawText(vShortcutRect.adjusted(1,1,1,1), text_flags, s.mid(t + 1));
                        p->setPen(textColor);
                    }
                    p->drawText(vShortcutRect, text_flags, s.mid(t + 1));
                    s = s.left(t);
                }
                QFont font = menuitem->font;
                if (menuitem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                    font.setBold(true);
                p->setFont(font);
                if (dis && !act && proxy()->styleHint(SH_EtchDisabledText, option, widget)) {
                    p->setPen(menuitem->palette.light().color());
                    p->drawText(vTextRect.adjusted(1,1,1,1), text_flags, s.left(t));
                    p->setPen(textColor);
                }
                p->drawText(vTextRect, text_flags, s);
                p->restore();
            }

            // draw sub menu arrow --------------------------------------------
            if (menuitem->menuItemType == QStyleOptionMenuItem::SubMenu) {
                int dim = (h - 2) / 2;
                PrimitiveElement arrow;
                arrow = (option->direction == Qt::RightToLeft) ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight;
                xpos = x + w - windowsArrowHMargin - windowsItemFrame - dim;
                QRect vSubMenuRect = visualRect(option->direction, option->rect, QRect(xpos, y + h / 2 - dim / 2, dim, dim));
                QStyleOptionMenuItem newMI = *menuitem;
                newMI.rect = vSubMenuRect;
                newMI.state = dis ? State_None : State_Enabled;
                if (act)
                    newMI.palette.setColor(QPalette::ButtonText, newMI.palette.highlightedText().color());
                proxy()->drawPrimitive(arrow, &newMI, p, widget);
            }
        }
        return;

    case CE_MenuBarItem:
        if (const QStyleOptionMenuItem *mbi = qstyleoption_cast<const QStyleOptionMenuItem *>(option))
        {
            if (mbi->menuItemType == QStyleOptionMenuItem::DefaultItem)
                break;

            bool act = mbi->state & State_Selected;
            bool dis = !(mbi->state & State_Enabled);

            QBrush fill = mbi->palette.brush(act ? QPalette::Highlight : QPalette::Button);
            QPalette::ColorRole textRole = dis ? QPalette::Text:
                                           act ? QPalette::HighlightedText : QPalette::ButtonText;
            QPixmap pix = mbi->icon.pixmap(proxy()->pixelMetric(PM_SmallIconSize, option, widget), QIcon::Normal);

            uint alignment = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
            if (!proxy()->styleHint(SH_UnderlineShortcut, mbi, widget))
                alignment |= Qt::TextHideMnemonic;

            p->fillRect(rect, fill);
            if (!pix.isNull())
                drawItemPixmap(p, mbi->rect, alignment, pix);
            else
                drawItemText(p, mbi->rect, alignment, mbi->palette, mbi->state & State_Enabled, mbi->text, textRole);
        }
        return;
#ifndef QT_NO_DOCKWIDGET
    case CE_DockWidgetTitle:
        if (const QStyleOptionDockWidget *dwOpt = qstyleoption_cast<const QStyleOptionDockWidget *>(option))
        {
            int buttonMargin = 4;
            int mw = proxy()->pixelMetric(QStyle::PM_DockWidgetTitleMargin, dwOpt, widget);
            int fw = proxy()->pixelMetric(PM_DockWidgetFrameWidth, dwOpt, widget);
            bool isFloating = widget && widget->isWindow();
            bool isActive = dwOpt->state & State_Active;

            const QStyleOptionDockWidgetV2 *v2
                = qstyleoption_cast<const QStyleOptionDockWidgetV2*>(dwOpt);
            bool verticalTitleBar = v2 == 0 ? false : v2->verticalTitleBar;

            if (verticalTitleBar) {
                QSize s = rect.size();
                s.transpose();
                rect.setSize(s);

                p->translate(rect.left() - 1, rect.top() + rect.width());
                p->rotate(-90);
                p->translate(-rect.left() + 1, -rect.top());
            }
            QRect r = rect.adjusted(0, 2, -1, -3);
            QRect titleRect = r;

            if (dwOpt->closable) {
                QSize sz = proxy()->standardIcon(QStyle::SP_TitleBarCloseButton, dwOpt, widget).actualSize(QSize(10, 10));
                titleRect.adjust(0, 0, -sz.width() - mw - buttonMargin, 0);
            }

            if (dwOpt->floatable) {
                QSize sz = proxy()->standardIcon(QStyle::SP_TitleBarMaxButton, dwOpt, widget).actualSize(QSize(10, 10));
                titleRect.adjust(0, 0, -sz.width() - mw - buttonMargin, 0);
            }

            if (isFloating) {
                titleRect.adjust(0, -fw, 0, 0);
                if (widget != 0 && widget->windowIcon().cacheKey() != QApplication::windowIcon().cacheKey())
                    titleRect.adjust(titleRect.height() + mw, 0, 0, 0);
            } else {
                titleRect.adjust(mw, 0, 0, 0);
                if (!dwOpt->floatable && !dwOpt->closable)
                    titleRect.adjust(0, 0, -mw, 0);
            }

            if (!verticalTitleBar)
                titleRect = visualRect(dwOpt->direction, r, titleRect);

            if (!isFloating) {
                QPen oldPen = p->pen();
                QString titleText = p->fontMetrics().elidedText(dwOpt->title, Qt::ElideRight, titleRect.width());
                p->setPen(dwOpt->palette.color(QPalette::Dark));
                p->drawRect(r);

                if (!titleText.isEmpty()) {
                    drawItemText(p, titleRect,
                                Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, dwOpt->palette,
                                dwOpt->state & State_Enabled, titleText,
                                QPalette::WindowText);
                }

                p->setPen(oldPen);
            } else {
                themeNumber = QWindowsXPStylePrivate::WindowTheme;
                if (isActive)
                    stateId = CS_ACTIVE;
                else
                    stateId = CS_INACTIVE;

                int titleHeight = rect.height() - 2;
                rect = rect.adjusted(-fw, -fw, fw, 0);

                XPThemeData theme(widget, p, themeNumber, 0, stateId);
                if (!theme.isValid())
                    break;

                // Draw small type title bar
                theme.rect = rect;
                theme.partId = WP_SMALLCAPTION;
                d->drawBackground(theme);

                // Figure out maximal button space on title bar

                QIcon ico = widget->windowIcon();
                bool hasIcon = (ico.cacheKey() != QApplication::windowIcon().cacheKey());
                if (hasIcon) {
                    QPixmap pxIco = ico.pixmap(titleHeight);
                    if (!verticalTitleBar && dwOpt->direction == Qt::RightToLeft)
                        p->drawPixmap(rect.width() - titleHeight - pxIco.width(), rect.bottom() - titleHeight - 2, pxIco);
                    else
                        p->drawPixmap(fw, rect.bottom() - titleHeight - 2, pxIco);
                }
                if (!dwOpt->title.isEmpty()) {
                    QPen oldPen = p->pen();
                    QFont oldFont = p->font();
                    QFont titleFont = oldFont;
                    titleFont.setBold(true);
                    p->setFont(titleFont);
                    QString titleText
                        = p->fontMetrics().elidedText(dwOpt->title, Qt::ElideRight, titleRect.width());

                    int result = TST_NONE;
                    pGetThemeEnumValue(theme.handle(), WP_SMALLCAPTION, isActive ? CS_ACTIVE : CS_INACTIVE, TMT_TEXTSHADOWTYPE, &result);
                    if (result != TST_NONE) {
                        COLORREF textShadowRef;
                        pGetThemeColor(theme.handle(), WP_SMALLCAPTION, isActive ? CS_ACTIVE : CS_INACTIVE, TMT_TEXTSHADOWCOLOR, &textShadowRef);
                        QColor textShadow = qRgb(GetRValue(textShadowRef), GetGValue(textShadowRef), GetBValue(textShadowRef));
                        p->setPen(textShadow);
                        drawItemText(p, titleRect.adjusted(1, 1, 1, 1),
                                    Qt::AlignLeft | Qt::AlignBottom, dwOpt->palette,
                                    dwOpt->state & State_Enabled, titleText);
                    }

                    COLORREF captionText = GetSysColor(isActive ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT);
                    QColor textColor = qRgb(GetRValue(captionText), GetGValue(captionText), GetBValue(captionText));
                    p->setPen(textColor);
                    drawItemText(p, titleRect,
                                Qt::AlignLeft | Qt::AlignBottom, dwOpt->palette,
                                dwOpt->state & State_Enabled, titleText);
                    p->setFont(oldFont);
                    p->setPen(oldPen);
                }

            }

            return;
        }
        break;
#endif // QT_NO_DOCKWIDGET
#ifndef QT_NO_RUBBERBAND
    case CE_RubberBand:
        if (qstyleoption_cast<const QStyleOptionRubberBand *>(option)) {
            QColor highlight = option->palette.color(QPalette::Active, QPalette::Highlight);
            p->save();
            p->setPen(highlight.darker(120));
            QColor dimHighlight(qMin(highlight.red()/2 + 110, 255),
                                qMin(highlight.green()/2 + 110, 255),
                                qMin(highlight.blue()/2 + 110, 255),
                                (widget && widget->isTopLevel())? 255 : 127);
            p->setBrush(dimHighlight);
            p->drawRect(option->rect.adjusted(0, 0, -1, -1));
            p->restore();
            return;
        }
#endif // QT_NO_RUBBERBAND
    case CE_HeaderEmptyArea:
        if (option->state & State_Horizontal)
        {
            themeNumber = QWindowsXPStylePrivate::HeaderTheme;
            stateId = HIS_NORMAL;
        }
        else {
            QWindowsStyle::drawControl(CE_HeaderEmptyArea, option, p, widget);
            return;
        }
        break;
    default:
        break;
    }

    XPThemeData theme(widget, p, themeNumber, partId, stateId, rect);
    if (!theme.isValid()) {
        QWindowsStyle::drawControl(element, option, p, widget);
        return;
    }

    theme.rotate = rotate;
    theme.mirrorHorizontally = hMirrored;
    theme.mirrorVertically = vMirrored;
    d->drawBackground(theme);
}


/*!
    \reimp
*/
void QWindowsXPStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex *option,
                                         QPainter *p, const QWidget *widget) const
{
    QWindowsXPStylePrivate *d = const_cast<QWindowsXPStylePrivate*>(d_func());

    if (!QWindowsXPStylePrivate::useXP()) {
        QWindowsStyle::drawComplexControl(cc, option, p, widget);
        return;
    }

    State flags = option->state;
    SubControls sub = option->subControls;
    QRect r = option->rect;

    int partId = 0;
    int stateId = 0;
    if (widget && widget->testAttribute(Qt::WA_UnderMouse) && widget->isActiveWindow())
        flags |= State_MouseOver;

    switch (cc) {
#ifndef QT_NO_SPINBOX
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(option))
        {
            XPThemeData theme(widget, p, QWindowsXPStylePrivate::SpinTheme);

            if (sb->frame && (sub & SC_SpinBoxFrame)) {
                partId = EP_EDITTEXT;
                if (!(flags & State_Enabled))
                    stateId = ETS_DISABLED;
                else if (flags & State_HasFocus)
                    stateId = ETS_FOCUSED;
                else
                    stateId = ETS_NORMAL;

                XPThemeData ftheme(widget, p, QWindowsXPStylePrivate::EditTheme,
                                   partId, stateId, r);
                ftheme.noContent = true;
                d->drawBackground(ftheme);
            }
            if (sub & SC_SpinBoxUp) {
                theme.rect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxUp, widget);
                partId = SPNP_UP;
                if (!(sb->stepEnabled & QAbstractSpinBox::StepUpEnabled) || !(flags & State_Enabled))
                    stateId = UPS_DISABLED;
                else if (sb->activeSubControls == SC_SpinBoxUp && (sb->state & State_Sunken))
                    stateId = UPS_PRESSED;
                else if (sb->activeSubControls == SC_SpinBoxUp && (sb->state & State_MouseOver))
                    stateId = UPS_HOT;
                else
                    stateId = UPS_NORMAL;
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
            if (sub & SC_SpinBoxDown) {
                theme.rect = proxy()->subControlRect(CC_SpinBox, option, SC_SpinBoxDown, widget);
                partId = SPNP_DOWN;
                if (!(sb->stepEnabled & QAbstractSpinBox::StepDownEnabled) || !(flags & State_Enabled))
                    stateId = DNS_DISABLED;
                else if (sb->activeSubControls == SC_SpinBoxDown && (sb->state & State_Sunken))
                    stateId = DNS_PRESSED;
                else if (sb->activeSubControls == SC_SpinBoxDown && (sb->state & State_MouseOver))
                    stateId = DNS_HOT;
                else
                    stateId = DNS_NORMAL;
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
        }
        break;
#endif // QT_NO_SPINBOX
#ifndef QT_NO_COMBOBOX
    case CC_ComboBox:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option))
        {
            if (sub & SC_ComboBoxEditField) {
                if (cmb->frame) {
                    partId = EP_EDITTEXT;
                    if (!(flags & State_Enabled))
                        stateId = ETS_DISABLED;
                    else if (flags & State_HasFocus)
                        stateId = ETS_FOCUSED;
                    else
                        stateId = ETS_NORMAL;
                    XPThemeData theme(widget, p, QWindowsXPStylePrivate::EditTheme, partId, stateId, r);
                    d->drawBackground(theme);
                } else {
                    QBrush editBrush = cmb->palette.brush(QPalette::Base);
                    p->fillRect(option->rect, editBrush);
                }
                if (!cmb->editable) {
                    QRect re = proxy()->subControlRect(CC_ComboBox, option, SC_ComboBoxEditField, widget);
                    if (option->state & State_HasFocus) {
                        p->fillRect(re, option->palette.highlight());
                        p->setPen(option->palette.highlightedText().color());
                        p->setBackground(option->palette.highlight());
                    } else {
                        p->fillRect(re, option->palette.base());
                        p->setPen(option->palette.text().color());
                        p->setBackground(option->palette.base());
                    }
                }
            }

            if (sub & SC_ComboBoxArrow) {
                XPThemeData theme(widget, p, QWindowsXPStylePrivate::ComboboxTheme);
                theme.rect = proxy()->subControlRect(CC_ComboBox, option, SC_ComboBoxArrow, widget);
                partId = CP_DROPDOWNBUTTON;
                if (!(flags & State_Enabled))
                    stateId = CBXS_DISABLED;
                else if (cmb->activeSubControls == SC_ComboBoxArrow && (cmb->state & State_Sunken))
                    stateId = CBXS_PRESSED;
                else if (cmb->activeSubControls == SC_ComboBoxArrow && (cmb->state & State_MouseOver))
                    stateId = CBXS_HOT;
                else
                    stateId = CBXS_NORMAL;
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
        }
        break;
#endif // QT_NO_COMBOBOX
    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(option))
        {
            XPThemeData theme(widget, p, QWindowsXPStylePrivate::ScrollBarTheme);
            bool maxedOut = (scrollbar->maximum == scrollbar->minimum);
            if (maxedOut)
                flags &= ~State_Enabled;

            bool isHorz = flags & State_Horizontal;
            bool isRTL  = option->direction == Qt::RightToLeft;
            if (sub & SC_ScrollBarAddLine) {
                theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarAddLine, widget);
                partId = SBP_ARROWBTN;
                if (!(flags & State_Enabled))
                    stateId = (isHorz ? (isRTL ? ABS_LEFTDISABLED : ABS_RIGHTDISABLED) : ABS_DOWNDISABLED);
                else if (scrollbar->activeSubControls & SC_ScrollBarAddLine && (scrollbar->state & State_Sunken))
                    stateId = (isHorz ? (isRTL ? ABS_LEFTPRESSED : ABS_RIGHTPRESSED) : ABS_DOWNPRESSED);
                else if (scrollbar->activeSubControls & SC_ScrollBarAddLine && (scrollbar->state & State_MouseOver))
                    stateId = (isHorz ? (isRTL ? ABS_LEFTHOT : ABS_RIGHTHOT) : ABS_DOWNHOT);
                else
                    stateId = (isHorz ? (isRTL ? ABS_LEFTNORMAL : ABS_RIGHTNORMAL) : ABS_DOWNNORMAL);
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
            if (sub & SC_ScrollBarSubLine) {
                theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSubLine, widget);
                partId = SBP_ARROWBTN;
                if (!(flags & State_Enabled))
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTDISABLED : ABS_LEFTDISABLED) : ABS_UPDISABLED);
                else if (scrollbar->activeSubControls & SC_ScrollBarSubLine && (scrollbar->state & State_Sunken))
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTPRESSED : ABS_LEFTPRESSED) : ABS_UPPRESSED);
                else if (scrollbar->activeSubControls & SC_ScrollBarSubLine && (scrollbar->state & State_MouseOver))
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTHOT : ABS_LEFTHOT) : ABS_UPHOT);
                else
                    stateId = (isHorz ? (isRTL ? ABS_RIGHTNORMAL : ABS_LEFTNORMAL) : ABS_UPNORMAL);
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
            if (maxedOut) {
                theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
                theme.rect = theme.rect.united(proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSubPage, widget));
                theme.rect = theme.rect.united(proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarAddPage, widget));
                partId = scrollbar->orientation == Qt::Horizontal ? SBP_LOWERTRACKHORZ : SBP_LOWERTRACKVERT;
                stateId = SCRBS_DISABLED;
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            } else {
                if (sub & SC_ScrollBarSubPage) {
                    theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSubPage, widget);
                    partId = flags & State_Horizontal ? SBP_UPPERTRACKHORZ : SBP_UPPERTRACKVERT;
                    if (!(flags & State_Enabled))
                        stateId = SCRBS_DISABLED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarSubPage && (scrollbar->state & State_Sunken))
                        stateId = SCRBS_PRESSED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarSubPage && (scrollbar->state & State_MouseOver))
                        stateId = SCRBS_HOT;
                    else
                        stateId = SCRBS_NORMAL;
                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
                if (sub & SC_ScrollBarAddPage) {
                    theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarAddPage, widget);
                    partId = flags & State_Horizontal ? SBP_LOWERTRACKHORZ : SBP_LOWERTRACKVERT;
                    if (!(flags & State_Enabled))
                        stateId = SCRBS_DISABLED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarAddPage && (scrollbar->state & State_Sunken))
                        stateId = SCRBS_PRESSED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarAddPage && (scrollbar->state & State_MouseOver))
                        stateId = SCRBS_HOT;
                    else
                        stateId = SCRBS_NORMAL;
                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
                if (sub & SC_ScrollBarSlider) {
                    theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
                    if (!(flags & State_Enabled))
                        stateId = SCRBS_DISABLED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarSlider && (scrollbar->state & State_Sunken))
                        stateId = SCRBS_PRESSED;
                    else if (scrollbar->activeSubControls & SC_ScrollBarSlider && (scrollbar->state & State_MouseOver))
                        stateId = SCRBS_HOT;
                    else
                        stateId = SCRBS_NORMAL;

                    // Draw handle
                    theme.rect = proxy()->subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
                    theme.partId = flags & State_Horizontal ? SBP_THUMBBTNHORZ : SBP_THUMBBTNVERT;
                    theme.stateId = stateId;
                    d->drawBackground(theme);

                    // Calculate rect of gripper
                    const int swidth = theme.rect.width();
                    const int sheight = theme.rect.height();

                    MARGINS contentsMargin;
                    RECT rect = theme.toRECT(theme.rect);
                    pGetThemeMargins(theme.handle(), 0, theme.partId, theme.stateId, TMT_SIZINGMARGINS, &rect, &contentsMargin);

                    SIZE size;
                    theme.partId = flags & State_Horizontal ? SBP_GRIPPERHORZ : SBP_GRIPPERVERT;
                    pGetThemePartSize(theme.handle(), 0, theme.partId, theme.stateId, 0, TS_TRUE, &size);
                    int gw = size.cx, gh = size.cy;


                    QRect gripperBounds;
                    if (flags & State_Horizontal && ((swidth - contentsMargin.cxLeftWidth - contentsMargin.cxRightWidth) > gw)) {
                        gripperBounds.setLeft(theme.rect.left() + swidth/2 - gw/2);
                        gripperBounds.setTop(theme.rect.top() + sheight/2 - gh/2);
                        gripperBounds.setWidth(gw);
                        gripperBounds.setHeight(gh);
                    } else if ((sheight - contentsMargin.cyTopHeight - contentsMargin.cyBottomHeight) > gh) {
                        gripperBounds.setLeft(theme.rect.left() + swidth/2 - gw/2);
                        gripperBounds.setTop(theme.rect.top() + sheight/2 - gh/2);
                        gripperBounds.setWidth(gw);
                        gripperBounds.setHeight(gh);
                    }

                    // Draw gripper if there is enough space
                    if (!gripperBounds.isEmpty()) {
                        p->save();
                        theme.rect = gripperBounds;
                        p->setClipRegion(d->region(theme));// Only change inside the region of the gripper
                        d->drawBackground(theme);          // Transparent gripper ontop of background
                        p->restore();
                    }
                }
            }
        }
        break;

#ifndef QT_NO_SLIDER
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option))
        {
            XPThemeData theme(widget, p, QWindowsXPStylePrivate::TrackBarTheme);
            QRect slrect = slider->rect;
            QRegion tickreg = slrect;
            if (sub & SC_SliderGroove) {
                theme.rect = proxy()->subControlRect(CC_Slider, option, SC_SliderGroove, widget);
                if (slider->orientation == Qt::Horizontal) {
                    partId = TKP_TRACK;
                    stateId = TRS_NORMAL;
                    theme.rect = QRect(slrect.left(), theme.rect.center().y() - 2, slrect.width(), 4);
                } else {
                    partId = TKP_TRACKVERT;
                    stateId = TRVS_NORMAL;
                    theme.rect = QRect(theme.rect.center().x() - 2, slrect.top(), 4, slrect.height());
                }
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
                tickreg -= theme.rect;
            }
            if (sub & SC_SliderTickmarks) {
                int tickOffset = proxy()->pixelMetric(PM_SliderTickmarkOffset, slider, widget);
                int ticks = slider->tickPosition;
                int thickness = proxy()->pixelMetric(PM_SliderControlThickness, slider, widget);
                int len = proxy()->pixelMetric(PM_SliderLength, slider, widget);
                int available = proxy()->pixelMetric(PM_SliderSpaceAvailable, slider, widget);
                int interval = slider->tickInterval;
                if (interval <= 0) {
                    interval = slider->singleStep;
                    if (QStyle::sliderPositionFromValue(slider->minimum, slider->maximum, interval,
                                                        available)
                        - QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                                                          0, available) < 3)
                        interval = slider->pageStep;
                }
                if (!interval)
                    interval = 1;
                int fudge = len / 2;
                int pos;
                int bothOffset = (ticks & QSlider::TicksAbove && ticks & QSlider::TicksBelow) ? 1 : 0;
                p->setPen(d->sliderTickColor);
                QVarLengthArray<QLine, 32> lines;
                int v = slider->minimum;
                while (v <= slider->maximum + 1) {
                    if (v == slider->maximum + 1 && interval == 1)
                        break;
                    const int v_ = qMin(v, slider->maximum);
                    int tickLength = (v_ == slider->minimum || v_ >= slider->maximum) ? 4 : 3;
                    pos = QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                                                          v_, available) + fudge;
                    if (slider->orientation == Qt::Horizontal) {
                        if (ticks & QSlider::TicksAbove)
                            lines.append(QLine(pos, tickOffset - 1 - bothOffset,
                                               pos, tickOffset - 1 - bothOffset - tickLength));

                        if (ticks & QSlider::TicksBelow)
                            lines.append(QLine(pos, tickOffset + thickness + bothOffset,
                                               pos, tickOffset + thickness + bothOffset + tickLength));
                    } else {
                        if (ticks & QSlider::TicksAbove)
                            lines.append(QLine(tickOffset - 1 - bothOffset, pos,
                                               tickOffset - 1 - bothOffset - tickLength, pos));

                        if (ticks & QSlider::TicksBelow)
                            lines.append(QLine(tickOffset + thickness + bothOffset, pos,
                                               tickOffset + thickness + bothOffset + tickLength, pos));
                    }
                    // in the case where maximum is max int
                    int nextInterval = v + interval;
                    if (nextInterval < v)
                        break;
                    v = nextInterval;
                }
                if (lines.size() > 0) {
                    p->save();
                    p->translate(slrect.topLeft());
                    p->drawLines(lines.constData(), lines.size());
                    p->restore();
                }
            }
            if (sub & SC_SliderHandle) {
                theme.rect = proxy()->subControlRect(CC_Slider, option, SC_SliderHandle, widget);
                if (slider->orientation == Qt::Horizontal) {
                    if (slider->tickPosition == QSlider::TicksAbove)
                        partId = TKP_THUMBTOP;
                    else if (slider->tickPosition == QSlider::TicksBelow)
                        partId = TKP_THUMBBOTTOM;
                    else
                        partId = TKP_THUMB;

                    if (!(slider->state & State_Enabled))
                        stateId = TUS_DISABLED;
                    else if (slider->activeSubControls & SC_SliderHandle && (slider->state & State_Sunken))
                        stateId = TUS_PRESSED;
                    else if (slider->activeSubControls & SC_SliderHandle && (slider->state & State_MouseOver))
                        stateId = TUS_HOT;
                    else if (flags & State_HasFocus)
                        stateId = TUS_FOCUSED;
                    else
                        stateId = TUS_NORMAL;
                } else {
                    if (slider->tickPosition == QSlider::TicksLeft)
                        partId = TKP_THUMBLEFT;
                    else if (slider->tickPosition == QSlider::TicksRight)
                        partId = TKP_THUMBRIGHT;
                    else
                        partId = TKP_THUMBVERT;

                    if (!(slider->state & State_Enabled))
                        stateId = TUVS_DISABLED;
                    else if (slider->activeSubControls & SC_SliderHandle && (slider->state & State_Sunken))
                        stateId = TUVS_PRESSED;
                    else if (slider->activeSubControls & SC_SliderHandle && (slider->state & State_MouseOver))
                        stateId = TUVS_HOT;
                    else if (flags & State_HasFocus)
                        stateId = TUVS_FOCUSED;
                    else
                        stateId = TUVS_NORMAL;
                }
                theme.partId = partId;
                theme.stateId = stateId;
                d->drawBackground(theme);
            }
            if (slider->state & State_HasFocus) {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*slider);
                fropt.rect = subElementRect(SE_SliderFocusRect, slider, widget);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fropt, p, widget);
            }
        }
        break;
#endif
#ifndef QT_NO_TOOLBUTTON
    case CC_ToolButton:
        if (const QStyleOptionToolButton *toolbutton
            = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            QRect button, menuarea;
            button = proxy()->subControlRect(cc, toolbutton, SC_ToolButton, widget);
            menuarea = proxy()->subControlRect(cc, toolbutton, SC_ToolButtonMenu, widget);

            State bflags = toolbutton->state & ~State_Sunken;
            State mflags = bflags;
            bool autoRaise = flags & State_AutoRaise;
            if (autoRaise) {
                if (!(bflags & State_MouseOver) || !(bflags & State_Enabled)) {
                    bflags &= ~State_Raised;
                }
            }

            if (toolbutton->state & State_Sunken) {
                if (toolbutton->activeSubControls & SC_ToolButton) {
                    bflags |= State_Sunken;
                    mflags |= State_MouseOver | State_Sunken;
                } else if (toolbutton->activeSubControls & SC_ToolButtonMenu) {
                    mflags |= State_Sunken;
                    bflags |= State_MouseOver;
                }
            }

            QStyleOption tool(0);
            tool.palette = toolbutton->palette;
            if (toolbutton->subControls & SC_ToolButton) {
                if (flags & (State_Sunken | State_On | State_Raised) || !autoRaise) {
                    if (toolbutton->features & QStyleOptionToolButton::MenuButtonPopup && autoRaise) {
                        XPThemeData theme(widget, p, QWindowsXPStylePrivate::ToolBarTheme);
                        theme.partId = TP_SPLITBUTTON;
                        theme.rect = button;
                        if (!(bflags & State_Enabled))
                            stateId = TS_DISABLED;
                        else if (bflags & State_Sunken)
                            stateId = TS_PRESSED;
                        else if (bflags & State_MouseOver || !(flags & State_AutoRaise))
                            stateId = flags & State_On ? TS_HOTCHECKED : TS_HOT;
                        else if (bflags & State_On)
                            stateId = TS_CHECKED;
                        else
                            stateId = TS_NORMAL;
                        if (option->direction == Qt::RightToLeft)
                            theme.mirrorHorizontally = true;
                        theme.stateId = stateId;
                        d->drawBackground(theme);
                    } else {
                        tool.rect = option->rect;
                        tool.state = bflags;
                        if (autoRaise) // for tool bars
                            proxy()->drawPrimitive(PE_PanelButtonTool, &tool, p, widget);
                        else
                            proxy()->drawPrimitive(PE_PanelButtonBevel, &tool, p, widget);
                    }
                }
            }

            if (toolbutton->state & State_HasFocus) {
                QStyleOptionFocusRect fr;
                fr.QStyleOption::operator=(*toolbutton);
                fr.rect.adjust(3, 3, -3, -3);
                if (toolbutton->features & QStyleOptionToolButton::MenuButtonPopup)
                    fr.rect.adjust(0, 0, -proxy()->pixelMetric(QStyle::PM_MenuButtonIndicator,
                                                      toolbutton, widget), 0);
                proxy()->drawPrimitive(PE_FrameFocusRect, &fr, p, widget);
            }
            QStyleOptionToolButton label = *toolbutton;
            label.state = bflags;
            int fw = 2;
            if (!autoRaise)
                label.state &= ~State_Sunken;
            label.rect = button.adjusted(fw, fw, -fw, -fw);
            proxy()->drawControl(CE_ToolButtonLabel, &label, p, widget);

            if (toolbutton->subControls & SC_ToolButtonMenu) {
                tool.rect = menuarea;
                tool.state = mflags;
                if (autoRaise) {
                    proxy()->drawPrimitive(PE_IndicatorButtonDropDown, &tool, p, widget);
                } else {
                    tool.state = mflags;
                    menuarea.adjust(-2, 0, 0, 0);
                    // Draw menu button
                    if ((bflags & State_Sunken) != (mflags & State_Sunken)){
                        p->save();
                        p->setClipRect(menuarea);
                        tool.rect = option->rect;
                        proxy()->drawPrimitive(PE_PanelButtonBevel, &tool, p, 0);
                        p->restore();
                    }
                    // Draw arrow
                    p->save();
                    p->setPen(option->palette.dark().color());
                    p->drawLine(menuarea.left(), menuarea.top() + 3,
                                menuarea.left(), menuarea.bottom() - 3);
                    p->setPen(option->palette.light().color());
                    p->drawLine(menuarea.left() - 1, menuarea.top() + 3,
                                menuarea.left() - 1, menuarea.bottom() - 3);

                    tool.rect = menuarea.adjusted(2, 3, -2, -1);
                    proxy()->drawPrimitive(PE_IndicatorArrowDown, &tool, p, widget);
                    p->restore();
                }
            } else if (toolbutton->features & QStyleOptionToolButton::HasMenu) {
                int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, toolbutton, widget);
                QRect ir = toolbutton->rect;
                QStyleOptionToolButton newBtn = *toolbutton;
                newBtn.rect = QRect(ir.right() + 4 - mbi, ir.height() - mbi + 4, mbi - 5, mbi - 5);
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &newBtn, p, widget);
            }
        }
        break;
#endif // QT_NO_TOOLBUTTON

    case CC_TitleBar:
        {
            if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(option))
            {
                bool isActive = tb->titleBarState & QStyle::State_Active;
                XPThemeData theme(widget, p, QWindowsXPStylePrivate::WindowTheme);
                if (sub & SC_TitleBarLabel) {

                        partId = (tb->titleBarState & Qt::WindowMinimized) ? WP_MINCAPTION : WP_CAPTION;
                    theme.rect = option->rect;
                    if (widget && !widget->isEnabled())
                        stateId = CS_DISABLED;
                    else if (isActive)
                        stateId = CS_ACTIVE;
                    else
                        stateId = CS_INACTIVE;

                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);

                    QRect ir = proxy()->subControlRect(CC_TitleBar, tb, SC_TitleBarLabel, widget);

                    int result = TST_NONE;
                    pGetThemeEnumValue(theme.handle(), WP_CAPTION, isActive ? CS_ACTIVE : CS_INACTIVE, TMT_TEXTSHADOWTYPE, &result);
                    if (result != TST_NONE) {
                        COLORREF textShadowRef;
                        pGetThemeColor(theme.handle(), WP_CAPTION, isActive ? CS_ACTIVE : CS_INACTIVE, TMT_TEXTSHADOWCOLOR, &textShadowRef);
                        QColor textShadow = qRgb(GetRValue(textShadowRef), GetGValue(textShadowRef), GetBValue(textShadowRef));
                        p->setPen(textShadow);
                        p->drawText(ir.x() + 3, ir.y() + 2, ir.width() - 1, ir.height(),
                                    Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, tb->text);
                    }
                    COLORREF captionText = GetSysColor(isActive ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT);
                    QColor textColor = qRgb(GetRValue(captionText), GetGValue(captionText), GetBValue(captionText));
                    p->setPen(textColor);
                    p->drawText(ir.x() + 2, ir.y() + 1, ir.width() - 2, ir.height(),
                                Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, tb->text);
                }
                if (sub & SC_TitleBarSysMenu && tb->titleBarFlags & Qt::WindowSystemMenuHint) {
                    theme.rect = proxy()->subControlRect(CC_TitleBar, option, SC_TitleBarSysMenu, widget);
                    partId = WP_SYSBUTTON;
                    if ((widget && !widget->isEnabled()) || !isActive)
                        stateId = SBS_DISABLED;
                    else if (option->activeSubControls == SC_TitleBarSysMenu && (option->state & State_Sunken))
                        stateId = SBS_PUSHED;
                    else if (option->activeSubControls == SC_TitleBarSysMenu && (option->state & State_MouseOver))
                        stateId = SBS_HOT;
                    else
                        stateId = SBS_NORMAL;
                    if (!tb->icon.isNull()) {
                        tb->icon.paint(p, theme.rect);
                    } else {
                        theme.partId = partId;
                        theme.stateId = stateId;
                        SIZE sz;
                        pGetThemePartSize(theme.handle(), qt_win_display_dc(), theme.partId, theme.stateId, 0, TS_TRUE, &sz);
                        if (sz.cx == 0 || sz.cy == 0) {
                            int iconSize = proxy()->pixelMetric(PM_SmallIconSize, tb, widget);
                            QPixmap pm = proxy()->standardIcon(SP_TitleBarMenuButton, tb, widget).pixmap(iconSize, iconSize);
                            p->save();
                            drawItemPixmap(p, theme.rect, Qt::AlignCenter, pm);
                            p->restore();
                        } else {
                            d->drawBackground(theme);
                        }
                    }
                }

                if (sub & SC_TitleBarMinButton && tb->titleBarFlags & Qt::WindowMinimizeButtonHint
                        && !(tb->titleBarState & Qt::WindowMinimized)) {
                    theme.rect = proxy()->subControlRect(CC_TitleBar, option, SC_TitleBarMinButton, widget);
                    partId = WP_MINBUTTON;
                    if (widget && !widget->isEnabled())
                        stateId = MINBS_DISABLED;
                    else if (option->activeSubControls == SC_TitleBarMinButton && (option->state & State_Sunken))
                        stateId = MINBS_PUSHED;
                    else if (option->activeSubControls == SC_TitleBarMinButton && (option->state & State_MouseOver))
                        stateId = MINBS_HOT;
                    else if (!isActive)
                        stateId = MINBS_INACTIVE;
                    else
                        stateId = MINBS_NORMAL;
                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
                if (sub & SC_TitleBarMaxButton && tb->titleBarFlags & Qt::WindowMaximizeButtonHint
                        && !(tb->titleBarState & Qt::WindowMaximized)) {
                    theme.rect = proxy()->subControlRect(CC_TitleBar, option, SC_TitleBarMaxButton, widget);
                    partId = WP_MAXBUTTON;
                    if (widget && !widget->isEnabled())
                        stateId = MAXBS_DISABLED;
                    else if (option->activeSubControls == SC_TitleBarMaxButton && (option->state & State_Sunken))
                        stateId = MAXBS_PUSHED;
                    else if (option->activeSubControls == SC_TitleBarMaxButton && (option->state & State_MouseOver))
                        stateId = MAXBS_HOT;
                    else if (!isActive)
                        stateId = MAXBS_INACTIVE;
                    else
                        stateId = MAXBS_NORMAL;
                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
                if (sub & SC_TitleBarContextHelpButton
                    && tb->titleBarFlags & Qt::WindowContextHelpButtonHint) {
                    theme.rect = proxy()->subControlRect(CC_TitleBar, option, SC_TitleBarContextHelpButton, widget);
                    partId = WP_HELPBUTTON;
                    if (widget && !widget->isEnabled())
                        stateId = MINBS_DISABLED;
                    else if (option->activeSubControls == SC_TitleBarContextHelpButton && (option->state & State_Sunken))
                        stateId = MINBS_PUSHED;
                    else if (option->activeSubControls == SC_TitleBarContextHelpButton && (option->state & State_MouseOver))
                        stateId = MINBS_HOT;
                    else if (!isActive)
                        stateId = MINBS_INACTIVE;
                    else
                        stateId = MINBS_NORMAL;
                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
                bool drawNormalButton = (sub & SC_TitleBarNormalButton)
                                        && (((tb->titleBarFlags & Qt::WindowMinimizeButtonHint)
                                        && (tb->titleBarState & Qt::WindowMinimized))
                                        || ((tb->titleBarFlags & Qt::WindowMaximizeButtonHint)
                                        && (tb->titleBarState & Qt::WindowMaximized)));
                if (drawNormalButton) {
                    theme.rect = proxy()->subControlRect(CC_TitleBar, option, SC_TitleBarNormalButton, widget);
                    partId = WP_RESTOREBUTTON;
                    if (widget && !widget->isEnabled())
                        stateId = RBS_DISABLED;
                    else if (option->activeSubControls == SC_TitleBarNormalButton && (option->state & State_Sunken))
                        stateId = RBS_PUSHED;
                    else if (option->activeSubControls == SC_TitleBarNormalButton && (option->state & State_MouseOver))
                        stateId = RBS_HOT;
                    else if (!isActive)
                        stateId = RBS_INACTIVE;
                    else
                        stateId = RBS_NORMAL;
                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
                if (sub & SC_TitleBarShadeButton && tb->titleBarFlags & Qt::WindowShadeButtonHint
                        && !(tb->titleBarState & Qt::WindowMinimized)) {
                    theme.rect = proxy()->subControlRect(CC_TitleBar, option, SC_TitleBarShadeButton, widget);
                    partId = WP_MINBUTTON;
                    if (widget && !widget->isEnabled())
                        stateId = MINBS_DISABLED;
                    else if (option->activeSubControls == SC_TitleBarShadeButton && (option->state & State_Sunken))
                        stateId = MINBS_PUSHED;
                    else if (option->activeSubControls == SC_TitleBarShadeButton && (option->state & State_MouseOver))
                        stateId = MINBS_HOT;
                    else if (!isActive)
                        stateId = MINBS_INACTIVE;
                    else
                        stateId = MINBS_NORMAL;
                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
                if (sub & SC_TitleBarUnshadeButton && tb->titleBarFlags & Qt::WindowShadeButtonHint
                        && tb->titleBarState & Qt::WindowMinimized) {
                    theme.rect = proxy()->subControlRect(CC_TitleBar, option, SC_TitleBarUnshadeButton, widget);
                    partId = WP_RESTOREBUTTON;
                    if (widget && !widget->isEnabled())
                        stateId = RBS_DISABLED;
                    else if (option->activeSubControls == SC_TitleBarUnshadeButton && (option->state & State_Sunken))
                        stateId = RBS_PUSHED;
                    else if (option->activeSubControls == SC_TitleBarUnshadeButton && (option->state & State_MouseOver))
                        stateId = RBS_HOT;
                    else if (!isActive)
                        stateId = RBS_INACTIVE;
                    else
                        stateId = RBS_NORMAL;
                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
                if (sub & SC_TitleBarCloseButton && tb->titleBarFlags & Qt::WindowSystemMenuHint) {
                    theme.rect = proxy()->subControlRect(CC_TitleBar, option, SC_TitleBarCloseButton, widget);
                    //partId = titlebar->testWFlags(Qt::WA_WState_Tool) ? WP_SMALLCLOSEBUTTON : WP_CLOSEBUTTON;
                    partId = WP_CLOSEBUTTON;
                    if (widget && !widget->isEnabled())
                        stateId = CBS_DISABLED;
                    else if (option->activeSubControls == SC_TitleBarCloseButton && (option->state & State_Sunken))
                        stateId = CBS_PUSHED;
                    else if (option->activeSubControls == SC_TitleBarCloseButton && (option->state & State_MouseOver))
                        stateId = CBS_HOT;
                    else if (!isActive)
                        stateId = CBS_INACTIVE;
                    else
                        stateId = CBS_NORMAL;
                    theme.partId = partId;
                    theme.stateId = stateId;
                    d->drawBackground(theme);
                }
            }
        }
        break;

#ifndef QT_NO_MDIAREA
    case CC_MdiControls:
        {
            QRect buttonRect;
            XPThemeData theme(widget, p, QWindowsXPStylePrivate::WindowTheme, WP_MDICLOSEBUTTON, CBS_NORMAL);

            if (option->subControls & SC_MdiCloseButton) {
                buttonRect = proxy()->subControlRect(CC_MdiControls, option, SC_MdiCloseButton, widget);
                if (theme.isValid()) {
                    theme.partId = WP_MDICLOSEBUTTON;
                    theme.rect = buttonRect;
                    if (!(flags & State_Enabled))
                        theme.stateId = CBS_INACTIVE;
                    else if (flags & State_Sunken && (option->activeSubControls & SC_MdiCloseButton))
                        theme.stateId = CBS_PUSHED;
                    else if (flags & State_MouseOver && (option->activeSubControls & SC_MdiCloseButton))
                        theme.stateId = CBS_HOT;
                    else
                        theme.stateId = CBS_NORMAL;
                    d->drawBackground(theme);
                }
            }
            if (option->subControls & SC_MdiNormalButton) {
                buttonRect = proxy()->subControlRect(CC_MdiControls, option, SC_MdiNormalButton, widget);
                if (theme.isValid()) {
                    theme.partId = WP_MDIRESTOREBUTTON;
                    theme.rect = buttonRect;
                    if (!(flags & State_Enabled))
                        theme.stateId = CBS_INACTIVE;
                    else if (flags & State_Sunken && (option->activeSubControls & SC_MdiNormalButton))
                        theme.stateId = CBS_PUSHED;
                    else if (flags & State_MouseOver && (option->activeSubControls & SC_MdiNormalButton))
                        theme.stateId = CBS_HOT;
                    else
                        theme.stateId = CBS_NORMAL;
                    d->drawBackground(theme);
                }
            }
            if (option->subControls & QStyle::SC_MdiMinButton) {
                buttonRect = proxy()->subControlRect(CC_MdiControls, option, SC_MdiMinButton, widget);
                if (theme.isValid()) {
                    theme.partId = WP_MDIMINBUTTON;
                    theme.rect = buttonRect;
                    if (!(flags & State_Enabled))
                        theme.stateId = CBS_INACTIVE;
                    else if (flags & State_Sunken && (option->activeSubControls & SC_MdiMinButton))
                        theme.stateId = CBS_PUSHED;
                    else if (flags & State_MouseOver && (option->activeSubControls & SC_MdiMinButton))
                        theme.stateId = CBS_HOT;
                    else
                        theme.stateId = CBS_NORMAL;
                    d->drawBackground(theme);
                }
            }
        }
        break;
#endif //QT_NO_MDIAREA
#ifndef QT_NO_DIAL
    case CC_Dial:
        if (const QStyleOptionSlider *dial = qstyleoption_cast<const QStyleOptionSlider *>(option))
            QStyleHelper::drawDial(dial, p);
        break;
#endif // QT_NO_DIAL
    default:
        QWindowsStyle::drawComplexControl(cc, option, p, widget);
        break;
    }
}

/*! \reimp */
int QWindowsXPStyle::pixelMetric(PixelMetric pm, const QStyleOption *option, const QWidget *widget) const
{
    if (!QWindowsXPStylePrivate::useXP())
        return QWindowsStyle::pixelMetric(pm, option, widget);

    int res = 0;
    switch (pm) {
    case PM_MenuBarPanelWidth:
        res = 0;
        break;

    case PM_DefaultFrameWidth:
        if (qobject_cast<const QListView*>(widget))
            res = 2;
        else
            res = 1;
        break;
    case PM_MenuPanelWidth:
    case PM_SpinBoxFrameWidth:
        res = 1;
        break;

    case PM_TabBarTabOverlap:
    case PM_MenuHMargin:
    case PM_MenuVMargin:
        res = 2;
        break;

    case PM_TabBarBaseOverlap:
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            switch (tab->shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                res = 1;
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                res = 2;
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                res = 3;
                break;
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                res = 1;
                break;
            }
        }
        break;

    case PM_SplitterWidth:
        res = qMax(int(QStyleHelper::dpiScaled(5.)), QApplication::globalStrut().width());
        break;

    case PM_IndicatorWidth:
    case PM_IndicatorHeight:
        {
            XPThemeData theme(widget, 0, QWindowsXPStylePrivate::ButtonTheme, BP_CHECKBOX, CBS_UNCHECKEDNORMAL);
            if (theme.isValid()) {
                SIZE size;
                pGetThemePartSize(theme.handle(), 0, theme.partId, theme.stateId, 0, TS_TRUE, &size);
                res = (pm == PM_IndicatorWidth) ? size.cx : size.cy;
            }
        }
        break;

    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight:
        {
            XPThemeData theme(widget, 0, QWindowsXPStylePrivate::ButtonTheme, BP_RADIOBUTTON, RBS_UNCHECKEDNORMAL);
            if (theme.isValid()) {
                SIZE size;
                pGetThemePartSize(theme.handle(), 0, theme.partId, theme.stateId, 0, TS_TRUE, &size);
                res = (pm == PM_ExclusiveIndicatorWidth) ? size.cx : size.cy;
            }
        }
        break;

    case PM_ProgressBarChunkWidth:
        {
            Qt::Orientation orient = Qt::Horizontal;
            if (const QStyleOptionProgressBarV2 *pb2 = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(option))
                orient = pb2->orientation;
            XPThemeData theme(widget, 0, QWindowsXPStylePrivate::ProgressTheme,
                              (orient == Qt::Horizontal) ? PP_CHUNK : PP_CHUNKVERT);
            if (theme.isValid()) {
                SIZE size;
                pGetThemePartSize(theme.handle(), 0, theme.partId, theme.stateId, 0, TS_TRUE, &size);
                res = (orient == Qt::Horizontal) ? size.cx : size.cy;
            }
        }
        break;

    case PM_SliderThickness:
        {
            XPThemeData theme(widget, 0, QWindowsXPStylePrivate::TrackBarTheme,
                              TKP_THUMB);
            if (theme.isValid()) {
                SIZE size;
                pGetThemePartSize(theme.handle(), 0, theme.partId, theme.stateId, 0, TS_TRUE, &size);
                res = size.cy;
            }
        }
        break;

    case PM_TitleBarHeight:
        {
            if (widget && (widget->windowType() == Qt::Tool))
                res = GetSystemMetrics(SM_CYSMCAPTION) + GetSystemMetrics(SM_CXSIZEFRAME);
            else
                res = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CXSIZEFRAME);
        }
        break;

    case PM_MdiSubWindowFrameWidth:
        {
            XPThemeData theme(widget, 0, QWindowsXPStylePrivate::WindowTheme, WP_FRAMELEFT, FS_ACTIVE);
            if (theme.isValid()) {
                SIZE size;
                pGetThemePartSize(theme.handle(), 0, WP_FRAMELEFT, FS_ACTIVE, 0, TS_TRUE, &size);
                res = size.cx-1;
            }
        }
        break;

    case PM_MdiSubWindowMinimizedWidth:
        res = 160;
        break;

#ifndef QT_NO_TOOLBAR
    case PM_ToolBarHandleExtent:
        res = int(QStyleHelper::dpiScaled(8.));
        break;

#endif // QT_NO_TOOLBAR
    case PM_DockWidgetFrameWidth:
    {
        XPThemeData theme(widget, 0, QWindowsXPStylePrivate::WindowTheme, WP_SMALLFRAMERIGHT, FS_ACTIVE);
        if (theme.isValid()) {
            SIZE size;
            pGetThemePartSize(theme.handle(), 0, theme.partId, theme.stateId, 0, TS_TRUE, &size);
            res = size.cx;
        }
    }
    break;
    case PM_DockWidgetSeparatorExtent:
        res = int(QStyleHelper::dpiScaled(4.));
        break;
    case PM_DockWidgetTitleMargin:
        res = int(QStyleHelper::dpiScaled(4.));
        break;

    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        if (qstyleoption_cast<const QStyleOptionToolButton *>(option))
            res = 1;
        else
            res = 0;
        break;

    case PM_ButtonDefaultIndicator:
        res = 0;
        break;

    default:
        res = QWindowsStyle::pixelMetric(pm, option, widget);
    }

    return res;
}

/*
  This function is used by subControlRect to check if a button
  should be drawn for the given subControl given a set of window flags.
*/
static bool buttonVisible(const QStyle::SubControl sc, const QStyleOptionTitleBar *tb){

    bool isMinimized = tb->titleBarState & Qt::WindowMinimized;
    bool isMaximized = tb->titleBarState & Qt::WindowMaximized;
    const uint flags = tb->titleBarFlags;
    bool retVal = false;
    switch (sc) {
    case QStyle::SC_TitleBarContextHelpButton:
        if (flags & Qt::WindowContextHelpButtonHint)
            retVal = true;
        break;
    case QStyle::SC_TitleBarMinButton:
        if (!isMinimized && (flags & Qt::WindowMinimizeButtonHint))
            retVal = true;
        break;
    case QStyle::SC_TitleBarNormalButton:
        if (isMinimized && (flags & Qt::WindowMinimizeButtonHint))
            retVal = true;
        else if (isMaximized && (flags & Qt::WindowMaximizeButtonHint))
            retVal = true;
        break;
    case QStyle::SC_TitleBarMaxButton:
        if (!isMaximized && (flags & Qt::WindowMaximizeButtonHint))
            retVal = true;
        break;
    case QStyle::SC_TitleBarShadeButton:
        if (!isMinimized &&  flags & Qt::WindowShadeButtonHint)
            retVal = true;
        break;
    case QStyle::SC_TitleBarUnshadeButton:
        if (isMinimized && flags & Qt::WindowShadeButtonHint)
            retVal = true;
        break;
    case QStyle::SC_TitleBarCloseButton:
        if (flags & Qt::WindowSystemMenuHint)
            retVal = true;
        break;
    case QStyle::SC_TitleBarSysMenu:
        if (flags & Qt::WindowSystemMenuHint)
            retVal = true;
        break;
    default :
        retVal = true;
    }
    return retVal;
}

/*!
    \reimp
*/
QRect QWindowsXPStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *option,
                                      SubControl subControl, const QWidget *widget) const
{
    if (!QWindowsXPStylePrivate::useXP())
        return QWindowsStyle::subControlRect(cc, option, subControl, widget);

    QRect rect;

    switch (cc) {
    case CC_TitleBar:
        if (const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(option)) {
            if (!buttonVisible(subControl, tb))
                return rect;
            const bool isToolTitle = false;
            const int height = tb->rect.height();
            const int width = tb->rect.width();
            int buttonHeight = GetSystemMetrics(SM_CYSIZE) - 4;
            int buttonWidth = GetSystemMetrics(SM_CXSIZE) - 4;
            const int delta = buttonWidth + 2;
            int controlTop = option->rect.bottom() - buttonHeight - 2;
            const int frameWidth = proxy()->pixelMetric(PM_MdiSubWindowFrameWidth, option, widget);
            const bool sysmenuHint  = (tb->titleBarFlags & Qt::WindowSystemMenuHint) != 0;
            const bool minimizeHint = (tb->titleBarFlags & Qt::WindowMinimizeButtonHint) != 0;
            const bool maximizeHint = (tb->titleBarFlags & Qt::WindowMaximizeButtonHint) != 0;
            const bool contextHint = (tb->titleBarFlags & Qt::WindowContextHelpButtonHint) != 0;
            const bool shadeHint = (tb->titleBarFlags & Qt::WindowShadeButtonHint) != 0;
            bool isMinimized = tb->titleBarState & Qt::WindowMinimized;
            bool isMaximized = tb->titleBarState & Qt::WindowMaximized;
            int offset = 0;

            switch (subControl) {
            case SC_TitleBarLabel:
                rect = QRect(frameWidth, 0, width - (buttonWidth + frameWidth + 10), height);
                if (isToolTitle) {
                    if (sysmenuHint) {
                        rect.adjust(0, 0, -buttonWidth - 3, 0);
                    }
                    if (minimizeHint || maximizeHint)
                        rect.adjust(0, 0, -buttonWidth - 2, 0);
                } else {
                    if (sysmenuHint) {
                        const int leftOffset = height - 8;
                        rect.adjust(leftOffset, 0, 0, 0);
                    }
                    if (minimizeHint)
                        rect.adjust(0, 0, -buttonWidth - 2, 0);
                    if (maximizeHint)
                        rect.adjust(0, 0, -buttonWidth - 2, 0);
                    if (contextHint)
                        rect.adjust(0, 0, -buttonWidth - 2, 0);
                    if (shadeHint)
                        rect.adjust(0, 0, -buttonWidth - 2, 0);
                }
                break;

            case SC_TitleBarContextHelpButton:
                if (tb->titleBarFlags & Qt::WindowContextHelpButtonHint)
                    offset += delta;
                //fall through
            case SC_TitleBarMinButton:
                if (!isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += delta;
                else if (subControl == SC_TitleBarMinButton)
                    break;
                //fall through
            case SC_TitleBarNormalButton:
                if (isMinimized && (tb->titleBarFlags & Qt::WindowMinimizeButtonHint))
                    offset += delta;
                else if (isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += delta;
                else if (subControl == SC_TitleBarNormalButton)
                    break;
                //fall through
            case SC_TitleBarMaxButton:
                if (!isMaximized && (tb->titleBarFlags & Qt::WindowMaximizeButtonHint))
                    offset += delta;
                else if (subControl == SC_TitleBarMaxButton)
                    break;
                //fall through
            case SC_TitleBarShadeButton:
                if (!isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += delta;
                else if (subControl == SC_TitleBarShadeButton)
                    break;
                //fall through
            case SC_TitleBarUnshadeButton:
                if (isMinimized && (tb->titleBarFlags & Qt::WindowShadeButtonHint))
                    offset += delta;
                else if (subControl == SC_TitleBarUnshadeButton)
                    break;
                //fall through
            case SC_TitleBarCloseButton:
                if (tb->titleBarFlags & Qt::WindowSystemMenuHint)
                    offset += delta;
                else if (subControl == SC_TitleBarCloseButton)
                    break;

                rect.setRect(width - offset - controlTop + 1, controlTop,
                             buttonWidth, buttonHeight);
                break;

            case SC_TitleBarSysMenu:
                {
                    const int controlTop = 6;
                    const int controlHeight = height - controlTop - 3;
                    const int iconExtent = proxy()->pixelMetric(PM_SmallIconSize);
                    QSize iconSize = tb->icon.actualSize(QSize(iconExtent, iconExtent));
                    if (tb->icon.isNull())
                        iconSize = QSize(controlHeight, controlHeight);
                    int hPad = (controlHeight - iconSize.height())/2;
                    int vPad = (controlHeight - iconSize.width())/2;
                    rect = QRect(frameWidth + hPad, controlTop + vPad, iconSize.width(), iconSize.height());
                }
                break;
            default:
                break;
            }
        }
        break;

    case CC_ComboBox:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            int x = cmb->rect.x(), y = cmb->rect.y(), wi = cmb->rect.width(), he = cmb->rect.height();
            int xpos = x;
            xpos += wi - 1 - 16;

            switch (subControl) {
            case SC_ComboBoxFrame:
                rect = cmb->rect;
                break;

            case SC_ComboBoxArrow:
                rect = QRect(xpos, y+1, 16, he-2);
                break;

            case SC_ComboBoxEditField:
                rect = QRect(x+2, y+2, wi-3-16, he-4);
                break;

            case SC_ComboBoxListBoxPopup:
                rect = cmb->rect;
                break;

            default:
                break;
            }
        }
        break;
#ifndef QT_NO_MDIAREA
    case CC_MdiControls:
    {
        int numSubControls = 0;
        if (option->subControls & SC_MdiCloseButton)
            ++numSubControls;
        if (option->subControls & SC_MdiMinButton)
            ++numSubControls;
        if (option->subControls & SC_MdiNormalButton)
            ++numSubControls;
        if (numSubControls == 0)
            break;

        int buttonWidth = option->rect.width() / numSubControls;
        int offset = 0;
        switch (subControl) {
        case SC_MdiCloseButton:
            // Only one sub control, no offset needed.
            if (numSubControls == 1)
                break;
            offset += buttonWidth;
            //FALL THROUGH
        case SC_MdiNormalButton:
            // No offset needed if
            // 1) There's only one sub control
            // 2) We have a close button and a normal button (offset already added in SC_MdiClose)
            if (numSubControls == 1 || (numSubControls == 2 && !(option->subControls & SC_MdiMinButton)))
                break;
            if (option->subControls & SC_MdiNormalButton)
                offset += buttonWidth;
            break;
        default:
            break;
        }
        rect = QRect(offset, 0, buttonWidth, option->rect.height());
        break;
    }
#endif // QT_NO_MDIAREA

    default:
        rect = visualRect(option->direction, option->rect,
                          QWindowsStyle::subControlRect(cc, option, subControl, widget));
        break;
    }
    return visualRect(option->direction, option->rect, rect);
}

/*!
    \reimp
*/
QSize QWindowsXPStyle::sizeFromContents(ContentsType ct, const QStyleOption *option,
                                        const QSize &contentsSize, const QWidget *widget) const
{
    if (!QWindowsXPStylePrivate::useXP())
        return QWindowsStyle::sizeFromContents(ct, option, contentsSize, widget);

    QSize sz(contentsSize);
    switch (ct) {
    case CT_LineEdit:
    case CT_ComboBox:
        {
            XPThemeData buttontheme(widget, 0, QWindowsXPStylePrivate::ButtonTheme);
            HTHEME theme = buttontheme.handle();
            MARGINS borderSize;
            if (theme) {
                int result = pGetThemeMargins(theme,
                                              NULL,
                                              BP_PUSHBUTTON,
                                              PBS_NORMAL,
                                              TMT_CONTENTMARGINS,
                                              NULL,
                                              &borderSize);
                if (result == S_OK) {
                    sz += QSize(borderSize.cxLeftWidth + borderSize.cxRightWidth - 2,
                                borderSize.cyBottomHeight + borderSize.cyTopHeight - 2);
                }
                const int textMargins = 2*(proxy()->pixelMetric(PM_FocusFrameHMargin) + 1);
                sz += QSize(qMax(pixelMetric(QStyle::PM_ScrollBarExtent, option, widget)
                                 + textMargins, 23), 0); //arrow button
            }
        }
        break;
    case CT_SpinBox:
        {
            //Spinbox adds frame twice
            sz = QWindowsStyle::sizeFromContents(ct, option, contentsSize, widget);
            int border = proxy()->pixelMetric(PM_SpinBoxFrameWidth, option, widget);
            sz -= QSize(2*border, 2*border);
        }
        break;
    case CT_TabWidget:
        sz += QSize(6, 6);
        break;
    case CT_Menu:
        sz += QSize(1, 0);
        break;
#ifndef QT_NO_MENUBAR
    case CT_MenuBarItem:
        if (!sz.isEmpty())
            sz += QSize(windowsItemHMargin * 5 + 1, 6);
        break;
#endif
    case CT_MenuItem:
        if (const QStyleOptionMenuItem *menuitem = qstyleoption_cast<const QStyleOptionMenuItem *>(option))
        {
            if (menuitem->menuItemType != QStyleOptionMenuItem::Separator) {
                sz = QWindowsStyle::sizeFromContents(ct, option, sz, widget);
                sz.setHeight(sz.height() - 2);
                return sz;
            }
        }
        sz = QWindowsStyle::sizeFromContents(ct, option, sz, widget);
        break;

    case CT_MdiControls:
        if (const QStyleOptionComplex *styleOpt = qstyleoption_cast<const QStyleOptionComplex *>(option)) {
            int width = 0;
            if (styleOpt->subControls & SC_MdiMinButton)
                width += 17 + 1;
            if (styleOpt->subControls & SC_MdiNormalButton)
                width += 17 + 1;
            if (styleOpt->subControls & SC_MdiCloseButton)
                width += 17 + 1;
            sz = QSize(width, 19);
        } else {
            sz = QSize(54, 19);
        }
        break;

    default:
        sz = QWindowsStyle::sizeFromContents(ct, option, sz, widget);
        break;
    }

    return sz;
}


/*! \reimp */
int QWindowsXPStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                               QStyleHintReturn *returnData) const
{
  QWindowsXPStylePrivate *d = const_cast<QWindowsXPStylePrivate*>(d_func());
    if (!QWindowsXPStylePrivate::useXP())
        return QWindowsStyle::styleHint(hint, option, widget, returnData);

    int res = 0;
    switch (hint) {

    case SH_EtchDisabledText:
        res = (qobject_cast<const QLabel*>(widget) != 0);
        break;

    case SH_SpinControls_DisableOnBounds:
        res = 0;
        break;

    case SH_TitleBar_AutoRaise:
    case SH_TitleBar_NoBorder:
        res = 1;
        break;

    case SH_GroupBox_TextLabelColor:
        if (!widget || (widget && widget->isEnabled()))
            res = d->groupBoxTextColor;
        else
            res = d->groupBoxTextColorDisabled;
        break;

    case SH_Table_GridLineColor:
        res = 0xC0C0C0;
        break;

    case SH_WindowFrame_Mask:
        {
            res = 1;
            QStyleHintReturnMask *mask = qstyleoption_cast<QStyleHintReturnMask *>(returnData);
            const QStyleOptionTitleBar *titlebar = qstyleoption_cast<const QStyleOptionTitleBar *>(option);
            if (mask && titlebar) {
                // Note certain themes will not return the whole window frame but only the titlebar part when
                // queried This function needs to return the entire window mask, hence we will only fetch the mask for the
                // titlebar itself and add the remaining part of the window rect at the bottom.
                int tbHeight = proxy()->pixelMetric(PM_TitleBarHeight, option, widget);
                QRect titleBarRect = option->rect;
                titleBarRect.setHeight(tbHeight);
                XPThemeData themeData;
                if (titlebar->titleBarState & Qt::WindowMinimized) {
                    themeData = XPThemeData(widget, 0,
                                            QWindowsXPStylePrivate::WindowTheme,
                                            WP_MINCAPTION, CS_ACTIVE, titleBarRect);
                } else
                    themeData = XPThemeData(widget, 0,
                                            QWindowsXPStylePrivate::WindowTheme,
                                            WP_CAPTION, CS_ACTIVE, titleBarRect);
                mask->region = d->region(themeData) +
                               QRect(0, tbHeight, option->rect.width(), option->rect.height() - tbHeight);
            }
        }
        break;
#ifndef QT_NO_RUBBERBAND
    case SH_RubberBand_Mask:
        if (qstyleoption_cast<const QStyleOptionRubberBand *>(option)) {
            res = 0;
            break;
        }
#endif // QT_NO_RUBBERBAND

    case SH_ItemView_DrawDelegateFrame:
        res = 1;
        break;

    default:
        res =QWindowsStyle::styleHint(hint, option, widget, returnData);
    }

    return res;
}

/*! \reimp */
QPalette QWindowsXPStyle::standardPalette() const
{
    if (QWindowsXPStylePrivate::useXP() && QApplicationPrivate::sys_pal)
        return *QApplicationPrivate::sys_pal;
    else
        return QWindowsStyle::standardPalette();
}

/*!
    \reimp
*/
QPixmap QWindowsXPStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *option,
                                        const QWidget *widget) const
{
    if (!QWindowsXPStylePrivate::useXP())
        return QWindowsStyle::standardPixmap(standardPixmap, option, widget);

    switch(standardPixmap) {
    case SP_TitleBarMaxButton:
    case SP_TitleBarCloseButton:
        if (qstyleoption_cast<const QStyleOptionDockWidget *>(option))
        {
            if (widget && widget->isWindow()) {
                XPThemeData theme(widget, 0, QWindowsXPStylePrivate::WindowTheme, WP_SMALLCLOSEBUTTON, CBS_NORMAL);
                if (theme.isValid()) {
                    SIZE sz;
                    pGetThemePartSize(theme.handle(), 0, theme.partId, theme.stateId, 0, TS_TRUE, &sz);
                    return QIcon(QWindowsStyle::standardPixmap(standardPixmap, option, widget)).pixmap(QSize(sz.cx, sz.cy));
                }
            }
        }
        break;
    default:
        break;
    }
    return QWindowsStyle::standardPixmap(standardPixmap, option, widget);
}

/*!
    \reimp
*/
QIcon QWindowsXPStyle::standardIcon(StandardPixmap standardIcon,
                                    const QStyleOption *option,
                                    const QWidget *widget) const
{
    if (!QWindowsXPStylePrivate::useXP()) {
        return QWindowsStyle::standardIcon(standardIcon, option, widget);
    }

    QWindowsXPStylePrivate *d = const_cast<QWindowsXPStylePrivate*>(d_func());
    switch(standardIcon) {
    case SP_TitleBarMaxButton:
        if (qstyleoption_cast<const QStyleOptionDockWidget *>(option))
        {
            if (d->dockFloat.isNull()) {
                XPThemeData themeSize(0, 0, QWindowsXPStylePrivate::WindowTheme,
                                      WP_SMALLCLOSEBUTTON, CBS_NORMAL);
                XPThemeData theme(0, 0, QWindowsXPStylePrivate::WindowTheme,
                                  WP_MAXBUTTON, MAXBS_NORMAL);
                if (theme.isValid()) {
                    SIZE size;
                    pGetThemePartSize(themeSize.handle(), 0, themeSize.partId, themeSize.stateId, 0, TS_TRUE, &size);
                    QPixmap pm = QPixmap(size.cx, size.cy);
                    pm.fill(Qt::transparent);
                    QPainter p(&pm);
                    theme.painter = &p;
                    theme.rect = QRect(0, 0, size.cx, size.cy);
                    d->drawBackground(theme);
                    d->dockFloat.addPixmap(pm, QIcon::Normal, QIcon::Off);    // Normal
                    pm.fill(Qt::transparent);
                    theme.stateId = MAXBS_PUSHED;
                    d->drawBackground(theme);
                    d->dockFloat.addPixmap(pm, QIcon::Normal, QIcon::On);     // Pressed
                    pm.fill(Qt::transparent);
                    theme.stateId = MAXBS_HOT;
                    d->drawBackground(theme);
                    d->dockFloat.addPixmap(pm, QIcon::Active, QIcon::Off);    // Hover
                    pm.fill(Qt::transparent);
                    theme.stateId = MAXBS_INACTIVE;
                    d->drawBackground(theme);
                    d->dockFloat.addPixmap(pm, QIcon::Disabled, QIcon::Off);  // Disabled
                }
            }
            if (widget && widget->isWindow())
                return d->dockFloat;

        }
        break;
    case SP_TitleBarCloseButton:
        if (qstyleoption_cast<const QStyleOptionDockWidget *>(option))
        {
            if (d->dockClose.isNull()) {
                XPThemeData theme(0, 0, QWindowsXPStylePrivate::WindowTheme,
                                  WP_SMALLCLOSEBUTTON, CBS_NORMAL);
                if (theme.isValid()) {
                    SIZE size;
                    pGetThemePartSize(theme.handle(), 0, theme.partId, theme.stateId, 0, TS_TRUE, &size);
                    QPixmap pm = QPixmap(size.cx, size.cy);
                    pm.fill(Qt::transparent);
                    QPainter p(&pm);
                    theme.painter = &p;
                    theme.partId = WP_CLOSEBUTTON; // ####
                    theme.rect = QRect(0, 0, size.cx, size.cy);
                    d->drawBackground(theme);
                    d->dockClose.addPixmap(pm, QIcon::Normal, QIcon::Off);    // Normal
                    pm.fill(Qt::transparent);
                    theme.stateId = CBS_PUSHED;
                    d->drawBackground(theme);
                    d->dockClose.addPixmap(pm, QIcon::Normal, QIcon::On);     // Pressed
                    pm.fill(Qt::transparent);
                    theme.stateId = CBS_HOT;
                    d->drawBackground(theme);
                    d->dockClose.addPixmap(pm, QIcon::Active, QIcon::Off);    // Hover
                    pm.fill(Qt::transparent);
                    theme.stateId = CBS_INACTIVE;
                    d->drawBackground(theme);
                    d->dockClose.addPixmap(pm, QIcon::Disabled, QIcon::Off);  // Disabled
                }
            }
            if (widget && widget->isWindow())
                return d->dockClose;
        }
        break;
    case SP_TitleBarNormalButton:
        if (qstyleoption_cast<const QStyleOptionDockWidget *>(option))
        {
            if (d->dockFloat.isNull()) {
                XPThemeData themeSize(0, 0, QWindowsXPStylePrivate::WindowTheme,
                                      WP_SMALLCLOSEBUTTON, CBS_NORMAL);
                XPThemeData theme(0, 0, QWindowsXPStylePrivate::WindowTheme,
                                  WP_RESTOREBUTTON, RBS_NORMAL);
                if (theme.isValid()) {
                    SIZE size;
                    pGetThemePartSize(themeSize.handle(), 0, themeSize.partId, themeSize.stateId, 0, TS_TRUE, &size);
                    QPixmap pm = QPixmap(size.cx, size.cy);
                    pm.fill(Qt::transparent);
                    QPainter p(&pm);
                    theme.painter = &p;
                    theme.rect = QRect(0, 0, size.cx, size.cy);
                    d->drawBackground(theme);
                    d->dockFloat.addPixmap(pm, QIcon::Normal, QIcon::Off);    // Normal
                    pm.fill(Qt::transparent);
                    theme.stateId = RBS_PUSHED;
                    d->drawBackground(theme);
                    d->dockFloat.addPixmap(pm, QIcon::Normal, QIcon::On);     // Pressed
                    pm.fill(Qt::transparent);
                    theme.stateId = RBS_HOT;
                    d->drawBackground(theme);
                    d->dockFloat.addPixmap(pm, QIcon::Active, QIcon::Off);    // Hover
                    pm.fill(Qt::transparent);
                    theme.stateId = RBS_INACTIVE;
                    d->drawBackground(theme);
                    d->dockFloat.addPixmap(pm, QIcon::Disabled, QIcon::Off);  // Disabled
                }
            }
            if (widget && widget->isWindow())
                return d->dockFloat;

        }
        break;
    default:
        break;
    }

    return QWindowsStyle::standardIcon(standardIcon, option, widget);
}

/*!
    \internal

    Constructs a QWindowsXPStyle object.
*/
QWindowsXPStyle::QWindowsXPStyle(QWindowsXPStylePrivate &dd) : QWindowsStyle(dd)
{
}


// Debugging code ---------------------------------------------------------------------[ START ]---
// The code for this point on is not compiled by default, but only used as assisting
// debugging code when you uncomment the DEBUG_XP_STYLE define at the top of the file.

#ifdef DEBUG_XP_STYLE
// The schema file expects these to be defined by the user.
#define TMT_ENUMDEF 8
#define TMT_ENUMVAL TEXT('A')
#define TMT_ENUM    TEXT('B')
#define SCHEMA_STRINGS // For 2nd pass on schema file
QT_BEGIN_INCLUDE_NAMESPACE
#include <tmschema.h>
QT_END_INCLUDE_NAMESPACE

// A property's value, type and name combo
struct PropPair {
    int propValue;
    int propType;
    LPCWSTR propName;
};

// Operator for sorting of PropPairs
bool operator<(PropPair a, PropPair b) {
    return wcscmp(a.propName, b.propName) < 0;
}

// Our list of all possible properties
static QList<PropPair> all_props;


/*! \internal
    Dumps a portion of the full native DIB section double buffer.
    The DIB section double buffer is only used when doing special
    transformations to the theme part, or when the real double
    buffer in the paintengine does not have an HDC we may use
    directly.
    Since we cannot rely on the pixel data we get from Microsoft
    when drawing into the DIB section, we use this function to
    see the actual data we got, and can determin the appropriate
    action.
*/
void QWindowsXPStylePrivate::dumpNativeDIB(int w, int h)
{
    if (w && h) {
        static int pCount = 0;
        DWORD *bufPix = (DWORD*)bufferPixels;

        char *bufferDump = new char[bufferH * bufferW * 16];
        char *bufferPos = bufferDump;

        memset(bufferDump, 0, sizeof(bufferDump));
        bufferPos += sprintf(bufferPos, "const int pixelBufferW%d = %d;\n", pCount, w);
        bufferPos += sprintf(bufferPos, "const int pixelBufferH%d = %d;\n", pCount, h);
        bufferPos += sprintf(bufferPos, "const unsigned DWORD pixelBuffer%d[] = {", pCount);
        for (int iy = 0; iy < h; ++iy) {
            bufferPos += sprintf(bufferPos, "\n    ");
            bufPix = (DWORD*)(bufferPixels + (iy * bufferW * 4));
            for (int ix = 0; ix < w; ++ix) {
                bufferPos += sprintf(bufferPos, "0x%08x, ", *bufPix);
                ++bufPix;
            }
        }
        bufferPos += sprintf(bufferPos, "\n};\n\n");
        printf(bufferDump);

        delete[] bufferDump;
        ++pCount;
    }
}

/*! \internal
    Shows the value of a given property for a part.
*/
static void showProperty(XPThemeData &themeData, const PropPair &prop)
{
    PROPERTYORIGIN origin = PO_NOTFOUND;
    pGetThemePropertyOrigin(themeData.handle(), themeData.partId, themeData.stateId, prop.propValue, &origin);
    const char *originStr;
    switch(origin) {
    case PO_STATE:
        originStr = "State ";
        break;
    case PO_PART:
        originStr = "Part  ";
        break;
    case PO_CLASS:
        originStr = "Class ";
        break;
    case PO_GLOBAL:
        originStr = "Globl ";
        break;
    case PO_NOTFOUND:
    default:
        originStr = "Unkwn ";
        break;
    }

    switch(prop.propType) {
    case TMT_STRING:
        {
            wchar_t buffer[512];
            pGetThemeString(themeData.handle(), themeData.partId, themeData.stateId, prop.propValue, buffer, 512);
            printf("  (%sString)  %-20S: %S\n", originStr, prop.propName, buffer);
        }
        break;
    case TMT_ENUM:
        {
            int result = -1;
            pGetThemeEnumValue(themeData.handle(), themeData.partId, themeData.stateId, prop.propValue, &result);
            printf("  (%sEnum)    %-20S: %d\n", originStr, prop.propName, result);
        }
        break;
    case TMT_INT:
        {
            int result = -1;
            pGetThemeInt(themeData.handle(), themeData.partId, themeData.stateId, prop.propValue, &result);
            printf("  (%sint)     %-20S: %d\n", originStr, prop.propName, result);
        }
        break;
    case TMT_BOOL:
        {
            BOOL result = false;
            pGetThemeBool(themeData.handle(), themeData.partId, themeData.stateId, prop.propValue, &result);
            printf("  (%sbool)    %-20S: %d\n", originStr, prop.propName, result);
        }
        break;
    case TMT_COLOR:
        {
            COLORREF result = 0;
            pGetThemeColor(themeData.handle(), themeData.partId, themeData.stateId, prop.propValue, &result);
            printf("  (%scolor)   %-20S: 0x%08X\n", originStr, prop.propName, result);
        }
        break;
    case TMT_MARGINS:
        {
            MARGINS result;
            memset(&result, 0, sizeof(result));
            pGetThemeMargins(themeData.handle(), 0, themeData.partId, themeData.stateId, prop.propValue, 0, &result);
            printf("  (%smargins) %-20S: (%d, %d, %d, %d)\n", originStr,
                   prop.propName, result.cxLeftWidth, result.cyTopHeight, result.cxRightWidth, result.cyBottomHeight);
        }
        break;
    case TMT_FILENAME:
        {
            wchar_t buffer[512];
            pGetThemeFilename(themeData.handle(), themeData.partId, themeData.stateId, prop.propValue, buffer, 512);
            printf("  (%sfilename)%-20S: %S\n", originStr, prop.propName, buffer);
        }
        break;
    case TMT_SIZE:
        {
            SIZE result1;
            SIZE result2;
            SIZE result3;
            memset(&result1, 0, sizeof(result1));
            memset(&result2, 0, sizeof(result2));
            memset(&result3, 0, sizeof(result3));
            pGetThemePartSize(themeData.handle(), 0, themeData.partId, themeData.stateId, 0, TS_MIN,  &result1);
            pGetThemePartSize(themeData.handle(), 0, themeData.partId, themeData.stateId, 0, TS_TRUE, &result2);
            pGetThemePartSize(themeData.handle(), 0, themeData.partId, themeData.stateId, 0, TS_DRAW, &result3);
            printf("  (%ssize)    %-20S: Min (%d, %d),  True(%d, %d),  Draw(%d, %d)\n", originStr, prop.propName,
                   result1.cx, result1.cy, result2.cx, result2.cy, result3.cx, result3.cy);
        }
        break;
    case TMT_POSITION:
        {
            POINT result;
            memset(&result, 0, sizeof(result));
            pGetThemePosition(themeData.handle(), themeData.partId, themeData.stateId, prop.propValue, &result);
            printf("  (%sPosition)%-20S: (%d, %d)\n", originStr, prop.propName, result.x, result.y);
        }
        break;
    case TMT_RECT:
        {
            RECT result;
            memset(&result, 0, sizeof(result));
            pGetThemeRect(themeData.handle(), themeData.partId, themeData.stateId, prop.propValue, &result);
            printf("  (%sRect)    %-20S: (%d, %d, %d, %d)\n", originStr, prop.propName, result.left, result.top, result.right, result.bottom);
        }
        break;
    case TMT_FONT:
        {
            LOGFONT result;
            memset(&result, 0, sizeof(result));
            pGetThemeFont(themeData.handle(), 0, themeData.partId, themeData.stateId, prop.propValue, &result);
            printf("  (%sFont)    %-20S: %S  height(%d)  width(%d)  weight(%d)\n", originStr, prop.propName,
                   result.lfFaceName, result.lfHeight, result.lfWidth, result.lfWeight);
        }
        break;
    case TMT_INTLIST:
        {
            INTLIST result;
            memset(&result, 0, sizeof(result));
            pGetThemeIntList(themeData.handle(), themeData.partId, themeData.stateId, prop.propValue, &result);
            printf("  (%sInt list)%-20S: { ", originStr, prop.propName);
            for (int i = 0; i < result.iValueCount; ++i)
                printf("%d ", result.iValues[i]);
            printf("}\n");
        }
        break;
    default:
        printf("    %s%S : Unknown property type (%d)!\n", originStr, prop.propName, prop.propType);
    }
}

/*! \internal
    Dump all valid properties for a part.
    If it's the first time this function is called, then the name,
    enum value and documentation of all properties are shown, as
    well as all global properties.
*/
void QWindowsXPStylePrivate::showProperties(XPThemeData &themeData)
{
    if (!all_props.count()) {
        const TMSCHEMAINFO *infoTable = GetSchemaInfo();
        for (int i = 0; i < infoTable->iPropCount; ++i) {
            int propType  = infoTable->pPropTable[i].bPrimVal;
            int propValue = infoTable->pPropTable[i].sEnumVal;
            LPCWSTR propName = infoTable->pPropTable[i].pszName;

            switch(propType) {
            case TMT_ENUMDEF:
            case TMT_ENUMVAL:
                continue;
            default:
                if (propType != propValue) {
                    PropPair prop;
                    prop.propValue = propValue;
                    prop.propName  = propName;
                    prop.propType  = propType;
                    all_props.append(prop);
                }
            }
        }
        std::sort(all_props.begin(), all_props.end());

        {// List all properties
            printf("part properties count = %d:\n", all_props.count());
            printf("      Enum  Property Name        Description\n");
            printf("-----------------------------------------------------------\n");
            wchar_t themeName[256];
            pGetCurrentThemeName(themeName, 256, 0, 0, 0, 0);
            for (int j = 0; j < all_props.count(); ++j) {
                PropPair prop = all_props.at(j);
                wchar_t buf[500];
                pGetThemeDocumentationProperty(themeName, prop.propName, buf, 500);
                printf("%3d: (%4d) %-20S %S\n", j, prop.propValue, prop.propName, buf);
            }
        }

        {// Show Global values
            printf("Global Properties:\n");
            for (int j = 0; j < all_props.count(); ++j) {
                PropPair prop = all_props.at(j);
                PROPERTYORIGIN origin = PO_NOTFOUND;
                pGetThemePropertyOrigin(themeData.handle(), themeData.partId, themeData.stateId, prop.propValue, &origin);
                if (origin == PO_GLOBAL) {
                    showProperty(themeData, prop);
                }
            }
        }
    }

    for (int j = 0; j < all_props.count(); ++j) {
        PropPair prop = all_props.at(j);
        PROPERTYORIGIN origin = PO_NOTFOUND;
        pGetThemePropertyOrigin(themeData.handle(), themeData.partId, themeData.stateId, prop.propValue, &origin);
        if (origin != PO_NOTFOUND)
        {
            showProperty(themeData, prop);
        }
    }
}
#endif
// Debugging code -----------------------------------------------------------------------[ END ]---


QT_END_NAMESPACE

#endif //QT_NO_WINDOWSXP
