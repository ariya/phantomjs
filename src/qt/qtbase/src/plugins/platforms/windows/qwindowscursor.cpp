/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef  QT_NO_CURSOR
#include "qwindowscursor.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"
#include "qwindowsscreen.h"

#include <QtGui/QBitmap>
#include <QtGui/QImage>
#include <QtGui/QBitmap>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/private/qguiapplication_p.h> // getPixmapCursor()

#include <QtCore/QDebug>
#include <QtCore/QScopedArrayPointer>

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT HBITMAP qt_pixmapToWinHBITMAP(const QPixmap &p, int hbitmapFormat = 0);
Q_GUI_EXPORT HBITMAP qt_createIconMask(const QBitmap &bitmap);

/*!
    \class QWindowsCursorCacheKey
    \brief Cache key for storing values in a QHash with a QCursor as key.

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsCursorCacheKey::QWindowsCursorCacheKey(const QCursor &c)
    : shape(c.shape()), bitmapCacheKey(0), maskCacheKey(0)
{
    if (shape == Qt::BitmapCursor) {
        const qint64 pixmapCacheKey = c.pixmap().cacheKey();
        if (pixmapCacheKey) {
            bitmapCacheKey = pixmapCacheKey;
        } else {
            Q_ASSERT(c.bitmap());
            Q_ASSERT(c.mask());
            bitmapCacheKey = c.bitmap()->cacheKey();
            maskCacheKey = c.mask()->cacheKey();
        }
    }
}

/*!
    \class QWindowsCursor
    \brief Platform cursor implementation

    Note that whereas under X11, a cursor can be set as a property of
    a window, there is only a global SetCursor() function on Windows.
    Each Window sets on the global cursor on receiving a Enter-event
    as do the Window manager frames (resize/move handles).

    \internal
    \ingroup qt-lighthouse-win
    \sa QWindowsWindowCursor
*/

HCURSOR QWindowsCursor::createPixmapCursor(const QPixmap &pixmap, int hotX, int hotY)
{
    HCURSOR cur = 0;
    QBitmap mask = pixmap.mask();
    if (mask.isNull()) {
        mask = QBitmap(pixmap.size());
        mask.fill(Qt::color1);
    }

    HBITMAP ic = qt_pixmapToWinHBITMAP(pixmap, /* HBitmapAlpha */ 2);
    const HBITMAP im = qt_createIconMask(mask);

    ICONINFO ii;
    ii.fIcon     = 0;
    ii.xHotspot  = hotX;
    ii.yHotspot  = hotY;
    ii.hbmMask   = im;
    ii.hbmColor  = ic;

    cur = CreateIconIndirect(&ii);

    DeleteObject(ic);
    DeleteObject(im);
    return cur;
}

HCURSOR QWindowsCursor::createSystemCursor(const QCursor &c)
{
    int hx = c.hotSpot().x();
    int hy = c.hotSpot().y();
    const Qt::CursorShape cshape = c.shape();
    if (cshape == Qt::BitmapCursor) {
        const QPixmap pixmap = c.pixmap();
        if (!pixmap.isNull())
            if (const HCURSOR hc = createPixmapCursor(pixmap, hx, hy))
                return hc;
    }

    // Non-standard Windows cursors are created from bitmaps

    static const uchar vsplit_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x00,
        0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uchar vsplitm_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0xf0, 0x07, 0x00,
        0x00, 0xf8, 0x0f, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
        0x00, 0xc0, 0x01, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
        0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
        0x80, 0xff, 0xff, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
        0x00, 0xc0, 0x01, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x00, 0xf0, 0x07, 0x00,
        0x00, 0xe0, 0x03, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uchar hsplit_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
        0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
        0x00, 0x41, 0x82, 0x00, 0x80, 0x41, 0x82, 0x01, 0xc0, 0x7f, 0xfe, 0x03,
        0x80, 0x41, 0x82, 0x01, 0x00, 0x41, 0x82, 0x00, 0x00, 0x40, 0x02, 0x00,
        0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
        0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uchar hsplitm_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
        0x00, 0xe0, 0x07, 0x00, 0x00, 0xe2, 0x47, 0x00, 0x00, 0xe3, 0xc7, 0x00,
        0x80, 0xe3, 0xc7, 0x01, 0xc0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x07,
        0xc0, 0xff, 0xff, 0x03, 0x80, 0xe3, 0xc7, 0x01, 0x00, 0xe3, 0xc7, 0x00,
        0x00, 0xe2, 0x47, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
        0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
   static const uchar openhand_bits[] = {
        0x80,0x01,0x58,0x0e,0x64,0x12,0x64,0x52,0x48,0xb2,0x48,0x92,
        0x16,0x90,0x19,0x80,0x11,0x40,0x02,0x40,0x04,0x40,0x04,0x20,
        0x08,0x20,0x10,0x10,0x20,0x10,0x00,0x00};
    static const uchar openhandm_bits[] = {
       0x80,0x01,0xd8,0x0f,0xfc,0x1f,0xfc,0x5f,0xf8,0xff,0xf8,0xff,
       0xf6,0xff,0xff,0xff,0xff,0x7f,0xfe,0x7f,0xfc,0x7f,0xfc,0x3f,
       0xf8,0x3f,0xf0,0x1f,0xe0,0x1f,0x00,0x00};
    static const uchar closedhand_bits[] = {
        0x00,0x00,0x00,0x00,0x00,0x00,0xb0,0x0d,0x48,0x32,0x08,0x50,
        0x10,0x40,0x18,0x40,0x04,0x40,0x04,0x20,0x08,0x20,0x10,0x10,
        0x20,0x10,0x20,0x10,0x00,0x00,0x00,0x00};
    static const uchar closedhandm_bits[] = {
        0x00,0x00,0x00,0x00,0x00,0x00,0xb0,0x0d,0xf8,0x3f,0xf8,0x7f,
        0xf0,0x7f,0xf8,0x7f,0xfc,0x7f,0xfc,0x3f,0xf8,0x3f,0xf0,0x1f,
        0xe0,0x1f,0xe0,0x1f,0x00,0x00,0x00,0x00};

    wchar_t *sh = 0;
    switch (c.shape()) {                        // map to windows cursor
    case Qt::ArrowCursor:
        sh = IDC_ARROW;
        break;
    case Qt::UpArrowCursor:
        sh = IDC_UPARROW;
        break;
    case Qt::CrossCursor:
        sh = IDC_CROSS;
        break;
    case Qt::WaitCursor:
        sh = IDC_WAIT;
        break;
    case Qt::IBeamCursor:
        sh = IDC_IBEAM;
        break;
    case Qt::SizeVerCursor:
        sh = IDC_SIZENS;
        break;
    case Qt::SizeHorCursor:
        sh = IDC_SIZEWE;
        break;
    case Qt::SizeBDiagCursor:
        sh = IDC_SIZENESW;
        break;
    case Qt::SizeFDiagCursor:
        sh = IDC_SIZENWSE;
        break;
    case Qt::SizeAllCursor:
        sh = IDC_SIZEALL;
        break;
    case Qt::ForbiddenCursor:
        sh = IDC_NO;
        break;
    case Qt::WhatsThisCursor:
        sh = IDC_HELP;
        break;
    case Qt::BusyCursor:
        sh = IDC_APPSTARTING;
        break;
    case Qt::PointingHandCursor:
        sh = IDC_HAND;
        break;
    case Qt::BlankCursor:
    case Qt::SplitVCursor:
    case Qt::SplitHCursor:
    case Qt::OpenHandCursor:
    case Qt::ClosedHandCursor:
    case Qt::BitmapCursor: {
        QImage bbits, mbits;
        bool invb, invm;
        if (cshape == Qt::BlankCursor) {
            bbits = QImage(32, 32, QImage::Format_Mono);
            bbits.fill(0);                // ignore color table
            mbits = bbits.copy();
            hx = hy = 16;
            invb = invm = false;
        } else if (cshape == Qt::OpenHandCursor || cshape == Qt::ClosedHandCursor) {
            bool open = cshape == Qt::OpenHandCursor;
            QBitmap cb = QBitmap::fromData(QSize(16, 16), open ? openhand_bits : closedhand_bits);
            QBitmap cm = QBitmap::fromData(QSize(16, 16), open ? openhandm_bits : closedhandm_bits);
            bbits = cb.toImage().convertToFormat(QImage::Format_Mono);
            mbits = cm.toImage().convertToFormat(QImage::Format_Mono);
            hx = hy = 8;
            invb = invm = false;
        } else if (cshape == Qt::BitmapCursor) {
            bbits = c.bitmap()->toImage().convertToFormat(QImage::Format_Mono);
            mbits = c.mask()->toImage().convertToFormat(QImage::Format_Mono);
            invb = bbits.colorCount() > 1 && qGray(bbits.color(0)) < qGray(bbits.color(1));
            invm = mbits.colorCount() > 1 && qGray(mbits.color(0)) < qGray(mbits.color(1));
        } else { // Qt::SplitVCursor, Qt::SplitHCursor
            const QBitmap cb = QBitmap::fromData(QSize(32, 32), cshape == Qt::SplitVCursor ? vsplit_bits : hsplit_bits);
            const QBitmap cm = QBitmap::fromData(QSize(32, 32), cshape == Qt::SplitVCursor ? vsplitm_bits : hsplitm_bits);
            bbits = cb.toImage().convertToFormat(QImage::Format_Mono);
            mbits = cm.toImage().convertToFormat(QImage::Format_Mono);
            hx = hy = 16;
            invb = invm = false;
        }
        const int n = qMax(1, bbits.width() / 8);
        const int h = bbits.height();
#if !defined(Q_OS_WINCE)
        QScopedArrayPointer<uchar> xBits(new uchar[h * n]);
        QScopedArrayPointer<uchar> xMask(new uchar[h * n]);
        int x = 0;
        for (int i = 0; i < h; ++i) {
            uchar *bits = bbits.scanLine(i);
            uchar *mask = mbits.scanLine(i);
            for (int j = 0; j < n; ++j) {
                uchar b = bits[j];
                uchar m = mask[j];
                if (invb)
                    b ^= 0xff;
                if (invm)
                    m ^= 0xff;
                xBits[x] = ~m;
                xMask[x] = b ^ m;
                ++x;
            }
        }
        return CreateCursor(GetModuleHandle(0), hx, hy, bbits.width(), bbits.height(),
                            xBits.data(), xMask.data());
#elif defined(GWES_ICONCURS) // Q_WS_WINCE
        // Windows CE only supports fixed cursor size.
        int sysW = GetSystemMetrics(SM_CXCURSOR);
        int sysH = GetSystemMetrics(SM_CYCURSOR);
        int sysN = qMax(1, sysW / 8);
        uchar* xBits = new uchar[sysH * sysN];
        uchar* xMask = new uchar[sysH * sysN];
        int x = 0;
        for (int i = 0; i < sysH; ++i) {
            if (i >= h) {
                memset(&xBits[x] , 255, sysN);
                memset(&xMask[x] ,   0, sysN);
                x += sysN;
            } else {
                int fillWidth = n > sysN ? sysN : n;
                uchar *bits = bbits.scanLine(i);
                uchar *mask = mbits.scanLine(i);
                for (int j = 0; j < fillWidth; ++j) {
                    uchar b = bits[j];
                    uchar m = mask[j];
                    if (invb)
                        b ^= 0xFF;
                    if (invm)
                        m ^= 0xFF;
                    xBits[x] = ~m;
                    xMask[x] = b ^ m;
                    ++x;
                }
                for (int j = fillWidth; j < sysN; ++j ) {
                    xBits[x] = 255;
                    xMask[x] = 0;
                    ++x;
                }
            }
        }

        HCURSOR hcurs = CreateCursor(qWinAppInst(), hx, hy, sysW, sysH,
                                   xBits, xMask);
        delete [] xBits;
        delete [] xMask;
        return hcurs;
#else
        Q_UNUSED(n);
        Q_UNUSED(h);
        return 0;
#endif

    }
    case Qt::DragCopyCursor:
    case Qt::DragMoveCursor:
    case Qt::DragLinkCursor: {
        const QPixmap pixmap = QGuiApplicationPrivate::instance()->getPixmapCursor(cshape);
        return createPixmapCursor(pixmap, hx, hy);
    }
    default:
        qWarning("%s: Invalid cursor shape %d", __FUNCTION__, cshape);
        return 0;
    }
#ifdef Q_OS_WINCE
    return LoadCursor(0, sh);
#else
    return (HCURSOR)LoadImage(0, sh, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
#endif
}

/*!
    \brief Return cached standard cursor resources or create new ones.
*/

QWindowsWindowCursor QWindowsCursor::standardWindowCursor(Qt::CursorShape shape)
{
    const QWindowsCursorCacheKey key(shape);
    CursorCache::iterator it = m_cursorCache.find(key);
    if (it == m_cursorCache.end())
        it = m_cursorCache.insert(key, QWindowsWindowCursor(QCursor(shape)));
    return it.value();
}

/*!
    \brief Return cached pixmap cursor or create new one.
*/

QWindowsWindowCursor QWindowsCursor::pixmapWindowCursor(const QCursor &c)
{
    const  QWindowsCursorCacheKey cacheKey(c);
    CursorCache::iterator it = m_cursorCache.find(cacheKey);
    if (it == m_cursorCache.end())
        it = m_cursorCache.insert(cacheKey, QWindowsWindowCursor(c));
    return it.value();
}

/*!
    \brief Set a cursor on a window.

    This is called frequently as the mouse moves over widgets in the window
    (QLineEdits, etc).
*/

void QWindowsCursor::changeCursor(QCursor *cursorIn, QWindow *window)
{
    if (!window)
        return;
    if (!cursorIn) {
        QWindowsWindow::baseWindowOf(window)->setCursor(QWindowsWindowCursor());
        return;
    }
    const QWindowsWindowCursor wcursor =
        cursorIn->shape() == Qt::BitmapCursor ?
        pixmapWindowCursor(*cursorIn) : standardWindowCursor(cursorIn->shape());
    if (wcursor.handle()) {
        QWindowsWindow::baseWindowOf(window)->setCursor(wcursor);
    } else {
        qWarning("%s: Unable to obtain system cursor for %d",
                 __FUNCTION__, cursorIn->shape());
    }
}

QPoint QWindowsCursor::mousePosition()
{
    POINT p;
    GetCursorPos(&p);
    return QPoint(p.x, p.y);
}

QWindowsCursor::CursorState QWindowsCursor::cursorState()
{
#ifndef Q_OS_WINCE
    enum { cursorShowing = 0x1, cursorSuppressed = 0x2 }; // Windows 8: CURSOR_SUPPRESSED
    CURSORINFO cursorInfo;
    cursorInfo.cbSize = sizeof(CURSORINFO);
    if (GetCursorInfo(&cursorInfo)) {
        if (cursorInfo.flags & CursorShowing)
            return CursorShowing;
        if (cursorInfo.flags & cursorSuppressed)
            return CursorSuppressed;
    }
#endif // !Q_OS_WINCE
    return CursorHidden;
}

void QWindowsCursor::setPos(const QPoint &pos)
{
    SetCursorPos(pos.x(), pos.y());
}

/*!
    \class QWindowsWindowCursor
    \brief Per-Window cursor. Contains a QCursor and manages its associated system
     cursor handle resource.

    Based on QSharedDataPointer, so that it can be passed around and
    used as a property of QWindowsBaseWindow.

    \internal
    \ingroup qt-lighthouse-win
    \sa QWindowsCursor
*/

class QWindowsWindowCursorData : public QSharedData
{
public:
    QWindowsWindowCursorData() : m_cursor(Qt::ArrowCursor), m_handle(0) {}
    explicit QWindowsWindowCursorData(const QCursor &c);
    ~QWindowsWindowCursorData();

    const QCursor m_cursor;
    const HCURSOR m_handle;
};

QWindowsWindowCursorData::QWindowsWindowCursorData(const QCursor &c) :
    m_cursor(c),
    m_handle(QWindowsCursor::createSystemCursor(c))
{
}

QWindowsWindowCursorData::~QWindowsWindowCursorData()
{
    if (m_handle)
        DestroyCursor(m_handle);
}

QWindowsWindowCursor::QWindowsWindowCursor() :
    m_data(new QWindowsWindowCursorData)
{
}

QWindowsWindowCursor::QWindowsWindowCursor(const QCursor &c) :
    m_data(new QWindowsWindowCursorData(c))
{
}

QWindowsWindowCursor::~QWindowsWindowCursor()
{
}

QWindowsWindowCursor::QWindowsWindowCursor(const QWindowsWindowCursor &rhs) :
    m_data(rhs.m_data)
{
}

QWindowsWindowCursor & QWindowsWindowCursor::operator =(const QWindowsWindowCursor &rhs)
{
    if (this != &rhs)
        m_data.operator =(rhs.m_data);
    return *this;
}

bool QWindowsWindowCursor::isNull() const
{
    return m_data->m_handle == 0;
}

QCursor QWindowsWindowCursor::cursor() const
{
    return m_data->m_cursor;
}

HCURSOR QWindowsWindowCursor::handle() const
{
    return m_data->m_handle;
}

QT_END_NAMESPACE

#endif // !QT_NO_CURSOR
