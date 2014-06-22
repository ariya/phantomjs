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

#include "qpagesetupdialog.h"

#ifndef QT_NO_PRINTDIALOG
#include "qpagesetupdialog_unix_p.h"

#include <private/qpagesetupdialog_p.h>
#include <private/qprintdevice_p.h>
#include <private/qcups_p.h>

#include "qpainter.h"
#include "qprintdialog.h"
#include "qdialogbuttonbox.h"
#include <ui_qpagesetupwidget.h>

#include <QtPrintSupport/qprinter.h>

#include <qpa/qplatformprintplugin.h>
#include <qpa/qplatformprintersupport.h>

QT_BEGIN_NAMESPACE

// Disabled until we have support for papersources on unix
// #define PSD_ENABLE_PAPERSOURCE

#ifdef PSD_ENABLE_PAPERSOURCE
static const char *paperSourceNames[] = {
    "Only One",
    "Lower",
    "Middle",
    "Manual",
    "Envelope",
    "Envelope manual",
    "Auto",
    "Tractor",
    "Small format",
    "Large format",
    "Large capacity",
    "Cassette",
    "Form source",
    0
};

struct PaperSourceNames
{
    PaperSourceNames(const char *nam, QPrinter::PaperSource ps)
        : paperSource(ps), name(nam) {}
    QPrinter::PaperSource paperSource;
    const char *name;
};
#endif


// QPagePreview
// - Private widget to display preview of page layout
// - Embedded in QPageSetupWidget

class QPagePreview : public QWidget
{
public:
    QPagePreview(QWidget *parent) : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(50, 50);
    }

    void setPageLayout(const QPageLayout &layout)
    {
        m_pageLayout = layout;
        update();
    }

    void setPagePreviewLayout(int columns, int rows)
    {
      m_pagePreviewColumns = columns;
      m_pagePreviewRows = rows;
      update();
    }

protected:
    void paintEvent(QPaintEvent *)
    {
        QSize pageSize = m_pageLayout.fullRectPoints().size();
        QSizeF scaledSize = pageSize.scaled(width() - 10, height() - 10, Qt::KeepAspectRatio);
        QRect pageRect = QRect(QPoint(0,0), scaledSize.toSize());
        pageRect.moveCenter(rect().center());
        qreal width_factor = scaledSize.width() / pageSize.width();
        qreal height_factor = scaledSize.height() / pageSize.height();
        QMarginsF margins = m_pageLayout.margins(QPageLayout::Point);
        int left = qRound(margins.left() * width_factor);
        int top = qRound(margins.top() * height_factor);
        int right = qRound(margins.right() * width_factor);
        int bottom = qRound(margins.bottom() * height_factor);
        QRect marginRect(pageRect.x() + left, pageRect.y() + top,
                         pageRect.width() - (left + right + 1), pageRect.height() - (top + bottom + 1));

        QPainter p(this);
        QColor shadow(palette().mid().color());
        for (int i=1; i<6; ++i) {
            shadow.setAlpha(180-i*30);
            QRect offset(pageRect.adjusted(i, i, i, i));
            p.setPen(shadow);
            p.drawLine(offset.left(), offset.bottom(), offset.right(), offset.bottom());
            p.drawLine(offset.right(), offset.top(), offset.right(), offset.bottom()-1);
        }
        p.fillRect(pageRect, palette().light());

        if (marginRect.isValid()) {
            p.setPen(QPen(palette().color(QPalette::Dark), 0, Qt::DotLine));
            p.drawRect(marginRect);

            marginRect.adjust(2, 2, -1, -1);
            p.setClipRect(marginRect);
            QFont font;
            font.setPointSizeF(font.pointSizeF()*0.25);
            p.setFont(font);
            p.setPen(palette().color(QPalette::Dark));
            QString text(QLatin1String("Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet dolore magna aliquam erat volutpat. Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lobortis nisl ut aliquip ex ea commodo consequat. Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla facilisi."));
            for (int i=0; i<3; ++i)
                text += text;

            const int spacing = pageRect.width() * 0.1;
            const int textWidth = (marginRect.width() - (spacing * (m_pagePreviewColumns-1))) / m_pagePreviewColumns;
            const int textHeight = (marginRect.height() - (spacing * (m_pagePreviewRows-1))) / m_pagePreviewRows;

            for (int x = 0 ; x < m_pagePreviewColumns; ++x) {
                for (int y = 0 ; y < m_pagePreviewRows; ++y) {
                    QRect textRect(marginRect.left() + x * (textWidth + spacing),
                                   marginRect.top() + y * (textHeight + spacing),
                                   textWidth, textHeight);
                    p.drawText(textRect, Qt::TextWordWrap|Qt::AlignVCenter, text);
                }
            }
        }
    }

private:
    // Page Layout
    QPageLayout m_pageLayout;
    // Pages Per Sheet / n-up layout
    int m_pagePreviewColumns, m_pagePreviewRows;
};


// QUnixPageSetupDialogPrivate
// - Linux / Cups implementation of QPageSetupDialogPrivate
// - Embeds QPageSetupWidget

class QUnixPageSetupDialogPrivate : public QPageSetupDialogPrivate
{
    Q_DECLARE_PUBLIC(QPageSetupDialog)

public:
    QUnixPageSetupDialogPrivate(QPrinter *printer);
    ~QUnixPageSetupDialogPrivate();
    void init();

    QPageSetupWidget *widget;
};

QUnixPageSetupDialogPrivate::QUnixPageSetupDialogPrivate(QPrinter *printer) : QPageSetupDialogPrivate(printer)
{
}

QUnixPageSetupDialogPrivate::~QUnixPageSetupDialogPrivate()
{
}

void QUnixPageSetupDialogPrivate::init()
{
    Q_Q(QPageSetupDialog);

    widget = new QPageSetupWidget(q);
    widget->setPrinter(printer);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                                     | QDialogButtonBox::Cancel,
                                                     Qt::Horizontal, q);
    QObject::connect(buttons, SIGNAL(accepted()), q, SLOT(accept()));
    QObject::connect(buttons, SIGNAL(rejected()), q, SLOT(reject()));

    QVBoxLayout *lay = new QVBoxLayout(q);
    lay->addWidget(widget);
    lay->addWidget(buttons);
}

// QPageSetupWidget
// - Private widget implementation for Linux / CUPS
// - Embeds QPagePreview
// - TODO Could be made public as a stand-alone widget?

QPageSetupWidget::QPageSetupWidget(QWidget *parent)
    : QWidget(parent),
      m_pagePreview(0),
      m_printer(0),
      m_outputFormat(QPrinter::PdfFormat),
      m_units(QPageLayout::Point),
      m_blockSignals(false)
{
    m_ui.setupUi(this);

    QVBoxLayout *lay = new QVBoxLayout(m_ui.preview);
    m_ui.preview->setLayout(lay);
    m_pagePreview = new QPagePreview(m_ui.preview);
    m_pagePreview->setPagePreviewLayout(1, 1);

    lay->addWidget(m_pagePreview);

    setAttribute(Qt::WA_WState_Polished, false);

#ifdef PSD_ENABLE_PAPERSOURCE
    for (int i=0; paperSourceNames[i]; ++i)
        m_ui.paperSource->insertItem(paperSourceNames[i]);
#else
    m_ui.paperSourceLabel->setVisible(false);
    m_ui.paperSource->setVisible(false);
#endif

    m_ui.reverseLandscape->setVisible(false);
    m_ui.reversePortrait->setVisible(false);

    initUnits();
    initPagesPerSheet();

    connect(m_ui.unitCombo, SIGNAL(activated(int)), this, SLOT(unitChanged()));

    connect(m_ui.pageSizeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(pageSizeChanged()));
    connect(m_ui.pageWidth, SIGNAL(valueChanged(double)), this, SLOT(pageSizeChanged()));
    connect(m_ui.pageHeight, SIGNAL(valueChanged(double)), this, SLOT(pageSizeChanged()));

    connect(m_ui.leftMargin, SIGNAL(valueChanged(double)), this, SLOT(leftMarginChanged(double)));
    connect(m_ui.topMargin, SIGNAL(valueChanged(double)), this, SLOT(topMarginChanged(double)));
    connect(m_ui.rightMargin, SIGNAL(valueChanged(double)), this, SLOT(rightMarginChanged(double)));
    connect(m_ui.bottomMargin, SIGNAL(valueChanged(double)), this, SLOT(bottomMarginChanged(double)));

    connect(m_ui.portrait, SIGNAL(clicked()), this, SLOT(pageOrientationChanged()));
    connect(m_ui.landscape, SIGNAL(clicked()), this, SLOT(pageOrientationChanged()));

    connect(m_ui.pagesPerSheetCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(pagesPerSheetChanged()));
}

// Init the Units combo box
void QPageSetupWidget::initUnits()
{
    m_ui.unitCombo->addItem(tr("Millimeters (mm)"), QVariant::fromValue(QPageLayout::Millimeter));
    m_ui.unitCombo->addItem(tr("Inches (in)"), QVariant::fromValue(QPageLayout::Inch));
    m_ui.unitCombo->addItem(tr("Points (pt)"), QVariant::fromValue(QPageLayout::Point));
    m_ui.unitCombo->addItem(tr("Pica (P̸)"), QVariant::fromValue(QPageLayout::Pica));
    m_ui.unitCombo->addItem(tr("Didot (DD)"), QVariant::fromValue(QPageLayout::Didot));
    m_ui.unitCombo->addItem(tr("Cicero (CC)"), QVariant::fromValue(QPageLayout::Cicero));

    // Initailly default to locale measurement system, mm if metric, in otherwise
    m_ui.unitCombo->setCurrentIndex(QLocale().measurementSystem() != QLocale::MetricSystem);
}

// Init the Pages Per Sheet (n-up) combo boxes if using CUPS
void QPageSetupWidget::initPagesPerSheet()
{
#if !defined(QT_NO_CUPS)
    m_ui.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Left to Right, Top to Bottom"),
                                           QVariant::fromValue(QCUPSSupport::LeftToRightTopToBottom));
    m_ui.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Left to Right, Bottom to Top"),
                                           QVariant::fromValue(QCUPSSupport::LeftToRightBottomToTop));
    m_ui.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Right to Left, Bottom to Top"),
                                           QVariant::fromValue(QCUPSSupport::RightToLeftBottomToTop));
    m_ui.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Right to Left, Top to Bottom"),
                                           QVariant::fromValue(QCUPSSupport::RightToLeftTopToBottom));
    m_ui.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Bottom to Top, Left to Right"),
                                           QVariant::fromValue(QCUPSSupport::BottomToTopLeftToRight));
    m_ui.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Bottom to Top, Right to Left"),
                                           QVariant::fromValue(QCUPSSupport::BottomToTopRightToLeft));
    m_ui.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Top to Bottom, Left to Right"),
                                           QVariant::fromValue(QCUPSSupport::TopToBottomLeftToRight));
    m_ui.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Top to Bottom, Right to Left"),
                                           QVariant::fromValue(QCUPSSupport::TopToBottomRightToLeft));

    m_ui.pagesPerSheetCombo->addItem(QPrintDialog::tr("1 (1x1)"),
                                     QVariant::fromValue(QCUPSSupport::OnePagePerSheet));
    m_ui.pagesPerSheetCombo->addItem(QPrintDialog::tr("2 (2x1)"),
                                     QVariant::fromValue(QCUPSSupport::TwoPagesPerSheet));
    m_ui.pagesPerSheetCombo->addItem(QPrintDialog::tr("4 (2x2)"),
                                     QVariant::fromValue(QCUPSSupport::FourPagesPerSheet));
    m_ui.pagesPerSheetCombo->addItem(QPrintDialog::tr("6 (2x3)"),
                                     QVariant::fromValue(QCUPSSupport::SixPagesPerSheet));
    m_ui.pagesPerSheetCombo->addItem(QPrintDialog::tr("9 (3x3)"),
                                     QVariant::fromValue(QCUPSSupport::NinePagesPerSheet));
    m_ui.pagesPerSheetCombo->addItem(QPrintDialog::tr("16 (4x4)"),
                                     QVariant::fromValue(QCUPSSupport::SixteenPagesPerSheet));

    // Set to QCUPSSupport::OnePagePerSheet
    m_ui.pagesPerSheetCombo->setCurrentIndex(0);
    // Set to QCUPSSupport::LeftToRightTopToBottom
    m_ui.pagesPerSheetLayoutCombo->setCurrentIndex(0);
#else
    // Disable if CUPS wasn't found
    m_ui.pagesPerSheetButtonGroup->hide();
#endif
}

void QPageSetupWidget::initPageSizes()
{
    m_blockSignals = true;

    m_ui.pageSizeCombo->clear();

    if (m_outputFormat == QPrinter::NativeFormat && !m_printerName.isEmpty()) {
        QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
        if (ps) {
            QPrintDevice printDevice = ps->createPrintDevice(m_printerName);
            foreach (const QPageSize &pageSize, printDevice.supportedPageSizes()) {
                m_ui.pageSizeCombo->addItem(pageSize.name(), QVariant::fromValue(pageSize.id()));
            }
            if (m_ui.pageSizeCombo->count() > 0 && printDevice.supportsCustomPageSizes()) {
                m_ui.pageSizeCombo->addItem(tr("Custom"), QVariant::fromValue(QPageSize::Custom));
                m_blockSignals = false;
                return;
            }
        }
    }

    // If PdfFormat or no available printer page sizes, populate with all page sizes
    for (int id = 0; id < QPageSize::LastPageSize; ++id) {
        if (QPageSize::PageSizeId(id) == QPageSize::Custom) {
            m_ui.pageSizeCombo->addItem(tr("Custom"), id);
        } else {
            QPageSize pageSize = QPageSize(QPageSize::PageSizeId(id));
            m_ui.pageSizeCombo->addItem(pageSize.name(), id);
        }
    }

    m_blockSignals = false;
}

// Set the dialog to use the given QPrinter
// Usually only called on first creation
void QPageSetupWidget::setPrinter(QPrinter *printer)
{
    m_printer = printer;

    // Initialize the layout to the current QPrinter layout
    m_pageLayout = m_printer->pageLayout();
    // Assume if margins are Points then is by default, so set to locale default units
    if (m_pageLayout.units() == QPageLayout::Point) {
        if (QLocale().measurementSystem() == QLocale::MetricSystem)
            m_pageLayout.setUnits(QPageLayout::Millimeter);
        else
            m_pageLayout.setUnits(QPageLayout::Inch);
    }
    m_units = m_pageLayout.units();
    m_pagePreview->setPageLayout(m_pageLayout);

    // Then update the widget with the current printer details
    selectPrinter(m_printer->outputFormat(), m_printer->printerName());
}

// The printer selected in the QPrintDialog has been changed, update the widget to reflect this
// Note the QPrinter is not updated at this time in case the user presses the Cancel button in QPrintDialog
void QPageSetupWidget::selectPrinter(QPrinter::OutputFormat outputFormat, const QString &printerName)
{
    m_outputFormat = outputFormat;
    m_printerName = printerName;
    initPageSizes();
    updateWidget();
}

// Update the widget with the current settings
// TODO Break up into more intelligent chunks?
void QPageSetupWidget::updateWidget()
{
    m_blockSignals = true;

    QString suffix;
    switch (m_units) {
    case QPageLayout::Millimeter:
        //: Unit 'Millimeter'
        suffix = tr("mm");
        break;
    case QPageLayout::Point:
        //: Unit 'Points'
        suffix = tr("pt");
        break;
    case QPageLayout::Inch:
        //: Unit 'Inch'
        suffix = tr("in");
        break;
    case QPageLayout::Pica:
        //: Unit 'Pica'
        suffix = tr("P̸");
        break;
    case QPageLayout::Didot:
        //: Unit 'Didot'
        suffix = tr("DD");
        break;
    case QPageLayout::Cicero:
        //: Unit 'Cicero'
        suffix = tr("CC");
        break;
    }

    m_ui.unitCombo->setCurrentIndex(m_ui.unitCombo->findData(QVariant::fromValue(m_units)));

    m_ui.pageSizeCombo->setCurrentIndex(m_ui.pageSizeCombo->findData(QVariant::fromValue(m_pageLayout.pageSize().id())));

    QMarginsF min;
    QMarginsF max;

    if (m_pageLayout.mode() == QPageLayout::FullPageMode) {
        min = QMarginsF(0.0, 0.0, 0.0, 0.0);
        max = QMarginsF(9999.9999, 9999.9999, 9999.9999, 9999.9999);
    } else {
        min = m_pageLayout.minimumMargins();
        max = m_pageLayout.maximumMargins();
    }

    m_ui.leftMargin->setSuffix(suffix);
    m_ui.leftMargin->setMinimum(min.left());
    m_ui.leftMargin->setMaximum(max.left());
    m_ui.leftMargin->setValue(m_pageLayout.margins().left());

    m_ui.rightMargin->setSuffix(suffix);
    m_ui.rightMargin->setMinimum(min.right());
    m_ui.rightMargin->setMaximum(max.right());
    m_ui.rightMargin->setValue(m_pageLayout.margins().right());

    m_ui.topMargin->setSuffix(suffix);
    m_ui.topMargin->setMinimum(min.top());
    m_ui.topMargin->setMaximum(max.top());
    m_ui.topMargin->setValue(m_pageLayout.margins().top());

    m_ui.bottomMargin->setSuffix(suffix);
    m_ui.bottomMargin->setMinimum(min.bottom());
    m_ui.bottomMargin->setMaximum(max.bottom());
    m_ui.bottomMargin->setValue(m_pageLayout.margins().bottom());

    bool isCustom = m_ui.pageSizeCombo->currentData().value<QPageSize::PageSizeId>() == QPageSize::Custom;

    m_ui.pageWidth->setSuffix(suffix);
    m_ui.pageWidth->setValue(m_pageLayout.fullRect(m_units).width());
    m_ui.pageWidth->setEnabled(isCustom);
    m_ui.widthLabel->setEnabled(isCustom);

    m_ui.pageHeight->setSuffix(suffix);
    m_ui.pageHeight->setValue(m_pageLayout.fullRect(m_units).height());
    m_ui.pageHeight->setEnabled(isCustom);
    m_ui.heightLabel->setEnabled(isCustom);

    m_ui.landscape->setChecked(m_pageLayout.orientation() == QPageLayout::Landscape);

    m_ui.pagesPerSheetButtonGroup->setEnabled(m_outputFormat == QPrinter::NativeFormat);

#ifdef PSD_ENABLE_PAPERSOURCE
    m_ui.paperSource->setCurrentItem(printer->paperSource());
#endif

    m_blockSignals = false;
}

// Set the dialog chosen options on the QPrinter
// Normally only called when the QPrintDialog or QPageSetupDialog OK button is pressed
void QPageSetupWidget::setupPrinter() const
{
    m_printer->setPageLayout(m_pageLayout);
#if !defined(QT_NO_CUPS)
    QCUPSSupport::PagesPerSheet pagesPerSheet = m_ui.pagesPerSheetCombo->currentData()
                                                    .value<QCUPSSupport::PagesPerSheet>();
    QCUPSSupport::PagesPerSheetLayout pagesPerSheetLayout = m_ui.pagesPerSheetLayoutCombo->currentData()
                                                                .value<QCUPSSupport::PagesPerSheetLayout>();
    QCUPSSupport::setPagesPerSheetLayout(m_printer, pagesPerSheet, pagesPerSheetLayout);
#endif
#ifdef PSD_ENABLE_PAPERSOURCE
    m_printer->setPaperSource((QPrinter::PaperSource)m_ui.paperSource->currentIndex());
#endif
}

// Updates size/preview after the combobox has been changed.
void QPageSetupWidget::pageSizeChanged()
{
    if (m_blockSignals)
        return;

    QPageSize::PageSizeId id = m_ui.pageSizeCombo->currentData().value<QPageSize::PageSizeId>();
    if (id != QPageSize::Custom) {
        // TODO Set layout margin min/max to printer custom min/max
        m_pageLayout.setPageSize(QPageSize(id));
    } else {
        QSizeF customSize;
        if (m_pageLayout.orientation() == QPageLayout::Landscape)
            customSize = QSizeF(m_ui.pageHeight->value(), m_ui.pageWidth->value());
        else
            customSize = QSizeF(m_ui.pageWidth->value(), m_ui.pageHeight->value());
        // TODO Set layout margin min/max to printer min/max for page size
        m_pageLayout.setPageSize(QPageSize(customSize, QPageSize::Unit(m_units)));
    }
    m_pagePreview->setPageLayout(m_pageLayout);

    updateWidget();
}

void QPageSetupWidget::pageOrientationChanged()
{
    if (m_blockSignals)
        return;
    m_pageLayout.setOrientation(m_ui.portrait->isChecked() ? QPageLayout::Portrait : QPageLayout::Landscape);
    m_pagePreview->setPageLayout(m_pageLayout);
    updateWidget();
}

void QPageSetupWidget::pagesPerSheetChanged()
{
#if !defined(QT_NO_CUPS)
    switch (m_ui.pagesPerSheetCombo->currentData().toInt()) {
    case QCUPSSupport::OnePagePerSheet:
        m_pagePreview->setPagePreviewLayout(1, 1);
        break;
    case QCUPSSupport::TwoPagesPerSheet:
        m_pagePreview->setPagePreviewLayout(1, 2);
        break;
    case QCUPSSupport::FourPagesPerSheet:
        m_pagePreview->setPagePreviewLayout(2, 2);
        break;
    case QCUPSSupport::SixPagesPerSheet:
        m_pagePreview->setPagePreviewLayout(3, 2);
        break;
    case QCUPSSupport::NinePagesPerSheet:
        m_pagePreview->setPagePreviewLayout(3, 3);
        break;
    case QCUPSSupport::SixteenPagesPerSheet:
        m_pagePreview->setPagePreviewLayout(4, 4);
        break;
    }
#endif
}

void QPageSetupWidget::unitChanged()
{
    if (m_blockSignals)
        return;
    m_units = m_ui.unitCombo->currentData().value<QPageLayout::Unit>();
    m_pageLayout.setUnits(m_units);
    updateWidget();
}

void QPageSetupWidget::topMarginChanged(double newValue)
{
    if (m_blockSignals)
        return;
    m_pageLayout.setTopMargin(newValue);
    m_pagePreview->setPageLayout(m_pageLayout);
}

void QPageSetupWidget::bottomMarginChanged(double newValue)
{
    if (m_blockSignals)
        return;
    m_pageLayout.setBottomMargin(newValue);
    m_pagePreview->setPageLayout(m_pageLayout);
}

void QPageSetupWidget::leftMarginChanged(double newValue)
{
    if (m_blockSignals)
        return;
    m_pageLayout.setLeftMargin(newValue);
    m_pagePreview->setPageLayout(m_pageLayout);
}

void QPageSetupWidget::rightMarginChanged(double newValue)
{
    if (m_blockSignals)
        return;
    m_pageLayout.setRightMargin(newValue);
    m_pagePreview->setPageLayout(m_pageLayout);
}

// QPageSetupDialog
// - Public Linux / CUPS class implementation

QPageSetupDialog::QPageSetupDialog(QPrinter *printer, QWidget *parent)
    : QDialog(*(new QUnixPageSetupDialogPrivate(printer)), parent)
{
    Q_D(QPageSetupDialog);
    setWindowTitle(QCoreApplication::translate("QPrintPreviewDialog", "Page Setup"));
    static_cast<QUnixPageSetupDialogPrivate *>(d)->init();
}

QPageSetupDialog::QPageSetupDialog(QWidget *parent)
    : QDialog(*(new QUnixPageSetupDialogPrivate(0)), parent)
{
    Q_D(QPageSetupDialog);
    setWindowTitle(QCoreApplication::translate("QPrintPreviewDialog", "Page Setup"));
    static_cast<QUnixPageSetupDialogPrivate *>(d)->init();
}

int QPageSetupDialog::exec()
{
    Q_D(QPageSetupDialog);

    int ret = QDialog::exec();
    if (ret == Accepted)
        static_cast <QUnixPageSetupDialogPrivate*>(d)->widget->setupPrinter();
    return ret;
}

QT_END_NAMESPACE

#include "moc_qpagesetupdialog.cpp"

#endif // QT_NO_PRINTDIALOG
