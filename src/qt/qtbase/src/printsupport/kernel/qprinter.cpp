/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qprinter.h"
#include "qprinter_p.h"

#ifndef QT_NO_PRINTER

#include <qpa/qplatformprintplugin.h>
#include <qpa/qplatformprintersupport.h>

#include "qprintengine.h"
#include "qlist.h"
#include <qcoreapplication.h>
#include <qfileinfo.h>

#include <private/qpagedpaintdevice_p.h>

#include "qprintengine_pdf_p.h"

#include <qpicture.h>
#include <private/qpaintengine_preview_p.h>

QT_BEGIN_NAMESPACE

#define ABORT_IF_ACTIVE(location) \
    if (d->printEngine->printerState() == QPrinter::Active) { \
        qWarning("%s: Cannot be changed while printer is active", location); \
        return; \
    }

#define ABORT_IF_ACTIVE_RETURN(location, retValue) \
    if (d->printEngine->printerState() == QPrinter::Active) { \
        qWarning("%s: Cannot be changed while printer is active", location); \
        return retValue; \
    }

extern qreal qt_pixelMultiplier(int resolution);
extern QMarginsF qt_convertMargins(const QMarginsF &margins, QPageLayout::Unit fromUnits, QPageLayout::Unit toUnits);

/// return the multiplier of converting from the unit value to postscript-points.
Q_PRINTSUPPORT_EXPORT double qt_multiplierForUnit(QPrinter::Unit unit, int resolution)
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
Q_PRINTSUPPORT_EXPORT QSizeF qt_printerPaperSize(QPrinter::Orientation orientation,
                           QPrinter::PaperSize paperSize,
                           QPrinter::Unit unit,
                           int resolution)
{
    QPageSize pageSize = QPageSize(QPageSize::PageSizeId(paperSize));
    QSizeF sizef;
    if (unit == QPrinter::DevicePixel)
        sizef = pageSize.size(QPageSize::Point) * qt_multiplierForUnit(unit, resolution);
    else
        sizef = pageSize.size(QPageSize::Unit(unit));
    return orientation == QPrinter::Landscape ? sizef.transposed() : sizef;
}

QPrinterInfo QPrinterPrivate::findValidPrinter(const QPrinterInfo &printer)
{
    // Try find a valid printer to use, either the one given, the default or the first available
    QPrinterInfo printerToUse = printer;
    if (printerToUse.isNull()) {
        printerToUse = QPrinterInfo::defaultPrinter();
        if (printerToUse.isNull()) {
            QList<QPrinterInfo> availablePrinters = QPrinterInfo::availablePrinters();
            if (!availablePrinters.isEmpty())
                printerToUse = availablePrinters.at(0);
        }
    }
    return printerToUse;
}

void QPrinterPrivate::initEngines(QPrinter::OutputFormat format, const QPrinterInfo &printer)
{
    // Default to PdfFormat
    outputFormat = QPrinter::PdfFormat;
    QPlatformPrinterSupport *ps = 0;
    QString printerName;

    // Only set NativeFormat if we have a valid plugin and printer to use
    if (format == QPrinter::NativeFormat) {
        ps = QPlatformPrinterSupportPlugin::get();
        QPrinterInfo printerToUse = findValidPrinter(printer);
        if (ps && !printerToUse.isNull()) {
            outputFormat = QPrinter::NativeFormat;
            printerName = printerToUse.printerName();
        }
    }

    if (outputFormat == QPrinter::NativeFormat) {
        printEngine = ps->createNativePrintEngine(printerMode);
        paintEngine = ps->createPaintEngine(printEngine, printerMode);
    } else {
        QPdfPrintEngine *pdfEngine = new QPdfPrintEngine(printerMode);
        paintEngine = pdfEngine;
        printEngine = pdfEngine;
    }

    use_default_engine = true;
    had_default_engines = true;
    setProperty(QPrintEngine::PPK_PrinterName, printerName);
    validPrinter = true;
}

void QPrinterPrivate::changeEngines(QPrinter::OutputFormat format, const QPrinterInfo &printer)
{
    QPrintEngine *oldPrintEngine = printEngine;
    const bool def_engine = use_default_engine;

    initEngines(format, printer);

    if (oldPrintEngine) {
        foreach (QPrintEngine::PrintEnginePropertyKey key, m_properties.values()) {
            QVariant prop;
            // PPK_NumberOfCopies need special treatmeant since it in most cases
            // will return 1, disregarding the actual value that was set
            // PPK_PrinterName also needs special treatment as initEngines has set it already
            if (key == QPrintEngine::PPK_NumberOfCopies)
                prop = QVariant(q_ptr->copyCount());
            else if (key != QPrintEngine::PPK_PrinterName)
                prop = oldPrintEngine->property(key);
            if (prop.isValid())
                setProperty(key, prop);
        }
    }

    if (def_engine)
        delete oldPrintEngine;
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

void QPrinterPrivate::setProperty(QPrintEngine::PrintEnginePropertyKey key, const QVariant &value)
{
    printEngine->setProperty(key, value);
    m_properties.insert(key);
}


class QPrinterPagedPaintDevicePrivate : public QPagedPaintDevicePrivate
{
public:
    QPrinterPagedPaintDevicePrivate(QPrinterPrivate *d)
        : QPagedPaintDevicePrivate(), pd(d)
    {}

    virtual ~QPrinterPagedPaintDevicePrivate()
    {}

    bool setPageLayout(const QPageLayout &newPageLayout) Q_DECL_OVERRIDE
    {
        if (pd->paintEngine->type() != QPaintEngine::Pdf
            && pd->printEngine->printerState() == QPrinter::Active) {
            qWarning("QPrinter::setPageLayout: Cannot be changed while printer is active");
            return false;
        }

        // Try to set the print engine page layout
        pd->setProperty(QPrintEngine::PPK_QPageLayout, QVariant::fromValue(newPageLayout));

        // Set QPagedPaintDevice layout to match the current print engine value
        m_pageLayout = pageLayout();

        return pageLayout().isEquivalentTo(newPageLayout);
    }

    bool setPageSize(const QPageSize &pageSize) Q_DECL_OVERRIDE
    {
        if (pd->paintEngine->type() != QPaintEngine::Pdf
            && pd->printEngine->printerState() == QPrinter::Active) {
            qWarning("QPrinter::setPageLayout: Cannot be changed while printer is active");
            return false;
        }


        // Try to set the print engine page size
        pd->setProperty(QPrintEngine::PPK_QPageSize, QVariant::fromValue(pageSize));

        // Set QPagedPaintDevice layout to match the current print engine value
        m_pageLayout = pageLayout();

        return pageLayout().pageSize().isEquivalentTo(pageSize);
    }

    bool setPageOrientation(QPageLayout::Orientation orientation) Q_DECL_OVERRIDE
    {
        // Set the print engine value
        pd->setProperty(QPrintEngine::PPK_Orientation, orientation);

        // Set QPagedPaintDevice layout to match the current print engine value
        m_pageLayout = pageLayout();

        return pageLayout().orientation() == orientation;
    }

    bool setPageMargins(const QMarginsF &margins) Q_DECL_OVERRIDE
    {
        return setPageMargins(margins, pageLayout().units());
    }

    bool setPageMargins(const QMarginsF &margins, QPageLayout::Unit units) Q_DECL_OVERRIDE
    {
        // Try to set print engine margins
        QPair<QMarginsF, QPageLayout::Unit> pair = qMakePair(margins, units);
        pd->setProperty(QPrintEngine::PPK_QPageMargins, QVariant::fromValue(pair));

        // Set QPagedPaintDevice layout to match the current print engine value
        m_pageLayout = pageLayout();

        return pageLayout().margins() == margins && pageLayout().units() == units;
    }

    QPageLayout pageLayout() const Q_DECL_OVERRIDE
    {
        return pd->printEngine->property(QPrintEngine::PPK_QPageLayout).value<QPageLayout>();
    }

    QPrinterPrivate *pd;
};


/*!
  \class QPrinter
  \reentrant

  \brief The QPrinter class is a paint device that paints on a printer.

  \ingroup printing
  \inmodule QtPrintSupport


  This device represents a series of pages of printed output, and is
  used in almost exactly the same way as other paint devices such as
  QWidget and QPixmap.
  A set of additional functions are provided to manage device-specific
  features, such as orientation and resolution, and to step through
  the pages in a document as it is generated.

  When printing directly to a printer on Windows or Mac OS X, QPrinter uses
  the built-in printer drivers. On X11, QPrinter uses the
  \l{Common Unix Printing System (CUPS)}
  to send PDF output to the printer. As an alternative,
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
  \li setOrientation() tells QPrinter which page orientation to use.
  \li setPaperSize() tells QPrinter what paper size to expect from the
  printer.
  \li setResolution() tells QPrinter what resolution you wish the
  printer to provide, in dots per inch (DPI).
  \li setFullPage() tells QPrinter whether you want to deal with the
  full page or just with the part the printer can draw on.
  \li setCopyCount() tells QPrinter how many copies of the document
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

  \sa QPrintDialog, {Qt Print Support}
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

    \value PrinterResolution This value is deprecated. It is
    equivalent to ScreenResolution on Unix and HighResolution on
    Windows and Mac. Due to the difference between ScreenResolution
    and HighResolution, use of this value may lead to non-portable
    printer code.

    \value HighResolution On Windows, sets the printer resolution to that
    defined for the printer in use. For PDF printing, sets the
    resolution of the PDF driver to 1200 dpi.

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

    \sa setPrintRange(), printRange(), QAbstractPrintDialog::PrintRange
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
    \value A10
    \value A3Extra
    \value A4Extra
    \value A4Plus
    \value A4Small
    \value A5Extra
    \value B5Extra
    \value JisB0
    \value JisB1
    \value JisB2
    \value JisB3
    \value JisB4
    \value JisB5
    \value JisB6,
    \value JisB7
    \value JisB8
    \value JisB9
    \value JisB10
    \value AnsiA = Letter
    \value AnsiB = Ledger
    \value AnsiC
    \value AnsiD
    \value AnsiE
    \value LegalExtra
    \value LetterExtra
    \value LetterPlus
    \value LetterSmall
    \value TabloidExtra
    \value ArchA
    \value ArchB
    \value ArchC
    \value ArchD
    \value ArchE
    \value Imperial7x9
    \value Imperial8x10
    \value Imperial9x11
    \value Imperial9x12
    \value Imperial10x11
    \value Imperial10x13
    \value Imperial10x14
    \value Imperial12x11
    \value Imperial15x11
    \value ExecutiveStandard
    \value Note
    \value Quarto
    \value Statement
    \value SuperA
    \value SuperB
    \value Postcard
    \value DoublePostcard
    \value Prc16K
    \value Prc32K
    \value Prc32KBig
    \value FanFoldUS
    \value FanFoldGerman
    \value FanFoldGermanLegal
    \value EnvelopeB4
    \value EnvelopeB5
    \value EnvelopeB6
    \value EnvelopeC0
    \value EnvelopeC1
    \value EnvelopeC2
    \value EnvelopeC3
    \value EnvelopeC4
    \value EnvelopeC5 = C5E
    \value EnvelopeC6
    \value EnvelopeC65
    \value EnvelopeC7
    \value EnvelopeDL = DLE
    \value Envelope9
    \value Envelope10 = Comm10E
    \value Envelope11
    \value Envelope12
    \value Envelope14
    \value EnvelopeMonarch
    \value EnvelopePersonal
    \value EnvelopeChou3
    \value EnvelopeChou4
    \value EnvelopeInvite
    \value EnvelopeItalian
    \value EnvelopeKaku2
    \value EnvelopeKaku3
    \value EnvelopePrc1
    \value EnvelopePrc2
    \value EnvelopePrc3
    \value EnvelopePrc4
    \value EnvelopePrc5
    \value EnvelopePrc6
    \value EnvelopePrc7
    \value EnvelopePrc8
    \value EnvelopePrc9
    \value EnvelopePrc10
    \value EnvelopeYou4
    \value LastPageSize = EnvelopeYou4
    \omitvalue NPageSize
    \omitvalue NPaperSize

    With setFullPage(false) (the default), the metrics will be a bit
    smaller; how much depends on the printer in use.

    Due to historic reasons QPageSize::Executive is not the same as the standard
    Postscript and Windows Executive size, use QPageSize::ExecutiveStandard instead.

    The Postscript standard size QPageSize::Folio is different to the Windows
    DMPAPER_FOLIO size, use the Postscript standard size QPageSize::FanFoldGermanLegal
    if needed.
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
  \value MaxPageSource Deprecated, use LastPaperSource instead
  \value Middle
  \value Manual
  \value OnlyOne
  \value Tractor
  \value SmallFormat
  \value Upper
  \value CustomSource A PaperSource defined by the printer that is unknown to Qt
  \value LastPaperSource The highest valid PaperSource value, currently CustomSource
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

/*!
    Creates a new printer object with the given \a mode.
*/
QPrinter::QPrinter(PrinterMode mode)
    : QPagedPaintDevice(),
      d_ptr(new QPrinterPrivate(this))
{
    delete d;
    d = new QPrinterPagedPaintDevicePrivate(d_func());
    d_ptr->init(QPrinterInfo(), mode);
}

/*!
    \since 4.4

    Creates a new printer object with the given \a printer and \a mode.
*/
QPrinter::QPrinter(const QPrinterInfo& printer, PrinterMode mode)
    : QPagedPaintDevice(),
      d_ptr(new QPrinterPrivate(this))
{
    delete d;
    d = new QPrinterPagedPaintDevicePrivate(d_func());
    d_ptr->init(printer, mode);
}

void QPrinterPrivate::init(const QPrinterInfo &printer, QPrinter::PrinterMode mode)
{
    if (!QCoreApplication::instance()) {
        qFatal("QPrinter: Must construct a QCoreApplication before a QPrinter");
        return;
    }

    printerMode = mode;

    initEngines(QPrinter::NativeFormat, printer);
}

/*!
    This function is used by subclasses of QPrinter to specify custom
    print and paint engines (\a printEngine and \a paintEngine,
    respectively).

    QPrinter does not take ownership of the engines, so you need to
    manage these engine instances yourself.

    Note that changing the engines will reset the printer state and
    all its properties.

    \sa printEngine(), paintEngine(), setOutputFormat()

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

    \sa outputFormat(), setOutputFormat(), setOutputFileName()
*/

/*!
    \since 4.1

    Sets the output format for this printer to \a format.

    If \a format is the same value as currently set then no change will be made.

    If \a format is NativeFormat then the printerName will be set to the default
    printer.  If there are no valid printers configured then no change will be made.
    If you want to set NativeFormat with a specific printerName then use
    setPrinterName().

    \sa setPrinterName()
*/
void QPrinter::setOutputFormat(OutputFormat format)
{
    Q_D(QPrinter);

    if (d->outputFormat == format)
        return;

    if (format == QPrinter::NativeFormat) {
        QPrinterInfo printerToUse = d->findValidPrinter();
        if (!printerToUse.isNull())
            d->changeEngines(format, printerToUse);
    } else {
        d->changeEngines(format, QPrinterInfo());
    }
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

    If the \a name is empty then the output format will be set to PdfFormat.

    If the \a name is not a valid printer then no change will be made.

    If the \a name is a valid printer then the output format will be set to NativeFormat.

    \sa printerName(), isValid(), setOutputFormat()
*/
void QPrinter::setPrinterName(const QString &name)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setPrinterName");

    if (printerName() == name)
        return;

    if (name.isEmpty()) {
        setOutputFormat(QPrinter::PdfFormat);
        return;
    }

    QPrinterInfo printerToUse = QPrinterInfo::printerInfo(name);
    if (printerToUse.isNull())
        return;

    if (outputFormat() == QPrinter::PdfFormat) {
        d->changeEngines(QPrinter::NativeFormat, printerToUse);
    } else {
        d->setProperty(QPrintEngine::PPK_PrinterName, name);
    }
}

/*!
  \since 4.4

  Returns \c true if the printer currently selected is a valid printer
  in the system, or a pure PDF printer; otherwise returns \c false.

  To detect other failures check the output of QPainter::begin() or QPrinter::newPage().

  \snippet printing-qprinter/errors.cpp 0

  \sa setPrinterName()
*/
bool QPrinter::isValid() const
{
    Q_D(const QPrinter);
    if (!qApp)
        return false;
    return d->validPrinter;
}

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

    This can change the value of outputFormat().
    If the file name has the ".pdf" suffix PDF is generated. If the file name
    has a suffix other than ".pdf", the output format used is the
    one set with setOutputFormat().

    QPrinter uses Qt's cross-platform PDF print engines
    respectively. If you can produce this format natively, for example
    Mac OS X can generate PDF's from its print engine, set the output format
    back to NativeFormat.

    \sa outputFileName(), setOutputFormat()
*/

void QPrinter::setOutputFileName(const QString &fileName)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setOutputFileName");

    QFileInfo fi(fileName);
    if (!fi.suffix().compare(QLatin1String("pdf"), Qt::CaseInsensitive))
        setOutputFormat(QPrinter::PdfFormat);
    else if (fileName.isEmpty())
        setOutputFormat(QPrinter::NativeFormat);

    d->setProperty(QPrintEngine::PPK_OutputFileName, fileName);
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

  On X11, this function sets the program to call with the PDF
  output. On other platforms, it has no effect.

  \sa printProgram()
*/
void QPrinter::setPrintProgram(const QString &printProg)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setPrintProgram");
    d->setProperty(QPrintEngine::PPK_PrinterProgram, printProg);
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
    d->setProperty(QPrintEngine::PPK_DocumentName, name);
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
    d->setProperty(QPrintEngine::PPK_Creator, creator);
}

// Defined in QPagedPaintDevice but non-virtual, add QPrinter specific doc here
#ifdef Q_QDOC
/*!
    \fn bool QPrinter::setPageLayout(const QPageLayout &newLayout)
    \since 5.3

    Sets the page layout to \a newLayout.

    If the \a newLayout is not valid for the current printer then the page
    layout will not be changed.  For example, if the page size is not supported
    by the printer, or if the margins fall outside the printable area.

    Returns true if the page layout was successfully set to \a newLayout.

    \sa pageLayout(), setPageSize(), setPageOrientation(), setPageMargins()
*/

/*!
    \fn bool QPrinter::setPageSize(const QPageSize &pageSize)
    \since 5.3

    Sets the page size to \a pageSize.

    If the \a pageSize is not valid for the current printer then the page
    size will not be changed.

    Changing the page size may affect the current page margins if they fall
    outside the printable margins for the new page size on the current printer.

    To obtain the current QPageSize use pageLayout().pageSize().

    Returns true if the page size was successfully set to \a pageSize.

    \sa pageLayout(), setPageLayout()
*/

/*!
    \fn bool QPrinter::setPageOrientation(QPageLayout::Orientation orientation)
    \since 5.3

    Sets the page \a orientation to QPageLayout::Portrait or QPageLayout::Landscape.

    The printer driver reads this setting and prints the page using the
    specified orientation.

    On Windows and Mac, this option can be changed while printing and will
    take effect from the next call to newPage().

    To obtain the current QPageLayout::Orientation use pageLayout().pageOrientation().

    Returns true if the page orientation was successfully set to \a orientation.

    \sa pageLayout(), setPageLayout()
*/

/*!
    \fn bool QPrinter::setPageMargins(const QMarginsF &margins, QPageLayout::Unit units)
    \since 5.3

    Set the page margins to \a margins in the given \a units.  If \a units are
    not provided then the current units are used.

    If in Full Page mode then no check is performed on the \a margins set,
    otherwise the \a margins must fall within the printable area for the page
    size on the current printer.

    To obtain the current page margins use pageLayout().pageMargins().

    Returns true if the page margins was successfully set to \a margins.

    \sa pageLayout(), setPageLayout()
*/

/*!
    \fn QPageLayout QPrinter::pageLayout() const
    \since 5.3

    Returns the current page layout.  Use this method to access the current
    QPageSize, QPageLayout::Orientation, QMarginsF, fullPageRect() and paintRect().

    Note that you cannot use the setters on the returned object, you must either
    call the QPrinter methods or setPageLayout().

    \sa setPageLayout(), setPageSize(), setPageOrientation(), setPageMargins()
*/
#endif

/*!
  \obsolete Use pageLayout().pageOrientation() instead.

  Returns the orientation setting. This is driver-dependent, but is usually
  QPrinter::Portrait.

  \sa pageLayout()
*/
QPrinter::Orientation QPrinter::orientation() const
{
    return QPrinter::Orientation(pageLayout().orientation());
}


/*!
  \obsolete Use setPageOrientation() instead.

  Sets the print orientation to \a orientation.

  The orientation can be either QPrinter::Portrait or
  QPrinter::Landscape.

  The printer driver reads this setting and prints using the
  specified orientation.

  On Windows and Mac, this option can be changed while printing and will
  take effect from the next call to newPage().

  \sa setPageOrientation()
*/

void QPrinter::setOrientation(Orientation orientation)
{
    setPageOrientation(QPageLayout::Orientation(orientation));
}

/*!
    \since 4.4

    \obsolete Use pageLayout().pageSize().id() instead.

    Returns the printer paper size. The default value is driver-dependent.

    \sa pageLayout()
*/

QPrinter::PaperSize QPrinter::paperSize() const
{
    return pageSize();
}

/*!
    \since 4.4

    \obsolete Use setPageSize(QPageSize) instead.

    Sets the printer paper size to \a newPaperSize if that size is
    supported. The result is undefined if \a newPaperSize is not
    supported.

    The default paper size is driver-dependent.

    This function is useful mostly for setting a default value that
    the user can override in the print dialog.

    \sa setPageSize()
*/
void QPrinter::setPaperSize(PaperSize newPaperSize)
{
    setPageSize(QPageSize(QPageSize::PageSizeId(newPaperSize)));
}

/*!
    \obsolete Use pageLayout().pageSize().id() instead.

    Returns the printer page size. The default value is driver-dependent.

    \sa pageLayout()
*/
QPrinter::PageSize QPrinter::pageSize() const
{
    return QPrinter::PaperSize(pageLayout().pageSize().id());
}


/*!
    \obsolete Use setPageSize(QPageSize) instead.

    Sets the printer page size based on \a newPageSize.

    \sa setPageSize()
*/

void QPrinter::setPageSize(PageSize newPageSize)
{
    setPageSize(QPageSize(QPageSize::PageSizeId(newPageSize)));
}

/*!
    \since 4.4

    \obsolete Use setPageSize(QPageSize) instead.

    Sets the paper size based on \a paperSize in \a unit.

    Note that the paper size is defined in a portrait layout, regardless of
    what the current printer orientation is set to.

    \sa setPageSize()
*/

void QPrinter::setPaperSize(const QSizeF &paperSize, QPrinter::Unit unit)
{
    setPageSize(QPageSize(paperSize, QPageSize::Unit(unit)));
}

/*!
    \reimp

    \obsolete Use setPageSize(QPageSize) instead.

    Use setPageSize(QPageSize) instead.

    Note that the page size is defined in a portrait layout, regardless of
    what the current printer orientation is set to.

    \sa setPageSize()
*/
void QPrinter::setPageSizeMM(const QSizeF &size)
{
    setPageSize(QPageSize(size, QPageSize::Millimeter));
}

/*!
    \since 4.4

    \obsolete Use pageLayout().pageSize().size() or
    pageLayout().fullPageSize() instead.

    Returns the paper size in \a unit.

    Note that the returned size reflects the current paper orientation.

    \sa pageLayout()
*/

QSizeF QPrinter::paperSize(Unit unit) const
{
    if (unit == QPrinter::DevicePixel)
        return pageLayout().fullRectPixels(resolution()).size();
    else
        return pageLayout().fullRect(QPageLayout::Unit(unit)).size();
}

/*!
    \since 5.1

    \obsolete Use setPageSize(QPageSize) instead.

    Sets the paper used by the printer to \a paperName.

    \sa setPageSize()
*/

void QPrinter::setPaperName(const QString &paperName)
{
    Q_D(QPrinter);
    if (d->paintEngine->type() != QPaintEngine::Pdf)
        ABORT_IF_ACTIVE("QPrinter::setPaperName");
    d->setProperty(QPrintEngine::PPK_PaperName, paperName);
}

/*!
    \since 5.1

    \obsolete Use pageLayout().pageSize().name() instead.

    Returns the paper name of the paper set on the printer.

    The default value for this is driver-dependent.

    \sa pageLayout()
*/

QString QPrinter::paperName() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_PaperName).toString();
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
    d->pageOrderAscending = (pageOrder == FirstPageFirst);

    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setPageOrder");
    d->setProperty(QPrintEngine::PPK_PageOrder, pageOrder);
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
    d->setProperty(QPrintEngine::PPK_ColorMode, newColorMode);
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
    d->setProperty(QPrintEngine::PPK_NumberOfCopies, numCopies);
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
    d->setProperty(QPrintEngine::PPK_CopyCount, count);
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

    Returns \c true if the printer supports printing multiple copies of the same
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

    Returns \c true if collation is turned on when multiple copies is selected.
    Returns \c false if it is turned off when multiple copies is selected.
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
    d->setProperty(QPrintEngine::PPK_CollateCopies, collate);
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

  \sa fullPage(), pageLayout(), setPageSize(), width(), height()
*/

void QPrinter::setFullPage(bool fp)
{
    Q_D(QPrinter);
    // Set the print engine
    d->setProperty(QPrintEngine::PPK_FullPage, fp);
    // Set QPagedPaintDevice layout to match the current print engine value
    devicePageLayout() = pageLayout();
}


/*!
  Returns \c true if the origin of the printer's coordinate system is
  at the corner of the page and false if it is at the edge of the
  printable area.

  See setFullPage() for details and caveats.

  \sa setFullPage(), pageLayout()
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

  \sa resolution(), setPaperSize()
*/

void QPrinter::setResolution(int dpi)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setResolution");
    d->setProperty(QPrintEngine::PPK_Resolution, dpi);
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
    d->setProperty(QPrintEngine::PPK_PaperSource, source);
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
    d->setProperty(QPrintEngine::PPK_FontEmbedding, enable);
}

/*!
  \since 4.1

  Returns \c true if font embedding is enabled.

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

  Returns \c true if double side printing is enabled.

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
    d->setProperty(QPrintEngine::PPK_Duplex, duplex);
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
    if (unit == QPrinter::DevicePixel)
        return pageLayout().paintRectPixels(resolution());
    else
        return pageLayout().paintRect(QPageLayout::Unit(unit));
}


/*!
    \since 4.4

    Returns the paper's rectangle in \a unit; this is usually larger
    than the pageRect().

   \sa pageRect()
*/
QRectF QPrinter::paperRect(Unit unit) const
{
    if (unit == QPrinter::DevicePixel)
        return pageLayout().fullRectPixels(resolution());
    else
        return pageLayout().fullRect(QPageLayout::Unit(unit));
}

/*!
    \obsolete Use pageLayout().paintRect() instead.

    Returns the page's rectangle; this is usually smaller than the
    paperRect() since the page normally has margins between its
    borders and the paper.

    The unit of the returned rectangle is DevicePixel.

    \sa pageLayout()
*/
QRect QPrinter::pageRect() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_PageRect).toRect();
}

/*!
    \obsolete Use pageLayout().fullPageRect() instead.

    Returns the paper's rectangle; this is usually larger than the
    pageRect().

    The unit of the returned rectangle is DevicePixel.

    \sa pageLayout()
*/
QRect QPrinter::paperRect() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_PaperRect).toRect();
}

/*!
    \since 4.4

    \obsolete Use setPageMargins(QMarginsF, QPageLayout::Unit) instead.

    This function sets the \a left, \a top, \a right and \a bottom
    page margins for this printer. The unit of the margins are
    specified with the \a unit parameter.

    \sa setPageMargins()
*/
void QPrinter::setPageMargins(qreal left, qreal top, qreal right, qreal bottom, QPrinter::Unit unit)
{
    if (unit == QPrinter::DevicePixel) {
        QMarginsF margins = QMarginsF(left, top, right, bottom);
        margins *= qt_pixelMultiplier(resolution());
        margins = qt_convertMargins(margins, QPageLayout::Point, pageLayout().units());
        setPageMargins(margins, pageLayout().units());
    } else {
        setPageMargins(QMarginsF(left, top, right, bottom), QPageLayout::Unit(unit));
    }
}

/*!
    \reimp

    \obsolete Use setPageMargins(QMarginsF, QPageLayout::Unit) instead.

    \sa setPageMargins()
  */
void QPrinter::setMargins(const Margins &m)
{
    setPageMargins(QMarginsF(m.left, m.top, m.right, m.bottom), QPageLayout::Millimeter);
}

/*!
    \since 4.4

    \obsolete Use pageLayout().pageMargins() instead.

    Returns the page margins for this printer in \a left, \a top, \a
    right, \a bottom. The unit of the returned margins are specified
    with the \a unit parameter.

    \sa pageLayout(), setPageMargins()
*/
void QPrinter::getPageMargins(qreal *left, qreal *top, qreal *right, qreal *bottom, QPrinter::Unit unit) const
{
    QMarginsF margins;
    if (unit == QPrinter::DevicePixel) {
        QMargins tmp = pageLayout().marginsPixels(resolution());
        margins = QMarginsF(tmp.left(), tmp.top(), tmp.right(), tmp.bottom());
    } else {
        margins = pageLayout().margins(QPageLayout::Unit(unit));
    }
    if (left)
        *left = margins.left();
    if (right)
        *right = margins.right();
    if (top)
        *top = margins.top();
    if (bottom)
        *bottom = margins.bottom();
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

/*!
    \obsolete Use QPageSize::id(windowsId) and setPageLayout(QPageSize) instead.

    Sets the page size to be used by the printer under Windows to \a
    pageSize.

    \sa pageLayout()
*/
void QPrinter::setWinPageSize(int pageSize)
{
    Q_D(QPrinter);
    ABORT_IF_ACTIVE("QPrinter::setWinPageSize");
    d->setProperty(QPrintEngine::PPK_WindowsPageSize, pageSize);
}

/*!
    \obsolete Use pageLayout.pageSize().windowsId() instead.

    Returns the page size used by the printer under Windows.

    \sa pageLayout()
*/
int QPrinter::winPageSize() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_WindowsPageSize).toInt();
}

/*!
    Returns a list of the resolutions (a list of dots-per-inch
    integers) that the printer says it supports.

    For X11 where all printing is directly to PDF, this
    function will always return a one item list containing only the
    PDF resolution, i.e., 72 (72 dpi -- but see PrinterMode).
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
    printing on a new page. Returns \c true if this was successful;
    otherwise returns \c false.

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
    Aborts the current print run. Returns \c true if the print run was
    successfully aborted and printerState() will return QPrinter::Aborted; otherwise
    returns \c false.

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

#ifdef Q_OS_WIN
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

#endif // Q_OS_WIN

/*!
    \fn QString QPrinter::printerSelectionOption() const

    Returns the printer options selection string. This is useful only
    if the print command has been explicitly set.

    The default value (an empty string) implies that the printer should
    be selected in a system-dependent manner.

    Any other value implies that the given value should be used.

    This function always returns an empty string on Windows and Mac.

    \sa setPrinterSelectionOption(), setPrintProgram()
*/

QString QPrinter::printerSelectionOption() const
{
    Q_D(const QPrinter);
    return d->printEngine->property(QPrintEngine::PPK_SelectionOption).toString();
}

/*!
    \fn void QPrinter::setPrinterSelectionOption(const QString &option)

    Sets the printer to use \a option to select the printer. \a option
    is null by default (which implies that Qt should be smart enough
    to guess correctly), but it can be set to other values to use a
    specific printer selection option.

    If the printer selection option is changed while the printer is
    active, the current print job may or may not be affected.

    This function has no effect on Windows or Mac.

    \sa printerSelectionOption(), setPrintProgram()
*/

void QPrinter::setPrinterSelectionOption(const QString &option)
{
    Q_D(QPrinter);
    d->setProperty(QPrintEngine::PPK_SelectionOption, option);
}

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
    if (from > to) {
        qWarning() << "QPrinter::setFromTo: 'from' must be less than or equal to 'to'";
        from = to;
    }
    d->fromPage = from;
    d->toPage = to;
}

/*!
    \since 4.1

    Sets the print range option in to be \a range.
*/
void QPrinter::setPrintRange( PrintRange range )
{
    d->printSelectionOnly = (range == Selection);

    Q_D(QPrinter);
    d->printRange = range;
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
    return d->printRange;
}


/*!
    \class QPrintEngine
    \reentrant

    \ingroup printing
    \inmodule QtPrintSupport

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

    \value PPK_PaperName A string specifying the name of the paper.

    \value PPK_PaperSize Specifies a QPrinter::PaperSize value.

    \value PPK_PrinterName A string specifying the name of the printer.

    \value PPK_PrinterProgram A string specifying the name of the
    printer program used for printing,

    \value PPK_Resolution An integer describing the dots per inch for
    this printer.

    \value PPK_SelectionOption

    \value PPK_SupportedResolutions A list of integer QVariants
    describing the set of supported resolutions that the printer has.

    \value PPK_WindowsPageSize An integer specifying a DM_PAPER entry
    on Windows.

    \value PPK_CustomPaperSize A QSizeF specifying a custom paper size
    in the QPrinter::Point unit.

    \value PPK_PageMargins A QList<QVariant> containing the left, top,
    right and bottom margin values in the QPrinter::Point unit.

    \value PPK_CopyCount An integer specifying the number of copies to print.

    \value PPK_SupportsMultipleCopies A boolean value indicating whether or not
    the printer supports printing multiple copies in one job.

    \value PPK_QPageSize Set the page size using a QPageSize object.

    \value PPK_QPageMargins Set the page margins using a QPair of QMarginsF and QPageLayout::Unit.

    \value PPK_QPageLayout Set the page layout using a QPageLayout object.

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

    Instructs the print engine to start a new page. Returns \c true if
    the printer was able to create the new page; otherwise returns \c false.
*/

/*!
    \fn bool QPrintEngine::abort()

    Instructs the print engine to abort the printing process. Returns
    true if successful; otherwise returns \c false.
*/

/*!
    \fn int QPrintEngine::metric(QPaintDevice::PaintDeviceMetric id) const

    Returns the metric for the given \a id.
*/

/*!
    \fn QPrinter::PrinterState QPrintEngine::printerState() const

    Returns the current state of the printer being used by the print engine.
*/

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
