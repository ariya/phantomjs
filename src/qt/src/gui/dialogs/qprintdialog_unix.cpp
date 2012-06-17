/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qplatformdefs.h"

#ifndef QT_NO_PRINTDIALOG

#include "private/qabstractprintdialog_p.h"
#include <QtGui/qmessagebox.h>
#include "qprintdialog.h"
#include "qfiledialog.h"
#include <QtCore/qdir.h>
#include <QtGui/qevent.h>
#include <QtGui/qfilesystemmodel.h>
#include <QtGui/qstyleditemdelegate.h>
#include <QtGui/qprinter.h>

#include <QtGui/qdialogbuttonbox.h>

#include "qfscompleter_p.h"
#include "ui_qprintpropertieswidget.h"
#include "ui_qprintsettingsoutput.h"
#include "ui_qprintwidget.h"

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
#  include <private/qcups_p.h>
#  include <cups/cups.h>
#  include <private/qpdf_p.h>
#else
#  include <QtCore/qlibrary.h>
#endif

#include <private/qprinterinfo_unix_p.h>

QT_BEGIN_NAMESPACE

class QOptionTreeItem;
class QPPDOptionsModel;

class QPrintPropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    QPrintPropertiesDialog(QAbstractPrintDialog *parent = 0);
    ~QPrintPropertiesDialog();

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    void setCups(QCUPSSupport *cups) { m_cups = cups; }
    void addItemToOptions(QOptionTreeItem *parent, QList<const ppd_option_t*>& options, QList<const char*>& markedOptions) const;
#endif

    void selectPrinter();
    void selectPdfPsPrinter(const QPrinter *p);

    /// copy printer properties to the widget
    void applyPrinterProperties(QPrinter *p);
    void setupPrinter() const;

protected:
    void showEvent(QShowEvent* event);

private:
    Ui::QPrintPropertiesWidget widget;
    QDialogButtonBox *m_buttons;
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    QCUPSSupport *m_cups;
    QPPDOptionsModel *m_cupsOptionsModel;
#endif
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
    void applyPrinterProperties(QPrinter *p);

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    void selectPrinter(QCUPSSupport *cups);
#endif

    void _q_chbPrintLastFirstToggled(bool);
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
};

#if defined (Q_OS_UNIX)
class QUnixPrintWidgetPrivate
{
public:
    QUnixPrintWidgetPrivate(QUnixPrintWidget *q);
    ~QUnixPrintWidgetPrivate();

    /// copy printer properties to the widget
    void applyPrinterProperties(QPrinter *p);
    bool checkFields();
    void setupPrinter();
    void setOptionsPane(QPrintDialogPrivate *pane);
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    void setCupsProperties();
#endif

// slots
    void _q_printerChanged(int index);
    void _q_btnPropertiesClicked();
    void _q_btnBrowseClicked();

    QUnixPrintWidget * const parent;
    QPrintPropertiesDialog *propertiesDialog;
    Ui::QPrintWidget widget;
    QAbstractPrintDialog * q;
    QPrinter *printer;
    QList<QPrinterDescription> lprPrinters;
    void updateWidget();

private:
    QPrintDialogPrivate *optionsPane;
    bool filePrintersAdded;
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    QCUPSSupport* cups;
    int cupsPrinterCount;
    const cups_dest_t* cupsPrinters;
    const ppd_file_t* cupsPPD;
#endif
};
#endif

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
class QOptionTreeItem
{
public:
    enum ItemType { Root, Group, Option, Choice };

    QOptionTreeItem(ItemType t, int i, const void* p, const char* desc, QOptionTreeItem* pi)
        : type(t),
          index(i),
          ptr(p),
          description(desc),
          selected(-1),
          selDescription(0),
          parentItem(pi) {}

    ~QOptionTreeItem() {
        while (!childItems.isEmpty())
            delete childItems.takeFirst();
    }

    ItemType type;
    int index;
    const void* ptr;
    const char* description;
    int selected;
    const char* selDescription;
    QOptionTreeItem* parentItem;
    QList<QOptionTreeItem*> childItems;
};

class QPPDOptionsModel : public QAbstractItemModel
{
    friend class QPPDOptionsEditor;
public:
    QPPDOptionsModel(QCUPSSupport *cups, QObject *parent = 0);
    ~QPPDOptionsModel();

    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;
    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    QOptionTreeItem* rootItem;
    QCUPSSupport *cups;
    const ppd_file_t* ppd;
    void parseItems();
    void parseGroups(QOptionTreeItem* parent);
    void parseOptions(QOptionTreeItem* parent);
    void parseChoices(QOptionTreeItem* parent);
};

class QPPDOptionsEditor : public QStyledItemDelegate
{
    Q_OBJECT
public:
    QPPDOptionsEditor(QObject* parent = 0) : QStyledItemDelegate(parent) {}
    ~QPPDOptionsEditor() {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void setEditorData(QWidget* editor, const QModelIndex& index) const;
    void setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;

private slots:
    void cbChanged(int index);

};

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QPrintPropertiesDialog::QPrintPropertiesDialog(QAbstractPrintDialog *parent)
    : QDialog(parent)
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    , m_cups(0), m_cupsOptionsModel(0)
#endif
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
}

QPrintPropertiesDialog::~QPrintPropertiesDialog()
{
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    delete m_cupsOptionsModel;
#else
    delete widget.cupsPropertiesPage;
#endif
}

void QPrintPropertiesDialog::applyPrinterProperties(QPrinter *p)
{
    widget.pageSetup->setPrinter(p);
}

void QPrintPropertiesDialog::setupPrinter() const
{
    widget.pageSetup->setupPrinter();

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    QPPDOptionsModel* model = static_cast<QPPDOptionsModel*>(widget.treeView->model());
    if (model) {
        QOptionTreeItem* rootItem = model->rootItem;
        QList<const ppd_option_t*> options;
        QList<const char*> markedOptions;

        addItemToOptions(rootItem, options, markedOptions);
        model->cups->saveOptions(options, markedOptions);
    }
#endif
}

void QPrintPropertiesDialog::selectPrinter()
{
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    widget.pageSetup->selectPrinter(m_cups);
    widget.treeView->setModel(0);
    if (m_cups && QCUPSSupport::isAvailable()) {

        if (m_cupsOptionsModel == 0) {
            m_cupsOptionsModel = new QPPDOptionsModel(m_cups);

            widget.treeView->setItemDelegate(new QPPDOptionsEditor(this));
        } else {
            // update the model
            m_cupsOptionsModel->parseItems();
        }

        if (m_cupsOptionsModel->rowCount() > 0) {
            widget.treeView->setModel(m_cupsOptionsModel);

            for (int i = 0; i < m_cupsOptionsModel->rowCount(); ++i)
                widget.treeView->expand(m_cupsOptionsModel->index(i,0));

            widget.tabs->setTabEnabled(1, true); // enable the advanced tab
        } else {
            widget.tabs->setTabEnabled(1, false);
        }

    } else
#endif
    {
        widget.cupsPropertiesPage->setEnabled(false);
        widget.pageSetup->selectPrinter(0);
    }
}

void QPrintPropertiesDialog::selectPdfPsPrinter(const QPrinter *p)
{
    widget.treeView->setModel(0);
    widget.pageSetup->selectPdfPsPrinter(p);
    widget.tabs->setTabEnabled(1, false); // disable the advanced tab
}

void QPrintPropertiesDialog::showEvent(QShowEvent* event)
{
    widget.treeView->resizeColumnToContents(0);
    event->accept();
}

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
void QPrintPropertiesDialog::addItemToOptions(QOptionTreeItem *parent, QList<const ppd_option_t*>& options, QList<const char*>& markedOptions) const
{
    for (int i = 0; i < parent->childItems.count(); ++i) {
        QOptionTreeItem *itm = parent->childItems.at(i);
        if (itm->type == QOptionTreeItem::Option) {
            const ppd_option_t* opt = reinterpret_cast<const ppd_option_t*>(itm->ptr);
            options << opt;
            if (qstrcmp(opt->defchoice, opt->choices[itm->selected].choice) != 0) {
                markedOptions << opt->keyword << opt->choices[itm->selected].choice;
            }
        } else {
            addItemToOptions(itm, options, markedOptions);
        }
    }
}
#endif

QPrintDialogPrivate::QPrintDialogPrivate()
    : top(0), bottom(0), buttons(0), collapseButton(0)
{
}

QPrintDialogPrivate::~QPrintDialogPrivate()
{
}

void QPrintDialogPrivate::init()
{
    Q_Q(QPrintDialog);

    top = new QUnixPrintWidget(0, q);
    bottom = new QWidget(q);
    options.setupUi(bottom);
    options.color->setIconSize(QSize(32, 32));
    options.color->setIcon(QIcon(QLatin1String(":/trolltech/dialogs/qprintdialog/images/status-color.png")));
    options.grayscale->setIconSize(QSize(32, 32));
    options.grayscale->setIcon(QIcon(QLatin1String(":/trolltech/dialogs/qprintdialog/images/status-gray-scale.png")));
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

    QPrinter* p = q->printer();

    applyPrinterProperties(p);

#ifdef QT_NO_MESSAGEBOX
    QObject::connect(buttons, SIGNAL(accepted()), q, SLOT(accept()));
#else
    QObject::connect(buttons, SIGNAL(accepted()), q, SLOT(_q_checkFields()));
#endif
    QObject::connect(buttons, SIGNAL(rejected()), q, SLOT(reject()));

    QObject::connect(options.reverse, SIGNAL(toggled(bool)),
                     q, SLOT(_q_chbPrintLastFirstToggled(bool)));

    QObject::connect(collapseButton, SIGNAL(released()), q, SLOT(_q_collapseOrExpandDialog()));
}

void QPrintDialogPrivate::applyPrinterProperties(QPrinter *p)
{
    if (p->colorMode() == QPrinter::Color)
        options.color->setChecked(true);
    else
        options.grayscale->setChecked(true);

    switch(p->duplex()) {
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
    top->d->applyPrinterProperties(p);
}

void QPrintDialogPrivate::_q_chbPrintLastFirstToggled(bool checked)
{
    Q_Q(QPrintDialog);
    if (checked)
        q->printer()->setPageOrder(QPrinter::LastPageFirst);
    else
        q->printer()->setPageOrder(QPrinter::FirstPageFirst);
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

void QPrintDialogPrivate::setupPrinter()
{
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

    p->setColorMode( options.color->isChecked() ? QPrinter::Color : QPrinter::GrayScale );

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
        p->setPrintRange(QPrinter::PageRange);
        p->setFromTo(options.from->value(), qMax(options.from->value(), options.to->value()));
    }

    // copies
    p->setCopyCount(options.copies->value());
    p->setCollateCopies(options.collate->isChecked());

    top->d->setupPrinter();
}

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

    switch (q->printRange()) {
    case QPrintDialog::AllPages:
        options.printAll->setChecked(true);
        break;
    case QPrintDialog::Selection:
        options.printSelection->setChecked(true);
        break;
    case QPrintDialog::PageRange:
        options.printRange->setChecked(true);
        break;
    case QPrintDialog::CurrentPage:
        if (q->isOptionEnabled(QPrintDialog::PrintCurrentPage))
            options.printCurrentPage->setChecked(true);
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
    while(options.tabs->count() > 2)
        delete options.tabs->widget(2);

    QList<QWidget*>::ConstIterator iter = tabWidgets.begin();
    while(iter != tabWidgets.constEnd()) {
        QWidget *tab = *iter;
        options.tabs->addTab(tab, tab->windowTitle());
        ++iter;
    }
}

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
void QPrintDialogPrivate::selectPrinter(QCUPSSupport *cups)
{
    options.duplex->setEnabled(cups && cups->ppdOption("Duplex"));
}
#endif

////////////////////////////////////////////////////////////////////////////////

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

#ifdef QT3_SUPPORT
QPrinter *QPrintDialog::printer() const
{
    Q_D(const QPrintDialog);
    return d->printer;
}

void QPrintDialog::setPrinter(QPrinter *printer, bool pickupSettings)
{
    if (!printer)
        return;

    Q_D(QPrintDialog);
    d->printer = printer;

    if (pickupSettings)
        d->applyPrinterProperties(printer);
}

void QPrintDialog::addButton(QPushButton *button)
{
    Q_D(QPrintDialog);
    d->buttons->addButton(button, QDialogButtonBox::HelpRole);
}
#endif // QT3_SUPPORT

#if defined (Q_OS_UNIX)

/*! \internal
*/
QUnixPrintWidgetPrivate::QUnixPrintWidgetPrivate(QUnixPrintWidget *p)
    : parent(p), propertiesDialog(0), printer(0), optionsPane(0), filePrintersAdded(false)
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    , cups(0), cupsPrinterCount(0), cupsPrinters(0), cupsPPD(0)
#endif
{
    q = 0;
    if (parent)
        q = qobject_cast<QAbstractPrintDialog*> (parent->parent());

    widget.setupUi(parent);

    int currentPrinterIndex = 0;
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    cups = new QCUPSSupport;
    if (QCUPSSupport::isAvailable()) {
        cupsPPD = cups->currentPPD();
        cupsPrinterCount = cups->availablePrintersCount();
        cupsPrinters = cups->availablePrinters();

        for (int i = 0; i < cupsPrinterCount; ++i) {
            QString printerName(QString::fromLocal8Bit(cupsPrinters[i].name));
            if (cupsPrinters[i].instance)
                printerName += QLatin1Char('/') + QString::fromLocal8Bit(cupsPrinters[i].instance);

            widget.printers->addItem(printerName);
            if (cupsPrinters[i].is_default)
                widget.printers->setCurrentIndex(i);
        }
        // the model depends on valid ppd. so before enabling the
        // properties button we make sure the ppd is in fact valid.
        if (cupsPrinterCount && cups->currentPPD()) {
            widget.properties->setEnabled(true);
        }
        currentPrinterIndex = cups->currentPrinterIndex();
    } else {
#endif
        currentPrinterIndex = qt_getLprPrinters(lprPrinters);
        // populating printer combo
        QList<QPrinterDescription>::const_iterator i = lprPrinters.constBegin();
        for(; i != lprPrinters.constEnd(); ++i)
            widget.printers->addItem((*i).name);
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    }
#endif

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
        widget.printers->addItem(QPrintDialog::tr("Print to File (Postscript)"));
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
            widget.printers->setCurrentIndex(widget.printers->count() - 2);
        else if (printer->outputFormat() == QPrinter::PostScriptFormat)
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
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    delete cups;
#endif
}

void QUnixPrintWidgetPrivate::_q_printerChanged(int index)
{
    if (index < 0)
        return;
    const int printerCount = widget.printers->count();
    widget.filename->setEnabled(false);
    widget.lOutput->setEnabled(false);

    if (filePrintersAdded) {
        Q_ASSERT(index != printerCount - 3); // separator
        if (index > printerCount - 3) { // PDF or postscript
            bool pdfPrinter = (index == printerCount - 2);
            widget.location->setText(QPrintDialog::tr("Local file"));
            widget.type->setText(QPrintDialog::tr("Write %1 file").arg(pdfPrinter ? QString::fromLatin1("PDF")
                                                                       : QString::fromLatin1("PostScript")));
            widget.properties->setEnabled(true);
            widget.filename->setEnabled(true);
            QString filename = widget.filename->text();
            QString suffix = QFileInfo(filename).suffix();
            if (pdfPrinter && suffix == QLatin1String("ps"))
                filename = filename.replace(QLatin1String(".ps"), QLatin1String(".pdf"));
            if (!pdfPrinter && suffix == QLatin1String("pdf"))
                filename = filename.replace(QLatin1String(".pdf"), QLatin1String(".ps"));
            widget.filename->setText(filename);
            widget.lOutput->setEnabled(true);
            if (propertiesDialog)
                propertiesDialog->selectPdfPsPrinter(printer);
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
            if (optionsPane)
                optionsPane->selectPrinter(0);
#endif
            return;
        }
    }

    widget.location->setText(QString());
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    if (QCUPSSupport::isAvailable()) {
        cups->setCurrentPrinter(index);

        const cups_option_t *opt = cups->printerOption(QString::fromLatin1("printer-location"));
        QString location;
        if (opt)
            location = QString::fromLocal8Bit(opt->value);
        widget.location->setText(location);

        cupsPPD = cups->currentPPD();
        // set printer type line
        QString type;
        if (cupsPPD)
            type = QString::fromLocal8Bit(cupsPPD->manufacturer) + QLatin1String(" - ") + QString::fromLocal8Bit(cupsPPD->modelname);
        widget.type->setText(type);
        if (propertiesDialog)
            propertiesDialog->selectPrinter();
        if (optionsPane)
            optionsPane->selectPrinter(cups);
    } else {
        if (optionsPane)
            optionsPane->selectPrinter(0);
#endif
        if (lprPrinters.count() > 0) {
            QString type = lprPrinters.at(index).name + QLatin1Char('@') + lprPrinters.at(index).host;
            if (!lprPrinters.at(index).comment.isEmpty())
            type += QLatin1String(", ") + lprPrinters.at(index).comment;
            widget.type->setText(type);
            if (propertiesDialog)
                propertiesDialog->selectPrinter();
        }
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    }
#endif
}

void QUnixPrintWidgetPrivate::setOptionsPane(QPrintDialogPrivate *pane)
{
    optionsPane = pane;
    if (optionsPane)
        _q_printerChanged(widget.printers->currentIndex());
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
        if (filename.endsWith(QString::fromLatin1(".ps"), Qt::CaseInsensitive))
            widget.printers->setCurrentIndex(widget.printers->count() - 1); // the postscript one
        else if (filename.endsWith(QString::fromLatin1(".pdf"), Qt::CaseInsensitive))
            widget.printers->setCurrentIndex(widget.printers->count() - 2); // the pdf one
        else if (widget.printers->currentIndex() != widget.printers->count() - 1) // if ps is not selected, pdf is default
            widget.printers->setCurrentIndex(widget.printers->count() - 2); // the pdf one
    }
}

void QUnixPrintWidgetPrivate::applyPrinterProperties(QPrinter *p)
{
    if (p == 0)
        return;
    printer = p;
    if (p->outputFileName().isEmpty()) {
        QString home = QString::fromLocal8Bit(qgetenv("HOME").constData());
        QString cur = QDir::currentPath();
        if (home.at(home.length()-1) != QLatin1Char('/'))
            home += QLatin1Char('/');
        if (cur.at(cur.length()-1) != QLatin1Char('/'))
            cur += QLatin1Char('/');
        if (cur.left(home.length()) != home)
            cur = home;
#ifdef Q_WS_X11
        if (p->docName().isEmpty()) {
            if (p->outputFormat() == QPrinter::PostScriptFormat)
                cur += QLatin1String("print.ps");
            else
                cur += QLatin1String("print.pdf");
        } else {
            QRegExp re(QString::fromLatin1("(.*)\\.\\S+"));
            if (re.exactMatch(p->docName()))
                cur += re.cap(1);
            else
                cur += p->docName();
            if (p->outputFormat() == QPrinter::PostScriptFormat)
                cur += QLatin1String(".ps");
            else
                cur += QLatin1String(".pdf");
        }
#endif
        widget.filename->setText(cur);
    }
    else
        widget.filename->setText( p->outputFileName() );
    QString printer = p->printerName();
    if (!printer.isEmpty()) {
        for (int i = 0; i < widget.printers->count(); ++i) {
            if (widget.printers->itemText(i) == printer) {
                widget.printers->setCurrentIndex(i);
                break;
            }
        }
    }
    // PDF and PS printers are not added to the dialog yet, we'll handle those cases in QUnixPrintWidgetPrivate::updateWidget

    if (propertiesDialog)
        propertiesDialog->applyPrinterProperties(p);
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

    // Every test passed. Accept the dialog.
    return true;
}
#endif // QT_NO_MESSAGEBOX

void QUnixPrintWidgetPrivate::_q_btnPropertiesClicked()
{
    if (!propertiesDialog) {
        propertiesDialog = new QPrintPropertiesDialog(q);
        propertiesDialog->setResult(QDialog::Rejected);
    }

    if (propertiesDialog->result() == QDialog::Rejected) {
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
        propertiesDialog->setCups(cups);
#endif
        propertiesDialog->applyPrinterProperties(q->printer());

        if (q->isOptionEnabled(QPrintDialog::PrintToFile)
            && (widget.printers->currentIndex() > widget.printers->count() - 3)) // PDF or postscript
            propertiesDialog->selectPdfPsPrinter(q->printer());
        else
            propertiesDialog->selectPrinter();
    }
    propertiesDialog->exec();
}

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
void QUnixPrintWidgetPrivate::setCupsProperties()
{
    if (cups && QCUPSSupport::isAvailable() && cups->pageSizes()) {
        QPrintEngine *engine = printer->printEngine();
        const ppd_option_t* pageSizes = cups->pageSizes();
        QByteArray cupsPageSize;
        for (int i = 0; i < pageSizes->num_choices; ++i) {
            if (static_cast<int>(pageSizes->choices[i].marked) == 1)
                cupsPageSize = pageSizes->choices[i].choice;
        }
        engine->setProperty(PPK_CupsStringPageSize, QString::fromLatin1(cupsPageSize));
        engine->setProperty(PPK_CupsOptions, cups->options());

        QRect pageRect = cups->pageRect(cupsPageSize);
        engine->setProperty(PPK_CupsPageRect, pageRect);

        QRect paperRect = cups->paperRect(cupsPageSize);
        engine->setProperty(PPK_CupsPaperRect, paperRect);

        for (int ps = 0; ps < QPrinter::NPaperSize; ++ps) {
            QPdf::PaperSize size = QPdf::paperSize(QPrinter::PaperSize(ps));
            if (size.width == paperRect.width() && size.height == paperRect.height())
                printer->setPaperSize(static_cast<QPrinter::PaperSize>(ps));
        }
    }
}
#endif

void QUnixPrintWidgetPrivate::setupPrinter()
{
    const int printerCount = widget.printers->count();
    const int index = widget.printers->currentIndex();

    if (filePrintersAdded && index > printerCount - 3) { // PDF or postscript
        printer->setPrinterName(QString());
        Q_ASSERT(index != printerCount - 3); // separator
        if (index == printerCount - 2)
            printer->setOutputFormat(QPrinter::PdfFormat);
        else
            printer->setOutputFormat(QPrinter::PostScriptFormat);
        QString path = widget.filename->text();
        if (QDir::isRelativePath(path))
            path = QDir::homePath() + QDir::separator() + path;
        printer->setOutputFileName(path);
    }
    else {
        printer->setPrinterName(widget.printers->currentText());
        printer->setOutputFileName(QString());
    }

    if (propertiesDialog && propertiesDialog->result() == QDialog::Accepted)
        propertiesDialog->setupPrinter();
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    if (!propertiesDialog)
        setCupsProperties();
#endif
}


/*! \internal
*/
QUnixPrintWidget::QUnixPrintWidget(QPrinter *printer, QWidget *parent)
    : QWidget(parent), d(new QUnixPrintWidgetPrivate(this))
{
    d->applyPrinterProperties(printer);
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

#endif

////////////////////////////////////////////////////////////////////////////////
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)

QPPDOptionsModel::QPPDOptionsModel(QCUPSSupport *c, QObject *parent)
    : QAbstractItemModel(parent), rootItem(0), cups(c), ppd(c->currentPPD())
{
    parseItems();
}

QPPDOptionsModel::~QPPDOptionsModel()
{
}

int QPPDOptionsModel::columnCount(const QModelIndex&) const
{
    return 2;
}

int QPPDOptionsModel::rowCount(const QModelIndex& parent) const
{
    QOptionTreeItem* itm;
    if (!parent.isValid())
        itm = rootItem;
    else
        itm = reinterpret_cast<QOptionTreeItem*>(parent.internalPointer());

    if (itm->type == QOptionTreeItem::Option)
        return 0;

    return itm->childItems.count();
}

QVariant QPPDOptionsModel::data(const QModelIndex& index, int role) const
{
    switch(role) {
        case Qt::FontRole: {
            QOptionTreeItem* itm = reinterpret_cast<QOptionTreeItem*>(index.internalPointer());
            if (itm && itm->type == QOptionTreeItem::Group){
                QFont font = QApplication::font();
                font.setBold(true);
                return QVariant(font);
            }
            return QVariant();
        }
        break;

        case Qt::DisplayRole: {
            QOptionTreeItem* itm;
            if (!index.isValid())
                itm = rootItem;
            else
                itm = reinterpret_cast<QOptionTreeItem*>(index.internalPointer());

            if (index.column() == 0)
                return cups->unicodeString(itm->description);
            else if (itm->type == QOptionTreeItem::Option && itm->selected > -1)
                return cups->unicodeString(itm->selDescription);
            else
                return QVariant();
        }
        break;

        default:
            return QVariant();
    }
    if (role != Qt::DisplayRole)
        return QVariant();
}

QModelIndex QPPDOptionsModel::index(int row, int column, const QModelIndex& parent) const
{
    QOptionTreeItem* itm;
    if (!parent.isValid())
        itm = rootItem;
    else
        itm = reinterpret_cast<QOptionTreeItem*>(parent.internalPointer());

    return createIndex(row, column, itm->childItems.at(row));
}


QModelIndex QPPDOptionsModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    QOptionTreeItem* itm = reinterpret_cast<QOptionTreeItem*>(index.internalPointer());

    if (itm->parentItem && itm->parentItem != rootItem)
        return createIndex(itm->parentItem->index, 0, itm->parentItem);
    else
        return QModelIndex();
}

Qt::ItemFlags QPPDOptionsModel::flags(const QModelIndex& index) const
{
    if (!index.isValid() || reinterpret_cast<QOptionTreeItem*>(index.internalPointer())->type == QOptionTreeItem::Group)
        return Qt::ItemIsEnabled;

    if (index.column() == 1)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void QPPDOptionsModel::parseItems()
{
    emit layoutAboutToBeChanged();
    ppd = cups->currentPPD();
    delete rootItem;
    rootItem = new QOptionTreeItem(QOptionTreeItem::Root, 0, ppd, "Root Item", 0);
    parseGroups(rootItem);
    emit layoutChanged();
}

void QPPDOptionsModel::parseGroups(QOptionTreeItem* parent)
{
    if (parent->type == QOptionTreeItem::Root) {

        const ppd_file_t* ppdFile = reinterpret_cast<const ppd_file_t*>(parent->ptr);

        if (ppdFile) {
            for (int i = 0; i < ppdFile->num_groups; ++i) {
                QOptionTreeItem* group = new QOptionTreeItem(QOptionTreeItem::Group, i, &ppdFile->groups[i], ppdFile->groups[i].text, parent);
                parent->childItems.append(group);
                parseGroups(group); // parse possible subgroups
                parseOptions(group); // parse options
            }
        }
    } else if (parent->type == QOptionTreeItem::Group) {

        const ppd_group_t* group = reinterpret_cast<const ppd_group_t*>(parent->ptr);

        if (group) {
            for (int i = 0; i < group->num_subgroups; ++i) {
                QOptionTreeItem* subgroup = new QOptionTreeItem(QOptionTreeItem::Group, i, &group->subgroups[i], group->subgroups[i].text, parent);
                parent->childItems.append(subgroup);
                parseGroups(subgroup); // parse possible subgroups
                parseOptions(subgroup); // parse options
            }
        }
    }
}

void QPPDOptionsModel::parseOptions(QOptionTreeItem* parent)
{
    const ppd_group_t* group = reinterpret_cast<const ppd_group_t*>(parent->ptr);
    for (int i = 0; i < group->num_options; ++i) {
        QOptionTreeItem* opt = new QOptionTreeItem(QOptionTreeItem::Option, i, &group->options[i], group->options[i].text, parent);
        parent->childItems.append(opt);
        parseChoices(opt);
    }
}

void QPPDOptionsModel::parseChoices(QOptionTreeItem* parent)
{
    const ppd_option_t* option = reinterpret_cast<const ppd_option_t*>(parent->ptr);
    bool marked = false;
    for (int i = 0; i < option->num_choices; ++i) {
        QOptionTreeItem* choice = new QOptionTreeItem(QOptionTreeItem::Choice, i, &option->choices[i], option->choices[i].text, parent);
        if (static_cast<int>(option->choices[i].marked) == 1) {
            parent->selected = i;
            parent->selDescription = option->choices[i].text;
            marked = true;
        } else if (!marked && qstrcmp(option->choices[i].choice, option->defchoice) == 0) {
            parent->selected = i;
            parent->selDescription = option->choices[i].text;
        }
        parent->childItems.append(choice);
    }
}

QVariant QPPDOptionsModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch(section){
        case 0:
            return QVariant(QApplication::translate("QPPDOptionsModel", "Name"));
        case 1:
            return QVariant(QApplication::translate("QPPDOptionsModel", "Value"));
        default:
            return QVariant();
    }
}

////////////////////////////////////////////////////////////////////////////////

QWidget* QPPDOptionsEditor::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const
{
    if (index.column() == 1 && reinterpret_cast<QOptionTreeItem*>(index.internalPointer())->type == QOptionTreeItem::Option)
        return new QComboBox(parent);
    else
        return 0;
}

void QPPDOptionsEditor::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (index.column() != 1)
        return;

    QComboBox* cb = static_cast<QComboBox*>(editor);
    QOptionTreeItem* itm = reinterpret_cast<QOptionTreeItem*>(index.internalPointer());

    if (itm->selected == -1)
        cb->addItem(QString());

    for (int i = 0; i < itm->childItems.count(); ++i)
        cb->addItem(QString::fromLocal8Bit(itm->childItems.at(i)->description));

    if (itm->selected > -1)
        cb->setCurrentIndex(itm->selected);

    connect(cb, SIGNAL(currentIndexChanged(int)), this, SLOT(cbChanged(int)));
}

void QPPDOptionsEditor::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QComboBox* cb = static_cast<QComboBox*>(editor);
    QOptionTreeItem* itm = reinterpret_cast<QOptionTreeItem*>(index.internalPointer());

    if (itm->selected == cb->currentIndex())
        return;

    const ppd_option_t* opt = reinterpret_cast<const ppd_option_t*>(itm->ptr);
    QPPDOptionsModel* m = static_cast<QPPDOptionsModel*>(model);

    if (m->cups->markOption(opt->keyword, opt->choices[cb->currentIndex()].choice) == 0) {
        itm->selected = cb->currentIndex();
        itm->selDescription = reinterpret_cast<const ppd_option_t*>(itm->ptr)->choices[itm->selected].text;
    }
}

void QPPDOptionsEditor::cbChanged(int)
{
/*
    emit commitData(static_cast<QWidget*>(sender()));
*/
}

#endif

QT_END_NAMESPACE

#include "moc_qprintdialog.cpp"
#include "qprintdialog_unix.moc"
#include "qrc_qprintdialog.cpp"

#endif // QT_NO_PRINTDIALOG

