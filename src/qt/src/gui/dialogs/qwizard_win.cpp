/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QT_NO_WIZARD
#ifndef QT_NO_STYLE_WINDOWSVISTA

#include "qwizard_win_p.h"
#include <private/qsystemlibrary_p.h>
#include "qwizard.h"
#include "qpaintengine.h"
#include "qapplication.h"
#include <QtGui/QMouseEvent>
#include <QtGui/QDesktopWidget>

// Note, these tests are duplicates in qwindowsxpstyle_p.h.
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

QT_BEGIN_NAMESPACE

//DWM related
typedef struct  {       //MARGINS       
    int cxLeftWidth;    // width of left border that retains its size
    int cxRightWidth;   // width of right border that retains its size
    int cyTopHeight;    // height of top border that retains its size
    int cyBottomHeight; // height of bottom border that retains its size
} WIZ_MARGINS;
typedef struct {        //DTTOPTS
    DWORD dwSize;
    DWORD dwFlags;
    COLORREF crText;
    COLORREF crBorder;
    COLORREF crShadow;
    int eTextShadowType;
    POINT ptShadowOffset;
    int iBorderSize;
    int iFontPropId;
    int iColorPropId;
    int iStateId;
    BOOL fApplyOverlay;
    int iGlowSize;
} WIZ_DTTOPTS;

typedef struct {
    DWORD dwFlags;
    DWORD dwMask;
} WIZ_WTA_OPTIONS;

#define WIZ_WM_THEMECHANGED                 0x031A
#define WIZ_WM_DWMCOMPOSITIONCHANGED        0x031E

enum WIZ_WINDOWTHEMEATTRIBUTETYPE {
    WIZ_WTA_NONCLIENT = 1
};

#define WIZ_WTNCA_NODRAWCAPTION 0x00000001
#define WIZ_WTNCA_NODRAWICON    0x00000002

#define WIZ_DT_CENTER                   0x00000001 //DT_CENTER
#define WIZ_DT_VCENTER                  0x00000004
#define WIZ_DT_SINGLELINE               0x00000020
#define WIZ_DT_NOPREFIX                 0x00000800

enum WIZ_NAVIGATIONPARTS {          //NAVIGATIONPARTS
	WIZ_NAV_BACKBUTTON = 1,
	WIZ_NAV_FORWARDBUTTON = 2,
	WIZ_NAV_MENUBUTTON = 3,
};

enum WIZ_NAV_BACKBUTTONSTATES {     //NAV_BACKBUTTONSTATES
	WIZ_NAV_BB_NORMAL = 1,
	WIZ_NAV_BB_HOT = 2,
	WIZ_NAV_BB_PRESSED = 3,
	WIZ_NAV_BB_DISABLED = 4,
};

#define WIZ_TMT_CAPTIONFONT (801)           //TMT_CAPTIONFONT
#define WIZ_DTT_COMPOSITED  (1UL << 13)     //DTT_COMPOSITED
#define WIZ_DTT_GLOWSIZE    (1UL << 11)     //DTT_GLOWSIZE

#define WIZ_WM_NCMOUSELEAVE 674             //WM_NCMOUSELEAVE

#define WIZ_WP_CAPTION             1 //WP_CAPTION
#define WIZ_CS_ACTIVE              1 //CS_ACTIVE
#define WIZ_TMT_FILLCOLORHINT   3821 //TMT_FILLCOLORHINT
#define WIZ_TMT_BORDERCOLORHINT 3822 //TMT_BORDERCOLORHINT

typedef BOOL (WINAPI *PtrDwmDefWindowProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
typedef HRESULT (WINAPI *PtrDwmIsCompositionEnabled)(BOOL* pfEnabled);
typedef HRESULT (WINAPI *PtrDwmExtendFrameIntoClientArea)(HWND hWnd, const WIZ_MARGINS* pMarInset);
typedef HRESULT (WINAPI *PtrSetWindowThemeAttribute)(HWND hwnd, enum WIZ_WINDOWTHEMEATTRIBUTETYPE eAttribute, PVOID pvAttribute, DWORD cbAttribute);

static PtrDwmDefWindowProc pDwmDefWindowProc = 0;
static PtrDwmIsCompositionEnabled pDwmIsCompositionEnabled = 0;
static PtrDwmExtendFrameIntoClientArea pDwmExtendFrameIntoClientArea = 0;
static PtrSetWindowThemeAttribute pSetWindowThemeAttribute = 0;

//Theme related
typedef bool (WINAPI *PtrIsAppThemed)();
typedef bool (WINAPI *PtrIsThemeActive)();
typedef HANDLE (WINAPI *PtrOpenThemeData)(HWND hwnd, LPCWSTR pszClassList);
typedef HRESULT (WINAPI *PtrCloseThemeData)(HANDLE hTheme);
typedef HRESULT (WINAPI *PtrGetThemeSysFont)(HANDLE hTheme, int iFontId, LOGFONTW *plf);
typedef HRESULT (WINAPI *PtrDrawThemeTextEx)(HANDLE hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int cchText, DWORD dwTextFlags, LPRECT pRect, const WIZ_DTTOPTS *pOptions);
typedef HRESULT (WINAPI *PtrDrawThemeBackground)(HANDLE hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, OPTIONAL const RECT *pClipRect);
typedef HRESULT (WINAPI *PtrGetThemePartSize)(HANDLE hTheme, HDC hdc, int iPartId, int iStateId, OPTIONAL RECT *prc, enum THEMESIZE eSize, OUT SIZE *psz);
typedef HRESULT (WINAPI *PtrGetThemeColor)(HANDLE hTheme, int iPartId, int iStateId, int iPropId, OUT COLORREF *pColor);

static PtrIsAppThemed pIsAppThemed = 0;
static PtrIsThemeActive pIsThemeActive = 0;
static PtrOpenThemeData pOpenThemeData = 0;
static PtrCloseThemeData pCloseThemeData = 0;
static PtrGetThemeSysFont pGetThemeSysFont = 0;
static PtrDrawThemeTextEx pDrawThemeTextEx = 0;
static PtrDrawThemeBackground pDrawThemeBackground = 0;
static PtrGetThemePartSize pGetThemePartSize = 0;
static PtrGetThemeColor pGetThemeColor = 0;

int QVistaHelper::instanceCount = 0;
bool QVistaHelper::is_vista = false;
QVistaHelper::VistaState QVistaHelper::cachedVistaState = QVistaHelper::Dirty;

/******************************************************************************
** QVistaBackButton
*/

QVistaBackButton::QVistaBackButton(QWidget *widget)
    : QAbstractButton(widget)
{
    setFocusPolicy(Qt::NoFocus);
}

QSize QVistaBackButton::sizeHint() const
{
    ensurePolished();
    int size = int(QStyleHelper::dpiScaled(32));
    int width = size, height = size;
/*
    HANDLE theme = pOpenThemeData(0, L"Navigation");
    SIZE size;
    if (pGetThemePartSize(theme, 0, WIZ_NAV_BACKBUTTON, WIZ_NAV_BB_NORMAL, 0, TS_TRUE, &size) == S_OK) {
        width = size.cx;
        height = size.cy;
    }
*/
    return QSize(width, height);
}

void QVistaBackButton::enterEvent(QEvent *event)
{
    if (isEnabled())
        update();
    QAbstractButton::enterEvent(event);
}

void QVistaBackButton::leaveEvent(QEvent *event)
{
    if (isEnabled())
        update();
    QAbstractButton::leaveEvent(event);
}

void QVistaBackButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QRect r = rect();
    HANDLE theme = pOpenThemeData(0, L"Navigation");
    //RECT rect;
    RECT clipRect;
    int xoffset = QWidget::mapToParent(r.topLeft()).x() - 1;
    int yoffset = QWidget::mapToParent(r.topLeft()).y() - 1;

    clipRect.top = r.top() + yoffset;
    clipRect.bottom = r.bottom() + yoffset;
    clipRect.left = r.left() + xoffset;
    clipRect.right = r.right()  + xoffset;

    int state = WIZ_NAV_BB_NORMAL;
    if (!isEnabled())
        state = WIZ_NAV_BB_DISABLED;
    else if (isDown())
        state = WIZ_NAV_BB_PRESSED;
    else if (underMouse())
        state = WIZ_NAV_BB_HOT;

   pDrawThemeBackground(theme, p.paintEngine()->getDC(), WIZ_NAV_BACKBUTTON, state, &clipRect, &clipRect); 
}

/******************************************************************************
** QVistaHelper
*/

QVistaHelper::QVistaHelper(QWizard *wizard)
    : QObject(wizard)
    , pressed(false)
    , wizard(wizard)
    , backButton_(0)
{
    is_vista = resolveSymbols();
    if (instanceCount++ == 0)
        cachedVistaState = Dirty;
    if (is_vista)
        backButton_ = new QVistaBackButton(wizard);

    // Handle diff between Windows 7 and Vista
    iconSpacing = QStyleHelper::dpiScaled(7);
    textSpacing = QSysInfo::WindowsVersion >= QSysInfo::WV_WINDOWS7 ?
                  iconSpacing : QStyleHelper::dpiScaled(20);
}

QVistaHelper::~QVistaHelper()
{
    --instanceCount;
}

bool QVistaHelper::isCompositionEnabled()
{
    bool value = is_vista;
    if (is_vista) {
        HRESULT hr;
        BOOL bEnabled;

        hr = pDwmIsCompositionEnabled(&bEnabled);
        value = (SUCCEEDED(hr) && bEnabled);
    }
    return value;
}

bool QVistaHelper::isThemeActive()
{
    return is_vista && pIsThemeActive();
}

QVistaHelper::VistaState QVistaHelper::vistaState()
{
    if (instanceCount == 0 || cachedVistaState == Dirty)
        cachedVistaState =
            isCompositionEnabled() ? VistaAero : isThemeActive() ? VistaBasic : Classic;
    return cachedVistaState;
}

QColor QVistaHelper::basicWindowFrameColor()
{
    DWORD rgb;
    HANDLE hTheme = pOpenThemeData(QApplication::desktop()->winId(), L"WINDOW");
    pGetThemeColor(
        hTheme, WIZ_WP_CAPTION, WIZ_CS_ACTIVE,
        wizard->isActiveWindow() ? WIZ_TMT_FILLCOLORHINT : WIZ_TMT_BORDERCOLORHINT,
        &rgb);
    BYTE r = GetRValue(rgb);
    BYTE g = GetGValue(rgb);
    BYTE b = GetBValue(rgb);
    return QColor(r, g, b);
}

bool QVistaHelper::setDWMTitleBar(TitleBarChangeType type)
{
    bool value = false;
    if (vistaState() == VistaAero) {
        WIZ_MARGINS mar = {0};
        if (type == NormalTitleBar)
            mar.cyTopHeight = 0;
        else
            mar.cyTopHeight = titleBarSize() + topOffset();
        HRESULT hr = pDwmExtendFrameIntoClientArea(wizard->winId(), &mar);
        value = SUCCEEDED(hr);
    }
    return value;
}

void QVistaHelper::drawTitleBar(QPainter *painter)
{
    HDC hdc = painter->paintEngine()->getDC();

    if (vistaState() == VistaAero)
        drawBlackRect(QRect(0, 0, wizard->width(),
                            titleBarSize() + topOffset()), hdc);
    Q_ASSERT(backButton_);
    const int btnTop = backButton_->mapToParent(QPoint()).y();
    const int btnHeight = backButton_->size().height();
    const int verticalCenter = (btnTop + btnHeight / 2) - 1;

    const QString text = wizard->window()->windowTitle();
    const QFont font = QApplication::font("QWorkspaceTitleBar");
    const QFontMetrics fontMetrics(font);
    const QRect brect = fontMetrics.boundingRect(text);
    int textHeight = brect.height();
    int textWidth = brect.width();
    int glowOffset = 0;

    if (vistaState() == VistaAero) {
        textHeight += 2 * glowSize();
        textWidth += 2 * glowSize();
        glowOffset = glowSize();
    }

    drawTitleText(
        painter, text,
        QRect(titleOffset() - glowOffset, verticalCenter - textHeight / 2, textWidth, textHeight),
        hdc);

    if (!wizard->windowIcon().isNull()) {
        QRect rect(leftMargin(), verticalCenter - iconSize() / 2, iconSize(), iconSize());
        HICON hIcon = wizard->windowIcon().pixmap(iconSize()).toWinHICON();
        DrawIconEx(hdc, rect.left(), rect.top(), hIcon, 0, 0, 0, NULL, DI_NORMAL | DI_COMPAT);
        DestroyIcon(hIcon);
    }
}

void QVistaHelper::setTitleBarIconAndCaptionVisible(bool visible)
{
    if (is_vista) {
        WIZ_WTA_OPTIONS opt;
        opt.dwFlags = WIZ_WTNCA_NODRAWICON | WIZ_WTNCA_NODRAWCAPTION;
        if (visible)
            opt.dwMask = 0;
        else
            opt.dwMask = WIZ_WTNCA_NODRAWICON | WIZ_WTNCA_NODRAWCAPTION;
        pSetWindowThemeAttribute(wizard->winId(), WIZ_WTA_NONCLIENT, &opt, sizeof(WIZ_WTA_OPTIONS));
    }
}

bool QVistaHelper::winEvent(MSG* msg, long* result)
{
    switch (msg->message) {
    case WM_NCHITTEST: {
        LRESULT lResult;
        // Perform hit testing using DWM
        if (pDwmDefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam, &lResult)) {
            // DWM returned a hit, no further processing necessary
            *result = lResult;
        } else {
            // DWM didn't return a hit, process using DefWindowProc
            lResult = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
            // If DefWindowProc returns a window caption button, just return HTCLIENT (client area).
            // This avoid unnecessary hits to Windows NT style caption buttons which aren't visible but are
            // located just under the Aero style window close button.
            if (lResult == HTCLOSE || lResult == HTMAXBUTTON || lResult == HTMINBUTTON || lResult == HTHELP)
                *result = HTCLIENT;
            else
                *result = lResult;
        }
        break;
    }
    case WM_NCCALCSIZE: {
        NCCALCSIZE_PARAMS* lpncsp = (NCCALCSIZE_PARAMS*)msg->lParam;
        *result = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
        lpncsp->rgrc[0].top -= (vistaState() == VistaAero ? titleBarSize() : 0);
        break;
    }
    default:
        LRESULT lResult;
        // Pass to DWM to handle
        if (pDwmDefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam, &lResult))
            *result = lResult;
        // If the message wasn't handled by DWM, continue processing it as normal
        else
            return false;
    }

    return true;
}

void QVistaHelper::setMouseCursor(QPoint pos)
{
#ifndef QT_NO_CURSOR
    if (rtTop.contains(pos))
        wizard->setCursor(Qt::SizeVerCursor);
    else
        wizard->setCursor(Qt::ArrowCursor);
#endif
}

void QVistaHelper::mouseEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseMove:
        mouseMoveEvent(static_cast<QMouseEvent *>(event));
        break;
    case QEvent::MouseButtonPress:
        mousePressEvent(static_cast<QMouseEvent *>(event));
        break;
    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(static_cast<QMouseEvent *>(event));
        break;
    default:
        break;
    }
}

// The following hack ensures that the titlebar is updated correctly
// when the wizard style changes to and from AeroStyle. Specifically,
// this function causes a Windows message of type WM_NCCALCSIZE to
// be triggered.
void QVistaHelper::setWindowPosHack()
{
    const int x = wizard->geometry().x(); // ignored by SWP_NOMOVE
    const int y = wizard->geometry().y(); // ignored by SWP_NOMOVE
    const int w = wizard->width();
    const int h = wizard->height();
    SetWindowPos(wizard->winId(), 0, x, y, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

// The following hack allows any QWidget subclass to access
// QWidgetPrivate::topData() without being declared as a
// friend by QWidget.
class QHackWidget : public QWidget
{
public:
    Q_DECLARE_PRIVATE(QWidget)
    QTLWExtra* topData() { return d_func()->topData(); }
};

void QVistaHelper::collapseTopFrameStrut()
{
    QTLWExtra *top = ((QHackWidget *)wizard)->d_func()->topData();
    int x1, y1, x2, y2;
    top->frameStrut.getCoords(&x1, &y1, &x2, &y2);
    top->frameStrut.setCoords(x1, 0, x2, y2);
}

bool QVistaHelper::handleWinEvent(MSG *message, long *result)
{
    if (message->message == WIZ_WM_THEMECHANGED || message->message == WIZ_WM_DWMCOMPOSITIONCHANGED)
        cachedVistaState = Dirty;

    bool status = false;
    if (wizard->wizardStyle() == QWizard::AeroStyle && vistaState() == VistaAero) {
        status = winEvent(message, result);
        if (message->message == WM_NCCALCSIZE) {
            if (status)
                collapseTopFrameStrut();
        } else if (message->message == WM_NCPAINT) {
            wizard->update();
        }
    }
    return status;
}

void QVistaHelper::resizeEvent(QResizeEvent * event)
{
    Q_UNUSED(event);
    rtTop = QRect (0, 0, wizard->width(), frameSize());
    int height = captionSize() + topOffset();
    if (vistaState() == VistaBasic)
        height -= titleBarSize();
    rtTitle = QRect (0, frameSize(), wizard->width(), height);
}

void QVistaHelper::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(wizard);
    drawTitleBar(&painter);
}

void QVistaHelper::mouseMoveEvent(QMouseEvent *event)
{
    if (wizard->windowState() & Qt::WindowMaximized) {
        event->ignore();
        return;
    }

    QRect rect = wizard->geometry();
    if (pressed) {
        switch (change) {
        case resizeTop:
            {
                const int dy = event->pos().y() - pressedPos.y();
                if ((dy > 0 && rect.height() > wizard->minimumHeight())
                    || (dy < 0 && rect.height() < wizard->maximumHeight()))
                    rect.setTop(rect.top() + dy);
            }
            break;
        case movePosition: {
            QPoint newPos = event->pos() - pressedPos;
            rect.moveLeft(rect.left() + newPos.x());
            rect.moveTop(rect.top() + newPos.y());
            break; }
        default:
            break;
        }
        wizard->setGeometry(rect);

    } else if (vistaState() == VistaAero) {
        setMouseCursor(event->pos());
    }
    event->ignore();
}

void QVistaHelper::mousePressEvent(QMouseEvent *event)
{
    change = noChange;

    if (event->button() != Qt::LeftButton || wizard->windowState() & Qt::WindowMaximized) {
        event->ignore();
        return;
    }

    if (rtTitle.contains(event->pos())) {
        change = movePosition;
    } else if (rtTop.contains(event->pos()))
        change = (vistaState() == VistaAero) ? resizeTop : movePosition;

    if (change != noChange) {
        if (vistaState() == VistaAero)
            setMouseCursor(event->pos());
        pressed = true;
        pressedPos = event->pos();
    } else {
        event->ignore();
    }
}

void QVistaHelper::mouseReleaseEvent(QMouseEvent *event)
{
    change = noChange;
    if (pressed) {
        pressed = false;
        wizard->releaseMouse();
        if (vistaState() == VistaAero)
            setMouseCursor(event->pos());
    }
    event->ignore();
}

bool QVistaHelper::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != wizard)
        return QObject::eventFilter(obj, event);

    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        long result;
        MSG msg;
        msg.message = WM_NCHITTEST;
        msg.wParam  = 0;
        msg.lParam = MAKELPARAM(mouseEvent->globalX(), mouseEvent->globalY());
        msg.hwnd = wizard->winId();
        winEvent(&msg, &result);
        msg.wParam = result;
        msg.message = WM_NCMOUSEMOVE;
        winEvent(&msg, &result);
     } else if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            long result;
            MSG msg;
            msg.message = WM_NCHITTEST;
            msg.wParam  = 0;
            msg.lParam = MAKELPARAM(mouseEvent->globalX(), mouseEvent->globalY());
            msg.hwnd = wizard->winId();
            winEvent(&msg, &result);
            msg.wParam = result;
            msg.message = WM_NCLBUTTONDOWN;
            winEvent(&msg, &result);
        }
     } else if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            long result;
            MSG msg;
            msg.message = WM_NCHITTEST;
            msg.wParam  = 0;
            msg.lParam = MAKELPARAM(mouseEvent->globalX(), mouseEvent->globalY());
            msg.hwnd = wizard->winId();
            winEvent(&msg, &result);
            msg.wParam = result;
            msg.message = WM_NCLBUTTONUP;
            winEvent(&msg, &result);
        }
     }

     return false;
}

HFONT QVistaHelper::getCaptionFont(HANDLE hTheme)
{
    LOGFONT lf = {0};

    if (!hTheme)
        pGetThemeSysFont(hTheme, WIZ_TMT_CAPTIONFONT, &lf);
    else
    {
        NONCLIENTMETRICS ncm = {sizeof(NONCLIENTMETRICS)};
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, false);
        lf = ncm.lfMessageFont;
    }
    return CreateFontIndirect(&lf);
}

bool QVistaHelper::drawTitleText(QPainter *painter, const QString &text, const QRect &rect, HDC hdc)
{
    bool value = false;
    if (vistaState() == VistaAero) {
        HANDLE hTheme = pOpenThemeData(QApplication::desktop()->winId(), L"WINDOW");
        if (!hTheme) return false;
        // Set up a memory DC and bitmap that we'll draw into
        HDC dcMem;
        HBITMAP bmp;
        BITMAPINFO dib = {{0}};
        dcMem = CreateCompatibleDC(hdc);

        dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        dib.bmiHeader.biWidth = rect.width();
        dib.bmiHeader.biHeight = -rect.height();
        dib.bmiHeader.biPlanes = 1;
        dib.bmiHeader.biBitCount = 32;
        dib.bmiHeader.biCompression = BI_RGB;
 
        bmp = CreateDIBSection(hdc, &dib, DIB_RGB_COLORS, NULL, NULL, 0);

        // Set up the DC
        HFONT hCaptionFont = getCaptionFont(hTheme);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(dcMem, (HGDIOBJ) bmp);
        HFONT hOldFont = (HFONT)SelectObject(dcMem, (HGDIOBJ) hCaptionFont);
 
        // Draw the text!
        WIZ_DTTOPTS dto = { sizeof(WIZ_DTTOPTS) };
        const UINT uFormat = WIZ_DT_SINGLELINE|WIZ_DT_CENTER|WIZ_DT_VCENTER|WIZ_DT_NOPREFIX;
        RECT rctext ={0,0, rect.width(), rect.height()};

        dto.dwFlags = WIZ_DTT_COMPOSITED|WIZ_DTT_GLOWSIZE;
        dto.iGlowSize = glowSize();
 
        pDrawThemeTextEx(hTheme, dcMem, 0, 0, (LPCWSTR)text.utf16(), -1, uFormat, &rctext, &dto );
        BitBlt(hdc, rect.left(), rect.top(), rect.width(), rect.height(), dcMem, 0, 0, SRCCOPY);
        SelectObject(dcMem, (HGDIOBJ) hOldBmp);
        SelectObject(dcMem, (HGDIOBJ) hOldFont);
        DeleteObject(bmp);
        DeleteObject(hCaptionFont);
        DeleteDC(dcMem);
        //ReleaseDC(hwnd, hdc);
    } else if (vistaState() == VistaBasic) {
        painter->drawText(rect, text);
    }
    return value;
}

bool QVistaHelper::drawBlackRect(const QRect &rect, HDC hdc)
{
    bool value = false;
    if (vistaState() == VistaAero) {
        // Set up a memory DC and bitmap that we'll draw into
        HDC dcMem;
        HBITMAP bmp;
        BITMAPINFO dib = {{0}};
        dcMem = CreateCompatibleDC(hdc);

        dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        dib.bmiHeader.biWidth = rect.width();
        dib.bmiHeader.biHeight = -rect.height();
        dib.bmiHeader.biPlanes = 1;
        dib.bmiHeader.biBitCount = 32;
        dib.bmiHeader.biCompression = BI_RGB;
 
        bmp = CreateDIBSection(hdc, &dib, DIB_RGB_COLORS, NULL, NULL, 0);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(dcMem, (HGDIOBJ) bmp);

        BitBlt(hdc, rect.left(), rect.top(), rect.width(), rect.height(), dcMem, 0, 0, SRCCOPY);
        SelectObject(dcMem, (HGDIOBJ) hOldBmp);

        DeleteObject(bmp);
        DeleteDC(dcMem);
    }
    return value;
}

bool QVistaHelper::resolveSymbols()
{
    static bool tried = false;
    if (!tried) {
        QSystemLibrary dwmLib(L"dwmapi");
        pDwmIsCompositionEnabled =
            (PtrDwmIsCompositionEnabled)dwmLib.resolve("DwmIsCompositionEnabled");
        if (pDwmIsCompositionEnabled) {
            pDwmDefWindowProc = (PtrDwmDefWindowProc)dwmLib.resolve("DwmDefWindowProc");
            pDwmExtendFrameIntoClientArea =
                (PtrDwmExtendFrameIntoClientArea)dwmLib.resolve("DwmExtendFrameIntoClientArea");
        }
        QSystemLibrary themeLib(L"uxtheme");
        pIsAppThemed = (PtrIsAppThemed)themeLib.resolve("IsAppThemed");
        if (pIsAppThemed) {
            pDrawThemeBackground = (PtrDrawThemeBackground)themeLib.resolve("DrawThemeBackground");
            pGetThemePartSize = (PtrGetThemePartSize)themeLib.resolve("GetThemePartSize");
            pGetThemeColor = (PtrGetThemeColor)themeLib.resolve("GetThemeColor");
            pIsThemeActive = (PtrIsThemeActive)themeLib.resolve("IsThemeActive");
            pOpenThemeData = (PtrOpenThemeData)themeLib.resolve("OpenThemeData");
            pCloseThemeData = (PtrCloseThemeData)themeLib.resolve("CloseThemeData");
            pGetThemeSysFont = (PtrGetThemeSysFont)themeLib.resolve("GetThemeSysFont");
            pDrawThemeTextEx = (PtrDrawThemeTextEx)themeLib.resolve("DrawThemeTextEx");
            pSetWindowThemeAttribute = (PtrSetWindowThemeAttribute)themeLib.resolve("SetWindowThemeAttribute");
        }
        tried = true;
    }

    return (
        pDwmIsCompositionEnabled != 0
        && pDwmDefWindowProc != 0
        && pDwmExtendFrameIntoClientArea != 0
        && pIsAppThemed != 0
        && pDrawThemeBackground != 0
        && pGetThemePartSize != 0
        && pGetThemeColor != 0
        && pIsThemeActive != 0
        && pOpenThemeData != 0
        && pCloseThemeData != 0
        && pGetThemeSysFont != 0
        && pDrawThemeTextEx != 0
        && pSetWindowThemeAttribute != 0
        );
}

int QVistaHelper::titleOffset()
{
    int iconOffset = wizard ->windowIcon().isNull() ? 0 : iconSize() + textSpacing;
    return leftMargin() + iconOffset;
}

QT_END_NAMESPACE

#endif // QT_NO_STYLE_WINDOWSVISTA

#endif // QT_NO_WIZARD
