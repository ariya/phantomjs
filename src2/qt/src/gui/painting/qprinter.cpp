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

#include "qprinter_p.h"
#include "qprinter.h"
#include "qprintengine.h"
#include "qprinterinfo.h"
#include "qlist.h"
#include <qpagesetupdialog.h>
#include <qapplication.h>
#include <qfileinfo.h>
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
#include "private/qcups_p.h"
#endif

#ifndef QT_NO_PRINTER

#if defined (Q_WS_WIN)
#include <private/qprintengine_win_p.h>
#elif defined (Q_WS_MAC)
#include <private/qprintengine_mac_p.h>
#elif defined (QTOPIA_PRINTENGINE)
#include <private/qprintengine_qws_p.h>
#endif
#include <private/qprintengine_ps_p.h>

#if defined(Q_WS_X11)
#include <private/qt_x11_p.h>
#endif

#ifndef QT_NO_PDF
#include "qprintengine_pdf_p.h"
#endif

#include <qpicture.h>
#include <private/qpaintengine_preview_p.h>

#if defined(QT3_SUPPORT)
#  include "qprintdialog.h"
#endif // QT3_SUPPORT

QT_BEGIN_NAMESPACE

#define ABORT_IF_ACTIVE(location) \
    if (d->printEngine->printerState() == QPrinter::Active) { \
        qWarning("%s: Cannot be changed while printer is active", location); \
        return; \
    }

// NB! This table needs to be in sync with QPrinter::PaperSize
static const float qt_paperSizes[][2] = {
    {210, 297}, // A4
    {176, 250}, // B5
    {215.9f, 279.4f}, // Letter
    {215.9f, 355.6f}, // Legal
    {190.5f, 254}, // Executive
    {841, 1189}, // A0
    {594, 841}, // A1
    {420, 594}, // A2
    {297, 420}, // A3
    {148, 210}, // A5
    {105, 148}, // A6
    {74, 105}, // A7
    {52, 74}, // A8
    {37, 52}, // A8
    {1000, 1414}, // B0
    {707, 1000}, // B1
    {31, 44}, // B10
    {500, 707}, // B2
    {353, 500}, // B3
    {250, 353}, // B4
    {125, 176}, // B6
    {88, 125}, // B7
    {62, 88}, // B8
    {33, 62}, // B9
    {163, 229}, // C5E
    {105, 241}, // US Common
    {110, 220}, // DLE
    {210, 330}, // Folio
    {431.8f, 279.4f}, // Ledger
    {279.4f, 431.8f} // Tabloid
};

/// return the multiplier of converting from the unit value to postscript-points.
double qt_multiplierForUnit(QPrinter::Unit unit, int resolution)
{
    switch(unit) {
    case QPrinter::Millimeter:
        return 2.83464566929;
    case QPrinter::Point:
        return 1.0;
    case QPrinter::Inch:
        return 72.0;
    case QPrinter::Pica:
        return 12;
    case QPrinter::Didot:
        return 1.065826771;
    case QPrinter::Cicero:
        return 12.789921252;
    case QPrinter::DevicePixel:
        return 72.0/resolution;
    }
    return 1.0;
}

// not static: it's needed in qpagesetupdialog_unix.cpp
QSizeF qt_printerPaperSize(QPrinter::Orientation orientation,
                           QPrinter::PaperSize paperSize,
                           QPrinter::Unit unit,
                           int resolution)
{
    int width_index = 0;
    int height_index = 1;
    if (orientation == QPrinter::Landscape) {
        width_index = 1;
        height_index = 0;
    }
    const qreal multiplier = qt_multiplierForUnit(unit, resolution);
    return QSizeF((qt_paperSizes[paperSize][width_index] * 72 / 25.4) / multiplier,
                  (qt_paperSizes[paperSize][height_index] * 72 / 25.4) / multiplier);
}

void QPrinterPrivate::createDefaultEngines()
{
    QPrinter::OutputFormat realOutputFormat = outputFormat;
#if !defined (QTOPIA_PRINTENGINE)
#if defined (Q_OS_UNIX) && ! defined (Q_WS_MAC)
    if(outputFormat == QPrinter::NativeFormat) {
        realOutputFormat = QPrinter::PostScriptFormat;
    }
#endif
#endif

    switch (realOutputFormat) {
    case QPrinter::NativeFormat: {
#if defined (Q_WS_WIN)
        QWin32PrintEngine *winEngine = new QWin32PrintEngine(printerMode);
        paintEngine = winEngine;
        printEngine = winEngine;
#elif defined (Q_WS_MAC)
        QMacPrintEngine *macEngine = new QMacPrintEngine(printerMode);
        paintEngine = macEngine;
        printEngine = macEngine;
#elif defined (QTOPIA_PRINTENGINE)
        QtopiaPrintEngine *qwsEngine = new QtopiaPrintEngine(printerMode);
        paintEngine = qwsEngine;
        printEngine = qwsEngine;
#elif defined (Q_OS_UNIX)
        Q_ASSERT(false);
#endif
        }
        break;
    case QPrinter::PdfFormat: {
        QPdfEngine *pdfEngine = new QPdfEngine(printerMode);
        paintEngine = pdfEngine;
        printEngine = pdfEngine;
    }
        break;
    case QPrinter::PostScriptFormat: {
        QPSPrintEngine *psEngine = new QPSPrintEngine(printerMode);
        paintEngine = psEngine;
        printEngine = psEngine;
    }
        break;
    }
    use_default_engine = true;
    had_default_engines = true;
}

#ifndef QT_NO_PRINTPREVIEWWIDGET
QList<const QPicture *> QPrinterPrivate::previewPages() const
{
    if (previewEngine)
        return previewEngine->pages();
    return QList<const QPicture *>();
}

void QPrinterPrivate::setPreviewMode(bool enable)
{
    Q_Q(QPrinter);
    if (enable) {
        if (!previewEngine)
            previewEngine = new QPreviewPaintEngine;
        had_default_engines = use_default_engine;
        use_default_engine = false;
        realPrintEngine = printEngine;
        realPaintEngine = paintEngine;
        q->setEngines(previewEngine, previewEngine);
        previewEngine->setProxyEngines(realPrintEngine, realPaintEngine);
    } else {
        q->setEngines(realPrintEngine, realPaintEngine);
        use_default_engine = had_default_engines;
    }
}
#endif // QT_NO_PRINTPREVIEWWIDGET

void QPrinterPrivate::addToManualSetList(QPrintEngine::PrintEnginePropertyKey key)
{
    for (int c = 0; c < manualSetList.size(); ++c) {
        if (manualSetList[c] == key) return;
    }
    manualSetList.append(key);
}


/*!
  \class QPrinter
  \reentrant

  \brief The QPrinter class is a paint device that paints on a printer.

  \ingroup printing


  This device represents a series of pages of printed output, and is
  used in almost exactly the same way as other paint devices such as
  QWidget and QPixmap.
  A set of additional functions are provided to manage device-specific
  features, such as orientation and resolution, and to step through
  the pages in a document as it is generated.

  When printing directly to a printer on Windows or Mac OS X, QPrinter uses
  the built-in printer drivers. On X11, QPrinter uses the
  \l{Common Unix Printing System (CUPS)} or the standard Unix \l lpr utility
  to send PostScript or PDF output to the printer. As an alternative,
  the printProgram() function can be used to specify the command or utility
  to use instead of the system default.

  Note that setting parameters like paper size and resolution on an
  invalid printer is undefined. You can use QPrinter::isValid() to
  verify this before changing any parameters.

  QPrinter supports a number of parameters, most of which can be
  changed by the end user through a \l{QPrintDialog}{print dialog}. In
  general, QPrinter passes these functions onto the underlying QPrintEngine.

  The most important parameters are:
  \list
  \i setOrientation() tells QPrinter which page orientation to use.
  \i setPaperSize() tells QPrinter what paper size to expect from the
  printer.
  \i setResolution() tells QPrinter what resolution you wish the
  printer to provide, in dots per inch (DPI).
  \i setFullPage() tells QPrinter whether you want to deal with the
  full page or just with the part the printer can draw on.
  \i setCopyCount() tells QPrinter how many copies of the document
  it should print.
  \endlist

  Many of these functions can only be called before the actual printing
  begins (i.e., before QPainter::begin() is called). This usually makes
  sense because, for example, it's not possible to change the number of
  copies when you are halfway through printing. There are also some
  settings that the user sets (through the printer dialog) and that
  applications are expected to obey. See QAbstractPrintDialog's
  documentation for more details.

  When QPainter::begin() is called, the QPrinter it operates on is prepared for
  a new page, enabling the QPainter to be used immediately to paint the first
  page in a document. Once the first page has been painted, newPage() can be
  called to request a new blank page to paint on, or QPainter::end() can be
  called to finish printing. The second page and all following pages are
  prepared using a call to newPage() before they are painted.

  The first page in a document does not need to be preceded by a call to
  newPage(). You only need to calling newPage() after QPainter::begin() if you
  need to insert a blank page at the beginning of a printed document.
  Similarly, calling newPage() after the last page in a document is painted will
  result in a trailing blank page appended to the end of the printed document.

  If you want to abort the print job, abort() will try its best to
  stop printing. It may cancel the entire job or just part of it.

  Since QPrinter can print to any QPrintEngine subclass, it is possible to
  extend printing support to cover new types of printing subsystem by
  subclassing QPrintEngine and reimplementing its interface.

  \sa QPrintDialog, {Printing with Qt}
*/

/*!
    \enum QPrinter::PrinterState

    \value Idle
    \value Active
    \value Aborted
    \value Error
*/

/*!
    \enum QPrinter::PrinterMode

    This enum describes the mode the printer should work in. It
    basically presets a certain resolution and working mode.

    \value ScreenResolution Sets the resolution of the print device to
    the screen resolution. This has the big advantage that the results
    obtained when painting on the printer will match more or less
    exactly the visible output on the screen. It is the easiest to
    use, as font metrics on the screen and on the printer are the
    same. This is the default value. ScreenResolution will produce a
    lower quality output than HighResolution and should only be used
    for drafts.

    \value PrinterResolution This value is deprecated. Is is
    equivalent to ScreenResolution on Unix and HighResolution on
    Windows and Mac. Due do the difference between ScreenResolution
    and HighResolution, use of this value may lead to non-portable
    printer code.

    \value HighResolution On Windows, sets the printer resolution to that
    defined for the printer in use. For PostScript printing, sets the
    resolution of the PostScript driver to 1200 dpi.

    \note When rendering text on a QPrinter device, it is important
    to realize that the size of text, when specified in points, is
    independent of the resolution specified for the device itself.
    Therefore, it may be useful to specify the font size in pixels
    when combining text with graphics to ensure that their relative
    sizes are what you expect.
*/

/*!
  \enum QPrinter::Orientation

  This enum type (not to be confused with \c Orientation) is used
  to specify each page's orientation.

  \value Portrait the page's height is greater than its width.

  \value Landscape the page's width is greater than its height.

  This type interacts with \l QPrinter::PaperSize and
  QPrinter::setFullPage() to determine the final size of the page
  available to the application.
*/


/*!
    \enum QPrinter::PrintRange

    Used to specify the print range selection option.

    \value AllPages All pages should be printed.
    \value Selection Only the selection should be printed.
    \value PageRange The specified page range should be printed.
    \value CurrentPage Only the current page should be printed.

    \sa QAbstractPrintDialog::PrintRange
*/

/*!
    \enum QPrinter::PrinterOption
    \compat

    Use QAbstractPrintDialog::PrintDialogOption instead.

    \value PrintToFile
    \value PrintSelection
    \value PrintPageRange
*/

/*!
  \enum QPrinter::PageSize

  \obsolete
  Use QPrinter::PaperSize instead.

  \value A0 841 x 1189 mm
  \value A1 594 x 841 mm
  \value A2 420 x 594 mm
  \value A3 297 x 420 mm
  \value A4 210 x 297 mm, 8.26 x 11.69 inches
  \value A5 148 x 210 mm
  \value A6 105 x 148 mm
  \value A7 74 x 105 mm
  \value A8 52 x 74 mm
  \value A9 37 x 52 mm
  \value B0 1030 x 1456 mm
  \value B1 728 x 1030 mm
  \value B10 32 x 45 mm
  \value B2 515 x 728 mm
  \value B3 364 x 515 mm
  \value B4 257 x 364 mm
  \value B5 182 x 257 mm, 7.17 x 10.13 inches
  \value B6 128 x 182 mm
  \value B7 91 x 128 mm
  \value B8 64 x 91 mm
  \value B9 45 x 64 mm
  \value C5E 163 x 229 mm
  \value Comm10E 105 x 241 mm, U.S. Common 10 Envelope
  \value DLE 110 x 220 mm
  \value Executive 7.5 x 10 inches, 191 x 254 mm
  \value Folio 210 x 330 mm
  \value Ledger 432 x 279 mm
  \value Legal 8.5 x 14 inches, 216 x 356 mm
  \value Letter 8.5 x 11 inches, 216 x 279 mm
  \value Tabloid 279 x 432 mm
  \value Custom Unknown, or a user defined size.

  \omitvalue NPageSize
  */

/*!
  \enum QPrinter::PaperSize
  \since 4.4

  This enum type specifies what paper size QPrinter should use.
  QPrinter does not check that the paper size is available; it just
  uses this information, together with QPrinter::Orientation and
  QPrinter::setFullPage(), to determine the printable area.

  The defined sizes (with setFullPage(true)) are:

  \value A0 841 x 1189 mm
  \value A1 594 x 841 mm
  \value A2 420 x 594 mm
  \value A3 297 x 420 mm
  \value A4 210 x 297 mm, 8.26 x 11.69 inches
  \value A5 148 x 210 mm
  \value A6 105 x 148 mm
  \value A7 74 x 105 mm
  \value A8 52 x 74 mm
  \value A9 37 x 52 mm
  \value B0 1000 x 1414 mm
  \value B1 707 x 1000 mm
  \value B2 500 x 707 mm
  \value B3 353 x 500 mm
  \value B4 250 x 353 mm
  \value B5 176 x 250 mm, 6.93 x 9.84 inches
  \value B6 125 x 176 mm
  \value B7 88 x 125 mm
  \value B8 62 x 88 mm
  \value B9 33 x 62 mm
  \value B10 31 x 44 mm
  \value C5E 163 x 229 mm
  \value Comm10E 105 x 241 mm, U.S. Common 10 Envelope
  \value DLE 110 x 220 mm
  \value Executive 7.5 x 10 inches, 190.5 x 254 mm
  \value Folio 210 x 330 mm
  \value Ledger 431.8 x 279.4 mm
  \value Legal 8.5 x 14 inches, 215.9 x 355.6 mm
  \value Letter 8.5 x 11 inches, 215.9 x 279.4 mm
  \value Tabloid 279.4 x 431.8 mm
  \value Custom Unknown, or a user defined size.

  With setFullPage(false) (the default), the metrics will be a bit
  smaller; how much depends on the printer in use.

  \omitvalue NPageSize
  \omitvalue NPaperSize
*/


/*!
  \enum QPrinter::PageOrder

  This enum type is used by QPrinter to tell the application program
  how to print.

  \value FirstPageFirst  the lowest-numbered page should be printed
  first.

  \value LastPageFirst  the highest-numbered page should be printed
  first.
*/

/*!
  \enum QPrinter::ColorMode

  This enum type is used to indicate whether QPrinter should print
  in color or not.

  \value Color  print in color if available, otherwise in grayscale.

  \value GrayScale  print in grayscale, even on color printers.
*/

/*!
  \enum QPrinter::PaperSource

  This enum type specifies what paper source QPrinter is to use.
  QPrinter does not check that the paper source is available; it
  just uses this information to try and set the paper source.
  Whether it will set the paper source depends on whether the
  printer has that particular source.

  \warning This is currently only implemented for Windows.

  \value Auto
  \value Cassette
  \value Envelope
  \value EnvelopeManual
  \value FormSource
  \value LargeCapacity
  \value LargeFormat
  \value Lower
  \value MaxPageSource
  \value Middle
  \value Manual
  \value OnlyOne
  \value Tractor
  \value SmallFormat
*/

/*!
  \enum QPrinter::Unit
  \since 4.4

  This enum type is used to specify the measurement unit for page and
  paper sizes.

  \value Millimeter
  \value Point
  \value Inch
  \value Pica
  \value Didot
  \value Cicero
  \value DevicePixel

  Note the difference between Point and DevicePixel. The Point unit is
  defined to be 1/72th of an inch, while the DevicePixel unit is
  resolution dependant and is based on the actual pixels, or dots, on
  the printer.
*/


/*
  \enum QPrinter::PrintRange

  This enum is used to specify which print range the application
  should use to print.

  \value AllPages All the pages should be printed.
  \value Selection Only the selection should be printed.
  \value PageRange Print according to the from page and to page options.
  \value CurrentPage Only the current page should be printed.

  \sa setPrintRange(), printRange()
*/

/*
  \enum QPrinter::PrinterOption

  This enum describes various printer options that appear in the
  printer setup dialog. It is used to enable and disable these
  options in the setup dialog.

  \value PrintToFile Describes if print to file should be enabled.
  \value PrintSelection Describes if printing selections should be enabled.
  \value PrintPageRange Describes if printing page ranges (from, to) should
  be enabled
  \value PrintCurrentPage if Print Current Page option should be enabled

  \sa setOptionEnabled(), isOptionEnabled()
*/

/*!
    Creates a new printer object with the given \a mode.
*/
QPrinter::QPrinter(PrinterMode mode)
    : QPaintDevice(),
      d_ptr(new QPrinterPrivate(this))
{
    init(mode);
    QPrinterInfo defPrn(QPrinterInfo::defaultPrinter());
    if (!defPrn.isNull()) {
        setPrinterName(defPrn.printerName());
    } else if (QPrinterInfo::availablePrinters().isEmpty()
               && d_ptr->paintEngine->type() != QPaintEngine::Windows
               && d_ptr->paintEngine->type() != QPaintEngine::MacPrinter) {
        setOutputFormat(QPrinter::PdfFormat);
    }
}

/*!
    \since 4.4

    Creates a new printer object with the given \a printer and \a mode.
*/
QPrinter::QPrinter(const QPrinterInfo& printer, PrinterMode mode)
    : QPaintDevice(),
      d_ptr(new QPrinterPrivate(this))
{
    init(mode);
    setPrinterName(printer.printerName());
}

void QPrinter::init(PrinterMode mode)
{
#if !defined(Q_WS_X11)
    if (!qApp) {
#else
    if (!qApp || !X11) {
#endif
        qFatal("QPrinter: Must construct a QApplication before a QPaintDevice");
        return;
    }
    Q_D(QPrinter);

    d->printerMode = mode;
    d->outputFormat = QPrinter::NativeFormat;
    d->createDefaultEngines();

#ifndef QT_NO_PRINTPREVIEWWIDGET
    d->previewEngine = 0;
#endif
    d->realPrintEngine = 0;
    d->realPaintEngine = 0;

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    if (QCUPSSupport::cupsVersion() >= 10200 && QCUPSSupport().currentPPD()) {
        setOutputFormat(QPrinter::PdfFormat);
        d->outputFormat = QPrinter::NativeFormat;
    }
#endif
}

/*!
    This function is used by subclasses of QPrinter to specify custom
    print and paint engines (\a printEngine and \a paintEngine,
    respectively).

    QPrinter does not take ownership of the engines, so you need to
    manage these engine instances yourself.

    Note that changing the engines will reset the printer state and
    all its properties.

    \sa printEngine() paintEngine() setOutputFormat()

    \since 4.1
*/
void QPrinter::setEngines(QPrintEngine *printEngine, QPaintEngine *paintEngine)
{
    Q_D(QPrinter);

    if (d->use_default_engine)
        delete d->printEngine;

    d->printEngine = printEngine;
    d->paintEngine = paintEngine;
    d->use_default_engine = false;
}

/*!
    Destroys the printer object and frees any allocated resources. If
    the printer is destroyed while a print job is in progress this may
    or may not affect the print job.
*/
QPrinter::~QPrinter()
{
    Q_D(QPrinter);
    if (d->use_default_engine)
        delete d->printEngine;
#ifndef QT_NO_PRINTPREVIEWWIDGET
    delete d->previewEngine;
#endif
}

/*!
    \enum QPrinter::OutputFormat

    The OutputFormat enum is used to describe the format QPrinter should
    use for printing.

    \value NativeFormat QPrinter will print output using a method defined
    by the platform it is running on. This mode is the default when printing
    directly to a printer.

    \value PdfFormat QPrinter will generate its output as a searchable PDF file.
    This mode is the default when printing to a file.

    \value PostScriptFormat QPrinter will generate its output as in the PostScript format.
    (This feature was introduced in Qt 4.2.)

    \sa outputFormat(), setOutputFormat(), setOutputFileName()
*/

/*!
    \since 4.1

    Sets the output format for this printer to \a format.
*/
void QPrinter::setOutputFormat(OutputFormat format)
{

#ifndef QT_NO_PDF
    Q_D(QPrinter);
    if (d->validPrinter && d->outputFormat == format)
        return;
    d->outputFormat = format;

    QPrintEngine *oldPrintEngine = d->printEngine;
    const bool def_engine = d->use_default_engine;
    d->printEngine = 0;

    d->createDefaultEngines();

    if (oldPrintEngine) {
        for (int i = 0; i < d->manualSetList.size(); ++i) {
            QPrintEngine::PrintEnginePropertyKey key = d->manualSetList[i];
            QVariant prop;
            // PPK_NumberOfCopies need special treatmeant since it in most cases
            // will return 1, disregarding the actual value that was set
            if (key == QPrintEngine::PPK_NumberOfCopies)
                prop = QVariant(copyCount());
            else
                prop = oldPrintEngine->property(key);
            if (prop.isValid())
                d->printEngine->setProperty(key, prop);
        }
    }

    if (def_engine)
        delete oldPrintEngine;

    if (d->outputFormat == QPrinter::PdfFormat || d->outputFormat == QPrinter::PostScriptFormat)
        d->validPrinter = true;
#else
    Q_UNUSED(format);
#endif
}

/*!
    \since 4.1

    Returns the output format for this printer.
*/
QPrinter::OutputFormat QPrinter::outputFormat() const
{
    Q_D(const QPrinter);
    return d->outputFormat;
}



/*! \internal
*/
int QPrinter::devType() const
{
    return QInternal::Printer;
}

/*!
    Returns the printer name. This value is initially set to the name
    of the default printer.

    \sa setPrinterName()
*/
QString QPrinter::printerName() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_PrinterName).toString();
}

/*!
    Sets the printer name to \a name.

    \sa printerName(), isValid()
*/
void QPrinter::setPrinterName(const QString &name)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setPrinterName");

#if defined(Q_OS_UNIX) && !defined(QT_NO_CUPS)
    if(d->use_default_engine
        && d->outputFormat == QPrinter::NativeFormat) {
        if (QCUPSSupport::cupsVersion() >= 10200
            && QCUPSSupport::printerHasPPD(name.toLocal8Bit().constData()))
            setOutputFormat(QPrinter::PdfFormat);
        else
            setOutputFormat(QPrinter::PostScriptFormat);
        d->outputFormat = QPrinter::NativeFormat;
    }
#endif

    QList<QPrinterInfo> prnList = QPrinterInfo::availablePrinters();
    if (name.isEmpty()) {
        d->validPrinter = d->outputFormat == QPrinter::PdfFormat || d->outputFormat == QPrinter::PostScriptFormat;
    } else {
        d->validPrinter = false;
        for (int i = 0; i < prnList.size(); ++i) {
            if (prnList[i].printerName() == name) {
                d->validPrinter = true;
                break;
            }
        }
    }

    d->printEngine->setProperty(QPrintEngine::PPK_PrinterName, name);
    d->addToManualSetList(QPrintEngine::PPK_PrinterName);
}


/*!
  \since 4.4

  Returns true if the printer currently selected is a valid printer
  in the system, or a pure PDF/PostScript printer; otherwise returns false.

  To detect other failures check the output of QPainter::begin() or QPrinter::newPage().

  \snippet doc/src/snippets/printing-qprinter/errors.cpp 0

  \sa setPrinterName()
*/
bool QPrinter::isValid() const
{
    Q_D(const QPrinter);
#if defined(Q_WS_X11)
    if (!qApp || !X11) {
        return false;
    }
#endif
    return d->validPrinter;
}


/*!
  \fn bool QPrinter::outputToFile() const

  Returns true if the output should be written to a file, or false
  if the output should be sent directly to the printer. The default
  setting is false.

  \sa setOutputToFile(), setOutputFileName()
*/


/*!
  \fn void QPrinter::setOutputToFile(bool enable)

  Specifies whether the output should be written to a file or sent
  directly to the printer.

  Will output to a file if \a enable is true, or will output
  directly to the printer if \a enable is false.

  \sa outputToFile(), setOutputFileName()
*/


/*!
  \fn QString QPrinter::outputFileName() const

  Returns the name of the output file. By default, this is an empty string
  (indicating that the printer shouldn't print to file).

  \sa QPrintEngine::PrintEnginePropertyKey

*/

QString QPrinter::outputFileName() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_OutputFileName).toString();
}

/*!
    Sets the name of the output file to \a fileName.

    Setting a null or empty name (0 or "") disables printing to a file.
    Setting a non-empty name enables printing to a file.

    This can change the value of outputFormat().  If the file name has the
    suffix ".ps" then PostScript is automatically selected as output format.
    If the file name has the ".pdf" suffix PDF is generated. If the file name
    has a suffix other than ".ps" and ".pdf", the output format used is the
    one set with setOutputFormat().

    QPrinter uses Qt's cross-platform PostScript or PDF print engines
    respectively. If you can produce this format natively, for example
    Mac OS X can generate PDF's from its print engine, set the output format
    back to NativeFormat.

    \sa outputFileName() setOutputToFile() setOutputFormat()
*/

void QPrinter::setOutputFileName(const QString &fileName)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setOutputFileName");

    QFileInfo fi(fileName);
    if (!fi.suffix().compare(QLatin1String("ps"), Qt::CaseInsensitive))
        setOutputFormat(QPrinter::PostScriptFormat);
    else if (!fi.suffix().compare(QLatin1String("pdf"), Qt::CaseInsensitive))
        setOutputFormat(QPrinter::PdfFormat);
    else if (fileName.isEmpty())
        setOutputFormat(QPrinter::NativeFormat);

    d->printEngine->setProperty(QPrintEngine::PPK_OutputFileName, fileName);
    d->addToManualSetList(QPrintEngine::PPK_OutputFileName);
}

/*!
    Add a section to the document outline. All following sections will be added
    to as subsections to this section, until endSectionOutline() has been called.

    \a name is the name of the added section. \a anchor is the name of an anchor
    indicating the beginning of the section.  This anchor must be added by calling
    QPainter::addAnchor().

    Note that for output formats not supporting outlines, currently all other then PDF,
    this call has no effect.

    \sa endSectionOutline() QPainter::addAnchor() 

    \since 4.7
*/
void QPrinter::beginSectionOutline(const QString &name, const QString &anchor)
{
    Q_D(QPrinter);
    d->printEngine->beginSectionOutline(name, anchor);
}

/*!
    End the current section.

    \sa beginSectionOutline()

    \since 4.7
*/
void QPrinter::endSectionOutline() 
{
    Q_D(QPrinter);
    d->printEngine->endSectionOutline();
}

/*!
  Returns the name of the program that sends the print output to the
  printer.

  The default is to return an empty string; meaning that QPrinter will try to
  be smart in a system-dependent way. On X11 only, you can set it to something
  different to use a specific print program. On the other platforms, this
  returns an empty string.

  \sa setPrintProgram(), setPrinterSelectionOption()
*/
QString QPrinter::printProgram() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_PrinterProgram).toString();
}


/*!
  Sets the name of the program that should do the print job to \a
  printProg.

  On X11, this function sets the program to call with the PostScript
  output. On other platforms, it has no effect.

  \sa printProgram()
*/
void QPrinter::setPrintProgram(const QString &printProg)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setPrintProgram");
    d->printEngine->setProperty(QPrintEngine::PPK_PrinterProgram, printProg);
    d->addToManualSetList(QPrintEngine::PPK_PrinterProgram);
}


/*!
  Returns the document name.

  \sa setDocName(), QPrintEngine::PrintEnginePropertyKey
*/
QString QPrinter::docName() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_DocumentName).toString();
}


/*!
  Sets the document name to \a name.

  On X11, the document name is for example used as the default
  output filename in QPrintDialog. Note that the document name does
  not affect the file name if the printer is printing to a file.
  Use the setOutputFile() function for this.

  \sa docName(), QPrintEngine::PrintEnginePropertyKey
*/
void QPrinter::setDocName(const QString &name)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setDocName");
    d->printEngine->setProperty(QPrintEngine::PPK_DocumentName, name);
    d->addToManualSetList(QPrintEngine::PPK_DocumentName);
}


/*!
  Returns the name of the application that created the document.

  \sa setCreator()
*/
QString QPrinter::creator() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_Creator).toString();
}


/*!
  Sets the name of the application that created the document to \a
  creator.

  This function is only applicable to the X11 version of Qt. If no
  creator name is specified, the creator will be set to "Qt"
  followed by some version number.

  \sa creator()
*/
void QPrinter::setCreator(const QString &creator)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setCreator");
    d->printEngine->setProperty(QPrintEngine::PPK_Creator, creator);
    d->addToManualSetList(QPrintEngine::PPK_Creator);
}


/*!
  Returns the orientation setting. This is driver-dependent, but is usually
  QPrinter::Portrait.

  \sa setOrientation()
*/
QPrinter::Orientation QPrinter::orientation() const
{
    Q_D(const QPrinter);
    return QPrinter::Orientation(d->printEngine->property(QPrintEngine::PPK_Orientation).toInt());
}


/*!
  Sets the print orientation to \a orientation.

  The orientation can be either QPrinter::Portrait or
  QPrinter::Landscape.

  The printer driver reads this setting and prints using the
  specified orientation.

  On Windows, this option can be changed while printing and will
  take effect from the next call to newPage().

  On Mac OS X, changing the orientation during a print job has no effect.

  \sa orientation()
*/

void QPrinter::setOrientation(Orientation orientation)
{
    Q_D(QPrinter);
    d->printEngine->setProperty(QPrintEngine::PPK_Orientation, orientation);
    d->addToManualSetList(QPrintEngine::PPK_Orientation);
}


/*!
    \since 4.4
    Returns the printer paper size. The default value is driver-dependent.

    \sa setPaperSize() pageRect() paperRect()
*/

QPrinter::PaperSize QPrinter::paperSize() const
{
    Q_D(const QPrinter);
    return QPrinter::PaperSize(d->printEngine->property(QPrintEngine::PPK_PaperSize).toInt());
}

/*!
    \since 4.4

    Sets the printer paper size to \a newPaperSize if that size is
    supported. The result is undefined if \a newPaperSize is not
    supported.

    The default paper size is driver-dependent.

    This function is useful mostly for setting a default value that
    the user can override in the print dialog.

    \sa paperSize() PaperSize setFullPage() setResolution() pageRect() paperRect()
*/
void QPrinter::setPaperSize(PaperSize newPaperSize)
{
    Q_D(QPrinter);
    if (d->paintEngine->type() != QPaintEngine::Pdf)
        ABORT_IF_ACTIVE("QPrinter::setPaperSize");
    if (newPaperSize < 0 || newPaperSize >= NPaperSize) {
        qWarning("QPrinter::setPaperSize: Illegal paper size %d", newPaperSize);
        return;
    }
    d->printEngine->setProperty(QPrintEngine::PPK_PaperSize, newPaperSize);
    d->addToManualSetList(QPrintEngine::PPK_PaperSize);
    d->hasUserSetPageSize = true;
}

/*!
    \obsolete

    Returns the printer page size. The default value is driver-dependent.

    Use paperSize() instead.
*/
QPrinter::PageSize QPrinter::pageSize() const
{
    return paperSize();
}


/*!
    \obsolete

    Sets the printer page size based on \a newPageSize.

    Use setPaperSize() instead.
*/

void QPrinter::setPageSize(PageSize newPageSize)
{
    setPaperSize(newPageSize);
}

/*!
    \since 4.4

    Sets the paper size based on \a paperSize in \a unit.

    \sa paperSize()
*/

void QPrinter::setPaperSize(const QSizeF &paperSize, QPrinter::Unit unit)
{
    Q_D(QPrinter);
    if (d->paintEngine->type() != QPaintEngine::Pdf)
        ABORT_IF_ACTIVE("QPrinter::setPaperSize");
    const qreal multiplier = qt_multiplierForUnit(unit, resolution());
    QSizeF size(paperSize.width() * multiplier, paperSize.height() * multiplier);
    d->printEngine->setProperty(QPrintEngine::PPK_CustomPaperSize, size);
    d->addToManualSetList(QPrintEngine::PPK_CustomPaperSize);
    d->hasUserSetPageSize = true;
}

/*!
    \since 4.4

    Returns the paper size in \a unit.

    \sa setPaperSize()
*/

QSizeF QPrinter::paperSize(Unit unit) const
{
    Q_D(const QPrinter);
    int res = resolution();
    const qreal multiplier = qt_multiplierForUnit(unit, res);
    PaperSize paperType = paperSize();
    if (paperType == Custom) {
        QSizeF size = d->printEngine->property(QPrintEngine::PPK_CustomPaperSize).toSizeF();
        return QSizeF(size.width() / multiplier, size.height() / multiplier);
    }
    else {
        return qt_printerPaperSize(orientation(), paperType, unit, res);
    }
}

/*!
    Sets the page order to \a pageOrder.

    The page order can be QPrinter::FirstPageFirst or
    QPrinter::LastPageFirst. The application is responsible for
    reading the page order and printing accordingly.

    This function is mostly useful for setting a default value that
    the user can override in the print dialog.

    This function is only supported under X11.
*/

void QPrinter::setPageOrder(PageOrder pageOrder)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setPageOrder");
    d->printEngine->setProperty(QPrintEngine::PPK_PageOrder, pageOrder);
    d->addToManualSetList(QPrintEngine::PPK_PageOrder);
}


/*!
  Returns the current page order.

  The default page order is \c FirstPageFirst.
*/

QPrinter::PageOrder QPrinter::pageOrder() const
{
    Q_D(const QPrinter);
    return QPrinter::PageOrder(d->printEngine->property(QPrintEngine::PPK_PageOrder).toInt());
}


/*!
  Sets the printer's color mode to \a newColorMode, which can be
  either \c Color or \c GrayScale.

  \sa colorMode()
*/

void QPrinter::setColorMode(ColorMode newColorMode)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setColorMode");
    d->printEngine->setProperty(QPrintEngine::PPK_ColorMode, newColorMode);
    d->addToManualSetList(QPrintEngine::PPK_ColorMode);
}


/*!
  Returns the current color mode.

  \sa setColorMode()
*/
QPrinter::ColorMode QPrinter::colorMode() const
{
    Q_D(const QPrinter);
    return QPrinter::ColorMode(d->printEngine->property(QPrintEngine::PPK_ColorMode).toInt());
}


/*!
  \obsolete
  Returns the number of copies to be printed. The default value is 1.

  On Windows, Mac OS X and X11 systems that support CUPS, this will always
  return 1 as these operating systems can internally handle the number
  of copies.

  On X11, this value will return the number of times the application is
  required to print in order to match the number specified in the printer setup
  dialog. This has been done since some printer drivers are not capable of
  buffering up the copies and in those cases the application must make an
  explicit call to the print code for each copy.

  Use copyCount() in conjunction with supportsMultipleCopies() instead.

  \sa setNumCopies(), actualNumCopies()
*/

int QPrinter::numCopies() const
{
    Q_D(const QPrinter);
   return d->printEngine->property(QPrintEngine::PPK_NumberOfCopies).toInt();
}


/*!
    \obsolete
    \since 4.6

    Returns the number of copies that will be printed. The default
    value is 1.

    This function always returns the actual value specified in the print
    dialog or using setNumCopies().

    Use copyCount() instead.

    \sa setNumCopies(), numCopies()
*/
int QPrinter::actualNumCopies() const
{
    return copyCount();
}



/*!
  \obsolete
  Sets the number of copies to be printed to \a numCopies.

  The printer driver reads this setting and prints the specified
  number of copies.

  Use setCopyCount() instead.

  \sa numCopies()
*/

void QPrinter::setNumCopies(int numCopies)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setNumCopies");
    d->printEngine->setProperty(QPrintEngine::PPK_NumberOfCopies, numCopies);
    d->addToManualSetList(QPrintEngine::PPK_NumberOfCopies);
}

/*!
    \since 4.7

    Sets the number of copies to be printed to \a count.

    The printer driver reads this setting and prints the specified number of
    copies.

    \sa copyCount(), supportsMultipleCopies()
*/

void QPrinter::setCopyCount(int count)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setCopyCount;");
    d->printEngine->setProperty(QPrintEngine::PPK_CopyCount, count);
    d->addToManualSetList(QPrintEngine::PPK_CopyCount);
}

/*!
    \since 4.7

    Returns the number of copies that will be printed. The default value is 1.

    \sa setCopyCount(), supportsMultipleCopies()
*/

int QPrinter::copyCount() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_CopyCount).toInt();
}

/*!
    \since 4.7

    Returns true if the printer supports printing multiple copies of the same
    document in one job; otherwise false is returned.

    On most systems this function will return true. However, on X11 systems
    that do not support CUPS, this function will return false. That means the
    application has to handle the number of copies by printing the same
    document the required number of times.

    \sa setCopyCount(), copyCount()
*/

bool QPrinter::supportsMultipleCopies() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_SupportsMultipleCopies).toBool();
}

/*!
    \since 4.1

    Returns true if collation is turned on when multiple copies is selected.
    Returns false if it is turned off when multiple copies is selected.
    When collating is turned off the printing of each individual page will be repeated
    the numCopies() amount before the next page is started. With collating turned on
    all pages are printed before the next copy of those pages is started.

    \sa setCollateCopies()
*/
bool QPrinter::collateCopies() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_CollateCopies).toBool();
}


/*!
    \since 4.1

    Sets the default value for collation checkbox when the print
    dialog appears.  If \a collate is true, it will enable
    setCollateCopiesEnabled().  The default value is false. This value
    will be changed by what the user presses in the print dialog.

    \sa collateCopies()
*/
void QPrinter::setCollateCopies(bool collate)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setCollateCopies");
    d->printEngine->setProperty(QPrintEngine::PPK_CollateCopies, collate);
    d->addToManualSetList(QPrintEngine::PPK_CollateCopies);
}



/*!
  If \a fp is true, enables support for painting over the entire page;
  otherwise restricts painting to the printable area reported by the
  device.

  By default, full page printing is disabled. In this case, the origin
  of the QPrinter's coordinate system coincides with the top-left
  corner of the printable area.

  If full page printing is enabled, the origin of the QPrinter's
  coordinate system coincides with the top-left corner of the paper
  itself. In this case, the
  \l{QPaintDevice::PaintDeviceMetric}{device metrics} will report
  the exact same dimensions as indicated by \l{PaperSize}. It may not
  be possible to print on the entire physical page because of the
  printer's margins, so the application must account for the margins
  itself.

  \sa fullPage(), setPaperSize(), width(), height(), {Printing with Qt}
*/

void QPrinter::setFullPage(bool fp)
{
    Q_D(QPrinter);
    d->printEngine->setProperty(QPrintEngine::PPK_FullPage, fp);
    d->addToManualSetList(QPrintEngine::PPK_FullPage);
}


/*!
  Returns true if the origin of the printer's coordinate system is
  at the corner of the page and false if it is at the edge of the
  printable area.

  See setFullPage() for details and caveats.

  \sa setFullPage() PaperSize
*/

bool QPrinter::fullPage() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_FullPage).toBool();
}


/*!
  Requests that the printer prints at \a dpi or as near to \a dpi as
  possible.

  This setting affects the coordinate system as returned by, for
  example QPainter::viewport().

  This function must be called before QPainter::begin() to have an effect on
  all platforms.

  \sa resolution() setPaperSize()
*/

void QPrinter::setResolution(int dpi)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setResolution");
    d->printEngine->setProperty(QPrintEngine::PPK_Resolution, dpi);
    d->addToManualSetList(QPrintEngine::PPK_Resolution);
}


/*!
  Returns the current assumed resolution of the printer, as set by
  setResolution() or by the printer driver.

  \sa setResolution()
*/

int QPrinter::resolution() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_Resolution).toInt();
}

/*!
  Sets the paper source setting to \a source.

  Windows only: This option can be changed while printing and will
  take effect from the next call to newPage()

  \sa paperSource()
*/

void QPrinter::setPaperSource(PaperSource source)
{
    Q_D(QPrinter);
    d->printEngine->setProperty(QPrintEngine::PPK_PaperSource, source);
    d->addToManualSetList(QPrintEngine::PPK_PaperSource);
}

/*!
    Returns the printer's paper source. This is \c Manual or a printer
    tray or paper cassette.
*/
QPrinter::PaperSource QPrinter::paperSource() const
{
    Q_D(const QPrinter);
    return QPrinter::PaperSource(d->printEngine->property(QPrintEngine::PPK_PaperSource).toInt());
}


/*!
  \since 4.1

  Enabled or disables font embedding depending on \a enable.

  Currently this option is only supported on X11.

  \sa fontEmbeddingEnabled()
*/
void QPrinter::setFontEmbeddingEnabled(bool enable)
{
    Q_D(QPrinter);
    d->printEngine->setProperty(QPrintEngine::PPK_FontEmbedding, enable);
    d->addToManualSetList(QPrintEngine::PPK_FontEmbedding);
}

/*!
  \since 4.1

  Returns true if font embedding is enabled.

  Currently this option is only supported on X11.

  \sa setFontEmbeddingEnabled()
*/
bool QPrinter::fontEmbeddingEnabled() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_FontEmbedding).toBool();
}

/*!
    \enum QPrinter::DuplexMode
    \since 4.4

    This enum is used to indicate whether printing will occur on one or both sides
    of each sheet of paper (simplex or duplex printing).

    \value DuplexNone       Single sided (simplex) printing only.
    \value DuplexAuto       The printer's default setting is used to determine whether
                            duplex printing is used.
    \value DuplexLongSide   Both sides of each sheet of paper are used for printing.
                            The paper is turned over its longest edge before the second
                            side is printed
    \value DuplexShortSide  Both sides of each sheet of paper are used for printing.
                            The paper is turned over its shortest edge before the second
                            side is printed
*/

/*!
  \since 4.2

  Enables double sided printing if \a doubleSided is true; otherwise disables it.

  Currently this option is only supported on X11.
*/
void QPrinter::setDoubleSidedPrinting(bool doubleSided)
{
    setDuplex(doubleSided ? DuplexAuto : DuplexNone);
}


/*!
  \since 4.2

  Returns true if double side printing is enabled.

  Currently this option is only supported on X11.
*/
bool QPrinter::doubleSidedPrinting() const
{
    return duplex() != DuplexNone;
}

/*!
  \since 4.4

  Enables double sided printing based on the \a duplex mode.

  Currently this option is only supported on X11.
*/
void QPrinter::setDuplex(DuplexMode duplex)
{
    Q_D(QPrinter);
    d->printEngine->setProperty(QPrintEngine::PPK_Duplex, duplex);
    d->addToManualSetList(QPrintEngine::PPK_Duplex);
}

/*!
  \since 4.4

  Returns the current duplex mode.

  Currently this option is only supported on X11.
*/
QPrinter::DuplexMode QPrinter::duplex() const
{
    Q_D(const QPrinter);
    return static_cast <DuplexMode> (d->printEngine->property(QPrintEngine::PPK_Duplex).toInt());
}

/*!
    \since 4.4

    Returns the page's rectangle in \a unit; this is usually smaller
    than the paperRect() since the page normally has margins between
    its borders and the paper.

    \sa paperSize()
*/
QRectF QPrinter::pageRect(Unit unit) const
{
    Q_D(const QPrinter);
    int res = resolution();
    const qreal multiplier = qt_multiplierForUnit(unit, res);
    // the page rect is in device pixels
    QRect devRect(d->printEngine->property(QPrintEngine::PPK_PageRect).toRect());
    if (unit == DevicePixel)
        return devRect;
    QRectF diRect(devRect.x()*72.0/res,
                  devRect.y()*72.0/res,
                  devRect.width()*72.0/res,
                  devRect.height()*72.0/res);
    return QRectF(diRect.x()/multiplier, diRect.y()/multiplier,
                  diRect.width()/multiplier, diRect.height()/multiplier);
}


/*!
    \since 4.4

    Returns the paper's rectangle in \a unit; this is usually larger
    than the pageRect().

   \sa pageRect()
*/
QRectF QPrinter::paperRect(Unit unit) const
{
    Q_D(const QPrinter);
    int res = resolution();
    const qreal multiplier = qt_multiplierForUnit(unit, resolution());
    // the page rect is in device pixels
    QRect devRect(d->printEngine->property(QPrintEngine::PPK_PaperRect).toRect());
    if (unit == DevicePixel)
        return devRect;
    QRectF diRect(devRect.x()*72.0/res,
                  devRect.y()*72.0/res,
                  devRect.width()*72.0/res,
                  devRect.height()*72.0/res);
    return QRectF(diRect.x()/multiplier, diRect.y()/multiplier,
                  diRect.width()/multiplier, diRect.height()/multiplier);
}

/*!
    Returns the page's rectangle; this is usually smaller than the
    paperRect() since the page normally has margins between its
    borders and the paper.

    The unit of the returned rectangle is DevicePixel.

    \sa paperSize()
*/
QRect QPrinter::pageRect() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_PageRect).toRect();
}

/*!
    Returns the paper's rectangle; this is usually larger than the
    pageRect().

    The unit of the returned rectangle is DevicePixel.

    \sa pageRect()
*/
QRect QPrinter::paperRect() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_PaperRect).toRect();
}


/*!
    \since 4.4

    This function sets the \a left, \a top, \a right and \a bottom
    page margins for this printer. The unit of the margins are
    specified with the \a unit parameter.
*/
void QPrinter::setPageMargins(qreal left, qreal top, qreal right, qreal bottom, QPrinter::Unit unit)
{
    Q_D(QPrinter);
    const qreal multiplier = qt_multiplierForUnit(unit, resolution());
    QList<QVariant> margins;
    margins << (left * multiplier) << (top * multiplier)
            << (right * multiplier) << (bottom * multiplier);
    d->printEngine->setProperty(QPrintEngine::PPK_PageMargins, margins);
    d->addToManualSetList(QPrintEngine::PPK_PageMargins);
    d->hasCustomPageMargins = true;
}

/*!
    \since 4.4

    Returns the page margins for this printer in \a left, \a top, \a
    right, \a bottom. The unit of the returned margins are specified
    with the \a unit parameter.
*/
void QPrinter::getPageMargins(qreal *left, qreal *top, qreal *right, qreal *bottom, QPrinter::Unit unit) const
{
    Q_D(const QPrinter);
    Q_ASSERT(left && top && right && bottom);
    const qreal multiplier = qt_multiplierForUnit(unit, resolution());
    QList<QVariant> margins(d->printEngine->property(QPrintEngine::PPK_PageMargins).toList());
    *left = margins.at(0).toReal() / multiplier;
    *top = margins.at(1).toReal() / multiplier;
    *right = margins.at(2).toReal() / multiplier;
    *bottom = margins.at(3).toReal() / multiplier;
}

/*!
    \internal

    Returns the metric for the given \a id.
*/
int QPrinter::metric(PaintDeviceMetric id) const
{
    Q_D(const QPrinter);
    return d->printEngine->metric(id);
}

/*!
    Returns the paint engine used by the printer.
*/
QPaintEngine *QPrinter::paintEngine() const
{
    Q_D(const QPrinter);
    return d->paintEngine;
}

/*!
    \since 4.1

    Returns the print engine used by the printer.
*/
QPrintEngine *QPrinter::printEngine() const
{
    Q_D(const QPrinter);
    return d->printEngine;
}

#if defined (Q_WS_WIN)
/*!
    Sets the page size to be used by the printer under Windows to \a
    pageSize.

    \warning This function is not portable so you may prefer to use
    setPaperSize() instead.

    \sa winPageSize()
*/
void QPrinter::setWinPageSize(int pageSize)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setWinPageSize");
    d->printEngine->setProperty(QPrintEngine::PPK_WindowsPageSize, pageSize);
    d->addToManualSetList(QPrintEngine::PPK_WindowsPageSize);
}

/*!
    Returns the page size used by the printer under Windows.

    \warning This function is not portable so you may prefer to use
    paperSize() instead.

    \sa setWinPageSize()
*/
int QPrinter::winPageSize() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_WindowsPageSize).toInt();
}
#endif // Q_WS_WIN

/*!
    Returns a list of the resolutions (a list of dots-per-inch
    integers) that the printer says it supports.

    For X11 where all printing is directly to postscript, this
    function will always return a one item list containing only the
    postscript resolution, i.e., 72 (72 dpi -- but see PrinterMode).
*/
QList<int> QPrinter::supportedResolutions() const
{
    Q_D(const QPrinter);
    QList<QVariant> varlist
        = d->printEngine->property(QPrintEngine::PPK_SupportedResolutions).toList();
    QList<int> intlist;
    for (int i=0; i<varlist.size(); ++i)
        intlist << varlist.at(i).toInt();
    return intlist;
}

/*!
    Tells the printer to eject the current page and to continue
    printing on a new page. Returns true if this was successful;
    otherwise returns false.

    Calling newPage() on an inactive QPrinter object will always
    fail.
*/
bool QPrinter::newPage()
{
    Q_D(QPrinter);
    if (d->printEngine->printerState() != QPrinter::Active)
        return false;
    return d->printEngine->newPage();
}

/*!
    Aborts the current print run. Returns true if the print run was
    successfully aborted and printerState() will return QPrinter::Aborted; otherwise
    returns false.

    It is not always possible to abort a print job. For example,
    all the data has gone to the printer but the printer cannot or
    will not cancel the job when asked to.
*/
bool QPrinter::abort()
{
    Q_D(QPrinter);
    return d->printEngine->abort();
}

/*!
    Returns the current state of the printer. This may not always be
    accurate (for example if the printer doesn't have the capability
    of reporting its state to the operating system).
*/
QPrinter::PrinterState QPrinter::printerState() const
{
    Q_D(const QPrinter);
    return d->printEngine->printerState();
}


/*! \fn void QPrinter::margins(uint *top, uint *left, uint *bottom, uint *right) const

    Sets *\a top, *\a left, *\a bottom, *\a right to be the top,
    left, bottom, and right margins.

    This function has been superseded by paperRect() and pageRect().
    Use paperRect().top() - pageRect().top() for the top margin,
    paperRect().left() - pageRect().left() for the left margin,
    paperRect().bottom() - pageRect().bottom() for the bottom margin,
    and papaerRect().right() - pageRect().right() for the right
    margin.

    \oldcode
        uint rightMargin;
        uint bottomMargin;
        printer->margins(0, 0, &bottomMargin, &rightMargin);
    \newcode
        int rightMargin = printer->paperRect().right() - printer->pageRect().right();
        int bottomMargin = printer->paperRect().bottom() - printer->pageRect().bottom();
    \endcode
*/

/*! \fn QSize QPrinter::margins() const

    \overload

    Returns a QSize containing the left margin and the top margin.

    This function has been superseded by paperRect() and pageRect().
    Use paperRect().left() - pageRect().left() for the left margin,
    and paperRect().top() - pageRect().top() for the top margin.

    \oldcode
        QSize margins = printer->margins();
        int leftMargin = margins.width();
        int topMargin = margins.height();
    \newcode
        int leftMargin = printer->paperRect().left() - printer->pageRect().left();
        int topMargin = printer->paperRect().top() - printer->pageRect().top();
    \endcode
*/

/*! \fn bool QPrinter::aborted()

    Use printerState() == QPrinter::Aborted instead.
*/

#ifdef Q_WS_WIN
/*!
    \internal
*/
HDC QPrinter::getDC() const
{
    Q_D(const QPrinter);
    return d->printEngine->getPrinterDC();
}

/*!
    \internal
*/
void QPrinter::releaseDC(HDC hdc) const
{
    Q_D(const QPrinter);
    d->printEngine->releasePrinterDC(hdc);
}

/*!
    Returns the supported paper sizes for this printer.

    The values will be either a value that matches an entry in the
    QPrinter::PaperSource enum or a driver spesific value. The driver
    spesific values are greater than the constant DMBIN_USER declared
    in wingdi.h.

    \warning This function is only available in windows.
*/

QList<QPrinter::PaperSource> QPrinter::supportedPaperSources() const
{
    Q_D(const QPrinter);
    QVariant v = d->printEngine->property(QPrintEngine::PPK_PaperSources);

    QList<QVariant> variant_list = v.toList();
    QList<QPrinter::PaperSource> int_list;
    for (int i=0; i<variant_list.size(); ++i)
        int_list << (QPrinter::PaperSource) variant_list.at(i).toInt();

    return int_list;
}

#endif

/*!
    \fn QString QPrinter::printerSelectionOption() const

    Returns the printer options selection string. This is useful only
    if the print command has been explicitly set.

    The default value (an empty string) implies that the printer should
    be selected in a system-dependent manner.

    Any other value implies that the given value should be used.

    \warning This function is not available on Windows.

    \sa setPrinterSelectionOption()
*/

/*!
    \fn void QPrinter::setPrinterSelectionOption(const QString &option)

    Sets the printer to use \a option to select the printer. \a option
    is null by default (which implies that Qt should be smart enough
    to guess correctly), but it can be set to other values to use a
    specific printer selection option.

    If the printer selection option is changed while the printer is
    active, the current print job may or may not be affected.

    \warning This function is not available on Windows.

    \sa printerSelectionOption()
*/

#ifndef Q_WS_WIN
QString QPrinter::printerSelectionOption() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_SelectionOption).toString();
}

void QPrinter::setPrinterSelectionOption(const QString &option)
{
    Q_D(QPrinter);
    d->printEngine->setProperty(QPrintEngine::PPK_SelectionOption, option);
    d->addToManualSetList(QPrintEngine::PPK_SelectionOption);
}
#endif

/*!
    \since 4.1
    \fn int QPrinter::fromPage() const

    Returns the number of the first page in a range of pages to be printed
    (the "from page" setting). Pages in a document are numbered according to
    the convention that the first page is page 1.

    By default, this function returns a special value of 0, meaning that
    the "from page" setting is unset.

    \note If fromPage() and toPage() both return 0, this indicates that
    \e{the whole document will be printed}.

    \sa setFromTo(), toPage()
*/

int QPrinter::fromPage() const
{
    Q_D(const QPrinter);
    return d->fromPage;
}

/*!
    \since 4.1

    Returns the number of the last page in a range of pages to be printed
    (the "to page" setting). Pages in a document are numbered according to
    the convention that the first page is page 1.

    By default, this function returns a special value of 0, meaning that
    the "to page" setting is unset.

    \note If fromPage() and toPage() both return 0, this indicates that
    \e{the whole document will be printed}.

    The programmer is responsible for reading this setting and
    printing accordingly.

    \sa setFromTo(), fromPage()
*/

int QPrinter::toPage() const
{
    Q_D(const QPrinter);
    return d->toPage;
}

/*!
    \since 4.1

    Sets the range of pages to be printed to cover the pages with numbers
    specified by \a from and \a to, where \a from corresponds to the first
    page in the range and \a to corresponds to the last.

    \note Pages in a document are numbered according to the convention that
    the first page is page 1. However, if \a from and \a to are both set to 0,
    the \e{whole document will be printed}.

    This function is mostly used to set a default value that the user can
    override in the print dialog when you call setup().

    \sa fromPage(), toPage()
*/

void QPrinter::setFromTo(int from, int to)
{
    Q_D(QPrinter);
    if (from > to) {
        qWarning() << "QPrinter::setFromTo: 'from' must be less than or equal to 'to'";
        from = to;
    }
    d->fromPage = from;
    d->toPage = to;

    if (d->minPage == 0 && d->maxPage == 0) {
        d->minPage = 1;
        d->maxPage = to;
        d->options |= QAbstractPrintDialog::PrintPageRange;
    }
}

/*!
    \since 4.1

    Sets the print range option in to be \a range.
*/
void QPrinter::setPrintRange( PrintRange range )
{
    Q_D(QPrinter);
    d->printRange = QAbstractPrintDialog::PrintRange(range);
}

/*!
    \since 4.1

    Returns the page range of the QPrinter. After the print setup
    dialog has been opened, this function returns the value selected
    by the user.

    \sa setPrintRange()
*/
QPrinter::PrintRange QPrinter::printRange() const
{
    Q_D(const QPrinter);
    return PrintRange(d->printRange);
}

#if defined(QT3_SUPPORT)

void QPrinter::setOutputToFile(bool f)
{
    if (f) {
        if (outputFileName().isEmpty())
            setOutputFileName(QLatin1String("untitled_printer_document"));
    } else {
        setOutputFileName(QString());
    }
}

bool qt_compat_QPrinter_printSetup(QPrinter *printer, QPrinterPrivate *pd, QWidget *parent)
{
    Q_UNUSED(pd);
    QPrintDialog dlg(printer, parent);
    return dlg.exec() != 0;
}


#ifdef Q_WS_MAC
bool qt_compat_QPrinter_pageSetup(QPrinter *p, QWidget *parent)
{
    QPageSetupDialog psd(p, parent);
    return psd.exec() != 0;
}

/*!
    Executes a page setup dialog so that the user can configure the type of
    page used for printing. Returns true if the contents of the dialog are
    accepted; returns false if the dialog is canceled.
*/
bool QPrinter::pageSetup(QWidget *parent)
{
    return qt_compat_QPrinter_pageSetup(this, parent);
}

/*!
    Executes a print setup dialog so that the user can configure the printing
    process. Returns true if the contents of the dialog are accepted; returns
    false if the dialog is canceled.
*/
bool QPrinter::printSetup(QWidget *parent)
{
    Q_D(QPrinter);
    return qt_compat_QPrinter_printSetup(this, d, parent);
}
#endif // Q_WS_MAC

/*!
    Use QPrintDialog instead.

    \oldcode
        if (printer->setup(parent))
            ...
    \newcode
        QPrintDialog dialog(printer, parent);
        if (dialog.exec())
            ...
    \endcode
*/
bool QPrinter::setup(QWidget *parent)
{
    Q_D(QPrinter);
    return qt_compat_QPrinter_printSetup(this, d, parent)
#ifdef Q_WS_MAC
        && qt_compat_QPrinter_pageSetup(this, parent);
#endif
        ;
}

/*!
    Use QPrintDialog::minPage() instead.
*/
int QPrinter::minPage() const
{
    Q_D(const QPrinter);
    return d->minPage;
}

/*!
    Use QPrintDialog::maxPage() instead.
*/
int QPrinter::maxPage() const
{
    Q_D(const QPrinter);
    return d->maxPage;
}

/*!
    Use QPrintDialog::setMinMax() instead.
*/
void QPrinter::setMinMax( int minPage, int maxPage )
{
    Q_D(QPrinter);
    Q_ASSERT_X(minPage <= maxPage, "QPrinter::setMinMax",
               "'min' must be less than or equal to 'max'");
    d->minPage = minPage;
    d->maxPage = maxPage;
    d->options |= QPrintDialog::PrintPageRange;
}

/*!
    Returns true if the printer is set up to collate copies of printed documents;
    otherwise returns false.

    Use QPrintDialog::isOptionEnabled(QPrintDialog::PrintCollateCopies)
    instead.

    \sa collateCopies()
*/
bool QPrinter::collateCopiesEnabled() const
{
    Q_D(const QPrinter);
    return (d->options & QPrintDialog::PrintCollateCopies);
}

/*!
    Use QPrintDialog::setOption(QPrintDialog::PrintCollateCopies)
    or QPrintDialog::setOptions(QPrintDialog::options()
    & ~QPrintDialog::PrintCollateCopies) instead, depending on \a
    enable.
*/
void QPrinter::setCollateCopiesEnabled(bool enable)
{
    Q_D(QPrinter);

    if (enable)
        d->options |= QPrintDialog::PrintCollateCopies;
    else
        d->options &= ~QPrintDialog::PrintCollateCopies;
}

/*!
    Use QPrintDialog instead.
*/
void QPrinter::setOptionEnabled( PrinterOption option, bool enable )
{
    Q_D(QPrinter);
    if (enable)
        d->options |= QPrintDialog::PrintDialogOption(1 << option);
    else
        d->options &= ~QPrintDialog::PrintDialogOption(1 << option);
}

/*!
    Use QPrintDialog instead.
*/
bool QPrinter::isOptionEnabled( PrinterOption option ) const
{
    Q_D(const QPrinter);
    return (d->options & QPrintDialog::PrintDialogOption(option));
}

#endif // QT3_SUPPORT

/*!
    \class QPrintEngine
    \reentrant

    \ingroup printing

    \brief The QPrintEngine class defines an interface for how QPrinter
    interacts with a given printing subsystem.

    The common case when creating your own print engine is to derive from both
    QPaintEngine and QPrintEngine. Various properties of a print engine are
    given with property() and set with setProperty().

    \sa QPaintEngine
*/

/*!
    \enum QPrintEngine::PrintEnginePropertyKey

    This enum is used to communicate properties between the print
    engine and QPrinter. A property may or may not be supported by a
    given print engine.

    \value PPK_CollateCopies A boolean value indicating whether the
    printout should be collated or not.

    \value PPK_ColorMode Refers to QPrinter::ColorMode, either color or
    monochrome.

    \value PPK_Creator A string describing the document's creator.

    \value PPK_Duplex A boolean value indicating whether both sides of
    the printer paper should be used for the printout.

    \value PPK_DocumentName A string describing the document name in
    the spooler.

    \value PPK_FontEmbedding A boolean value indicating whether data for
    the document's fonts should be embedded in the data sent to the
    printer.

    \value PPK_FullPage A boolean describing if the printer should be
    full page or not.

    \value PPK_NumberOfCopies Obsolete. An integer specifying the number of
    copies. Use PPK_CopyCount instead.

    \value PPK_Orientation Specifies a QPrinter::Orientation value.

    \value PPK_OutputFileName The output file name as a string. An
    empty file name indicates that the printer should not print to a file.

    \value PPK_PageOrder Specifies a QPrinter::PageOrder value.

    \value PPK_PageRect A QRect specifying the page rectangle

    \value PPK_PageSize Obsolete. Use PPK_PaperSize instead.

    \value PPK_PaperRect A QRect specifying the paper rectangle.

    \value PPK_PaperSource Specifies a QPrinter::PaperSource value.

    \value PPK_PaperSources Specifies more than one QPrinter::PaperSource value.

    \value PPK_PaperSize Specifies a QPrinter::PaperSize value.

    \value PPK_PrinterName A string specifying the name of the printer.

    \value PPK_PrinterProgram A string specifying the name of the
    printer program used for printing,

    \value PPK_Resolution An integer describing the dots per inch for
    this printer.

    \value PPK_SelectionOption

    \value PPK_SupportedResolutions A list of integer QVariants
    describing the set of supported resolutions that the printer has.

    \value PPK_SuppressSystemPrintStatus Suppress the built-in dialog for showing
    printing progress. As of 4.1 this only has effect on Mac OS X where, by default,
    a status dialog is shown.

    \value PPK_WindowsPageSize An integer specifying a DM_PAPER entry
    on Windows.

    \value PPK_CustomPaperSize A QSizeF specifying a custom paper size
    in the QPrinter::Point unit.

    \value PPK_PageMargins A QList<QVariant> containing the left, top,
    right and bottom margin values.

    \value PPK_CopyCount An integer specifying the number of copies to print.

    \value PPK_SupportsMultipleCopies A boolean value indicating whether or not
    the printer supports printing multiple copies in one job.

    \value PPK_CustomBase Basis for extension.
*/

/*!
    \fn QPrintEngine::~QPrintEngine()

    Destroys the print engine.
*/

/*!
    \fn void QPrintEngine::setProperty(PrintEnginePropertyKey key, const QVariant &value)

    Sets the print engine's property specified by \a key to the given \a value.

    \sa property()
*/

/*!
    \fn void QPrintEngine::property(PrintEnginePropertyKey key) const

    Returns the print engine's property specified by \a key.

    \sa setProperty()
*/

/*!
    \fn bool QPrintEngine::newPage()

    Instructs the print engine to start a new page. Returns true if
    the printer was able to create the new page; otherwise returns false.
*/

/*!
    \fn bool QPrintEngine::abort()

    Instructs the print engine to abort the printing process. Returns
    true if successful; otherwise returns false.
*/

/*!
    \fn int QPrintEngine::metric(QPaintDevice::PaintDeviceMetric id) const

    Returns the metric for the given \a id.
*/

/*!
    \fn QPrinter::PrinterState QPrintEngine::printerState() const

    Returns the current state of the printer being used by the print engine.
*/

/*!
    \fn HDC QPrintEngine::getPrinterDC() const
    \internal
*/

/*!
    \fn void QPrintEngine::releasePrinterDC(HDC) const
    \internal
*/

/*
    Returns the dimensions for the given paper size, \a size, in millimeters.
*/
QSizeF qt_paperSizeToQSizeF(QPrinter::PaperSize size)
{
    if (size == QPrinter::Custom) return QSizeF(0, 0);
    return QSizeF(qt_paperSizes[size][0], qt_paperSizes[size][1]);
}

/*
    Returns the PaperSize type that matches \a size, where \a size
    is in millimeters.

    Because dimensions may not always be completely accurate (for
    example when converting between units), a particular PaperSize
    will be returned if it matches within -1/+1 millimeters.
*/
QPrinter::PaperSize qSizeFTopaperSize(const QSizeF& size)
{
    for (int i = 0; i < static_cast<int>(QPrinter::NPaperSize); ++i) {
        if (qt_paperSizes[i][0] >= size.width() - 1 &&
                qt_paperSizes[i][0] <= size.width() + 1 &&
                qt_paperSizes[i][1] >= size.height() - 1 &&
                qt_paperSizes[i][1] <= size.height() + 1) {
            return QPrinter::PaperSize(i);
        }
    }

    return QPrinter::Custom;
}

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
