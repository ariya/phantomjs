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

#include "qpagesetupdialog.h"

#ifndef QT_NO_PRINTDIALOG
#include "qpagesetupdialog_unix_p.h"

#include "qpainter.h"
#include "qprintdialog.h"
#include "qdialogbuttonbox.h"
#include <ui_qpagesetupwidget.h>

#include <QtGui/qprinter.h>
#include <private/qabstractpagesetupdialog_p.h>
#include <private/qprinter_p.h>

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
#  include <private/qcups_p.h>
#  include <cups/cups.h>
#  include <private/qpdf_p.h>
#endif


QT_BEGIN_NAMESPACE

QSizeF qt_printerPaperSize(QPrinter::Orientation, QPrinter::PaperSize, QPrinter::Unit, int);

// Disabled until we have support for papersources on unix
// #define PSD_ENABLE_PAPERSOURCE

static void populatePaperSizes(QComboBox* cb)
{
    cb->addItem(QPrintDialog::tr("A0"), QPrinter::A0);
    cb->addItem(QPrintDialog::tr("A1"), QPrinter::A1);
    cb->addItem(QPrintDialog::tr("A2"), QPrinter::A2);
    cb->addItem(QPrintDialog::tr("A3"), QPrinter::A3);
    cb->addItem(QPrintDialog::tr("A4"), QPrinter::A4);
    cb->addItem(QPrintDialog::tr("A5"), QPrinter::A5);
    cb->addItem(QPrintDialog::tr("A6"), QPrinter::A6);
    cb->addItem(QPrintDialog::tr("A7"), QPrinter::A7);
    cb->addItem(QPrintDialog::tr("A8"), QPrinter::A8);
    cb->addItem(QPrintDialog::tr("A9"), QPrinter::A9);
    cb->addItem(QPrintDialog::tr("B0"), QPrinter::B0);
    cb->addItem(QPrintDialog::tr("B1"), QPrinter::B1);
    cb->addItem(QPrintDialog::tr("B2"), QPrinter::B2);
    cb->addItem(QPrintDialog::tr("B3"), QPrinter::B3);
    cb->addItem(QPrintDialog::tr("B4"), QPrinter::B4);
    cb->addItem(QPrintDialog::tr("B5"), QPrinter::B5);
    cb->addItem(QPrintDialog::tr("B6"), QPrinter::B6);
    cb->addItem(QPrintDialog::tr("B7"), QPrinter::B7);
    cb->addItem(QPrintDialog::tr("B8"), QPrinter::B8);
    cb->addItem(QPrintDialog::tr("B9"), QPrinter::B9);
    cb->addItem(QPrintDialog::tr("B10"), QPrinter::B10);
    cb->addItem(QPrintDialog::tr("C5E"), QPrinter::C5E);
    cb->addItem(QPrintDialog::tr("DLE"), QPrinter::DLE);
    cb->addItem(QPrintDialog::tr("Executive"), QPrinter::Executive);
    cb->addItem(QPrintDialog::tr("Folio"), QPrinter::Folio);
    cb->addItem(QPrintDialog::tr("Ledger"), QPrinter::Ledger);
    cb->addItem(QPrintDialog::tr("Legal"), QPrinter::Legal);
    cb->addItem(QPrintDialog::tr("Letter"), QPrinter::Letter);
    cb->addItem(QPrintDialog::tr("Tabloid"), QPrinter::Tabloid);
    cb->addItem(QPrintDialog::tr("US Common #10 Envelope"), QPrinter::Comm10E);
    cb->addItem(QPrintDialog::tr("Custom"), QPrinter::Custom);
}


static QSizeF sizeForOrientation(QPrinter::Orientation orientation, const QSizeF &size)
{
    return (orientation == QPrinter::Portrait) ? size : QSizeF(size.height(), size.width());
}

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


class QPagePreview : public QWidget
{
public:
    QPagePreview(QWidget *parent) : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(50, 50);
    }

    void setPaperSize(const QSizeF& size)
    {
        m_size = size;
        update();
    }

    void setMargins(qreal left, qreal top, qreal right, qreal bottom)
    {
        m_left = left;
        m_top = top;
        m_right = right;
        m_bottom = bottom;
        update();
    }

protected:
    void paintEvent(QPaintEvent *)
    {
        QRect pageRect;
        QSizeF adjustedSize(m_size);
        adjustedSize.scale(width()-10, height()-10, Qt::KeepAspectRatio);
        pageRect = QRect(QPoint(0,0), adjustedSize.toSize());
        pageRect.moveCenter(rect().center());

        qreal width_factor = pageRect.width() / m_size.width();
        qreal height_factor = pageRect.height() / m_size.height();
        int leftSize = qRound(m_left*width_factor);
        int topSize = qRound(m_top*height_factor);
        int rightSize = qRound(m_right*width_factor);
        int bottomSize = qRound(m_bottom * height_factor);
        QRect marginRect(pageRect.x()+leftSize,
                         pageRect.y()+topSize,
                         pageRect.width() - (leftSize+rightSize+1),
                         pageRect.height() - (topSize+bottomSize+1));

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
            p.drawText(marginRect, Qt::TextWordWrap|Qt::AlignVCenter, text);
        }
    }

private:
    // all these are in points
    qreal m_left, m_top, m_right, m_bottom;
    QSizeF m_size;
};


class QPageSetupDialogPrivate : public QAbstractPageSetupDialogPrivate
{
    Q_DECLARE_PUBLIC(QPageSetupDialog)

public:
    ~QPageSetupDialogPrivate();
    void init();

    QPageSetupWidget *widget;
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    QCUPSSupport *cups;
#endif
};

QPageSetupDialogPrivate::~QPageSetupDialogPrivate()
{
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    delete cups;
#endif
}

void QPageSetupDialogPrivate::init()
{
    Q_Q(QPageSetupDialog);

    widget = new QPageSetupWidget(q);
    widget->setPrinter(printer);
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    if (printer->outputFormat() == QPrinter::NativeFormat && QCUPSSupport::isAvailable()) {
        cups = new QCUPSSupport;
        widget->selectPrinter(cups);
    } else {
        cups = 0;
    }
#endif

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                                     | QDialogButtonBox::Cancel,
                                                     Qt::Horizontal, q);
    QObject::connect(buttons, SIGNAL(accepted()), q, SLOT(accept()));
    QObject::connect(buttons, SIGNAL(rejected()), q, SLOT(reject()));

    QVBoxLayout *lay = new QVBoxLayout(q);
    lay->addWidget(widget);
    lay->addWidget(buttons);
}

QPageSetupWidget::QPageSetupWidget(QWidget *parent)
    : QWidget(parent),
    m_printer(0),
    m_blockSignals(false),
    m_cups(0)
{
    widget.setupUi(this);

    QString suffix = (QLocale::system().measurementSystem() == QLocale::ImperialSystem)
                     ? QString::fromLatin1(" in")
                     : QString::fromLatin1(" mm");
    widget.topMargin->setSuffix(suffix);
    widget.bottomMargin->setSuffix(suffix);
    widget.leftMargin->setSuffix(suffix);
    widget.rightMargin->setSuffix(suffix);
    widget.paperWidth->setSuffix(suffix);
    widget.paperHeight->setSuffix(suffix);

    QVBoxLayout *lay = new QVBoxLayout(widget.preview);
    widget.preview->setLayout(lay);
    m_pagePreview = new QPagePreview(widget.preview);
    lay->addWidget(m_pagePreview);

    setAttribute(Qt::WA_WState_Polished, false);

#ifdef PSD_ENABLE_PAPERSOURCE
    for (int i=0; paperSourceNames[i]; ++i)
        widget.paperSource->insertItem(paperSourceNames[i]);
#else
    widget.paperSourceLabel->setVisible(false);
    widget.paperSource->setVisible(false);
#endif

    widget.reverseLandscape->setVisible(false);
    widget.reversePortrait->setVisible(false);

    populatePaperSizes(widget.paperSize);

    QStringList units;
    units << tr("Centimeters (cm)") << tr("Millimeters (mm)") << tr("Inches (in)") << tr("Points (pt)");
    widget.unit->addItems(units);
    connect(widget.unit, SIGNAL(activated(int)), this, SLOT(unitChanged(int)));
    widget.unit->setCurrentIndex((QLocale::system().measurementSystem() == QLocale::ImperialSystem) ? 2 : 1);

    connect(widget.paperSize, SIGNAL(currentIndexChanged(int)), this, SLOT(_q_paperSizeChanged()));
    connect(widget.paperWidth, SIGNAL(valueChanged(double)), this, SLOT(_q_paperSizeChanged()));
    connect(widget.paperHeight, SIGNAL(valueChanged(double)), this, SLOT(_q_paperSizeChanged()));

    connect(widget.leftMargin, SIGNAL(valueChanged(double)), this, SLOT(setLeftMargin(double)));
    connect(widget.topMargin, SIGNAL(valueChanged(double)), this, SLOT(setTopMargin(double)));
    connect(widget.rightMargin, SIGNAL(valueChanged(double)), this, SLOT(setRightMargin(double)));
    connect(widget.bottomMargin, SIGNAL(valueChanged(double)), this, SLOT(setBottomMargin(double)));

    connect(widget.portrait, SIGNAL(clicked()), this, SLOT(_q_pageOrientationChanged()));
    connect(widget.landscape, SIGNAL(clicked()), this, SLOT(_q_pageOrientationChanged()));
}

void QPageSetupWidget::setPrinter(QPrinter *printer)
{
    m_printer = printer;
    m_blockSignals = true;
    selectPdfPsPrinter(printer);
    printer->getPageMargins(&m_leftMargin, &m_topMargin, &m_rightMargin, &m_bottomMargin, QPrinter::Point);
    unitChanged(widget.unit->currentIndex());
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
    m_paperSize = printer->paperSize(QPrinter::Point);
    widget.paperWidth->setValue(m_paperSize.width() / m_currentMultiplier);
    widget.paperHeight->setValue(m_paperSize.height() / m_currentMultiplier);

    widget.landscape->setChecked(printer->orientation() == QPrinter::Landscape);

#ifdef PSD_ENABLE_PAPERSOURCE
    widget.paperSource->setCurrentItem(printer->paperSource());
#endif
    Q_ASSERT(m_blockSignals);
    m_blockSignals = false;
    _q_paperSizeChanged();
}

// set gui data on printer
void QPageSetupWidget::setupPrinter() const
{
    QPrinter::Orientation orientation = widget.portrait->isChecked()
                                        ? QPrinter::Portrait
                                        : QPrinter::Landscape;
    m_printer->setOrientation(orientation);
    // paper format
    QVariant val = widget.paperSize->itemData(widget.paperSize->currentIndex());
    int ps = m_printer->pageSize();
    if (val.type() == QVariant::Int) {
        ps = val.toInt();
    }
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    else if (m_cups && QCUPSSupport::isAvailable() && m_cups->currentPPD()) {
        QByteArray cupsPageSize = val.toByteArray();
        QPrintEngine *engine = m_printer->printEngine();
        engine->setProperty(PPK_CupsStringPageSize, QString::fromLatin1(cupsPageSize));
        engine->setProperty(PPK_CupsOptions, m_cups->options());

        QRect pageRect = m_cups->pageRect(cupsPageSize);
        engine->setProperty(PPK_CupsPageRect, pageRect);

        QRect paperRect = m_cups->paperRect(cupsPageSize);
        engine->setProperty(PPK_CupsPaperRect, paperRect);

        for(ps = 0; ps < QPrinter::NPaperSize; ++ps) {
            QPdf::PaperSize size = QPdf::paperSize(QPrinter::PaperSize(ps));
            if (size.width == paperRect.width() && size.height == paperRect.height())
                break;
        }
    }
#endif
    if (ps == QPrinter::Custom)
        m_printer->setPaperSize(sizeForOrientation(orientation, m_paperSize), QPrinter::Point);
    else
        m_printer->setPaperSize(static_cast<QPrinter::PaperSize>(ps));

#ifdef PSD_ENABLE_PAPERSOURCE
    m_printer->setPaperSource((QPrinter::PaperSource)widget.paperSource->currentIndex());
#endif
    m_printer->setPageMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin, QPrinter::Point);

}

void QPageSetupWidget::selectPrinter(QCUPSSupport *cups)
{
    m_cups = cups;
    widget.paperSize->clear();
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    if (m_cups && QCUPSSupport::isAvailable()) {
        const ppd_option_t* pageSizes = m_cups->pageSizes();
        const int numChoices = pageSizes ? pageSizes->num_choices : 0;

        int cupsDefaultSize = 0;
        QSize qtPreferredSize = m_printer->paperSize(QPrinter::Point).toSize();
        bool preferredSizeMatched = false;
        for (int i = 0; i < numChoices; ++i) {
            widget.paperSize->addItem(QString::fromLocal8Bit(pageSizes->choices[i].text), QByteArray(pageSizes->choices[i].choice));
            if (static_cast<int>(pageSizes->choices[i].marked) == 1)
                cupsDefaultSize = i;
            if (m_printer->d_func()->hasUserSetPageSize) {
                QRect cupsPaperSize = m_cups->paperRect(pageSizes->choices[i].choice);
                QSize diff = cupsPaperSize.size() - qtPreferredSize;
                if (qAbs(diff.width()) < 5 && qAbs(diff.height()) < 5) {
                    widget.paperSize->setCurrentIndex(i);
                    preferredSizeMatched = true;
                }
            }
        }
        if (!preferredSizeMatched)
            widget.paperSize->setCurrentIndex(cupsDefaultSize);
        if (m_printer->d_func()->hasCustomPageMargins) {
            m_printer->getPageMargins(&m_leftMargin, &m_topMargin, &m_rightMargin, &m_bottomMargin, QPrinter::Point);
        } else {
            QByteArray cupsPaperSizeChoice = widget.paperSize->itemData(widget.paperSize->currentIndex()).toByteArray();
            QRect paper = m_cups->paperRect(cupsPaperSizeChoice);
            QRect content = m_cups->pageRect(cupsPaperSizeChoice);

            m_leftMargin = content.x() - paper.x();
            m_topMargin = content.y() - paper.y();
            m_rightMargin = paper.right() - content.right();
            m_bottomMargin = paper.bottom() - content.bottom();
        }
    }
#endif
    if (widget.paperSize->count() == 0) {
        populatePaperSizes(widget.paperSize);
        widget.paperSize->setCurrentIndex(widget.paperSize->findData(
            QLocale::system().measurementSystem() == QLocale::ImperialSystem ? QPrinter::Letter : QPrinter::A4));
    }

    unitChanged(widget.unit->currentIndex());
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
}

void QPageSetupWidget::selectPdfPsPrinter(const QPrinter *p)
{
    m_cups = 0;
    widget.paperSize->clear();
    populatePaperSizes(widget.paperSize);
    widget.paperSize->setCurrentIndex(widget.paperSize->findData(p->paperSize()));

    m_leftMargin = 90;
    m_topMargin = 72;
    m_bottomMargin = 72;
    m_rightMargin = 90;
    unitChanged(widget.unit->currentIndex());
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
}

// Updates size/preview after the combobox has been changed.
void QPageSetupWidget::_q_paperSizeChanged()
{
    QVariant val = widget.paperSize->itemData(widget.paperSize->currentIndex());
    int index = m_printer->pageSize();
    if (val.type() == QVariant::Int) {
        index = val.toInt();
    }

    if (m_blockSignals) return;
    m_blockSignals = true;

    QPrinter::PaperSize size = QPrinter::PaperSize(index);
    QPrinter::Orientation orientation = widget.portrait->isChecked()
                                        ? QPrinter::Portrait
                                        : QPrinter::Landscape;

    bool custom = size == QPrinter::Custom;

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    custom = custom ? !m_cups : custom;
#endif

    widget.paperWidth->setEnabled(custom);
    widget.paperHeight->setEnabled(custom);
    widget.widthLabel->setEnabled(custom);
    widget.heightLabel->setEnabled(custom);
    if (custom) {
        m_paperSize.setWidth( widget.paperWidth->value() * m_currentMultiplier);
        m_paperSize.setHeight( widget.paperHeight->value() * m_currentMultiplier);
        m_pagePreview->setPaperSize(m_paperSize);
    } else {
        Q_ASSERT(m_printer);
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
        if (m_cups) { // combobox is filled with cups based data
            QByteArray cupsPageSize = widget.paperSize->itemData(widget.paperSize->currentIndex()).toByteArray();
            m_paperSize = m_cups->paperRect(cupsPageSize).size();
            if (orientation == QPrinter::Landscape)
                m_paperSize = QSizeF(m_paperSize.height(), m_paperSize.width()); // swap
        }
        else
#endif
            m_paperSize = qt_printerPaperSize(orientation, size, QPrinter::Point, 1);

        m_pagePreview->setPaperSize(m_paperSize);
        widget.paperWidth->setValue(m_paperSize.width() / m_currentMultiplier);
        widget.paperHeight->setValue(m_paperSize.height() / m_currentMultiplier);
    }
    m_blockSignals = false;
}

void QPageSetupWidget::_q_pageOrientationChanged()
{
    if (QPrinter::PaperSize(widget.paperSize->currentIndex()) == QPrinter::Custom) {
        double tmp = widget.paperWidth->value();
        widget.paperWidth->setValue(widget.paperHeight->value());
        widget.paperHeight->setValue(tmp);
    }
    _q_paperSizeChanged();
}

extern double qt_multiplierForUnit(QPrinter::Unit unit, int resolution);

void QPageSetupWidget::unitChanged(int item)
{
    QString suffix;
    switch(item) {
    case 0:
        m_currentMultiplier = 10 * qt_multiplierForUnit(QPrinter::Millimeter, 1);
        suffix = QString::fromLatin1(" cm");
        break;
    case 2:
        m_currentMultiplier = qt_multiplierForUnit(QPrinter::Inch, 1);
        suffix = QString::fromLatin1(" in");
        break;
    case 3:
        m_currentMultiplier = qt_multiplierForUnit(QPrinter::Point, 1);
        suffix = QString::fromLatin1(" pt");
        break;
    case 1:
    default:
        m_currentMultiplier = qt_multiplierForUnit(QPrinter::Millimeter, 1);
        suffix = QString::fromLatin1(" mm");
        break;
    }
    const bool old = m_blockSignals;
    m_blockSignals = true;
    widget.topMargin->setSuffix(suffix);
    widget.leftMargin->setSuffix(suffix);
    widget.rightMargin->setSuffix(suffix);
    widget.bottomMargin->setSuffix(suffix);
    widget.paperWidth->setSuffix(suffix);
    widget.paperHeight->setSuffix(suffix);
    widget.topMargin->setValue(m_topMargin / m_currentMultiplier);
    widget.leftMargin->setValue(m_leftMargin / m_currentMultiplier);
    widget.rightMargin->setValue(m_rightMargin / m_currentMultiplier);
    widget.bottomMargin->setValue(m_bottomMargin / m_currentMultiplier);
    widget.paperWidth->setValue(m_paperSize.width() / m_currentMultiplier);
    widget.paperHeight->setValue(m_paperSize.height() / m_currentMultiplier);
    m_blockSignals = old;
}

void QPageSetupWidget::setTopMargin(double newValue)
{
    if (m_blockSignals) return;
    m_topMargin = newValue * m_currentMultiplier;
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
}

void QPageSetupWidget::setBottomMargin(double newValue)
{
    if (m_blockSignals) return;
    m_bottomMargin = newValue * m_currentMultiplier;
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
}

void QPageSetupWidget::setLeftMargin(double newValue)
{
    if (m_blockSignals) return;
    m_leftMargin = newValue * m_currentMultiplier;
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
}

void QPageSetupWidget::setRightMargin(double newValue)
{
    if (m_blockSignals) return;
    m_rightMargin = newValue * m_currentMultiplier;
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
}



QPageSetupDialog::QPageSetupDialog(QPrinter *printer, QWidget *parent)
    : QAbstractPageSetupDialog(*(new QPageSetupDialogPrivate), printer, parent)
{
    Q_D(QPageSetupDialog);
    d->init();
}


QPageSetupDialog::QPageSetupDialog(QWidget *parent)
    : QAbstractPageSetupDialog(*(new QPageSetupDialogPrivate), 0, parent)
{
    Q_D(QPageSetupDialog);
    d->init();
}

/*!
    \internal
*/
int QPageSetupDialog::exec()
{
    Q_D(QPageSetupDialog);

    int ret = QDialog::exec();
    if (ret == Accepted)
        d->widget->setupPrinter();
    return ret;
}


QT_END_NAMESPACE

#include "moc_qpagesetupdialog.cpp"

#endif // QT_NO_PRINTDIALOG
