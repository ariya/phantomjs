/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QPRINTER_H
#define QPRINTER_H

#include <QtCore/qstring.h>
#include <QtCore/qscopedpointer.h>
#include <QtGui/qpaintdevice.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_PRINTER

#if defined(B0)
#undef B0 // Terminal hang-up.  We assume that you do not want that.
#endif

class QPrinterPrivate;
class QPaintEngine;
class QPrintEngine;
class QPrinterInfo;

class Q_GUI_EXPORT QPrinter : public QPaintDevice
{
    Q_DECLARE_PRIVATE(QPrinter)
public:
    enum PrinterMode { ScreenResolution, PrinterResolution, HighResolution };

    explicit QPrinter(PrinterMode mode = ScreenResolution);
    explicit QPrinter(const QPrinterInfo& printer, PrinterMode mode = ScreenResolution);
    ~QPrinter();

    int devType() const;

    enum Orientation { Portrait, Landscape };

#ifndef Q_QDOC
    enum PageSize { A4, B5, Letter, Legal, Executive,
                    A0, A1, A2, A3, A5, A6, A7, A8, A9, B0, B1,
                    B10, B2, B3, B4, B6, B7, B8, B9, C5E, Comm10E,
                    DLE, Folio, Ledger, Tabloid, Custom, NPageSize = Custom, NPaperSize = Custom };
    typedef PageSize PaperSize;
#else
    enum PageSize { A4, B5, Letter, Legal, Executive,
                    A0, A1, A2, A3, A5, A6, A7, A8, A9, B0, B1,
                    B10, B2, B3, B4, B6, B7, B8, B9, C5E, Comm10E,
                    DLE, Folio, Ledger, Tabloid, Custom, NPageSize = Custom };
    enum PaperSize { A4, B5, Letter, Legal, Executive,
                     A0, A1, A2, A3, A5, A6, A7, A8, A9, B0, B1,
                     B10, B2, B3, B4, B6, B7, B8, B9, C5E, Comm10E,
                     DLE, Folio, Ledger, Tabloid, Custom, NPageSize = Custom, NPaperSize = Custom };
#endif

    enum PageOrder   { FirstPageFirst,
                       LastPageFirst };

    enum ColorMode   { GrayScale,
                       Color };

    enum PaperSource { OnlyOne,
                       Lower,
                       Middle,
                       Manual,
                       Envelope,
                       EnvelopeManual,
                       Auto,
                       Tractor,
                       SmallFormat,
                       LargeFormat,
                       LargeCapacity,
                       Cassette,
                       FormSource,
                       MaxPageSource
    };

    enum PrinterState { Idle,
                        Active,
                        Aborted,
                        Error };

    enum OutputFormat { NativeFormat, PdfFormat, PostScriptFormat };

    // ### Qt 5: Merge with QAbstractPrintDialog::PrintRange
    enum PrintRange { AllPages, Selection, PageRange, CurrentPage };

    enum Unit {
        Millimeter,
        Point,
        Inch,
        Pica,
        Didot,
        Cicero,
        DevicePixel
    };

    enum DuplexMode {
        DuplexNone = 0,
        DuplexAuto,
        DuplexLongSide,
        DuplexShortSide
    };

#ifdef QT3_SUPPORT
    enum PrinterOption { PrintToFile, PrintSelection, PrintPageRange };
#endif // QT3_SUPPORT

    void setOutputFormat(OutputFormat format);
    OutputFormat outputFormat() const;

    void setPrinterName(const QString &);
    QString printerName() const;

    bool isValid() const;

    void setOutputFileName(const QString &);
    QString outputFileName()const;

    void setPrintProgram(const QString &);
    QString printProgram() const;

    void setDocName(const QString &);
    QString docName() const;

    void setCreator(const QString &);
    QString creator() const;

    void setOrientation(Orientation);
    Orientation orientation() const;

    void setPageSize(PageSize);
    PageSize pageSize() const;

    void setPaperSize(PaperSize);
    PaperSize paperSize() const;

    void setPaperSize(const QSizeF &paperSize, Unit unit);
    QSizeF paperSize(Unit unit) const;

    void setPageOrder(PageOrder);
    PageOrder pageOrder() const;

    void setResolution(int);
    int resolution() const;

    void setColorMode(ColorMode);
    ColorMode colorMode() const;

    void setCollateCopies(bool collate);
    bool collateCopies() const;

    void setFullPage(bool);
    bool fullPage() const;

    void setNumCopies(int);
    int numCopies() const;

    int actualNumCopies() const;

    void setCopyCount(int);
    int copyCount() const;
    bool supportsMultipleCopies() const;

    void setPaperSource(PaperSource);
    PaperSource paperSource() const;

    void setDuplex(DuplexMode duplex);
    DuplexMode duplex() const;

    QList<int> supportedResolutions() const;

#ifdef Q_WS_WIN
    QList<PaperSource> supportedPaperSources() const;
#endif

    void setFontEmbeddingEnabled(bool enable);
    bool fontEmbeddingEnabled() const;

    void setDoubleSidedPrinting(bool enable);
    bool doubleSidedPrinting() const;

#ifdef Q_WS_WIN
    void setWinPageSize(int winPageSize);
    int winPageSize() const;
#endif

    QRect paperRect() const;
    QRect pageRect() const;
    QRectF paperRect(Unit) const;
    QRectF pageRect(Unit) const;

#if !defined(Q_WS_WIN) || defined(qdoc)
    QString printerSelectionOption() const;
    void setPrinterSelectionOption(const QString &);
#endif

    bool newPage();
    bool abort();

    PrinterState printerState() const;

    QPaintEngine *paintEngine() const;
    QPrintEngine *printEngine() const;

#ifdef Q_WS_WIN
    HDC getDC() const;
    void releaseDC(HDC hdc) const;
#endif

    void setFromTo(int fromPage, int toPage);
    int fromPage() const;
    int toPage() const;

    void setPrintRange(PrintRange range);
    PrintRange printRange() const;

    void setPageMargins(qreal left, qreal top, qreal right, qreal bottom, Unit unit);
    void getPageMargins(qreal *left, qreal *top, qreal *right, qreal *bottom, Unit unit) const;

#ifdef QT3_SUPPORT
#ifdef Q_WS_MAC
    QT3_SUPPORT bool pageSetup(QWidget *parent = 0);
    QT3_SUPPORT bool printSetup(QWidget *parent = 0);
#endif

    QT3_SUPPORT bool setup(QWidget *parent = 0);

    QT3_SUPPORT void setMinMax(int minPage, int maxPage);
    QT3_SUPPORT int minPage() const;
    QT3_SUPPORT int maxPage() const;

    QT3_SUPPORT void setCollateCopiesEnabled(bool);
    QT3_SUPPORT bool collateCopiesEnabled() const;

    QT3_SUPPORT void setOptionEnabled(PrinterOption, bool enable);
    QT3_SUPPORT bool isOptionEnabled(PrinterOption) const;

    inline QT3_SUPPORT QSize margins() const;
    inline QT3_SUPPORT void margins(uint *top, uint *left, uint *bottom, uint *right) const;

    inline QT3_SUPPORT bool aborted() { return printerState() == Aborted; }

    QT3_SUPPORT void setOutputToFile(bool);
    inline QT3_SUPPORT bool outputToFile() const { return !outputFileName().isEmpty(); }
#endif

protected:
    int metric(PaintDeviceMetric) const;
    void setEngines(QPrintEngine *printEngine, QPaintEngine *paintEngine);

private:
    void init(PrinterMode mode);

    Q_DISABLE_COPY(QPrinter)

    QScopedPointer<QPrinterPrivate> d_ptr;

    friend class QPrintDialogPrivate;
    friend class QAbstractPrintDialog;
    friend class QAbstractPrintDialogPrivate;
    friend class QPrintPreviewWidgetPrivate;
    friend class QTextDocument;
    friend class QPageSetupWidget;
};

#ifdef QT3_SUPPORT
inline QSize QPrinter::margins() const
{
    QRect page = pageRect();
    QRect paper = paperRect();
    return QSize(page.left() - paper.left(), page.top() - paper.top());
}

inline void QPrinter::margins(uint *top, uint *left, uint *bottom, uint *right) const
{
    QRect page = pageRect();
    QRect paper = paperRect();
    if (top)
        *top = page.top() - paper.top();
    if (left)
        *left = page.left() - paper.left();
    if (bottom)
        *bottom = paper.bottom() - page.bottom();
    if (right)
        *right = paper.right() - page.right();
}
#endif

#endif // QT_NO_PRINTER

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPRINTER_H
