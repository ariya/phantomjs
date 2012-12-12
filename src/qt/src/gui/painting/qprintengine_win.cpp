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

#ifndef QT_NO_PRINTER

#include "qprinter_p.h"
#include "qprintengine_win_p.h"

#include <limits.h>

#include <private/qfont_p.h>
#include <private/qfontengine_p.h>
#include <private/qpainter_p.h>

#include <qbitmap.h>
#include <qdebug.h>
#include <qvector.h>
#include <qpicture.h>
#include <private/qpicture_p.h>

QT_BEGIN_NAMESPACE

extern QPainterPath qt_regionToPath(const QRegion &region);

// #define QT_DEBUG_DRAW

static void draw_text_item_win(const QPointF &_pos, const QTextItemInt &ti, HDC hdc,
                               bool convertToText, const QTransform &xform, const QPointF &topLeft);

static const struct {
    int winSizeName;
    QPrinter::PaperSize qtSizeName;
} dmMapping[] = {
    { DMPAPER_LETTER,             QPrinter::Letter },
    { DMPAPER_LETTERSMALL,        QPrinter::Letter },
    { DMPAPER_TABLOID,            QPrinter::Tabloid },
    { DMPAPER_LEDGER,             QPrinter::Ledger },
    { DMPAPER_LEGAL,              QPrinter::Legal },
    { DMPAPER_EXECUTIVE,          QPrinter::Executive },
    { DMPAPER_A3,                 QPrinter::A3 },
    { DMPAPER_A4,                 QPrinter::A4 },
    { DMPAPER_A4SMALL,            QPrinter::A4 },
    { DMPAPER_A5,                 QPrinter::A5 },
    { DMPAPER_B4,                 QPrinter::B4 },
    { DMPAPER_B5,                 QPrinter::B5 },
    { DMPAPER_FOLIO,              QPrinter::Folio },
    { DMPAPER_ENV_10,             QPrinter::Comm10E },
    { DMPAPER_ENV_DL,             QPrinter::DLE },
    { DMPAPER_ENV_C3,             QPrinter::C5E },
    { DMPAPER_LETTER_EXTRA,       QPrinter::Letter },
    { DMPAPER_LEGAL_EXTRA,        QPrinter::Legal },
    { DMPAPER_TABLOID_EXTRA,      QPrinter::Tabloid },
    { DMPAPER_A4_EXTRA,           QPrinter::A4},
    { DMPAPER_LETTER_TRANSVERSE,  QPrinter::Letter},
    { DMPAPER_A4_TRANSVERSE,      QPrinter::A4},
    { DMPAPER_LETTER_EXTRA_TRANSVERSE, QPrinter::Letter },
    { DMPAPER_A_PLUS,             QPrinter::A4 },
    { DMPAPER_B_PLUS,             QPrinter::A3 },
    { DMPAPER_LETTER_PLUS,        QPrinter::Letter },
    { DMPAPER_A4_PLUS,            QPrinter::A4 },
    { DMPAPER_A5_TRANSVERSE,      QPrinter::A5 },
    { DMPAPER_B5_TRANSVERSE,      QPrinter::B5 },
    { DMPAPER_A3_EXTRA,           QPrinter::A3 },
    { DMPAPER_A5_EXTRA,           QPrinter::A5 },
    { DMPAPER_B5_EXTRA,           QPrinter::B5 },
    { DMPAPER_A2,                 QPrinter::A2 },
    { DMPAPER_A3_TRANSVERSE,      QPrinter::A3 },
    { DMPAPER_A3_EXTRA_TRANSVERSE,QPrinter::A3 },
    { 0, QPrinter::Custom }
};

QPrinter::PaperSize mapDevmodePaperSize(int s)
{
    int i = 0;
    while ((dmMapping[i].winSizeName > 0) && (dmMapping[i].winSizeName != s))
        i++;
    return dmMapping[i].qtSizeName;
}

static int mapPaperSizeDevmode(QPrinter::PaperSize s)
{
    int i = 0;
 while ((dmMapping[i].winSizeName > 0) && (dmMapping[i].qtSizeName != s))
        i++;
    return dmMapping[i].winSizeName;
}

static const struct {
    int winSourceName;
    QPrinter::PaperSource qtSourceName;
}  sources[] = {
    { DMBIN_ONLYONE,        QPrinter::OnlyOne },
    { DMBIN_LOWER,          QPrinter::Lower },
    { DMBIN_MIDDLE,         QPrinter::Middle },
    { DMBIN_MANUAL,         QPrinter::Manual },
    { DMBIN_ENVELOPE,       QPrinter::Envelope },
    { DMBIN_ENVMANUAL,      QPrinter::EnvelopeManual },
    { DMBIN_AUTO,           QPrinter::Auto },
    { DMBIN_TRACTOR,        QPrinter::Tractor },
    { DMBIN_SMALLFMT,       QPrinter::SmallFormat },
    { DMBIN_LARGEFMT,       QPrinter::LargeFormat },
    { DMBIN_LARGECAPACITY,  QPrinter::LargeCapacity },
    { DMBIN_CASSETTE,       QPrinter::Cassette },
    { DMBIN_FORMSOURCE,     QPrinter::FormSource },
    { 0, (QPrinter::PaperSource) -1 }
};

static QPrinter::PaperSource mapDevmodePaperSource(int s)
{
    int i = 0;
    while ((sources[i].winSourceName > 0) && (sources[i].winSourceName != s))
        i++;
    return sources[i].winSourceName ? sources[i].qtSourceName : (QPrinter::PaperSource) s;
}

static int mapPaperSourceDevmode(QPrinter::PaperSource s)
{
    int i = 0;
    while ((sources[i].qtSourceName >= 0) && (sources[i].qtSourceName != s))
        i++;
    return sources[i].winSourceName ? sources[i].winSourceName : s;
}

QWin32PrintEngine::QWin32PrintEngine(QPrinter::PrinterMode mode)
    : QAlphaPaintEngine(*(new QWin32PrintEnginePrivate),
                   PaintEngineFeatures(PrimitiveTransform
                                       | PixmapTransform
                                       | PerspectiveTransform
                                       | PainterPaths
                                       | Antialiasing
                                       | PaintOutsidePaintEvent))
{
    Q_D(QWin32PrintEngine);
    d->docName = QLatin1String("document1");
    d->mode = mode;
    d->queryDefault();
    d->initialize();
}

bool QWin32PrintEngine::begin(QPaintDevice *pdev)
{
    Q_D(QWin32PrintEngine);

    QAlphaPaintEngine::begin(pdev);
    if (!continueCall())
        return true;

    if (d->reinit) {
       d->resetDC();
       d->reinit = false;
    }

    // ### set default colors and stuff...

    bool ok = d->state == QPrinter::Idle;

    if (!d->hdc)
        return false;

    // Assign the FILE: to get the query...
    if (d->printToFile && d->fileName.isEmpty())
        d->fileName = d->port;

    d->devMode->dmCopies = d->num_copies;

    DOCINFO di;
    memset(&di, 0, sizeof(DOCINFO));
    di.cbSize = sizeof(DOCINFO);
    di.lpszDocName = reinterpret_cast<const wchar_t *>(d->docName.utf16());
    if (d->printToFile && !d->fileName.isEmpty())
        di.lpszOutput = reinterpret_cast<const wchar_t *>(d->fileName.utf16());
    if (ok && StartDoc(d->hdc, &di) == SP_ERROR) {
        qErrnoWarning("QWin32PrintEngine::begin: StartDoc failed");
        ok = false;
    }

    if (StartPage(d->hdc) <= 0) {
        qErrnoWarning("QWin32PrintEngine::begin: StartPage failed");
        ok = false;
    }

    if (!ok) {
        d->state = QPrinter::Idle;
    } else {
        d->state = QPrinter::Active;
    }

    d->matrix = QTransform();
    d->has_pen = true;
    d->pen = QColor(Qt::black);
    d->has_brush = false;

    d->complex_xform = false;

    updateMatrix(d->matrix);

    if (!ok)
        cleanUp();

    return ok;
}

bool QWin32PrintEngine::end()
{
    Q_D(QWin32PrintEngine);

    if (d->hdc) {
        if (d->state == QPrinter::Aborted) {
            cleanUp();
            AbortDoc(d->hdc);
            return true;
        }
    }

    QAlphaPaintEngine::end();
    if (!continueCall())
        return true;

    if (d->hdc) {
        EndPage(d->hdc);                 // end; printing done
        EndDoc(d->hdc);
    }

    d->state = QPrinter::Idle;
    d->reinit = true;
    return true;
}

bool QWin32PrintEngine::newPage()
{
    Q_D(QWin32PrintEngine);
    Q_ASSERT(isActive());

    Q_ASSERT(d->hdc);

    flushAndInit();

    bool transparent = GetBkMode(d->hdc) == TRANSPARENT;

    if (!EndPage(d->hdc)) {
        qErrnoWarning("QWin32PrintEngine::newPage: EndPage failed");
        return false;
    }

    if (d->reinit) {
        if (!d->resetDC()) {
            qErrnoWarning("QWin32PrintEngine::newPage: ResetDC failed");
            return false;
        }
        d->reinit = false;
    }

    if (!StartPage(d->hdc)) {
        qErrnoWarning("Win32PrintEngine::newPage: StartPage failed");
        return false;
    }

    SetTextAlign(d->hdc, TA_BASELINE);
    if (transparent)
        SetBkMode(d->hdc, TRANSPARENT);

    // ###
    return true;

    bool success = false;
    if (d->hdc && d->state == QPrinter::Active) {
        if (EndPage(d->hdc) != SP_ERROR) {
            // reinitialize the DC before StartPage if needed,
            // because resetdc is disabled between calls to the StartPage and EndPage functions
            // (see StartPage documentation in the Platform SDK:Windows GDI)
//          state = PST_ACTIVEDOC;
//          reinit();
//          state = PST_ACTIVE;
            // start the new page now
            if (d->reinit) {
                if (!d->resetDC())
                    qErrnoWarning("QWin32PrintEngine::newPage(), ResetDC failed (2)");
                d->reinit = false;
            }
            success = (StartPage(d->hdc) != SP_ERROR);
        }
        if (!success) {
            d->state = QPrinter::Aborted;
            return false;
        }
    }
    return true;
}

bool QWin32PrintEngine::abort()
{
    // do nothing loop.
    return false;
}

void QWin32PrintEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
    Q_D(const QWin32PrintEngine);

    QAlphaPaintEngine::drawTextItem(p, textItem);
    if (!continueCall())
        return;

    const QTextItemInt &ti = static_cast<const QTextItemInt &>(textItem);
    QRgb brushColor = state->pen().brush().color().rgb();
    bool fallBack = state->pen().brush().style() != Qt::SolidPattern
                    || qAlpha(brushColor) != 0xff
                    || d->txop >= QTransform::TxProject
                    || ti.fontEngine->type() != QFontEngine::Win;


    if (!fallBack) {
        QFontEngineWin *fe = static_cast<QFontEngineWin *>(ti.fontEngine);

        // Try selecting the font to see if we get a substitution font
        SelectObject(d->hdc, fe->hfont);

        if (GetDeviceCaps(d->hdc, TECHNOLOGY) != DT_CHARSTREAM) {
            wchar_t n[64];
            GetTextFace(d->hdc, 64, n);
            fallBack = QString::fromWCharArray(n)
                    != QString::fromWCharArray(fe->logfont.lfFaceName);
        }
    }


    if (fallBack) {
        QPaintEngine::drawTextItem(p, textItem);
        return ;
    }

    // We only want to convert the glyphs to text if the entire string is compatible with ASCII
    // and if we actually have access to the chars.
    bool convertToText = ti.chars != 0;
    for (int i=0;  i < ti.num_chars; ++i) {
        if (ti.chars[i].unicode() >= 0x80) {
            convertToText = false;
            break;
        }

        if (ti.logClusters[i] != i) {
            convertToText = false;
            break;
        }
    }

    COLORREF cf = RGB(qRed(brushColor), qGreen(brushColor), qBlue(brushColor));
    SelectObject(d->hdc, CreateSolidBrush(cf));
    SelectObject(d->hdc, CreatePen(PS_SOLID, 1, cf));
    SetTextColor(d->hdc, cf);

    draw_text_item_win(p, ti, d->hdc, convertToText, d->matrix, d->devPaperRect.topLeft());
    DeleteObject(SelectObject(d->hdc,GetStockObject(HOLLOW_BRUSH)));
    DeleteObject(SelectObject(d->hdc,GetStockObject(BLACK_PEN)));
}

static inline qreal mmToInches(double mm)
{
    return mm*0.039370147;
}

static inline qreal inchesToMM(double in)
{
    return in/0.039370147;
}

int QWin32PrintEngine::metric(QPaintDevice::PaintDeviceMetric m) const
{
    Q_D(const QWin32PrintEngine);

    if (!d->hdc)
        return 0;

    int val;
    int res = d->resolution;

    switch (m) {
    case QPaintDevice::PdmWidth:
        if (d->has_custom_paper_size) {
            val =  qRound(d->paper_size.width() * res / 72.0);
        } else {
            int logPixelsX = GetDeviceCaps(d->hdc, LOGPIXELSX);
            if (logPixelsX == 0) {
                qWarning("QWin32PrintEngine::metric: GetDeviceCaps() failed, "
                        "might be a driver problem");
                logPixelsX = 600; // Reasonable default
            }
            val = res
                  * GetDeviceCaps(d->hdc, d->fullPage ? PHYSICALWIDTH : HORZRES)
                  / logPixelsX;
        }
        if (d->pageMarginsSet)
            val -= int(mmToInches((d->previousDialogMargins.left() +
                                   d->previousDialogMargins.width()) / 100.0) * res);
        break;
    case QPaintDevice::PdmHeight:
        if (d->has_custom_paper_size) {
            val = qRound(d->paper_size.height() * res / 72.0);
        } else {
            int logPixelsY = GetDeviceCaps(d->hdc, LOGPIXELSY);
            if (logPixelsY == 0) {
                qWarning("QWin32PrintEngine::metric: GetDeviceCaps() failed, "
                        "might be a driver problem");
                logPixelsY = 600; // Reasonable default
            }
            val = res
                  * GetDeviceCaps(d->hdc, d->fullPage ? PHYSICALHEIGHT : VERTRES)
                  / logPixelsY;
        }
        if (d->pageMarginsSet)
            val -= int(mmToInches((d->previousDialogMargins.top() +
                                   d->previousDialogMargins.height()) / 100.0) * res);
        break;
    case QPaintDevice::PdmDpiX:
        val = res;
        break;
    case QPaintDevice::PdmDpiY:
        val = res;
        break;
    case QPaintDevice::PdmPhysicalDpiX:
        val = GetDeviceCaps(d->hdc, LOGPIXELSX);
        break;
    case QPaintDevice::PdmPhysicalDpiY:
        val = GetDeviceCaps(d->hdc, LOGPIXELSY);
        break;
    case QPaintDevice::PdmWidthMM:
        if (d->has_custom_paper_size) {
            val = qRound(d->paper_size.width()*25.4/72);
        } else {
            if (!d->fullPage) {
                val = GetDeviceCaps(d->hdc, HORZSIZE);
            } else {
                float wi = 25.4 * GetDeviceCaps(d->hdc, PHYSICALWIDTH);
                int logPixelsX = GetDeviceCaps(d->hdc,  LOGPIXELSX);
                if (logPixelsX == 0) {
                    qWarning("QWin32PrintEngine::metric: GetDeviceCaps() failed, "
                            "might be a driver problem");
                    logPixelsX = 600; // Reasonable default
                }
                val = qRound(wi / logPixelsX);
            }
        }
        if (d->pageMarginsSet)
            val -= (d->previousDialogMargins.left() +
                    d->previousDialogMargins.width()) / 100.0;
        break;
    case QPaintDevice::PdmHeightMM:
        if (d->has_custom_paper_size) {
            val = qRound(d->paper_size.height()*25.4/72);
        } else {
            if (!d->fullPage) {
                val = GetDeviceCaps(d->hdc, VERTSIZE);
            } else {
                float hi = 25.4 * GetDeviceCaps(d->hdc, PHYSICALHEIGHT);
                int logPixelsY = GetDeviceCaps(d->hdc,  LOGPIXELSY);
                if (logPixelsY == 0) {
                    qWarning("QWin32PrintEngine::metric: GetDeviceCaps() failed, "
                            "might be a driver problem");
                    logPixelsY = 600; // Reasonable default
                }
                val = qRound(hi / logPixelsY);
            }
        }
        if (d->pageMarginsSet)
            val -= (d->previousDialogMargins.top() +
                    d->previousDialogMargins.height()) / 100.0;
        break;
    case QPaintDevice::PdmNumColors:
        {
            int bpp = GetDeviceCaps(d->hdc, BITSPIXEL);
            if(bpp==32)
                val = INT_MAX;
            else if(bpp<=8)
                val = GetDeviceCaps(d->hdc, NUMCOLORS);
            else
                val = 1 << (bpp * GetDeviceCaps(d->hdc, PLANES));
        }
        break;
    case QPaintDevice::PdmDepth:
        val = GetDeviceCaps(d->hdc, PLANES);
        break;
    default:
        qWarning("QPrinter::metric: Invalid metric command");
        return 0;
    }
    return val;
}

void QWin32PrintEngine::updateState(const QPaintEngineState &state)
{
    Q_D(QWin32PrintEngine);

    QAlphaPaintEngine::updateState(state);
    if (!continueCall())
        return;

    if (state.state() & DirtyTransform) {
        updateMatrix(state.transform());
    }

    if (state.state() & DirtyPen) {
        d->pen = state.pen();
        d->has_pen = d->pen.style() != Qt::NoPen && d->pen.isSolid();
    }

    if (state.state() & DirtyBrush) {
        QBrush brush = state.brush();
        d->has_brush = brush.style() == Qt::SolidPattern;
        d->brush_color = brush.color();
    }

    if (state.state() & DirtyClipEnabled) {
        if (state.isClipEnabled())
            updateClipPath(painter()->clipPath(), Qt::ReplaceClip);
        else
            updateClipPath(QPainterPath(), Qt::NoClip);
    }

    if (state.state() & DirtyClipPath) {
        updateClipPath(state.clipPath(), state.clipOperation());
    }

    if (state.state() & DirtyClipRegion) {
        QRegion clipRegion = state.clipRegion();
        QPainterPath clipPath = qt_regionToPath(clipRegion);
        updateClipPath(clipPath, state.clipOperation());
    }
}

void QWin32PrintEngine::updateClipPath(const QPainterPath &clipPath, Qt::ClipOperation op)
{
    Q_D(QWin32PrintEngine);

    bool doclip = true;
    if (op == Qt::NoClip) {
        SelectClipRgn(d->hdc, 0);
        doclip = false;
    }

    if (doclip) {
        QPainterPath xformed = clipPath * d->matrix;

        if (xformed.isEmpty()) {
            QRegion empty(-0x1000000, -0x1000000, 1, 1);
            SelectClipRgn(d->hdc, empty.handle());
        } else {
            d->composeGdiPath(xformed);
            const int ops[] = {
                -1,         // Qt::NoClip, covered above
                RGN_COPY,   // Qt::ReplaceClip
                RGN_AND,    // Qt::IntersectClip
                RGN_OR      // Qt::UniteClip
            };
            Q_ASSERT(op > 0 && unsigned(op) <= sizeof(ops) / sizeof(int));
            SelectClipPath(d->hdc, ops[op]);
        }
    }

    QPainterPath aclip = qt_regionToPath(alphaClipping());
    if (!aclip.isEmpty()) {
        QTransform tx(d->stretch_x, 0, 0, d->stretch_y, d->origin_x, d->origin_y);
        d->composeGdiPath(tx.map(aclip));
        SelectClipPath(d->hdc, RGN_DIFF);
    }
}

void QWin32PrintEngine::updateMatrix(const QTransform &m)
{
    Q_D(QWin32PrintEngine);

    QTransform stretch(d->stretch_x, 0, 0, d->stretch_y, d->origin_x, d->origin_y);
    d->painterMatrix = m;
    d->matrix = d->painterMatrix * stretch;
    d->txop = d->matrix.type();
    d->complex_xform = (d->txop > QTransform::TxScale);
}

void QWin32PrintEngine::drawPixmap(const QRectF &targetRect,
                                   const QPixmap &originalPixmap,
                                   const QRectF &sourceRect)
{
    Q_D(QWin32PrintEngine);

    QAlphaPaintEngine::drawPixmap(targetRect, originalPixmap, sourceRect);
    if (!continueCall())
        return;

    const int tileSize = 2048;

    QRectF r = targetRect;
    QRectF sr = sourceRect;

    QPixmap pixmap = originalPixmap;
    if (sr.size() != pixmap.size()) {
        pixmap = pixmap.copy(sr.toRect());
    }

    qreal scaleX = 1.0f;
    qreal scaleY = 1.0f;

    QTransform scaleMatrix = QTransform::fromScale(r.width() / pixmap.width(), r.height() / pixmap.height());
    QTransform adapted = QPixmap::trueMatrix(d->painterMatrix * scaleMatrix,
                                             pixmap.width(), pixmap.height());

    qreal xform_offset_x = adapted.dx();
    qreal xform_offset_y = adapted.dy();

    if (d->complex_xform) {
        pixmap = pixmap.transformed(adapted);
        scaleX = d->stretch_x;
        scaleY = d->stretch_y;
    } else {
        scaleX = d->stretch_x * (r.width() / pixmap.width()) * d->painterMatrix.m11();
        scaleY = d->stretch_y * (r.height() / pixmap.height()) * d->painterMatrix.m22();
    }

    QPointF topLeft = r.topLeft() * d->painterMatrix;
    int tx = int(topLeft.x() * d->stretch_x + d->origin_x);
    int ty = int(topLeft.y() * d->stretch_y + d->origin_y);
    int tw = qAbs(int(pixmap.width() * scaleX));
    int th = qAbs(int(pixmap.height() * scaleY));

    xform_offset_x *= d->stretch_x;
    xform_offset_y *= d->stretch_y;

    int dc_state = SaveDC(d->hdc);

    int tilesw = pixmap.width() / tileSize;
    int tilesh = pixmap.height() / tileSize;
    ++tilesw;
    ++tilesh;

    int txinc = tileSize*scaleX;
    int tyinc = tileSize*scaleY;

    for (int y = 0; y < tilesh; ++y) {
        int tposy = ty + (y * tyinc);
        int imgh = tileSize;
        int height = tyinc;
        if (y == (tilesh - 1)) {
            imgh = pixmap.height() - (y * tileSize);
            height = (th - (y * tyinc));
        }
        for (int x = 0; x < tilesw; ++x) {
            int tposx = tx + (x * txinc);
            int imgw = tileSize;
            int width = txinc;
            if (x == (tilesw - 1)) {
                imgw = pixmap.width() - (x * tileSize);
                width = (tw - (x * txinc));
            }

            QPixmap p = pixmap.copy(tileSize * x, tileSize * y, imgw, imgh);
            HBITMAP hbitmap = p.toWinHBITMAP(QPixmap::NoAlpha);
            HDC display_dc = GetDC(0);
            HDC hbitmap_hdc = CreateCompatibleDC(display_dc);
            HGDIOBJ null_bitmap = SelectObject(hbitmap_hdc, hbitmap);

            ReleaseDC(0, display_dc);

            if (!StretchBlt(d->hdc, qRound(tposx - xform_offset_x), qRound(tposy - xform_offset_y), width, height,
                            hbitmap_hdc, 0, 0, p.width(), p.height(), SRCCOPY))
                qErrnoWarning("QWin32PrintEngine::drawPixmap, StretchBlt failed");

            SelectObject(hbitmap_hdc, null_bitmap);
            DeleteObject(hbitmap);
            DeleteDC(hbitmap_hdc);
        }
    }

    RestoreDC(d->hdc, dc_state);
}


void QWin32PrintEngine::drawTiledPixmap(const QRectF &r, const QPixmap &pm, const QPointF &pos)
{
    Q_D(QWin32PrintEngine);

    QAlphaPaintEngine::drawTiledPixmap(r, pm, pos);
    if (!continueCall())
        return;

    if (d->complex_xform || !pos.isNull()) {
        QPaintEngine::drawTiledPixmap(r, pm, pos);
    } else {
        int dc_state = SaveDC(d->hdc);

        HDC display_dc = GetDC(0);
        HBITMAP hbitmap = pm.toWinHBITMAP(QPixmap::NoAlpha);
        HDC hbitmap_hdc = CreateCompatibleDC(display_dc);
        HGDIOBJ null_bitmap = SelectObject(hbitmap_hdc, hbitmap);

        ReleaseDC(0, display_dc);

        QRectF trect = d->painterMatrix.mapRect(r);
        int tx = int(trect.left() * d->stretch_x + d->origin_x);
        int ty = int(trect.top() * d->stretch_y + d->origin_y);

        int xtiles = int(trect.width() / pm.width()) + 1;
        int ytiles = int(trect.height() / pm.height()) + 1;
        int xinc = int(pm.width() * d->stretch_x);
        int yinc = int(pm.height() * d->stretch_y);

        for (int y = 0; y < ytiles; ++y) {
            int ity = ty + (yinc * y);
            int ith = pm.height();
            if (y == (ytiles - 1)) {
                ith = int(trect.height() - (pm.height() * y));
            }

            for (int x = 0; x < xtiles; ++x) {
                int itx = tx + (xinc * x);
                int itw = pm.width();
                if (x == (xtiles - 1)) {
                    itw = int(trect.width() - (pm.width() * x));
                }

                if (!StretchBlt(d->hdc, itx, ity, int(itw * d->stretch_x), int(ith * d->stretch_y),
                                hbitmap_hdc, 0, 0, itw, ith, SRCCOPY))
                    qErrnoWarning("QWin32PrintEngine::drawPixmap, StretchBlt failed");

            }
        }

        SelectObject(hbitmap_hdc, null_bitmap);
        DeleteObject(hbitmap);
        DeleteDC(hbitmap_hdc);

        RestoreDC(d->hdc, dc_state);
    }
}


void QWin32PrintEnginePrivate::composeGdiPath(const QPainterPath &path)
{
    if (!BeginPath(hdc))
        qErrnoWarning("QWin32PrintEnginePrivate::drawPath: BeginPath failed");

    // Drawing the subpaths
    int start = -1;
    for (int i=0; i<path.elementCount(); ++i) {
        const QPainterPath::Element &elm = path.elementAt(i);
        switch (elm.type) {
        case QPainterPath::MoveToElement:
            if (start >= 0
                && path.elementAt(start).x == path.elementAt(i-1).x
                && path.elementAt(start).y == path.elementAt(i-1).y)
                CloseFigure(hdc);
            start = i;
            MoveToEx(hdc, qRound(elm.x), qRound(elm.y), 0);
            break;
        case QPainterPath::LineToElement:
            LineTo(hdc, qRound(elm.x), qRound(elm.y));
            break;
        case QPainterPath::CurveToElement: {
            POINT pts[3] = {
                { qRound(elm.x), qRound(elm.y) },
                { qRound(path.elementAt(i+1).x), qRound(path.elementAt(i+1).y) },
                { qRound(path.elementAt(i+2).x), qRound(path.elementAt(i+2).y) }
            };
            i+=2;
            PolyBezierTo(hdc, pts, 3);
            break;
        }
        default:
            qFatal("QWin32PaintEngine::drawPath: Unhandled type: %d", elm.type);
        }
    }

    if (start >= 0
        && path.elementAt(start).x == path.elementAt(path.elementCount()-1).x
        && path.elementAt(start).y == path.elementAt(path.elementCount()-1).y)
        CloseFigure(hdc);

    if (!EndPath(hdc))
        qErrnoWarning("QWin32PaintEngine::drawPath: EndPath failed");

    SetPolyFillMode(hdc, path.fillRule() == Qt::WindingFill ? WINDING : ALTERNATE);
}


void QWin32PrintEnginePrivate::fillPath_dev(const QPainterPath &path, const QColor &color)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << " --- QWin32PrintEnginePrivate::fillPath() bound:" << path.boundingRect() << color;
#endif

    composeGdiPath(path);

    HBRUSH brush = CreateSolidBrush(RGB(color.red(), color.green(), color.blue()));
    HGDIOBJ old_brush = SelectObject(hdc, brush);
    FillPath(hdc);
    DeleteObject(SelectObject(hdc, old_brush));
}

void QWin32PrintEnginePrivate::strokePath_dev(const QPainterPath &path, const QColor &color, qreal penWidth)
{
    composeGdiPath(path);
    LOGBRUSH brush;
    brush.lbStyle = BS_SOLID;
    brush.lbColor = RGB(color.red(), color.green(), color.blue());
    DWORD capStyle = PS_ENDCAP_SQUARE;
    DWORD joinStyle = PS_JOIN_BEVEL;
    if (pen.capStyle() == Qt::FlatCap)
        capStyle = PS_ENDCAP_FLAT;
    else if (pen.capStyle() == Qt::RoundCap)
        capStyle = PS_ENDCAP_ROUND;

    if (pen.joinStyle() == Qt::MiterJoin)
        joinStyle = PS_JOIN_MITER;
    else if (pen.joinStyle() == Qt::RoundJoin)
        joinStyle = PS_JOIN_ROUND;

    HPEN pen = ExtCreatePen(((penWidth == 0) ? PS_COSMETIC : PS_GEOMETRIC)
                            | PS_SOLID | capStyle | joinStyle,
                            (penWidth == 0) ? 1 : penWidth, &brush, 0, 0);

    HGDIOBJ old_pen = SelectObject(hdc, pen);
    StrokePath(hdc);
    DeleteObject(SelectObject(hdc, old_pen));
}


void QWin32PrintEnginePrivate::fillPath(const QPainterPath &path, const QColor &color)
{
    fillPath_dev(path * matrix, color);
}

void QWin32PrintEnginePrivate::strokePath(const QPainterPath &path, const QColor &color)
{
    QPainterPathStroker stroker;
    if (pen.style() == Qt::CustomDashLine) {
        stroker.setDashPattern(pen.dashPattern());
        stroker.setDashOffset(pen.dashOffset());
    } else {
        stroker.setDashPattern(pen.style());
    }
    stroker.setCapStyle(pen.capStyle());
    stroker.setJoinStyle(pen.joinStyle());
    stroker.setMiterLimit(pen.miterLimit());

    QPainterPath stroke;
    qreal width = pen.widthF();
    if (pen.style() == Qt::SolidLine && (pen.isCosmetic() || matrix.type() < QTransform::TxScale)) {
        strokePath_dev(path * matrix, color, width);
    } else {
        stroker.setWidth(width);
        if (pen.isCosmetic()) {
            stroke = stroker.createStroke(path * matrix);
        } else {
            stroke = stroker.createStroke(path) * painterMatrix;
            QTransform stretch(stretch_x, 0, 0, stretch_y, origin_x, origin_y);
            stroke = stroke * stretch;
        }

        if (stroke.isEmpty())
            return;

        fillPath_dev(stroke, color);
    }
}


void QWin32PrintEngine::drawPath(const QPainterPath &path)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << " - QWin32PrintEngine::drawPath(), bounds: " << path.boundingRect();
#endif

    Q_D(QWin32PrintEngine);

    QAlphaPaintEngine::drawPath(path);
    if (!continueCall())
        return;

    if (d->has_brush)
        d->fillPath(path, d->brush_color);

    if (d->has_pen)
        d->strokePath(path, d->pen.color());
}


void QWin32PrintEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
#ifdef QT_DEBUG_DRAW
    qDebug() << " - QWin32PrintEngine::drawPolygon(), pointCount: " << pointCount;
#endif

    QAlphaPaintEngine::drawPolygon(points, pointCount, mode);
    if (!continueCall())
        return;

    Q_ASSERT(pointCount > 1);

    QPainterPath path(points[0]);

    for (int i=1; i<pointCount; ++i) {
        path.lineTo(points[i]);
    }

    Q_D(QWin32PrintEngine);

    bool has_brush = d->has_brush;

    if (mode == PolylineMode)
        d->has_brush = false; // No brush for polylines
    else
        path.closeSubpath(); // polygons are should always be closed.

    drawPath(path);
    d->has_brush = has_brush;
}

void QWin32PrintEnginePrivate::queryDefault()
{
    /* Read the default printer name, driver and port with the intuitive function
     * Strings "windows" and "device" are specified in the MSDN under EnumPrinters()
     */
    QString noPrinters(QLatin1String("qt_no_printers"));
    wchar_t buffer[256];
    GetProfileString(L"windows", L"device",
                     reinterpret_cast<const wchar_t *>(noPrinters.utf16()),
                     buffer, 256);
    QString output = QString::fromWCharArray(buffer);
    if (output.isEmpty() || output == noPrinters) // no printers
        return;

    QStringList info = output.split(QLatin1Char(','));
    int infoSize = info.size();
    if (infoSize > 0) {
        if (name.isEmpty())
            name = info.at(0);
        if (program.isEmpty() && infoSize > 1)
            program = info.at(1);
        if (port.isEmpty() && infoSize > 2)
            port = info.at(2);
    }
}

QWin32PrintEnginePrivate::~QWin32PrintEnginePrivate()
{
    if (hdc)
        release();
}

void QWin32PrintEnginePrivate::initialize()
{
    if (hdc)
        release();
    Q_ASSERT(!hPrinter);
    Q_ASSERT(!hdc);
    Q_ASSERT(!devMode);
    Q_ASSERT(!pInfo);

    if (name.isEmpty())
        return;

    txop = QTransform::TxNone;

    bool ok = OpenPrinter((LPWSTR)name.utf16(), (LPHANDLE)&hPrinter, 0);
    if (!ok) {
        qErrnoWarning("QWin32PrintEngine::initialize: OpenPrinter failed");
        return;
    }

    // Fetch the PRINTER_INFO_2 with DEVMODE data containing the
    // printer settings.
    DWORD infoSize, numBytes;
    GetPrinter(hPrinter, 2, NULL, 0, &infoSize);
    hMem = GlobalAlloc(GHND, infoSize);
    pInfo = (PRINTER_INFO_2*) GlobalLock(hMem);
    ok = GetPrinter(hPrinter, 2, (LPBYTE)pInfo, infoSize, &numBytes);

    if (!ok) {
        qErrnoWarning("QWin32PrintEngine::initialize: GetPrinter failed");
        GlobalUnlock(pInfo);
        GlobalFree(hMem);
        ClosePrinter(hPrinter);
        pInfo = 0;
        hMem = 0;
        hPrinter = 0;
        return;
    }

    devMode = pInfo->pDevMode;
    hdc = CreateDC(reinterpret_cast<const wchar_t *>(program.utf16()),
                   reinterpret_cast<const wchar_t *>(name.utf16()), 0, devMode);

    Q_ASSERT(hPrinter);
    Q_ASSERT(pInfo);

    if (devMode) {
        num_copies = devMode->dmCopies;
    }

    initHDC();

#ifdef QT_DEBUG_DRAW
    qDebug() << "QWin32PrintEngine::initialize()" << endl
             << " - paperRect" << devPaperRect << endl
             << " - pageRect" << devPageRect << endl
             << " - stretch_x" << stretch_x << endl
             << " - stretch_y" << stretch_y << endl
             << " - origin_x" << origin_x << endl
             << " - origin_y" << origin_y << endl;
#endif
}

void QWin32PrintEnginePrivate::initHDC()
{
    Q_ASSERT(hdc);

    HDC display_dc = GetDC(0);
    dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
    dpi_display = GetDeviceCaps(display_dc, LOGPIXELSY);
    ReleaseDC(0, display_dc);
    if (dpi_display == 0) {
        qWarning("QWin32PrintEngine::metric: GetDeviceCaps() failed, "
                "might be a driver problem");
        dpi_display = 96; // Reasonable default
    }

    switch(mode) {
    case QPrinter::ScreenResolution:
        resolution = dpi_display;
        stretch_x = dpi_x / double(dpi_display);
        stretch_y = dpi_y / double(dpi_display);
        break;
    case QPrinter::PrinterResolution:
    case QPrinter::HighResolution:
        resolution = dpi_y;
        stretch_x = 1;
        stretch_y = 1;
        break;
    default:
        break;
    }

    initDevRects();
}

void QWin32PrintEnginePrivate::initDevRects()
{
    devPaperRect = QRect(0, 0,
                         GetDeviceCaps(hdc, PHYSICALWIDTH),
                         GetDeviceCaps(hdc, PHYSICALHEIGHT));
    devPhysicalPageRect = QRect(GetDeviceCaps(hdc, PHYSICALOFFSETX),
                                GetDeviceCaps(hdc, PHYSICALOFFSETY),
                                GetDeviceCaps(hdc, HORZRES),
                                GetDeviceCaps(hdc, VERTRES));
    if (!pageMarginsSet)
        devPageRect = devPhysicalPageRect;
    else
        devPageRect = devPaperRect.adjusted(qRound(mmToInches(previousDialogMargins.left() / 100.0) * dpi_x),
                                            qRound(mmToInches(previousDialogMargins.top() / 100.0) * dpi_y),
                                            -qRound(mmToInches(previousDialogMargins.width() / 100.0) * dpi_x),
                                            -qRound(mmToInches(previousDialogMargins.height() / 100.0) * dpi_y));
    updateOrigin();
}

void QWin32PrintEnginePrivate::setPageMargins(int marginLeft, int marginTop, int marginRight, int marginBottom)
{
    pageMarginsSet = true;
    previousDialogMargins = QRect(marginLeft, marginTop, marginRight, marginBottom);

    devPageRect = devPaperRect.adjusted(qRound(mmToInches(marginLeft / 100.0) * dpi_x),
                                        qRound(mmToInches(marginTop / 100.0) * dpi_y),
                                        - qRound(mmToInches(marginRight / 100.0) * dpi_x),
                                        - qRound(mmToInches(marginBottom / 100.0) * dpi_y));
    updateOrigin();
}

QRect QWin32PrintEnginePrivate::getPageMargins() const
{
    if (pageMarginsSet)
        return previousDialogMargins;
    else
        return QRect(qRound(inchesToMM(devPhysicalPageRect.left()) * 100.0 / dpi_x),
                     qRound(inchesToMM(devPhysicalPageRect.top()) * 100.0 / dpi_y),
                     qRound(inchesToMM(devPaperRect.right() - devPhysicalPageRect.right()) * 100.0 / dpi_x),
                     qRound(inchesToMM(devPaperRect.bottom() - devPhysicalPageRect.bottom()) * 100.0 / dpi_y));
}

void QWin32PrintEnginePrivate::release()
{
    if (hdc == 0)
        return;

    if (globalDevMode) { // Devmode comes from print dialog
        GlobalUnlock(globalDevMode);
    } else {            // Devmode comes from initialize...
        // devMode is a part of the same memory block as pInfo so one free is enough...
        GlobalUnlock(hMem);
        GlobalFree(hMem);
    }
    if (hPrinter)
        ClosePrinter(hPrinter);
    DeleteDC(hdc);

    hdc = 0;
    hPrinter = 0;
    pInfo = 0;
    hMem = 0;
    devMode = 0;
}

QList<QVariant> QWin32PrintEnginePrivate::queryResolutions() const
{
    // Read the supported resolutions of the printer.
    QList<QVariant> list;

    DWORD numRes = DeviceCapabilities(reinterpret_cast<const wchar_t *>(name.utf16()),
                                      reinterpret_cast<const wchar_t *>(port.utf16()),
                                      DC_ENUMRESOLUTIONS, 0, 0);
    if (numRes == (DWORD)-1)
        return list;

    LONG *enumRes = (LONG*)malloc(numRes * 2 * sizeof(LONG));
    DWORD errRes = DeviceCapabilities(reinterpret_cast<const wchar_t *>(name.utf16()),
                                      reinterpret_cast<const wchar_t *>(port.utf16()),
                                      DC_ENUMRESOLUTIONS, (LPWSTR)enumRes, 0);

    if (errRes == (DWORD)-1) {
        qErrnoWarning("QWin32PrintEngine::queryResolutions: DeviceCapabilities failed");
        return list;
    }

    for (uint i=0; i<numRes; ++i)
        list.append(int(enumRes[i * 2]));

    return list;
}

void QWin32PrintEnginePrivate::doReinit()
{
    if (state == QPrinter::Active) {
        reinit = true;
    } else {
        resetDC();
        initDevRects();
        reinit = false;
    }
}

void QWin32PrintEnginePrivate::updateOrigin()
{
    if (fullPage) {
        // subtract physical margins to make (0,0) absolute top corner of paper
        // then add user defined margins
        origin_x = -devPhysicalPageRect.x();
        origin_y = -devPhysicalPageRect.y();
        if (pageMarginsSet) {
            origin_x += devPageRect.left();
            origin_y += devPageRect.top();
        }
    } else {
        origin_x = 0;
        origin_y = 0;
        if (pageMarginsSet) {
            origin_x = devPageRect.left() - devPhysicalPageRect.x();
            origin_y = devPageRect.top() - devPhysicalPageRect.y();
        }
    }
}

void QWin32PrintEngine::setProperty(PrintEnginePropertyKey key, const QVariant &value)
{
    Q_D(QWin32PrintEngine);
    switch (key) {
    case PPK_CollateCopies:
        {
            if (!d->devMode)
                break;
            d->devMode->dmCollate = value.toBool() ? DMCOLLATE_TRUE : DMCOLLATE_FALSE;
            d->doReinit();
        }
        break;

    case PPK_ColorMode:
        {
            if (!d->devMode)
                break;
            d->devMode->dmColor = (value.toInt() == QPrinter::Color) ? DMCOLOR_COLOR : DMCOLOR_MONOCHROME;
            d->doReinit();
        }
        break;

    case PPK_Creator:

        break;

    case PPK_DocumentName:
        if (isActive()) {
            qWarning("QWin32PrintEngine: Cannot change document name while printing is active");
            return;
        }
        d->docName = value.toString();
        break;

    case PPK_FullPage:
        d->fullPage = value.toBool();
        d->updateOrigin();
        break;

    case PPK_CopyCount: // fallthrough
    case PPK_NumberOfCopies:
        if (!d->devMode)
            break;
        d->num_copies = value.toInt();
        d->devMode->dmCopies = d->num_copies;
        d->doReinit();
        break;

    case PPK_Orientation:
        {
            if (!d->devMode)
                break;
            int orientation = value.toInt() == QPrinter::Landscape ? DMORIENT_LANDSCAPE : DMORIENT_PORTRAIT;
            int old_orientation = d->devMode->dmOrientation;
            d->devMode->dmOrientation = orientation;
            if (d->has_custom_paper_size && old_orientation != orientation)
                d->paper_size = QSizeF(d->paper_size.height(), d->paper_size.width());
            d->doReinit();
        }
        break;

    case PPK_OutputFileName:
        if (isActive()) {
            qWarning("QWin32PrintEngine: Cannot change filename while printing");
        } else {
            d->fileName = value.toString();
            d->printToFile = !value.toString().isEmpty();
        }
        break;

    case PPK_PaperSize:
        if (!d->devMode)
            break;
        d->devMode->dmPaperSize = mapPaperSizeDevmode(QPrinter::PaperSize(value.toInt()));
        d->has_custom_paper_size = (QPrinter::PaperSize(value.toInt()) == QPrinter::Custom);
        d->doReinit();
        break;

    case PPK_PaperSource:
        {
            if (!d->devMode)
                break;
            int dmMapped = DMBIN_AUTO;

            QList<QVariant> v = property(PPK_PaperSources).toList();
            if (v.contains(value))
                dmMapped = mapPaperSourceDevmode(QPrinter::PaperSource(value.toInt()));

            d->devMode->dmDefaultSource = dmMapped;
            d->doReinit();
        }
        break;

    case PPK_PrinterName:
        d->name = value.toString();
        if(d->name.isEmpty())
            d->queryDefault();
        d->initialize();
        break;

    case PPK_Resolution:
        {
            d->resolution = value.toInt();

            d->stretch_x = d->dpi_x / double(d->resolution);
            d->stretch_y = d->dpi_y / double(d->resolution);
        }
        break;

    case PPK_SelectionOption:

        break;

    case PPK_SupportedResolutions:

        break;


    case PPK_WindowsPageSize:
        if (!d->devMode)
            break;
        d->has_custom_paper_size = false;
        d->devMode->dmPaperSize = value.toInt();
        d->doReinit();
        break;

    case PPK_CustomPaperSize:
    {
        d->has_custom_paper_size = true;
        d->paper_size = value.toSizeF();
        if (!d->devMode)
            break;
        int orientation = d->devMode->dmOrientation;
        DWORD needed = 0;
        DWORD returned = 0;
        if (!EnumForms(d->hPrinter, 1, 0, 0, &needed, &returned)) {
            BYTE *forms = (BYTE *) malloc(needed);
            if (EnumForms(d->hPrinter, 1, forms, needed, &needed, &returned)) {
                for (DWORD i=0; i< returned; ++i) {
                    FORM_INFO_1 *formArray = reinterpret_cast<FORM_INFO_1 *>(forms);
                    // the form sizes are specified in 1000th of a mm,
                    // convert the size to Points
                    QSizeF size((formArray[i].Size.cx * 72/25.4)/1000.0,
                                (formArray[i].Size.cy * 72/25.4)/1000.0);
                    if (qAbs(d->paper_size.width() - size.width()) <= 2
                        && qAbs(d->paper_size.height() - size.height()) <= 2)
                    {
                        d->devMode->dmPaperSize = i + 1;
                        break;
                    }
                }
            }
            free(forms);
        }
        if (orientation != DMORIENT_PORTRAIT)
            d->paper_size = QSizeF(d->paper_size.height(), d->paper_size.width());
        break;
    }

    case PPK_PageMargins:
    {
        QList<QVariant> margins(value.toList());
        Q_ASSERT(margins.size() == 4);
        int left, top, right, bottom;
        // specified in 1/100 mm
        left = (margins.at(0).toReal()*25.4/72.0) * 100;
        top = (margins.at(1).toReal()*25.4/72.0) * 100;
        right = (margins.at(2).toReal()*25.4/72.0) * 100;
        bottom = (margins.at(3).toReal()*25.4/72.0) * 100;
        d->setPageMargins(left, top, right, bottom);
        break;
    }
    default:
        // Do nothing
        break;
    }
}

QVariant QWin32PrintEngine::property(PrintEnginePropertyKey key) const
{
    Q_D(const QWin32PrintEngine);
    QVariant value;
    switch (key) {

    case PPK_CollateCopies:
        value = false;
        break;

    case PPK_ColorMode:
        {
            if (!d->devMode) {
                value = QPrinter::Color;
            } else {
                value = (d->devMode->dmColor == DMCOLOR_COLOR) ? QPrinter::Color : QPrinter::GrayScale;
            }
        }
        break;

    case PPK_DocumentName:
        value = d->docName;
        break;

    case PPK_FullPage:
        value = d->fullPage;
        break;

    case PPK_CopyCount:
        value = d->num_copies;
        break;

    case PPK_SupportsMultipleCopies:
        value = true;
        break;

    case PPK_NumberOfCopies:
        value = 1;
        break;

    case PPK_Orientation:
        {
            if (!d->devMode) {
                value = QPrinter::Portrait;
            } else {
                value = (d->devMode->dmOrientation == DMORIENT_LANDSCAPE) ? QPrinter::Landscape : QPrinter::Portrait;
            }
        }
        break;

    case PPK_OutputFileName:
        value = d->fileName;
        break;

    case PPK_PageRect:
        if (d->has_custom_paper_size) {
            QRect rect(0, 0,
                       qRound(d->paper_size.width() * d->resolution / 72.0),
                       qRound(d->paper_size.height() * d->resolution / 72.0));
            if (d->pageMarginsSet) {
                rect = rect.adjusted(qRound(mmToInches(d->previousDialogMargins.left()/100.0) * d->resolution),
                                     qRound(mmToInches(d->previousDialogMargins.top()/100.0) * d->resolution),
                                     -qRound(mmToInches(d->previousDialogMargins.width()/100.0) * d->resolution),
                                     -qRound(mmToInches(d->previousDialogMargins.height()/100.0) * d->resolution));
            }
            value = rect;
        } else {
            value = QTransform(1/d->stretch_x, 0, 0, 1/d->stretch_y, 0, 0)
                    .mapRect(d->fullPage ? d->devPhysicalPageRect : d->devPageRect);
        }
        break;

    case PPK_PaperSize:
        if (d->has_custom_paper_size) {
            value = QPrinter::Custom;
        } else {
            if (!d->devMode) {
                value = QPrinter::A4;
            } else {
                value = mapDevmodePaperSize(d->devMode->dmPaperSize);
            }
        }
        break;

    case PPK_PaperRect:
        if (d->has_custom_paper_size) {
            value = QRect(0, 0,
                          qRound(d->paper_size.width() * d->resolution / 72.0),
                          qRound(d->paper_size.height() * d->resolution / 72.0));
        } else {
            value = QTransform(1/d->stretch_x, 0, 0, 1/d->stretch_y, 0, 0).mapRect(d->devPaperRect);
        }
        break;

    case PPK_PaperSource:
        if (!d->devMode) {
            value = QPrinter::Auto;
        } else {
            value = mapDevmodePaperSource(d->devMode->dmDefaultSource);
        }
        break;

    case PPK_PrinterName:
        value = d->name;
        break;

    case PPK_Resolution:
        if (d->resolution || !d->name.isEmpty())
            value = d->resolution;
        break;

    case PPK_SupportedResolutions:
        value = d->queryResolutions();
        break;

    case PPK_WindowsPageSize:
        if (!d->devMode) {
            value = -1;
        } else {
            value = d->devMode->dmPaperSize;
        }
        break;

    case PPK_PaperSources:
        {
            int available = DeviceCapabilities((const wchar_t *)d->name.utf16(),
                                               (const wchar_t *)d->port.utf16(), DC_BINS, 0, d->devMode);

            if (available <= 0)
                break;

            wchar_t *data = new wchar_t[available];
            int count = DeviceCapabilities((const wchar_t *)d->name.utf16(),
                                           (const wchar_t *)d->port.utf16(), DC_BINS, data, d->devMode);

            QList<QVariant> out;
            for (int i=0; i<count; ++i) {
                QPrinter::PaperSource src = mapDevmodePaperSource(data[i]);
                if (src != -1)
                    out << (int) src;
            }
            value = out;

            delete [] data;
        }
        break;

    case PPK_CustomPaperSize:
        value = d->paper_size;
        break;

    case PPK_PageMargins:
    {
        QList<QVariant> margins;
        QRect pageMargins(d->getPageMargins());

        // specified in 1/100 mm
        margins << (mmToInches(pageMargins.left()/100.0) * 72)
                << (mmToInches(pageMargins.top()/100.0) * 72)
                << (mmToInches(pageMargins.width()/100.0) * 72)
                << (mmToInches(pageMargins.height()/100.0) * 72);
        value = margins;
        break;
    }
    default:
        // Do nothing
        break;
    }
    return value;
}

QPrinter::PrinterState QWin32PrintEngine::printerState() const
{
    return d_func()->state;
}

HDC QWin32PrintEngine::getDC() const
{
    return d_func()->hdc;
}

void QWin32PrintEngine::releaseDC(HDC) const
{

}

HGLOBAL *QWin32PrintEnginePrivate::createDevNames()
{
    int size = sizeof(DEVNAMES)
               + program.length() * 2 + 2
               + name.length() * 2 + 2
               + port.length() * 2 + 2;
    HGLOBAL *hGlobal = (HGLOBAL *) GlobalAlloc(GMEM_MOVEABLE, size);
    DEVNAMES *dn = (DEVNAMES*) GlobalLock(hGlobal);

    dn->wDriverOffset = sizeof(DEVNAMES) / sizeof(wchar_t);
    dn->wDeviceOffset = dn->wDriverOffset + program.length() + 1;
    dn->wOutputOffset = dn->wDeviceOffset + name.length() + 1;

    memcpy((ushort*)dn + dn->wDriverOffset, program.utf16(), program.length() * 2 + 2);
    memcpy((ushort*)dn + dn->wDeviceOffset, name.utf16(), name.length() * 2 + 2);
    memcpy((ushort*)dn + dn->wOutputOffset, port.utf16(), port.length() * 2 + 2);
    dn->wDefault = 0;

    GlobalUnlock(hGlobal);

//         printf("QPrintDialogWinPrivate::createDevNames()\n"
//                " -> wDriverOffset: %d\n"
//                " -> wDeviceOffset: %d\n"
//                " -> wOutputOffset: %d\n",
//                dn->wDriverOffset,
//                dn->wDeviceOffset,
//                dn->wOutputOffset);

//         printf("QPrintDialogWinPrivate::createDevNames(): %s, %s, %s\n",
//                QString::fromWCharArray((wchar_t*)(dn) + dn->wDriverOffset).latin1(),
//                QString::fromWCharArray((wchar_t*)(dn) + dn->wDeviceOffset).latin1(),
//                QString::fromWCharArray((wchar_t*)(dn) + dn->wOutputOffset).latin1());

    return hGlobal;
}

void QWin32PrintEnginePrivate::readDevnames(HGLOBAL globalDevnames)
{
    if (globalDevnames) {
        DEVNAMES *dn = (DEVNAMES*) GlobalLock(globalDevnames);
        name = QString::fromWCharArray((wchar_t*)(dn) + dn->wDeviceOffset);
        port = QString::fromWCharArray((wchar_t*)(dn) + dn->wOutputOffset);
        program = QString::fromWCharArray((wchar_t*)(dn) + dn->wDriverOffset);
        GlobalUnlock(globalDevnames);
    }
}

void QWin32PrintEnginePrivate::readDevmode(HGLOBAL globalDevmode)
{
    if (globalDevmode) {
        DEVMODE *dm = (DEVMODE*) GlobalLock(globalDevmode);
        release();
        globalDevMode = globalDevmode;
        devMode = dm;
        hdc = CreateDC(reinterpret_cast<const wchar_t *>(program.utf16()),
                       reinterpret_cast<const wchar_t *>(name.utf16()), 0, dm);

        num_copies = devMode->dmCopies;
        if (!OpenPrinter((wchar_t*)name.utf16(), &hPrinter, 0))
            qWarning("QPrinter: OpenPrinter() failed after reading DEVMODE.");
    }

    if (hdc)
        initHDC();
}

static void draw_text_item_win(const QPointF &pos, const QTextItemInt &ti, HDC hdc,
                               bool convertToText, const QTransform &xform, const QPointF &topLeft)
{
    QFontEngine *fe = ti.fontEngine;
    QPointF baseline_pos = xform.inverted().map(xform.map(pos) - topLeft);

    SetTextAlign(hdc, TA_BASELINE);
    SetBkMode(hdc, TRANSPARENT);

    bool has_kerning = ti.f && ti.f->kerning();
    QFontEngineWin *winfe = (fe->type() == QFontEngine::Win) ? static_cast<QFontEngineWin *>(fe) : 0;

    HFONT hfont;
    bool ttf = false;

    if (winfe) {
        hfont = winfe->hfont;
        ttf = winfe->ttf;
    } else {
        hfont = (HFONT)GetStockObject(ANSI_VAR_FONT);
    }

    HGDIOBJ old_font = SelectObject(hdc, hfont);
    unsigned int options = (ttf && !convertToText) ? ETO_GLYPH_INDEX : 0;
    wchar_t *convertedGlyphs = (wchar_t *)ti.chars;
    QGlyphLayout glyphs = ti.glyphs;

    bool fast = !has_kerning && !(ti.flags & QTextItem::RightToLeft);
    for (int i = 0; fast && i < glyphs.numGlyphs; i++) {
        if (glyphs.offsets[i].x != 0 || glyphs.offsets[i].y != 0 || glyphs.justifications[i].space_18d6 != 0
            || glyphs.attributes[i].dontPrint) {
            fast = false;
            break;
        }
    }

#if !defined(Q_OS_WINCE)
    // Scale, rotate and translate here.
    XFORM win_xform;
    win_xform.eM11 = xform.m11();
    win_xform.eM12 = xform.m12();
    win_xform.eM21 = xform.m21();
    win_xform.eM22 = xform.m22();
    win_xform.eDx = xform.dx();
    win_xform.eDy = xform.dy();

    SetGraphicsMode(hdc, GM_ADVANCED);
    SetWorldTransform(hdc, &win_xform);
#endif

    if (fast) {
        // fast path
        QVarLengthArray<wchar_t> g(glyphs.numGlyphs);
        for (int i = 0; i < glyphs.numGlyphs; ++i)
            g[i] = glyphs.glyphs[i];
        ExtTextOut(hdc,
                   qRound(baseline_pos.x() + glyphs.offsets[0].x.toReal()),
                   qRound(baseline_pos.y() + glyphs.offsets[0].y.toReal()),
                   options, 0, convertToText ? convertedGlyphs : g.data(), glyphs.numGlyphs, 0);
    } else {
        QVarLengthArray<QFixedPoint> positions;
        QVarLengthArray<glyph_t> _glyphs;

        QTransform matrix = QTransform::fromTranslate(baseline_pos.x(), baseline_pos.y());
        ti.fontEngine->getGlyphPositions(ti.glyphs, matrix, ti.flags,
            _glyphs, positions);
        if (_glyphs.size() == 0) {
            SelectObject(hdc, old_font);
            return;
        }

        convertToText = convertToText && glyphs.numGlyphs == _glyphs.size();
        bool outputEntireItem = _glyphs.size() > 0;

        if (outputEntireItem) {
            options |= ETO_PDY;
            QVarLengthArray<INT> glyphDistances(_glyphs.size() * 2);
            QVarLengthArray<wchar_t> g(_glyphs.size());
            for (int i=0; i<_glyphs.size() - 1; ++i) {
                glyphDistances[i * 2] = qRound(positions[i + 1].x) - qRound(positions[i].x);
                glyphDistances[i * 2 + 1] = qRound(positions[i + 1].y) - qRound(positions[i].y);
                g[i] = _glyphs[i];
            }
            glyphDistances[(_glyphs.size() - 1) * 2] = 0;
            glyphDistances[(_glyphs.size() - 1) * 2 + 1] = 0;
            g[_glyphs.size() - 1] = _glyphs[_glyphs.size() - 1];
            ExtTextOut(hdc, qRound(positions[0].x), qRound(positions[0].y), options, 0,
                       convertToText ? convertedGlyphs : g.data(), _glyphs.size(),
                       glyphDistances.data());
        } else {
            int i = 0;
            while(i < _glyphs.size()) {
                wchar_t g = _glyphs[i];

                ExtTextOut(hdc, qRound(positions[i].x),
                           qRound(positions[i].y), options, 0,
                           convertToText ? convertedGlyphs + i : &g, 1, 0);
                ++i;
            }
        }
    }

#if !defined(Q_OS_WINCE)
        win_xform.eM11 = win_xform.eM22 = 1.0;
        win_xform.eM12 = win_xform.eM21 = win_xform.eDx = win_xform.eDy = 0.0;
        SetWorldTransform(hdc, &win_xform);
#endif

    SelectObject(hdc, old_font);
}


void QWin32PrintEnginePrivate::updateCustomPaperSize()
{
    uint paperSize = devMode->dmPaperSize;
    if (paperSize > 0 && mapDevmodePaperSize(paperSize) == QPrinter::Custom) {
        has_custom_paper_size = true;
        DWORD needed = 0;
        DWORD returned = 0;
        if (!EnumForms(hPrinter, 1, 0, 0, &needed, &returned)) {
            BYTE *forms = (BYTE *) malloc(needed);
            if (EnumForms(hPrinter, 1, forms, needed, &needed, &returned)) {
                if (paperSize <= returned) {
                    FORM_INFO_1 *formArray = (FORM_INFO_1 *) forms;
                    int width = formArray[paperSize - 1].Size.cx; // 1/1000 of a mm
                    int height = formArray[paperSize - 1].Size.cy; // 1/1000 of a mm
                    paper_size = QSizeF((width * 72 /25.4) / 1000.0, (height * 72 / 25.4) / 1000.0);
                } else {
                    has_custom_paper_size = false;
                }
            }
            free(forms);
        }
    } else {
        has_custom_paper_size = false;
    }
}

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
