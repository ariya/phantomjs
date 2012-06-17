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

#include <private/qabstractprintdialog_p.h>
#include "qprintdialog.h"

#ifndef QT_NO_PRINTDIALOG

#include "qapplication.h"
#include "qbuttongroup.h"
#include "qradiobutton.h"
#include "qcombobox.h"
#include "qspinbox.h"
#include "qprinter.h"
#include "qlineedit.h"
#include "qdir.h"
#include "qmessagebox.h"
#include "qinputdialog.h"
#include "qlayout.h"
#include "qlabel.h"

#include "qlibrary.h"

#ifndef QT_NO_NIS

#ifndef BOOL_DEFINED
#define BOOL_DEFINED
#endif

#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>

#endif //QT_NO_NIS

#include <ctype.h>
#include <stdlib.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

typedef void (*QPrintDialogCreator)(QPrintDialog *parent);
Q_GUI_EXPORT QPrintDialogCreator _qt_print_dialog_creator;

class QPrintDialogPrivate : public QAbstractPrintDialogPrivate
{
    Q_DECLARE_PUBLIC(QPrintDialog)
public:
    QButtonGroup *printerOrFile;
    bool outputToFile;
    QRadioButton *printToPrinterButton;
    QRadioButton *printToFileButton;
    QLineEdit *fileName;

    QButtonGroup *colorMode;
    QRadioButton *printColor;
    QRadioButton *printGray;
    QPrinter::ColorMode colorMode2;

    QComboBox *orientationCombo, *sizeCombo;
    QPrinter::PaperSize pageSize;
    QPrinter::Orientation orientation;

    QSpinBox *copies;
    int numCopies;
    QPrinter::PaperSize indexToPaperSize[QPrinter::NPaperSize];

    QComboBox *rangeCombo;
    QSpinBox *firstPage;
    QSpinBox *lastPage;

    QComboBox *pageOrderCombo;
    QPrinter::PageOrder pageOrder2;

    QString faxNum;

    void init();

    void _q_okClicked();
    void _q_printerOrFileSelected(QAbstractButton *b);
    void _q_paperSizeSelected(int);
    void _q_orientSelected(int);
    void _q_pageOrderSelected(int);
    void _q_colorModeSelected(QAbstractButton *);
    void _q_setNumCopies(int);
    void _q_printRangeSelected(int);
    void _q_setFirstPage(int);
    void _q_setLastPage(int);
    void _q_fileNameEditChanged(const QString &text);

    void setupDestination();
    void setupPrinterSettings();
    void setupPaper();
    void setupOptions();

    void setPrinter(QPrinter *p, bool pickUpSettings);
};

static void isc(QPrintDialogPrivate *d, const QString & text,
                 QPrinter::PaperSize ps);

void QPrintDialogPrivate::_q_okClicked()
{
    Q_Q(QPrintDialog);
#ifndef QT_NO_MESSAGEBOX
    if (outputToFile && fileName->isModified() && QFileInfo(fileName->text()).exists()) {
        int confirm = QMessageBox::warning(
            q, QPrintDialog::tr("File exists"),
            QPrintDialog::tr("<qt>Do you want to overwrite it?</qt>"),
            QMessageBox::Yes, QMessageBox::No);
        if (confirm == QMessageBox::No)
            return;
    }
#endif // QT_NO_MESSAGEBOX

    lastPage->interpretText();
    firstPage->interpretText();
    copies->interpretText();
    if (outputToFile) {
        printer->setOutputFileName(fileName->text());
    }
    printer->setOrientation(orientation);
    printer->setPaperSize(pageSize);
    printer->setPageOrder(pageOrder2);
    printer->setColorMode(colorMode2);
    printer->setCopyCount(numCopies);

    switch ((rangeCombo->itemData(rangeCombo->currentIndex())).toInt()){
    case (int)QPrintDialog::AllPages:
        q->setPrintRange(QPrintDialog::AllPages);
        q->setFromTo(0, 0);
        break;
    case (int)QPrintDialog::Selection:
        q->setPrintRange(QPrintDialog::Selection);
        q->setFromTo(0, 0);
        break;
    case (int)QPrintDialog::PageRange:
        q->setPrintRange(QPrintDialog::PageRange);
        q->setFromTo(firstPage->value(), lastPage->value());
        break;
    case (int)QPrintDialog::CurrentPage:
        q->setPrintRange(QPrintDialog::CurrentPage);
        q->setFromTo(0, 0);
        break;
    }
    q->accept();
}

void QPrintDialogPrivate::_q_printerOrFileSelected(QAbstractButton *b)
{
    outputToFile = (b == printToFileButton);
    if (outputToFile) {
        _q_fileNameEditChanged(fileName->text());
        if (!fileName->isModified() && fileName->text().isEmpty()) {
            QString file = "print.tiff";
            fileName->setText(file);
            fileName->setCursorPosition(file.length());
            fileName->selectAll();
            fileName->setModified(true); // confirm overwrite when OK clicked

        }
        fileName->setEnabled(true);
        fileName->setFocus();
    } else {
        fileName->setText(QString());
        if (fileName->isEnabled())
            fileName->setEnabled(false);
    }
}

void QPrintDialogPrivate::_q_paperSizeSelected(int id)
{
    if (id < QPrinter::NPaperSize)
        pageSize = QPrinter::PaperSize(indexToPaperSize[id]);
}

void QPrintDialogPrivate::_q_orientSelected(int id)
{
    orientation = (QPrinter::Orientation)id;
}

void QPrintDialogPrivate::_q_pageOrderSelected(int id)
{
    pageOrder2 = (QPrinter::PageOrder)id;
}

void QPrintDialogPrivate::_q_colorModeSelected(QAbstractButton *b)
{
    colorMode2 = (b == printColor) ? QPrinter::Color : QPrinter::GrayScale;
}

void QPrintDialogPrivate::_q_setNumCopies(int copies)
{
    numCopies = copies;
}

void QPrintDialogPrivate::_q_printRangeSelected(int id)
{
    bool enable = (rangeCombo->itemData(id).toInt() == (int)QPrintDialog::PageRange);
    firstPage->setEnabled(enable);
    lastPage->setEnabled(enable);
}

void QPrintDialogPrivate::_q_setFirstPage(int fp)
{
    Q_Q(QPrintDialog);
    if (printer) {
        lastPage->setMinimum(fp);
        lastPage->setMaximum(qMax(fp, q->maxPage()));
    }
}

void QPrintDialogPrivate::_q_setLastPage(int lp)
{
    Q_Q(QPrintDialog);
    if (printer) {
        firstPage->setMinimum(qMin(lp, q->minPage()));
        firstPage->setMaximum(lp);
    }
}

void QPrintDialogPrivate::_q_fileNameEditChanged(const QString &text)
{
    Q_UNUSED(text);
}

void QPrintDialogPrivate::setupDestination()
{
    Q_Q(QPrintDialog);

    // print destinations
    printerOrFile = new QButtonGroup(q);
    QObject::connect(printerOrFile, SIGNAL(buttonClicked(QAbstractButton*)),
            q, SLOT(_q_printerOrFileSelected(QAbstractButton*)));

    printToPrinterButton = q->findChild<QRadioButton *>("printToPrinterButton");
    printerOrFile->addButton(printToPrinterButton);
    printToFileButton = q->findChild<QRadioButton *>("printToFileButton");
    printerOrFile->addButton(printToFileButton);

    // file name
    fileName = q->findChild<QLineEdit *>("fileName");
    QObject::connect(fileName, SIGNAL(textChanged(QString)),
            q, SLOT(_q_fileNameEditChanged(QString)));

    outputToFile = false;
}

void QPrintDialogPrivate::setupPrinterSettings()
{
    Q_Q(QPrintDialog);

    // color mode
    colorMode = new QButtonGroup(q);
    QObject::connect(colorMode, SIGNAL(buttonClicked(QAbstractButton*)),
        q, SLOT(_q_colorModeSelected(QAbstractButton*)));

    printColor = q->findChild<QRadioButton *>("printColor");
    colorMode->addButton(printColor);
    printGray = q->findChild<QRadioButton *>("printGray");
    colorMode->addButton(printGray);
}

void isc(QPrintDialogPrivate *ptr, const QString & text, QPrinter::PaperSize ps)
{
    if (ptr && !text.isEmpty() && ps < QPrinter::NPaperSize) {
        ptr->sizeCombo->addItem(text);
        int index = ptr->sizeCombo->count()-1;
        if (index >= 0 && index < QPrinter::NPaperSize)
            ptr->indexToPaperSize[index] = ps;
    }
}

void QPrintDialogPrivate::setupPaper()
{
    Q_Q(QPrintDialog);

    pageSize = QPrinter::A4;

    // paper orientation
    orientationCombo = q->findChild<QComboBox *>("orientationCombo");
    orientation = QPrinter::Portrait;
    QObject::connect(orientationCombo, SIGNAL(activated(int)),
            q, SLOT(_q_orientSelected(int)));

    // paper size
    sizeCombo = q->findChild<QComboBox *>("sizeCombo");

    int n;
    for(n=0; n<QPrinter::NPaperSize; n++)
        indexToPaperSize[n] = QPrinter::A4;

    isc(this, QPrintDialog::tr("A0 (841 x 1189 mm)"), QPrinter::A0);
    isc(this, QPrintDialog::tr("A1 (594 x 841 mm)"), QPrinter::A1);
    isc(this, QPrintDialog::tr("A2 (420 x 594 mm)"), QPrinter::A2);
    isc(this, QPrintDialog::tr("A3 (297 x 420 mm)"), QPrinter::A3);
    isc(this, QPrintDialog::tr("A4 (210 x 297 mm, 8.26 x 11.7 inches)"), QPrinter::A4);
    isc(this, QPrintDialog::tr("A5 (148 x 210 mm)"), QPrinter::A5);
    isc(this, QPrintDialog::tr("A6 (105 x 148 mm)"), QPrinter::A6);
    isc(this, QPrintDialog::tr("A7 (74 x 105 mm)"), QPrinter::A7);
    isc(this, QPrintDialog::tr("A8 (52 x 74 mm)"), QPrinter::A8);
    isc(this, QPrintDialog::tr("A9 (37 x 52 mm)"), QPrinter::A9);
    isc(this, QPrintDialog::tr("B0 (1000 x 1414 mm)"), QPrinter::B0);
    isc(this, QPrintDialog::tr("B1 (707 x 1000 mm)"), QPrinter::B1);
    isc(this, QPrintDialog::tr("B2 (500 x 707 mm)"), QPrinter::B2);
    isc(this, QPrintDialog::tr("B3 (353 x 500 mm)"), QPrinter::B3);
    isc(this, QPrintDialog::tr("B4 (250 x 353 mm)"), QPrinter::B4);
    isc(this, QPrintDialog::tr("B5 (176 x 250 mm, 6.93 x 9.84 inches)"), QPrinter::B5);
    isc(this, QPrintDialog::tr("B6 (125 x 176 mm)"), QPrinter::B6);
    isc(this, QPrintDialog::tr("B7 (88 x 125 mm)"), QPrinter::B7);
    isc(this, QPrintDialog::tr("B8 (62 x 88 mm)"), QPrinter::B8);
    isc(this, QPrintDialog::tr("B9 (44 x 62 mm)"), QPrinter::B9);
    isc(this, QPrintDialog::tr("B10 (31 x 44 mm)"), QPrinter::B10);
    isc(this, QPrintDialog::tr("C5E (163 x 229 mm)"), QPrinter::C5E);
    isc(this, QPrintDialog::tr("DLE (110 x 220 mm)"), QPrinter::DLE);
    isc(this, QPrintDialog::tr("Executive (7.5 x 10 inches, 191 x 254 mm)"), QPrinter::Executive);
    isc(this, QPrintDialog::tr("Folio (210 x 330 mm)"), QPrinter::Folio);
    isc(this, QPrintDialog::tr("Ledger (432 x 279 mm)"), QPrinter::Ledger);
    isc(this, QPrintDialog::tr("Legal (8.5 x 14 inches, 216 x 356 mm)"), QPrinter::Legal);
    isc(this, QPrintDialog::tr("Letter (8.5 x 11 inches, 216 x 279 mm)"), QPrinter::Letter);
    isc(this, QPrintDialog::tr("Tabloid (279 x 432 mm)"), QPrinter::Tabloid);
    isc(this, QPrintDialog::tr("US Common #10 Envelope (105 x 241 mm)"), QPrinter::Comm10E);

    QObject::connect(sizeCombo, SIGNAL(activated(int)),
             q, SLOT(_q_paperSizeSelected(int)));
}

void QPrintDialogPrivate::setupOptions()
{
    Q_Q(QPrintDialog);

    // no. of copies
    copies = q->findChild<QSpinBox *>("copies");
    QObject::connect(copies, SIGNAL(valueChanged(int)),
            q, SLOT(_q_setNumCopies(int)));

    // print range
    rangeCombo = q->findChild<QComboBox *>("rangeCombo");
    rangeCombo->addItem(QPrintDialog::tr("Print all"), QPrintDialog::AllPages);
    rangeCombo->addItem(QPrintDialog::tr("Print selection"), QPrintDialog::Selection);
    rangeCombo->addItem(QPrintDialog::tr("Print range"), QPrintDialog::PageRange);
    rangeCombo->addItem(QPrintDialog::tr("Print current page"), QPrintDialog::CurrentPage);
    QObject::connect(rangeCombo, SIGNAL(activated(int)),
            q, SLOT(_q_printRangeSelected(int)));

    // page range
    firstPage = q->findChild<QSpinBox *>("firstPage");
    firstPage->setRange(1, 9999);
    firstPage->setValue(1);
    QObject::connect(firstPage, SIGNAL(valueChanged(int)),
            q, SLOT(_q_setFirstPage(int)));

    lastPage = q->findChild<QSpinBox *>("lastPage");
    lastPage->setRange(1, 9999);
    lastPage->setValue(1);
    QObject::connect(lastPage, SIGNAL(valueChanged(int)),
            q, SLOT(_q_setLastPage(int)));

    // print order
    pageOrderCombo = q->findChild<QComboBox *>("pageOrderCombo");
    QObject::connect(pageOrderCombo, SIGNAL(activated(int)),
            q, SLOT(_q_pageOrderSelected(int)));
}

bool QPrintDialog::eventFilter(QObject *o, QEvent *e)
{
    Q_UNUSED(o);

    Q_D(QPrintDialog);
    switch (e->type()){
    case QEvent::KeyPress:
        switch (static_cast<QKeyEvent*>(e)->key()) {
        case Qt::Key_Back:
            d->_q_okClicked();
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

QPrintDialog::QPrintDialog(QPrinter *printer, QWidget *parent)
    : QAbstractPrintDialog(*(new QPrintDialogPrivate), printer, parent)
{
    d_func()->init();
}

QPrintDialog::QPrintDialog(QWidget *parent)
    : QAbstractPrintDialog(*(new QPrintDialogPrivate), 0, parent)
{
    d_func()->init();
}

QPrintDialog::~QPrintDialog()
{
}

void QPrintDialogPrivate::setPrinter(QPrinter *p, bool pickUpSettings)
{
    Q_Q(QPrintDialog);
    printer = p;

    if (p && pickUpSettings) {
        // top to bottom in the old dialog.
        // printer or file
        outputToFile = !p->outputFileName().isEmpty() && q->isOptionEnabled(QPrintDialog::PrintToFile);
        if (outputToFile)
            printToFileButton->setChecked(true);
        else
            printToPrinterButton->setChecked(true);
        fileName->setEnabled(outputToFile);

        // file name
        if (q->isOptionEnabled(QPrintDialog::PrintToFile)) {
            fileName->setText(p->outputFileName());
            fileName->setModified(!fileName->text().isEmpty());
        } else {
            printToFileButton->setEnabled(false);
        }

        // orientation
        orientationCombo->setCurrentIndex((int)p->orientation());
        _q_orientSelected(p->orientation());

        // page size
        int n = 0;
        while (n < QPrinter::NPaperSize &&
                indexToPaperSize[n] != p->pageSize())
            n++;
        sizeCombo->setCurrentIndex(n);
        _q_paperSizeSelected(n);

        // page order
        pageOrder2 = p->pageOrder();
        pageOrderCombo->setCurrentIndex((int)pageOrder2);

        // color mode
        colorMode2 = p->colorMode();
        if (colorMode2 == QPrinter::Color)
            printColor->setChecked(true);
        else
            printGray->setChecked(true);

        // number of copies
        copies->setValue(p->copyCount());
        _q_setNumCopies(p->copyCount());
    }

    if (p) {
        if (!q->isOptionEnabled(QPrintDialog::PrintSelection)
                && rangeCombo->findData(QPrintDialog::Selection) > 0)
            rangeCombo->removeItem(rangeCombo->findData(QPrintDialog::Selection));
        if (!q->isOptionEnabled(QPrintDialog::PrintPageRange)
                && rangeCombo->findData(QPrintDialog::PageRange) > 0)
            rangeCombo->removeItem(rangeCombo->findData(QPrintDialog::PageRange));
        if (!q->isOptionEnabled(QPrintDialog::PrintCurrentPage)
                && rangeCombo->findData(QPrintDialog::CurrentPage) > 0)
            rangeCombo->removeItem(rangeCombo->findData(QPrintDialog::CurrentPage));

        switch (q->printRange()) {
        case QPrintDialog::AllPages:
            rangeCombo->setCurrentIndex((int)(QPrintDialog::AllPages));
            break;
        case QPrintDialog::Selection:
            rangeCombo->setCurrentIndex((int)(QPrintDialog::Selection));
            break;
        case QPrintDialog::PageRange:
            rangeCombo->setCurrentIndex((int)(QPrintDialog::PageRange));
            break;
        case QPrintDialog::CurrentPage:
            rangeCombo->setCurrentIndex((int)(QPrintDialog::CurrentPage));
            break;
        }
    }

    if (p && q->maxPage()) {
        int from = q->minPage();
        int to = q->maxPage();
        if (q->printRange() == QPrintDialog::PageRange) {
            from = q->fromPage();
            to = q->toPage();
        }
        firstPage->setRange(q->minPage(), to);
        lastPage->setRange(from, q->maxPage());
        firstPage->setValue(from);
        lastPage->setValue(to);
    }
}

int QPrintDialog::exec()
{
    Q_D(QPrintDialog);
    d->setPrinter(d->printer, true);
    return QDialog::exec();
}

void QPrintDialogPrivate::init()
{
    Q_Q(QPrintDialog);
    numCopies = 1;

    if (_qt_print_dialog_creator)
        (*_qt_print_dialog_creator)(q);

    setupDestination();
    setupPrinterSettings();
    setupPaper();
    setupOptions();

    setPrinter(printer, true);

    q->installEventFilter(q);
}

void QPrintDialog::setVisible(bool visible)
{
    QAbstractPrintDialog::setVisible(visible);
}

QT_END_NAMESPACE

#include "moc_qprintdialog.cpp"
#include "qrc_qprintdialog.cpp"

#endif // QT_NO_PRINTDIALOG
