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

#include "qplatformdefs.h"

#ifndef QT_NO_PRINTDIALOG

#include "private/qabstractprintdialog_p.h"
#include <QtWidgets/qmessagebox.h>
#include "qprintdialog.h"
#include "qfiledialog.h"
#include <QtCore/qdir.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qfilesystemmodel.h>
#include <QtWidgets/qstyleditemdelegate.h>
#include <QtPrintSupport/qprinter.h>

#include <qpa/qplatformprintplugin.h>
#include <qpa/qplatformprintersupport.h>

#include <private/qprintdevice_p.h>

#include <QtWidgets/qdialogbuttonbox.h>

#include "private/qfscompleter_p.h"
#include "ui_qprintpropertieswidget.h"
#include "ui_qprintsettingsoutput.h"
#include "ui_qprintwidget.h"

#ifndef QT_NO_CUPS
#include <private/qcups_p.h>
#include "qcupsjobwidget_p.h"
#endif

/*

Print dialog class declarations

    QPrintDialog:            The main Print Dialog, nothing really held here.

    QUnixPrintWidget:
    QUnixPrintWidgetPrivate: The real Unix Print Dialog implementation.

                             Directly includes the upper half of the Print Dialog
                             containing the Printer Selection widgets and
                             Properties button.

                             Embeds the Properties pop-up dialog from
                             QPrintPropertiesDialog

                             Embeds the lower half from separate widget class
                             QPrintDialogPrivate

                             Layout in qprintwidget.ui

    QPrintDialogPrivate:     The lower half of the Print Dialog containing the
                             Copies and Options tabs that expands when the
                             Options button is selected.

                             Layout in qprintsettingsoutput.ui

    QPrintPropertiesDialog:  Dialog displayed when clicking on Properties button to
                             allow editing of Page and Advanced tabs.

                             Layout in qprintpropertieswidget.ui
*/

static void initResources()
{
    Q_INIT_RESOURCE(qprintdialog);
}

QT_BEGIN_NAMESPACE

class QOptionTreeItem;
class QPPDOptionsModel;

class QPrintPropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    QPrintPropertiesDialog(QAbstractPrintDialog *parent = 0);
    ~QPrintPropertiesDialog();

    void selectPrinter(QPrinter::OutputFormat outputFormat, const QString &printerName);

    /// copy printer properties to the widget
    void applyPrinterProperties(QPrinter *p);
    void setupPrinter() const;

protected:
    void showEvent(QShowEvent* event);

private:
    friend class QUnixPrintWidgetPrivate;
    Ui::QPrintPropertiesWidget widget;
    QDialogButtonBox *m_buttons;
#ifndef QT_NO_CUPS
    QCupsJobWidget *m_jobOptions;
#endif
};

class QUnixPrintWidgetPrivate;

class QUnixPrintWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QUnixPrintWidget(QPrinter *printer, QWidget *parent = 0);
    ~QUnixPrintWidget();
    void updatePrinter();

private:
    friend class QPrintDialogPrivate;
    friend class QUnixPrintWidgetPrivate;
    QUnixPrintWidgetPrivate *d;
    Q_PRIVATE_SLOT(d, void _q_printerChanged(int))
    Q_PRIVATE_SLOT(d, void _q_btnBrowseClicked())
    Q_PRIVATE_SLOT(d, void _q_btnPropertiesClicked())
};

class QUnixPrintWidgetPrivate
{
public:
    QUnixPrintWidgetPrivate(QUnixPrintWidget *q, QPrinter *prn);
    ~QUnixPrintWidgetPrivate();

    /// copy printer properties to the widget
    void applyPrinterProperties();
    bool checkFields();
    void setupPrinter();
    void setOptionsPane(QPrintDialogPrivate *pane);
    void setupPrinterProperties();
// slots
    void _q_printerChanged(int index);
    void _q_btnPropertiesClicked();
    void _q_btnBrowseClicked();

    QUnixPrintWidget * const parent;
    QPrintPropertiesDialog *propertiesDialog;
    Ui::QPrintWidget widget;
    QAbstractPrintDialog * q;
    QPrinter *printer;
    QPrintDevice m_currentPrintDevice;

    void updateWidget();

private:
    QPrintDialogPrivate *optionsPane;
    bool filePrintersAdded;
    bool propertiesDialogShown;
};

class QPrintDialogPrivate : public QAbstractPrintDialogPrivate
{
    Q_DECLARE_PUBLIC(QPrintDialog)
    Q_DECLARE_TR_FUNCTIONS(QPrintDialog)
public:
    QPrintDialogPrivate();
    ~QPrintDialogPrivate();

    void init();
    /// copy printer properties to the widget
    void applyPrinterProperties();

    void selectPrinter(const QPrinter::OutputFormat outputFormat);

    void _q_togglePageSetCombo(bool);
#ifndef QT_NO_MESSAGEBOX
    void _q_checkFields();
#endif
    void _q_collapseOrExpandDialog();

    void setupPrinter();
    void updateWidgets();

    virtual void setTabs(const QList<QWidget*> &tabs);

    Ui::QPrintSettingsOutput options;
    QUnixPrintWidget *top;
    QWidget *bottom;
    QDialogButtonBox *buttons;
    QPushButton *collapseButton;
    QPrinter::OutputFormat printerOutputFormat;
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*

    QPrintPropertiesDialog

    Dialog displayed when clicking on Properties button to allow editing of Page
    and Advanced tabs.

*/

QPrintPropertiesDialog::QPrintPropertiesDialog(QAbstractPrintDialog *parent)
    : QDialog(parent)
{
    QVBoxLayout *lay = new QVBoxLayout(this);
    this->setLayout(lay);
    QWidget *content = new QWidget(this);
    widget.setupUi(content);
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    lay->addWidget(content);
    lay->addWidget(m_buttons);

    connect(m_buttons->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_buttons->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));

#ifndef QT_NO_CUPS
    m_jobOptions = new QCupsJobWidget();
    widget.tabs->addTab(m_jobOptions, tr("Job Options"));
#endif
}

QPrintPropertiesDialog::~QPrintPropertiesDialog()
{
}

void QPrintPropertiesDialog::applyPrinterProperties(QPrinter *p)
{
    widget.pageSetup->setPrinter(p);
#ifndef QT_NO_CUPS
    m_jobOptions->setPrinter(p);
#endif
}

void QPrintPropertiesDialog::setupPrinter() const
{
    widget.pageSetup->setupPrinter();
#ifndef QT_NO_CUPS
    m_jobOptions->setupPrinter();
#endif
}

void QPrintPropertiesDialog::selectPrinter(QPrinter::OutputFormat outputFormat, const QString &printerName)
{
    widget.pageSetup->selectPrinter(outputFormat, printerName);
}

void QPrintPropertiesDialog::showEvent(QShowEvent* event)
{
    event->accept();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*

    QPrintDialogPrivate

    The lower half of the Print Dialog containing the Copies and Options
    tabs that expands when the Options button is selected.

*/
QPrintDialogPrivate::QPrintDialogPrivate()
    : top(0), bottom(0), buttons(0), collapseButton(0)
{
    initResources();
}

QPrintDialogPrivate::~QPrintDialogPrivate()
{
}

void QPrintDialogPrivate::init()
{
    Q_Q(QPrintDialog);

    top = new QUnixPrintWidget(q->printer(), q);
    bottom = new QWidget(q);
    options.setupUi(bottom);
    options.color->setIconSize(QSize(32, 32));
    options.color->setIcon(QIcon(QLatin1String(":/qt-project.org/dialogs/qprintdialog/images/status-color.png")));
    options.grayscale->setIconSize(QSize(32, 32));
    options.grayscale->setIcon(QIcon(QLatin1String(":/qt-project.org/dialogs/qprintdialog/images/status-gray-scale.png")));

#ifndef QT_NO_CUPS
    // Add Page Set widget if CUPS is available
    options.pageSetCombo->addItem(tr("All Pages"), QVariant::fromValue(QCUPSSupport::AllPages));
    options.pageSetCombo->addItem(tr("Odd Pages"), QVariant::fromValue(QCUPSSupport::OddPages));
    options.pageSetCombo->addItem(tr("Even Pages"), QVariant::fromValue(QCUPSSupport::EvenPages));
#endif

    top->d->setOptionsPane(this);

    buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, q);
    collapseButton = new QPushButton(QPrintDialog::tr("&Options >>"), buttons);
    buttons->addButton(collapseButton, QDialogButtonBox::ResetRole);
    bottom->setVisible(false);

    QPushButton *printButton = buttons->button(QDialogButtonBox::Ok);
    printButton->setText(QPrintDialog::tr("&Print"));
    printButton->setDefault(true);

    QVBoxLayout *lay = new QVBoxLayout(q);
    q->setLayout(lay);
    lay->addWidget(top);
    lay->addWidget(bottom);
    lay->addWidget(buttons);

#ifdef QT_NO_MESSAGEBOX
    QObject::connect(buttons, SIGNAL(accepted()), q, SLOT(accept()));
#else
    QObject::connect(buttons, SIGNAL(accepted()), q, SLOT(_q_checkFields()));
#endif
    QObject::connect(buttons, SIGNAL(rejected()), q, SLOT(reject()));

    QObject::connect(options.printSelection, SIGNAL(toggled(bool)),
                     q, SLOT(_q_togglePageSetCombo(bool)));

    QObject::connect(options.printCurrentPage, SIGNAL(toggled(bool)),
                     q, SLOT(_q_togglePageSetCombo(bool)));

    QObject::connect(collapseButton, SIGNAL(released()), q, SLOT(_q_collapseOrExpandDialog()));
}

// initialize printer options
void QPrintDialogPrivate::selectPrinter(const QPrinter::OutputFormat outputFormat)
{
        Q_Q(QPrintDialog);
        QPrinter *p = q->printer();
        printerOutputFormat = outputFormat;

        if (p->colorMode() == QPrinter::Color)
            options.color->setChecked(true);
        else
            options.grayscale->setChecked(true);

        switch (p->duplex()) {
        case QPrinter::DuplexNone:
            options.noDuplex->setChecked(true); break;
        case QPrinter::DuplexLongSide:
        case QPrinter::DuplexAuto:
            options.duplexLong->setChecked(true); break;
        case QPrinter::DuplexShortSide:
            options.duplexShort->setChecked(true); break;
        }
        options.copies->setValue(p->copyCount());
        options.collate->setChecked(p->collateCopies());
        options.reverse->setChecked(p->pageOrder() == QPrinter::LastPageFirst);

        if (outputFormat == QPrinter::PdfFormat || options.printSelection->isChecked()
            || options.printCurrentPage->isChecked())

            options.pageSetCombo->setEnabled(false);
        else
            options.pageSetCombo->setEnabled(true);
}

void QPrintDialogPrivate::applyPrinterProperties()
{
    // apply printer options to property dialog
    top->d->applyPrinterProperties();
}

void QPrintDialogPrivate::setupPrinter()
{
    // First setup the requested OutputFormat, Printer and Page Size first
    top->d->setupPrinter();

    // Then setup Print Job options
    Q_Q(QPrintDialog);
    QPrinter* p = q->printer();

    if (options.duplex->isEnabled()) {
        if (options.noDuplex->isChecked())
            p->setDuplex(QPrinter::DuplexNone);
        else if (options.duplexLong->isChecked())
            p->setDuplex(QPrinter::DuplexLongSide);
        else
            p->setDuplex(QPrinter::DuplexShortSide);
    }

    p->setColorMode(options.color->isChecked() ? QPrinter::Color : QPrinter::GrayScale);
    p->setPageOrder(options.reverse->isChecked() ? QPrinter::LastPageFirst : QPrinter::FirstPageFirst);

    // print range
    if (options.printAll->isChecked()) {
        p->setPrintRange(QPrinter::AllPages);
        p->setFromTo(0,0);
    } else if (options.printSelection->isChecked()) {
        p->setPrintRange(QPrinter::Selection);
        p->setFromTo(0,0);
    } else if (options.printCurrentPage->isChecked()) {
        p->setPrintRange(QPrinter::CurrentPage);
        p->setFromTo(0,0);
    } else if (options.printRange->isChecked()) {
        if (q->isOptionEnabled(QPrintDialog::PrintPageRange)) {
            p->setPrintRange(QPrinter::PageRange);
            p->setFromTo(options.from->value(), qMax(options.from->value(), options.to->value()));
        } else {
            // This case happens when CUPS server-side page range is enabled
            // Setting the range to the printer occurs below
            p->setPrintRange(QPrinter::AllPages);
            p->setFromTo(0,0);
        }
    }

#ifndef QT_NO_CUPS
    // page set
    if (p->printRange() == QPrinter::AllPages || p->printRange() == QPrinter::PageRange) {
        //If the application is selecting pages and the first page number is even then need to adjust the odd-even accordingly
        QCUPSSupport::PageSet pageSet = options.pageSetCombo->itemData(options.pageSetCombo->currentIndex()).value<QCUPSSupport::PageSet>();
        if (q->isOptionEnabled(QPrintDialog::PrintPageRange)
            && p->printRange() == QPrinter::PageRange
            && (q->fromPage() % 2 == 0)) {

            switch (pageSet) {
            case QCUPSSupport::AllPages:
                break;
            case QCUPSSupport::OddPages:
                QCUPSSupport::setPageSet(p, QCUPSSupport::EvenPages);
                break;
            case QCUPSSupport::EvenPages:
                QCUPSSupport::setPageSet(p, QCUPSSupport::OddPages);
                break;
            }
        } else if (pageSet != QCUPSSupport::AllPages) {
            QCUPSSupport::setPageSet(p, pageSet);
        }

        // server-side page range, since we set the page range on the printer to 0-0/AllPages above,
        // we need to take the values directly from the widget as q->fromPage() will return 0
        if (!q->isOptionEnabled(QPrintDialog::PrintPageRange) && options.printRange->isChecked())
            QCUPSSupport::setPageRange(p, options.from->value(), qMax(options.from->value(), options.to->value()));
    }
#endif

    // copies
    p->setCopyCount(options.copies->value());
    p->setCollateCopies(options.collate->isChecked());
}

void QPrintDialogPrivate::_q_togglePageSetCombo(bool checked)
{
    if (printerOutputFormat == QPrinter::PdfFormat)
        return;

    options.pageSetCombo->setDisabled(checked);
}

void QPrintDialogPrivate::_q_collapseOrExpandDialog()
{
    int collapseHeight = 0;
    Q_Q(QPrintDialog);
    QWidget *widgetToHide = bottom;
    if (widgetToHide->isVisible()) {
        collapseButton->setText(QPrintDialog::tr("&Options >>"));
        collapseHeight = widgetToHide->y() + widgetToHide->height() - (top->y() + top->height());
    }
    else
        collapseButton->setText(QPrintDialog::tr("&Options <<"));
    widgetToHide->setVisible(! widgetToHide->isVisible());
    if (! widgetToHide->isVisible()) { // make it shrink
        q->layout()->activate();
        q->resize( QSize(q->width(), q->height() - collapseHeight) );
    }
}

#ifndef QT_NO_MESSAGEBOX
void QPrintDialogPrivate::_q_checkFields()
{
    Q_Q(QPrintDialog);
    if (top->d->checkFields())
        q->accept();
}
#endif // QT_NO_MESSAGEBOX


void QPrintDialogPrivate::updateWidgets()
{
    Q_Q(QPrintDialog);
    options.gbPrintRange->setVisible(q->isOptionEnabled(QPrintDialog::PrintPageRange) ||
                                     q->isOptionEnabled(QPrintDialog::PrintSelection) ||
                                     q->isOptionEnabled(QPrintDialog::PrintCurrentPage));

    options.printRange->setEnabled(q->isOptionEnabled(QPrintDialog::PrintPageRange));
    options.printSelection->setVisible(q->isOptionEnabled(QPrintDialog::PrintSelection));
    options.printCurrentPage->setVisible(q->isOptionEnabled(QPrintDialog::PrintCurrentPage));
    options.collate->setVisible(q->isOptionEnabled(QPrintDialog::PrintCollateCopies));

#ifndef QT_NO_CUPS
    // Don't display Page Set if only Selection or Current Page are enabled
    if (!q->isOptionEnabled(QPrintDialog::PrintPageRange)
        && (q->isOptionEnabled(QPrintDialog::PrintSelection) || q->isOptionEnabled(QPrintDialog::PrintCurrentPage))) {
        options.pageSetCombo->setVisible(false);
        options.pageSetLabel->setVisible(false);
    } else {
        options.pageSetCombo->setVisible(true);
        options.pageSetLabel->setVisible(true);
    }

    if (!q->isOptionEnabled(QPrintDialog::PrintPageRange)) {
        // If we can do CUPS server side pages selection,
        // display the page range widgets
        options.gbPrintRange->setVisible(true);
        options.printRange->setEnabled(true);
    }
#endif

    switch (q->printRange()) {
    case QPrintDialog::AllPages:
        options.printAll->setChecked(true);
        options.pageSetCombo->setEnabled(true);
        break;
    case QPrintDialog::Selection:
        options.printSelection->setChecked(true);
        options.pageSetCombo->setEnabled(false);
        break;
    case QPrintDialog::PageRange:
        options.printRange->setChecked(true);
        options.pageSetCombo->setEnabled(true);
        break;
    case QPrintDialog::CurrentPage:
        if (q->isOptionEnabled(QPrintDialog::PrintCurrentPage)) {
            options.printCurrentPage->setChecked(true);
            options.pageSetCombo->setEnabled(false);
        }
        break;
    default:
        break;
    }
    const int minPage = qMax(1, qMin(q->minPage() , q->maxPage()));
    const int maxPage = qMax(1, q->maxPage() == INT_MAX ? 9999 : q->maxPage());

    options.from->setMinimum(minPage);
    options.to->setMinimum(minPage);
    options.from->setMaximum(maxPage);
    options.to->setMaximum(maxPage);

    options.from->setValue(q->fromPage());
    options.to->setValue(q->toPage());
    top->d->updateWidget();
}

void QPrintDialogPrivate::setTabs(const QList<QWidget*> &tabWidgets)
{
    QList<QWidget*>::ConstIterator iter = tabWidgets.begin();
    while(iter != tabWidgets.constEnd()) {
        QWidget *tab = *iter;
        options.tabs->addTab(tab, tab->windowTitle());
        ++iter;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*

    QPrintDialog

    The main Print Dialog.

*/

QPrintDialog::QPrintDialog(QPrinter *printer, QWidget *parent)
    : QAbstractPrintDialog(*(new QPrintDialogPrivate), printer, parent)
{
    Q_D(QPrintDialog);
    d->init();
}

/*!
    Constructs a print dialog with the given \a parent.
*/
QPrintDialog::QPrintDialog(QWidget *parent)
    : QAbstractPrintDialog(*(new QPrintDialogPrivate), 0, parent)
{
    Q_D(QPrintDialog);
    d->init();
}

QPrintDialog::~QPrintDialog()
{
}

void QPrintDialog::setVisible(bool visible)
{
    Q_D(QPrintDialog);

    if (visible)
        d->updateWidgets();

    QAbstractPrintDialog::setVisible(visible);
}

int QPrintDialog::exec()
{
    return QDialog::exec();
}

void QPrintDialog::accept()
{
    Q_D(QPrintDialog);
    d->setupPrinter();
    QDialog::accept();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*

    QUnixPrintWidget && QUnixPrintWidgetPrivate

    The upper half of the Print Dialog containing the Printer Selection widgets

*/

#if defined (Q_OS_UNIX)

/*! \internal
*/
QUnixPrintWidgetPrivate::QUnixPrintWidgetPrivate(QUnixPrintWidget *p, QPrinter *prn)
    : parent(p), propertiesDialog(0), printer(prn), optionsPane(0),
      filePrintersAdded(false), propertiesDialogShown(false)
{
    q = 0;
    if (parent)
        q = qobject_cast<QAbstractPrintDialog*> (parent->parent());

    widget.setupUi(parent);

    int currentPrinterIndex = 0;
    QStringList printers;
    QString defaultPrinter;
    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (ps) {
        printers = ps->availablePrintDeviceIds();
        defaultPrinter = ps->defaultPrintDeviceId();
    }

    for (int i = 0; i < printers.size(); ++i) {
        widget.printers->addItem(printers.at(i));
        if (printers.at(i) == defaultPrinter)
            currentPrinterIndex = i;
    }
    widget.properties->setEnabled(true);

#if !defined(QT_NO_FILESYSTEMMODEL) && !defined(QT_NO_COMPLETER)
    QFileSystemModel *fsm = new QFileSystemModel(widget.filename);
    fsm->setRootPath(QDir::homePath());
    widget.filename->setCompleter(new QCompleter(fsm, widget.filename));
#endif
    _q_printerChanged(currentPrinterIndex);

    QObject::connect(widget.printers, SIGNAL(currentIndexChanged(int)),
                     parent, SLOT(_q_printerChanged(int)));
    QObject::connect(widget.fileBrowser, SIGNAL(clicked()), parent, SLOT(_q_btnBrowseClicked()));
    QObject::connect(widget.properties, SIGNAL(clicked()), parent, SLOT(_q_btnPropertiesClicked()));

    // disable features that QPrinter does not yet support.
    widget.preview->setVisible(false);
}

void QUnixPrintWidgetPrivate::updateWidget()
{
    const bool printToFile = q == 0 || q->isOptionEnabled(QPrintDialog::PrintToFile);
    if (printToFile && !filePrintersAdded) {
        if (widget.printers->count())
            widget.printers->insertSeparator(widget.printers->count());
        widget.printers->addItem(QPrintDialog::tr("Print to File (PDF)"));
        filePrintersAdded = true;
    }
    if (!printToFile && filePrintersAdded) {
        widget.printers->removeItem(widget.printers->count()-1);
        widget.printers->removeItem(widget.printers->count()-1);
        if (widget.printers->count())
            widget.printers->removeItem(widget.printers->count()-1); // remove separator
        filePrintersAdded = false;
    }
    if (printer && filePrintersAdded && (printer->outputFormat() != QPrinter::NativeFormat
                                         || printer->printerName().isEmpty()))
    {
        if (printer->outputFormat() == QPrinter::PdfFormat)
            widget.printers->setCurrentIndex(widget.printers->count() - 1);
        widget.filename->setEnabled(true);
        widget.lOutput->setEnabled(true);
    }

    widget.filename->setVisible(printToFile);
    widget.lOutput->setVisible(printToFile);
    widget.fileBrowser->setVisible(printToFile);

    widget.properties->setVisible(q->isOptionEnabled(QAbstractPrintDialog::PrintShowPageSize));
}

QUnixPrintWidgetPrivate::~QUnixPrintWidgetPrivate()
{
}

void QUnixPrintWidgetPrivate::_q_printerChanged(int index)
{
    if (index < 0)
        return;
    const int printerCount = widget.printers->count();
    widget.filename->setEnabled(false);
    widget.lOutput->setEnabled(false);

    // Reset properties dialog when printer is changed
    if (propertiesDialog){
        delete propertiesDialog;
        propertiesDialog = 0;
        propertiesDialogShown = false;
    }

    if (filePrintersAdded) {
        Q_ASSERT(index != printerCount - 2); // separator
        if (index == printerCount - 1) { // PDF
            widget.location->setText(QPrintDialog::tr("Local file"));
            widget.type->setText(QPrintDialog::tr("Write PDF file"));
            widget.properties->setEnabled(true);
            widget.filename->setEnabled(true);
            QString filename = widget.filename->text();
            widget.filename->setText(filename);
            widget.lOutput->setEnabled(true);
            if (optionsPane)
                optionsPane->selectPrinter(QPrinter::PdfFormat);
            return;
        }
    }

    if (printer) {
        QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
        if (ps)
            m_currentPrintDevice = ps->createPrintDevice(widget.printers->itemText(index));

        printer->setPrinterName(m_currentPrintDevice.id());

        widget.location->setText(m_currentPrintDevice.location());
        widget.type->setText(m_currentPrintDevice.makeAndModel());
        if (optionsPane)
            optionsPane->selectPrinter(QPrinter::NativeFormat);
    }
}

void QUnixPrintWidgetPrivate::setOptionsPane(QPrintDialogPrivate *pane)
{
    optionsPane = pane;
    if (optionsPane)
        optionsPane->selectPrinter(QPrinter::NativeFormat);
}

void QUnixPrintWidgetPrivate::_q_btnBrowseClicked()
{
    QString filename = widget.filename->text();
#ifndef QT_NO_FILEDIALOG
    filename = QFileDialog::getSaveFileName(parent, QPrintDialog::tr("Print To File ..."), filename,
                                            QString(), 0, QFileDialog::DontConfirmOverwrite);
#else
    filename.clear();
#endif
    if (!filename.isEmpty()) {
        widget.filename->setText(filename);
        widget.printers->setCurrentIndex(widget.printers->count() - 1); // the pdf one
    }
}

void QUnixPrintWidgetPrivate::applyPrinterProperties()
{
    if (printer == 0)
        return;
    if (printer->outputFileName().isEmpty()) {
        QString home = QDir::homePath();
        QString cur = QDir::currentPath();
        if (home.at(home.length()-1) != QLatin1Char('/'))
            home += QLatin1Char('/');
        if (cur.at(cur.length()-1) != QLatin1Char('/'))
            cur += QLatin1Char('/');
        if (cur.left(home.length()) != home)
            cur = home;
        if (QGuiApplication::platformName() == QLatin1String("xcb")) {
            if (printer->docName().isEmpty()) {
                cur += QLatin1String("print.pdf");
            } else {
                QRegExp re(QString::fromLatin1("(.*)\\.\\S+"));
                if (re.exactMatch(printer->docName()))
                    cur += re.cap(1);
                else
                    cur += printer->docName();
                cur += QLatin1String(".pdf");
            }
        } // xcb

        widget.filename->setText(cur);
    }
    else
        widget.filename->setText( printer->outputFileName() );
    QString printerName = printer->printerName();
    if (!printerName.isEmpty()) {
        for (int i = 0; i < widget.printers->count(); ++i) {
            if (widget.printers->itemText(i) == printerName) {
                widget.printers->setCurrentIndex(i);
                break;
            }
        }
    }
    // PDF printer not added to the dialog yet, we'll handle those cases in QUnixPrintWidgetPrivate::updateWidget

    if (propertiesDialog)
        propertiesDialog->applyPrinterProperties(printer);
}

#ifndef QT_NO_MESSAGEBOX
bool QUnixPrintWidgetPrivate::checkFields()
{
    if (widget.filename->isEnabled()) {
        QString file = widget.filename->text();
        QFile f(file);
        QFileInfo fi(f);
        bool exists = fi.exists();
        bool opened = false;
        if (exists && fi.isDir()) {
            QMessageBox::warning(q, q->windowTitle(),
                            QPrintDialog::tr("%1 is a directory.\nPlease choose a different file name.").arg(file));
            return false;
        } else if ((exists && !fi.isWritable()) || !(opened = f.open(QFile::Append))) {
            QMessageBox::warning(q, q->windowTitle(),
                            QPrintDialog::tr("File %1 is not writable.\nPlease choose a different file name.").arg(file));
            return false;
        } else if (exists) {
            int ret = QMessageBox::question(q, q->windowTitle(),
                                            QPrintDialog::tr("%1 already exists.\nDo you want to overwrite it?").arg(file),
                                            QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::No)
                return false;
        }
        if (opened) {
            f.close();
            if (!exists)
                f.remove();
        }
    }

#ifndef QT_NO_CUPS
    if (propertiesDialogShown) {
        QCUPSSupport::PagesPerSheet pagesPerSheet = propertiesDialog->widget.pageSetup->m_ui.pagesPerSheetCombo
                                                                    ->currentData().value<QCUPSSupport::PagesPerSheet>();

        QCUPSSupport::PageSet pageSet = optionsPane->options.pageSetCombo->currentData().value<QCUPSSupport::PageSet>();


        if (pagesPerSheet != QCUPSSupport::OnePagePerSheet
            && pageSet != QCUPSSupport::AllPages) {
            QMessageBox::warning(q, q->windowTitle(),
                                 QPrintDialog::tr("Options 'Pages Per Sheet' and 'Page Set' cannot be used together.\nPlease turn one of those options off."));
            return false;
        }
    }
#endif

    // Every test passed. Accept the dialog.
    return true;
}
#endif // QT_NO_MESSAGEBOX

void QUnixPrintWidgetPrivate::setupPrinterProperties()
{
    if (propertiesDialog)
        delete propertiesDialog;

    propertiesDialog = new QPrintPropertiesDialog(q);
    propertiesDialog->setResult(QDialog::Rejected);
    propertiesDialogShown = false;

    propertiesDialog->applyPrinterProperties(q->printer());

    if (q->isOptionEnabled(QPrintDialog::PrintToFile)
        && (widget.printers->currentIndex() == widget.printers->count() - 1)) {// PDF
        propertiesDialog->selectPrinter(QPrinter::PdfFormat, QString());
    }
    else
        propertiesDialog->selectPrinter(QPrinter::NativeFormat, widget.printers->currentText());
}

void QUnixPrintWidgetPrivate::_q_btnPropertiesClicked()
{
    if (!propertiesDialog)
        setupPrinterProperties();
    propertiesDialog->exec();
    if (!propertiesDialogShown && propertiesDialog->result() == QDialog::Rejected) {
        // If properties dialog was rejected the dialog is deleted and
        // the properties are set to defaults when printer is setup
        delete propertiesDialog;
        propertiesDialog = 0;
        propertiesDialogShown = false;
    } else
        // properties dialog was shown and accepted
        propertiesDialogShown = true;
}

void QUnixPrintWidgetPrivate::setupPrinter()
{
    const int printerCount = widget.printers->count();
    const int index = widget.printers->currentIndex();

    if (filePrintersAdded && index == printerCount - 1) { // PDF
        printer->setPrinterName(QString());
        Q_ASSERT(index != printerCount - 2); // separator
        printer->setOutputFormat(QPrinter::PdfFormat);
        QString path = widget.filename->text();
        if (QDir::isRelativePath(path))
            path = QDir::homePath() + QDir::separator() + path;
        printer->setOutputFileName(path);
    }
    else {
        printer->setPrinterName(widget.printers->currentText());
        printer->setOutputFileName(QString());
    }

    if (!propertiesDialog)
        setupPrinterProperties();

    if (propertiesDialog->result() == QDialog::Accepted || !propertiesDialogShown)
        propertiesDialog->setupPrinter();
}

/*! \internal
*/
QUnixPrintWidget::QUnixPrintWidget(QPrinter *printer, QWidget *parent)
    : QWidget(parent), d(new QUnixPrintWidgetPrivate(this, printer))
{
    d->applyPrinterProperties();
}

/*! \internal
*/
QUnixPrintWidget::~QUnixPrintWidget()
{
    delete d;
}

/*! \internal

    Updates the printer with the states held in the QUnixPrintWidget.
*/
void QUnixPrintWidget::updatePrinter()
{
    d->setupPrinter();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#endif // defined (Q_OS_UNIX)

QT_END_NAMESPACE

#include "moc_qprintdialog.cpp"
#include "qprintdialog_unix.moc"

#endif // QT_NO_PRINTDIALOG

