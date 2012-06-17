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

#include "qinputdialog.h"

#ifndef QT_NO_INPUTDIALOG

#include "qapplication.h"
#include "qcombobox.h"
#include "qdialogbuttonbox.h"
#include "qlabel.h"
#include "qlayout.h"
#include "qlineedit.h"
#include "qlistwidget.h"
#include "qpushbutton.h"
#include "qspinbox.h"
#include "qstackedlayout.h"
#include "qvalidator.h"
#include "qevent.h"
#include "qdialog_p.h"

QT_USE_NAMESPACE

static const char *signalForMember(const char *member)
{
    static const int NumCandidates = 4;
    static const char * const candidateSignals[NumCandidates] = {
        SIGNAL(textValueSelected(QString)),
        SIGNAL(intValueSelected(int)),
        SIGNAL(doubleValueSelected(double)),
        SIGNAL(accepted())
    };

    QByteArray normalizedMember(QMetaObject::normalizedSignature(member));

    int i = 0;
    while (i < NumCandidates - 1) { // sic
        if (QMetaObject::checkConnectArgs(candidateSignals[i], normalizedMember))
            break;
        ++i;
    }
    return candidateSignals[i];
}

QT_BEGIN_NAMESPACE

/*
    These internal classes add extra validation to QSpinBox and QDoubleSpinBox by emitting
    textChanged(bool) after events that may potentially change the visible text. Return or
    Enter key presses are not propagated if the visible text is invalid. Instead, the visible
    text is modified to the last valid value.
*/
class QInputDialogSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    QInputDialogSpinBox(QWidget *parent)
        : QSpinBox(parent) {
        connect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(notifyTextChanged()));
        connect(this, SIGNAL(editingFinished()), this, SLOT(notifyTextChanged()));
    }

signals:
    void textChanged(bool);

private slots:
    void notifyTextChanged() { emit textChanged(hasAcceptableInput()); }

private:
    void keyPressEvent(QKeyEvent *event) {
        if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && !hasAcceptableInput()) {
#ifndef QT_NO_PROPERTIES
            setProperty("value", property("value"));
#endif
        } else {
            QSpinBox::keyPressEvent(event);
        }
        notifyTextChanged();
    }

    void mousePressEvent(QMouseEvent *event) {
        QSpinBox::mousePressEvent(event);
        notifyTextChanged();
    }
};

class QInputDialogDoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT

public:
    QInputDialogDoubleSpinBox(QWidget *parent = 0)
        : QDoubleSpinBox(parent) {
        connect(lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(notifyTextChanged()));
        connect(this, SIGNAL(editingFinished()), this, SLOT(notifyTextChanged()));
    }

signals:
    void textChanged(bool);

private slots:
    void notifyTextChanged() { emit textChanged(hasAcceptableInput()); }

private:
    void keyPressEvent(QKeyEvent *event) {
        if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && !hasAcceptableInput()) {
#ifndef QT_NO_PROPERTIES
            setProperty("value", property("value"));
#endif
        } else {
            QDoubleSpinBox::keyPressEvent(event);
        }
        notifyTextChanged();
    }

    void mousePressEvent(QMouseEvent *event) {
        QDoubleSpinBox::mousePressEvent(event);
        notifyTextChanged();
    }
};

QT_BEGIN_INCLUDE_NAMESPACE
#include "qinputdialog.moc"
QT_END_INCLUDE_NAMESPACE

class QInputDialogPrivate : public QDialogPrivate
{
    Q_DECLARE_PUBLIC(QInputDialog)

public:
    QInputDialogPrivate();

    void ensureLayout();
    void ensureLineEdit();
    void ensureComboBox();
    void ensureListView();
    void ensureIntSpinBox();
    void ensureDoubleSpinBox();
    void ensureEnabledConnection(QAbstractSpinBox *spinBox);
    void setInputWidget(QWidget *widget);
    void chooseRightTextInputWidget();
    void setComboBoxText(const QString &text);
    void setListViewText(const QString &text);
    QString listViewText() const;
    void ensureLayout() const { const_cast<QInputDialogPrivate *>(this)->ensureLayout(); }
    bool useComboBoxOrListView() const { return comboBox && comboBox->count() > 0; }
    void _q_textChanged(const QString &text);
    void _q_currentRowChanged(const QModelIndex &newIndex, const QModelIndex &oldIndex);

    mutable QLabel *label;
    mutable QDialogButtonBox *buttonBox;
    mutable QLineEdit *lineEdit;
    mutable QSpinBox *intSpinBox;
    mutable QDoubleSpinBox *doubleSpinBox;
    mutable QComboBox *comboBox;
    mutable QListView *listView;
    mutable QWidget *inputWidget;
    mutable QVBoxLayout *mainLayout;
    QInputDialog::InputDialogOptions opts;
    QString textValue;
    QPointer<QObject> receiverToDisconnectOnClose;
    QByteArray memberToDisconnectOnClose;
};

QInputDialogPrivate::QInputDialogPrivate()
    : label(0), buttonBox(0), lineEdit(0), intSpinBox(0), doubleSpinBox(0), comboBox(0), listView(0),
      inputWidget(0), mainLayout(0)
{
}

void QInputDialogPrivate::ensureLayout()
{
    Q_Q(QInputDialog);

    if (mainLayout)
        return;

    if (!inputWidget) {
        ensureLineEdit();
        inputWidget = lineEdit;
    }

    if (!label)
        label = new QLabel(QInputDialog::tr("Enter a value:"), q);
#ifndef QT_NO_SHORTCUT
    label->setBuddy(inputWidget);
#endif
    label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, q);
    QObject::connect(buttonBox, SIGNAL(accepted()), q, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), q, SLOT(reject()));

    mainLayout = new QVBoxLayout(q);
    //we want to let the input dialog grow to available size on Symbian.
#ifndef Q_OS_SYMBIAN
    mainLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
#else
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
#endif
    mainLayout->addWidget(label);
    mainLayout->addWidget(inputWidget);
    mainLayout->addWidget(buttonBox);
    ensureEnabledConnection(qobject_cast<QAbstractSpinBox *>(inputWidget));
    inputWidget->show();
}

void QInputDialogPrivate::ensureLineEdit()
{
    Q_Q(QInputDialog);
    if (!lineEdit) {
        lineEdit = new QLineEdit(q);
#ifndef QT_NO_IM
        qt_widget_private(lineEdit)->inheritsInputMethodHints = 1;
#endif
        lineEdit->hide();
        QObject::connect(lineEdit, SIGNAL(textChanged(QString)),
                         q, SLOT(_q_textChanged(QString)));
    }
}

void QInputDialogPrivate::ensureComboBox()
{
    Q_Q(QInputDialog);
    if (!comboBox) {
        comboBox = new QComboBox(q);
#ifndef QT_NO_IM
        qt_widget_private(comboBox)->inheritsInputMethodHints = 1;
#endif
        comboBox->hide();
        QObject::connect(comboBox, SIGNAL(editTextChanged(QString)),
                         q, SLOT(_q_textChanged(QString)));
        QObject::connect(comboBox, SIGNAL(currentIndexChanged(QString)),
                         q, SLOT(_q_textChanged(QString)));
    }
}

void QInputDialogPrivate::ensureListView()
{
    Q_Q(QInputDialog);
    if (!listView) {
        ensureComboBox();

        listView = new QListView(q);
        listView->hide();
        listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        listView->setSelectionMode(QAbstractItemView::SingleSelection);
        listView->setModel(comboBox->model());
        listView->setCurrentIndex(QModelIndex()); // ###
        QObject::connect(listView->selectionModel(),
                         SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
                         q, SLOT(_q_currentRowChanged(QModelIndex,QModelIndex)));
    }
}

void QInputDialogPrivate::ensureIntSpinBox()
{
    Q_Q(QInputDialog);
    if (!intSpinBox) {
        intSpinBox = new QInputDialogSpinBox(q);
        intSpinBox->hide();
        QObject::connect(intSpinBox, SIGNAL(valueChanged(int)),
                         q, SIGNAL(intValueChanged(int)));
    }
}

void QInputDialogPrivate::ensureDoubleSpinBox()
{
    Q_Q(QInputDialog);
    if (!doubleSpinBox) {
        doubleSpinBox = new QInputDialogDoubleSpinBox(q);
        doubleSpinBox->hide();
        QObject::connect(doubleSpinBox, SIGNAL(valueChanged(double)),
                         q, SIGNAL(doubleValueChanged(double)));
    }
}

void QInputDialogPrivate::ensureEnabledConnection(QAbstractSpinBox *spinBox)
{
    if (spinBox) {
        QAbstractButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
        QObject::connect(spinBox, SIGNAL(textChanged(bool)), okButton, SLOT(setEnabled(bool)), Qt::UniqueConnection);
    }
}

void QInputDialogPrivate::setInputWidget(QWidget *widget)
{
    Q_ASSERT(widget);
    if (inputWidget == widget)
        return;

    if (mainLayout) {
        Q_ASSERT(inputWidget);
        mainLayout->removeWidget(inputWidget);
        inputWidget->hide();
        mainLayout->insertWidget(1, widget);
        widget->show();

        // disconnect old input widget
        QAbstractButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
        if (QAbstractSpinBox *spinBox = qobject_cast<QAbstractSpinBox *>(inputWidget))
            QObject::disconnect(spinBox, SIGNAL(textChanged(bool)), okButton, SLOT(setEnabled(bool)));

        // connect new input widget and update enabled state of OK button
        QAbstractSpinBox *spinBox = qobject_cast<QAbstractSpinBox *>(widget);
        ensureEnabledConnection(spinBox);
        okButton->setEnabled(!spinBox || spinBox->hasAcceptableInput());
    }

    inputWidget = widget;

    // synchronize the text shown in the new text editor with the current
    // textValue
    if (widget == lineEdit) {
        lineEdit->setText(textValue);
    } else if (widget == comboBox) {
        setComboBoxText(textValue);
    } else if (widget == listView) {
        setListViewText(textValue);
        ensureLayout();
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(listView->selectionModel()->hasSelection());
    }
}

void QInputDialogPrivate::chooseRightTextInputWidget()
{
    QWidget *widget;

    if (useComboBoxOrListView()) {
        if ((opts & QInputDialog::UseListViewForComboBoxItems) && !comboBox->isEditable()) {
            ensureListView();
            widget = listView;
        } else {
            widget = comboBox;
        }
    } else {
        ensureLineEdit();
        widget = lineEdit;
    }

    setInputWidget(widget);

    if (inputWidget == comboBox) {
        _q_textChanged(comboBox->currentText());
    } else if (inputWidget == listView) {
        _q_textChanged(listViewText());
    }
}

void QInputDialogPrivate::setComboBoxText(const QString &text)
{
    int index = comboBox->findText(text);
    if (index != -1) {
        comboBox->setCurrentIndex(index);
    } else if (comboBox->isEditable()) {
        comboBox->setEditText(text);
    }
}

void QInputDialogPrivate::setListViewText(const QString &text)
{
    int row = comboBox->findText(text);
    if (row != -1) {
        QModelIndex index(comboBox->model()->index(row, 0));
        listView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Clear
                                                         | QItemSelectionModel::SelectCurrent);
    }
}

QString QInputDialogPrivate::listViewText() const
{
    if (listView->selectionModel()->hasSelection()) {
        int row = listView->selectionModel()->selectedRows().value(0).row();
        return comboBox->itemText(row);
    } else {
        return QString();
    }
}

void QInputDialogPrivate::_q_textChanged(const QString &text)
{
    Q_Q(QInputDialog);
    if (textValue != text) {
        textValue = text;
        emit q->textValueChanged(text);
    }
}

void QInputDialogPrivate::_q_currentRowChanged(const QModelIndex &newIndex,
                                               const QModelIndex & /* oldIndex */)
{
    _q_textChanged(comboBox->model()->data(newIndex).toString());
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

/*!
    \class QInputDialog
    \brief The QInputDialog class provides a simple convenience dialog to get a
    single value from the user.
    \ingroup standard-dialogs


    The input value can be a string, a number or an item from a list. A label
    must be set to tell the user what they should enter.

    Four static convenience functions are provided: getText(), getInt(),
    getDouble(), and getItem(). All the functions can be used in a similar way,
    for example:

    \snippet examples/dialogs/standarddialogs/dialog.cpp 3

    The \c ok variable is set to true if the user clicks \gui OK; otherwise it
    is set to false.

    \img inputdialogs.png Input Dialogs

    The \l{dialogs/standarddialogs}{Standard Dialogs} example shows how to use
    QInputDialog as well as other built-in Qt dialogs.

    \sa QMessageBox, {Standard Dialogs Example}
*/

/*!
    \enum QInputDialog::InputMode
    \since 4.5

    This enum describes the different modes of input that can be selected for
    the dialog.

    \value TextInput   Used to input text strings.
    \value IntInput    Used to input integers.
    \value DoubleInput Used to input floating point numbers with double
                       precision accuracy.

    \sa inputMode
*/

/*!
    \since 4.5

    Constructs a new input dialog with the given \a parent and window \a flags.
*/
QInputDialog::QInputDialog(QWidget *parent, Qt::WindowFlags flags)
    : QDialog(*new QInputDialogPrivate, parent, flags)
{
}

/*!
    \since 4.5

    Destroys the input dialog.
*/
QInputDialog::~QInputDialog()
{
}

/*!
    \since 4.5

    \property QInputDialog::inputMode

    \brief the mode used for input

    This property help determines which widget is used for entering input into
    the dialog.
*/
void QInputDialog::setInputMode(InputMode mode)
{
    Q_D(QInputDialog);

    QWidget *widget;

    /*
        Warning: Some functions in QInputDialog rely on implementation details
        of the code below. Look for the comments that accompany the calls to
        setInputMode() throughout this file before you change the code below.
    */

    switch (mode) {
    case IntInput:
        d->ensureIntSpinBox();
        widget = d->intSpinBox;
        break;
    case DoubleInput:
        d->ensureDoubleSpinBox();
        widget = d->doubleSpinBox;
        break;
    default:
        Q_ASSERT(mode == TextInput);
        d->chooseRightTextInputWidget();
        return;
    }

    d->setInputWidget(widget);
}

QInputDialog::InputMode QInputDialog::inputMode() const
{
    Q_D(const QInputDialog);

    if (d->inputWidget) {
        if (d->inputWidget == d->intSpinBox) {
            return IntInput;
        } else if (d->inputWidget == d->doubleSpinBox) {
            return DoubleInput;
        }
    }

    return TextInput;
}

/*!
    \since 4.5

    \property QInputDialog::labelText

    \brief the text to for the label to describe what needs to be input
*/
void QInputDialog::setLabelText(const QString &text)
{
    Q_D(QInputDialog);
    if (!d->label) {
        d->label = new QLabel(text, this);
    } else {
        d->label->setText(text);
    }
#ifdef Q_OS_SYMBIAN
    d->label->setWordWrap(true);
#endif
}

QString QInputDialog::labelText() const
{
    Q_D(const QInputDialog);
    d->ensureLayout();
    return d->label->text();
}

/*!
    \enum QInputDialog::InputDialogOption

    \since 4.5

    This enum specifies various options that affect the look and feel
    of an input dialog.

    \value NoButtons Don't display \gui{OK} and \gui{Cancel} buttons. (Useful for "live dialogs".)
    \value UseListViewForComboBoxItems Use a QListView rather than a non-editable QComboBox for
                                       displaying the items set with setComboBoxItems().

    \sa options, setOption(), testOption()
*/

/*!
    Sets the given \a option to be enabled if \a on is true;
    otherwise, clears the given \a option.

    \sa options, testOption()
*/
void QInputDialog::setOption(InputDialogOption option, bool on)
{
    Q_D(QInputDialog);
    if (!(d->opts & option) != !on)
        setOptions(d->opts ^ option);
}

/*!
    Returns true if the given \a option is enabled; otherwise, returns
    false.

    \sa options, setOption()
*/
bool QInputDialog::testOption(InputDialogOption option) const
{
    Q_D(const QInputDialog);
    return (d->opts & option) != 0;
}

/*!
    \property QInputDialog::options
    \brief the various options that affect the look and feel of the dialog
    \since 4.5

    By default, all options are disabled.

    \sa setOption(), testOption()
*/
void QInputDialog::setOptions(InputDialogOptions options)
{
    Q_D(QInputDialog);

    InputDialogOptions changed = (options ^ d->opts);
    if (!changed)
        return;

    d->opts = options;
    d->ensureLayout();

    if (changed & NoButtons)
        d->buttonBox->setVisible(!(options & NoButtons));
    if ((changed & UseListViewForComboBoxItems) && inputMode() == TextInput)
        d->chooseRightTextInputWidget();
}

QInputDialog::InputDialogOptions QInputDialog::options() const
{
    Q_D(const QInputDialog);
    return d->opts;
}

/*!
    \since 4.5

    \property QInputDialog::textValue

    \brief the text value for the input dialog

    This property is only relevant when the input dialog is used in
    TextInput mode.
*/
void QInputDialog::setTextValue(const QString &text)
{
    Q_D(QInputDialog);

    setInputMode(TextInput);
    if (d->inputWidget == d->lineEdit) {
        d->lineEdit->setText(text);
    } else if (d->inputWidget == d->comboBox) {
        d->setComboBoxText(text);
    } else {
        d->setListViewText(text);
    }
}

QString QInputDialog::textValue() const
{
    Q_D(const QInputDialog);
    return d->textValue;
}

/*!
    \since 4.5

    \property QInputDialog::textEchoMode

    \brief the echo mode for the text value

    This property is only relevant when the input dialog is used in
    TextInput mode.
*/
void QInputDialog::setTextEchoMode(QLineEdit::EchoMode mode)
{
    Q_D(QInputDialog);
    d->ensureLineEdit();
    d->lineEdit->setEchoMode(mode);
}

QLineEdit::EchoMode QInputDialog::textEchoMode() const
{
    Q_D(const QInputDialog);
    if (d->lineEdit) {
        return d->lineEdit->echoMode();
    } else {
        return QLineEdit::Normal;
    }
}

/*!
    \since 4.5

    \property QInputDialog::comboBoxEditable

    \brief whether or not the combo box is used in the input dialog is editable
*/
void QInputDialog::setComboBoxEditable(bool editable)
{
    Q_D(QInputDialog);
    d->ensureComboBox();
    d->comboBox->setEditable(editable);
    if (inputMode() == TextInput)
        d->chooseRightTextInputWidget();
}

bool QInputDialog::isComboBoxEditable() const
{
    Q_D(const QInputDialog);
    if (d->comboBox) {
        return d->comboBox->isEditable();
    } else {
        return false;
    }
}

/*!
    \since 4.5

    \property QInputDialog::comboBoxItems

    \brief the items used in the combobox for the input dialog
*/
void QInputDialog::setComboBoxItems(const QStringList &items)
{
    Q_D(QInputDialog);

    d->ensureComboBox();
    d->comboBox->blockSignals(true);
    d->comboBox->clear();
    d->comboBox->addItems(items);
    d->comboBox->blockSignals(false);

    if (inputMode() == TextInput)
        d->chooseRightTextInputWidget();
}

QStringList QInputDialog::comboBoxItems() const
{
    Q_D(const QInputDialog);
    QStringList result;
    if (d->comboBox) {
        const int count = d->comboBox->count();
        for (int i = 0; i < count; ++i)
            result.append(d->comboBox->itemText(i));
    }
    return result;
}

/*!
    \property QInputDialog::intValue
    \since 4.5
    \brief the current integer value accepted as input

    This property is only relevant when the input dialog is used in
    IntInput mode.
*/
void QInputDialog::setIntValue(int value)
{
    Q_D(QInputDialog);
    setInputMode(IntInput);
    d->intSpinBox->setValue(value);
}

int QInputDialog::intValue() const
{
    Q_D(const QInputDialog);
    if (d->intSpinBox) {
        return d->intSpinBox->value();
    } else {
        return 0;
    }
}

/*!
    \property QInputDialog::intMinimum
    \since 4.5
    \brief the minimum integer value accepted as input

    This property is only relevant when the input dialog is used in
    IntInput mode.
*/
void QInputDialog::setIntMinimum(int min)
{
    Q_D(QInputDialog);
    d->ensureIntSpinBox();
    d->intSpinBox->setMinimum(min);
}

int QInputDialog::intMinimum() const
{
    Q_D(const QInputDialog);
    if (d->intSpinBox) {
        return d->intSpinBox->minimum();
    } else {
        return 0;
    }
}

/*!
    \property QInputDialog::intMaximum
    \since 4.5
    \brief the maximum integer value accepted as input

    This property is only relevant when the input dialog is used in
    IntInput mode.
*/
void QInputDialog::setIntMaximum(int max)
{
    Q_D(QInputDialog);
    d->ensureIntSpinBox();
    d->intSpinBox->setMaximum(max);
}

int QInputDialog::intMaximum() const
{
    Q_D(const QInputDialog);
    if (d->intSpinBox) {
        return d->intSpinBox->maximum();
    } else {
        return 99;
    }
}

/*!
    Sets the range of integer values accepted by the dialog when used in
    IntInput mode, with minimum and maximum values specified by \a min and
    \a max respectively.
*/
void QInputDialog::setIntRange(int min, int max)
{
    Q_D(QInputDialog);
    d->ensureIntSpinBox();
    d->intSpinBox->setRange(min, max);
}

/*!
    \property QInputDialog::intStep
    \since 4.5
    \brief the step by which the integer value is increased and decreased

    This property is only relevant when the input dialog is used in
    IntInput mode.
*/
void QInputDialog::setIntStep(int step)
{
    Q_D(QInputDialog);
    d->ensureIntSpinBox();
    d->intSpinBox->setSingleStep(step);
}

int QInputDialog::intStep() const
{
    Q_D(const QInputDialog);
    if (d->intSpinBox) {
        return d->intSpinBox->singleStep();
    } else {
        return 1;
    }
}

/*!
    \property QInputDialog::doubleValue
    \since 4.5
    \brief the current double precision floating point value accepted as input

    This property is only relevant when the input dialog is used in
    DoubleInput mode.
*/
void QInputDialog::setDoubleValue(double value)
{
    Q_D(QInputDialog);
    setInputMode(DoubleInput);
    d->doubleSpinBox->setValue(value);
}

double QInputDialog::doubleValue() const
{
    Q_D(const QInputDialog);
    if (d->doubleSpinBox) {
        return d->doubleSpinBox->value();
    } else {
        return 0.0;
    }
}

/*!
    \property QInputDialog::doubleMinimum
    \since 4.5
    \brief the minimum double precision floating point value accepted as input

    This property is only relevant when the input dialog is used in
    DoubleInput mode.
*/
void QInputDialog::setDoubleMinimum(double min)
{
    Q_D(QInputDialog);
    d->ensureDoubleSpinBox();
    d->doubleSpinBox->setMinimum(min);
}

double QInputDialog::doubleMinimum() const
{
    Q_D(const QInputDialog);
    if (d->doubleSpinBox) {
        return d->doubleSpinBox->minimum();
    } else {
        return 0.0;
    }
}

/*!
    \property QInputDialog::doubleMaximum
    \since 4.5
    \brief the maximum double precision floating point value accepted as input

    This property is only relevant when the input dialog is used in
    DoubleInput mode.
*/
void QInputDialog::setDoubleMaximum(double max)
{
    Q_D(QInputDialog);
    d->ensureDoubleSpinBox();
    d->doubleSpinBox->setMaximum(max);
}

double QInputDialog::doubleMaximum() const
{
    Q_D(const QInputDialog);
    if (d->doubleSpinBox) {
        return d->doubleSpinBox->maximum();
    } else {
        return 99.99;
    }
}

/*!
    Sets the range of double precision floating point values accepted by the
    dialog when used in DoubleInput mode, with minimum and maximum values
    specified by \a min and \a max respectively.
*/
void QInputDialog::setDoubleRange(double min, double max)
{
    Q_D(QInputDialog);
    d->ensureDoubleSpinBox();
    d->doubleSpinBox->setRange(min, max);
}

/*!
    \since 4.5

    \property QInputDialog::doubleDecimals

    \brief sets the percision of the double spinbox in decimals

    \sa QDoubleSpinBox::setDecimals()
*/
void QInputDialog::setDoubleDecimals(int decimals)
{
    Q_D(QInputDialog);
    d->ensureDoubleSpinBox();
    d->doubleSpinBox->setDecimals(decimals);
}

int QInputDialog::doubleDecimals() const
{
    Q_D(const QInputDialog);
    if (d->doubleSpinBox) {
        return d->doubleSpinBox->decimals();
    } else {
        return 2;
    }
}

/*!
    \since 4.5

    \property QInputDialog::okButtonText

    \brief the text for the button used to accept the entry in the dialog
*/
void QInputDialog::setOkButtonText(const QString &text)
{
    Q_D(const QInputDialog);
    d->ensureLayout();
    d->buttonBox->button(QDialogButtonBox::Ok)->setText(text);
}

QString QInputDialog::okButtonText() const
{
    Q_D(const QInputDialog);
    d->ensureLayout();
    return d->buttonBox->button(QDialogButtonBox::Ok)->text();
}

/*!
    \since 4.5

    \property QInputDialog::cancelButtonText
    \brief the text for the button used to cancel the dialog
*/
void QInputDialog::setCancelButtonText(const QString &text)
{
    Q_D(const QInputDialog);
    d->ensureLayout();
    d->buttonBox->button(QDialogButtonBox::Cancel)->setText(text);
}

QString QInputDialog::cancelButtonText() const
{
    Q_D(const QInputDialog);
    d->ensureLayout();
    return d->buttonBox->button(QDialogButtonBox::Cancel)->text();
}

/*!
    \since 4.5
    \overload

    This function connects one of its signals to the slot specified by \a receiver
    and \a member. The specific signal depends on the arguments that are specified
    in \a member. These are:

    \list
      \o textValueSelected() if \a member has a QString for its first argument.
      \o intValueSelected() if \a member has an int for its first argument.
      \o doubleValueSelected() if \a member has a double for its first argument.
      \o accepted() if \a member has NO arguments.
    \endlist

    The signal will be disconnected from the slot when the dialog is closed.
*/
void QInputDialog::open(QObject *receiver, const char *member)
{
    Q_D(QInputDialog);
    connect(this, signalForMember(member), receiver, member);
    d->receiverToDisconnectOnClose = receiver;
    d->memberToDisconnectOnClose = member;
    QDialog::open();
}

/*!
    \reimp
*/
QSize QInputDialog::minimumSizeHint() const
{
    Q_D(const QInputDialog);
    d->ensureLayout();
    return QDialog::minimumSizeHint();
}

/*!
    \reimp
*/
QSize QInputDialog::sizeHint() const
{
    Q_D(const QInputDialog);
    d->ensureLayout();
    return QDialog::sizeHint();
}

/*!
    \reimp
*/
void QInputDialog::setVisible(bool visible)
{
    Q_D(const QInputDialog);
    if (visible) {
        d->ensureLayout();
        d->inputWidget->setFocus();
        if (d->inputWidget == d->lineEdit) {
            d->lineEdit->selectAll();
        } else if (d->inputWidget == d->intSpinBox) {
            d->intSpinBox->selectAll();
        } else if (d->inputWidget == d->doubleSpinBox) {
            d->doubleSpinBox->selectAll();
        }
    }
    QDialog::setVisible(visible);
}

/*!
  Closes the dialog and sets its result code to \a result. If this dialog
  is shown with exec(), done() causes the local event loop to finish,
  and exec() to return \a result.

  \sa QDialog::done()
*/
void QInputDialog::done(int result)
{
    Q_D(QInputDialog);
    QDialog::done(result);
    if (result) {
        InputMode mode = inputMode();
        switch (mode) {
        case DoubleInput:
            emit doubleValueSelected(doubleValue());
            break;
        case IntInput:
            emit intValueSelected(intValue());
            break;
        default:
            Q_ASSERT(mode == TextInput);
            emit textValueSelected(textValue());
        }
    }
    if (d->receiverToDisconnectOnClose) {
        disconnect(this, signalForMember(d->memberToDisconnectOnClose),
                   d->receiverToDisconnectOnClose, d->memberToDisconnectOnClose);
        d->receiverToDisconnectOnClose = 0;
    }
    d->memberToDisconnectOnClose.clear();
}

/*!
    Static convenience function to get a string from the user.

    \a title is the text which is displayed in the title bar of the dialog.
    \a label is the text which is shown to the user (it should say what should
    be entered).
    \a text is the default text which is placed in the line edit.
    \a mode is the echo mode the line edit will use.
    \a inputMethodHints is the input method hints that will be used in the
    edit widget if an input method is active.

    If \a ok is nonnull \e *\a ok will be set to true if the user pressed
    \gui OK and to false if the user pressed \gui Cancel. The dialog's parent
    is \a parent. The dialog will be modal and uses the specified widget
    \a flags.

    If the dialog is accepted, this function returns the text in the dialog's
    line edit. If the dialog is rejected, a null QString is returned.

    Use this static function like this:

    \snippet examples/dialogs/standarddialogs/dialog.cpp 3

    \warning Do not delete \a parent during the execution of the dialog. If you
    want to do this, you should create the dialog yourself using one of the
    QInputDialog constructors.

    \sa getInt(), getDouble(), getItem()
*/

QString QInputDialog::getText(QWidget *parent, const QString &title, const QString &label,
                              QLineEdit::EchoMode mode, const QString &text, bool *ok,
                              Qt::WindowFlags flags, Qt::InputMethodHints inputMethodHints)
{
    QInputDialog dialog(parent, flags);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setTextValue(text);
    dialog.setTextEchoMode(mode);
    dialog.setInputMethodHints(inputMethodHints);

    int ret = dialog.exec();
    if (ok)
        *ok = !!ret;
    if (ret) {
        return dialog.textValue();
    } else {
        return QString();
    }
}

/*!
    \internal
*/
// ### Qt 5: Use only the version above.
QString QInputDialog::getText(QWidget *parent, const QString &title, const QString &label,
                              QLineEdit::EchoMode mode, const QString &text, bool *ok,
                              Qt::WindowFlags flags)
{
    return getText(parent, title, label, mode, text, ok, flags, Qt::ImhNone);
}

/*!
    \since 4.5

    Static convenience function to get an integer input from the user.

    \a title is the text which is displayed in the title bar of the dialog.
    \a label is the text which is shown to the user (it should say what should
    be entered).
    \a value is the default integer which the spinbox will be set to.
    \a min and \a max are the minimum and maximum values the user may choose.
    \a step is the amount by which the values change as the user presses the
    arrow buttons to increment or decrement the value.

    If \a ok is nonnull *\a ok will be set to true if the user pressed \gui OK
    and to false if the user pressed \gui Cancel. The dialog's parent is
    \a parent. The dialog will be modal and uses the widget \a flags.

    On success, this function returns the integer which has been entered by the
    user; on failure, it returns the initial \a value.

    Use this static function like this:

    \snippet examples/dialogs/standarddialogs/dialog.cpp 0

    \warning Do not delete \a parent during the execution of the dialog. If you
    want to do this, you should create the dialog yourself using one of the
    QInputDialog constructors.

    \sa getText(), getDouble(), getItem()
*/

int QInputDialog::getInt(QWidget *parent, const QString &title, const QString &label, int value,
                         int min, int max, int step, bool *ok, Qt::WindowFlags flags)
{
    QInputDialog dialog(parent, flags);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setIntRange(min, max);
    dialog.setIntValue(value);
    dialog.setIntStep(step);

    int ret = dialog.exec();
    if (ok)
        *ok = !!ret;
    if (ret) {
        return dialog.intValue();
    } else {
        return value;
    }
}

/*!
    Static convenience function to get a floating point number from the user.

    \a title is the text which is displayed in the title bar of the dialog.
    \a label is the text which is shown to the user (it should say what should
    be entered).
    \a value is the default floating point number that the line edit will be
    set to.
    \a min and \a max are the minimum and maximum values the user may choose.
    \a decimals is the maximum number of decimal places the number may have.

    If \a ok is nonnull, *\a ok will be set to true if the user pressed \gui OK
    and to false if the user pressed \gui Cancel. The dialog's parent is
    \a parent. The dialog will be modal and uses the widget \a flags.

    This function returns the floating point number which has been entered by
    the user.

    Use this static function like this:

    \snippet examples/dialogs/standarddialogs/dialog.cpp 1

    \warning Do not delete \a parent during the execution of the dialog. If you
    want to do this, you should create the dialog yourself using one of the
    QInputDialog constructors.

    \sa getText(), getInt(), getItem()
*/

double QInputDialog::getDouble(QWidget *parent, const QString &title, const QString &label,
                               double value, double min, double max, int decimals, bool *ok,
                               Qt::WindowFlags flags)
{
    QInputDialog dialog(parent, flags);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setDoubleDecimals(decimals);
    dialog.setDoubleRange(min, max);
    dialog.setDoubleValue(value);

    int ret = dialog.exec();
    if (ok)
        *ok = !!ret;
    if (ret) {
        return dialog.doubleValue();
    } else {
        return value;
    }
}

/*!
    Static convenience function to let the user select an item from a string
    list.

    \a title is the text which is displayed in the title bar of the dialog.
    \a label is the text which is shown to the user (it should say what should
    be entered).
    \a items is the string list which is inserted into the combobox.
    \a current is the number  of the item which should be the current item.
    \a inputMethodHints is the input method hints that will be used if the
    combobox is editable and an input method is active.

    If \a editable is true the user can enter their own text; otherwise the
    user may only select one of the existing items.

    If \a ok is nonnull \e *\a ok will be set to true if the user pressed
    \gui OK and to false if the user pressed \gui Cancel. The dialog's parent
    is \a parent. The dialog will be modal and uses the widget \a flags.

    This function returns the text of the current item, or if \a editable is
    true, the current text of the combobox.

    Use this static function like this:

    \snippet examples/dialogs/standarddialogs/dialog.cpp 2

    \warning Do not delete \a parent during the execution of the dialog. If you
    want to do this, you should create the dialog yourself using one of the
    QInputDialog constructors.

    \sa getText(), getInt(), getDouble()
*/

QString QInputDialog::getItem(QWidget *parent, const QString &title, const QString &label,
                              const QStringList &items, int current, bool editable, bool *ok,
                              Qt::WindowFlags flags, Qt::InputMethodHints inputMethodHints)
{
    QString text(items.value(current));

    QInputDialog dialog(parent, flags);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setComboBoxItems(items);
    dialog.setTextValue(text);
    dialog.setComboBoxEditable(editable);
    dialog.setInputMethodHints(inputMethodHints);

    int ret = dialog.exec();
    if (ok)
        *ok = !!ret;
    if (ret) {
        return dialog.textValue();
    } else {
        return text;
    }
}

/*!
    \internal
*/
// ### Qt 5: Use only the version above.
QString QInputDialog::getItem(QWidget *parent, const QString &title, const QString &label,
                              const QStringList &items, int current, bool editable, bool *ok,
                              Qt::WindowFlags flags)
{
    return getItem(parent, title, label, items, current, editable, ok, flags, Qt::ImhNone);
}

/*!
    \obsolete

    Use getInt() instead.
*/
int QInputDialog::getInteger(QWidget *parent, const QString &title, const QString &label,
                             int value, int min, int max, int step, bool *ok,
                             Qt::WindowFlags flags)
{
    return getInt(parent, title, label, value, min, max, step, ok, flags);
}

/*!
    \fn QString QInputDialog::getText(const QString &title, const QString &label,
                                      QLineEdit::EchoMode echo = QLineEdit::Normal,
                                      const QString &text = QString(), bool *ok = 0,
                                      QWidget *parent = 0, const char *name = 0, Qt::WindowFlags flags = 0)

    Call getText(\a parent, \a title, \a label, \a echo, \a text, \a
    ok, \a flags) instead.

    The \a name parameter is ignored.
*/

/*!
    \fn int QInputDialog::getInteger(const QString &title, const QString &label, int value = 0,
                                     int min = -2147483647, int max = 2147483647,
                                     int step = 1, bool *ok = 0,
                                     QWidget *parent = 0, const char *name = 0, Qt::WindowFlags flags = 0)


    Call getInteger(\a parent, \a title, \a label, \a value, \a
    min, \a max, \a step, \a ok, \a flags) instead.

    The \a name parameter is ignored.
*/

/*!
    \fn double QInputDialog::getDouble(const QString &title, const QString &label, double value = 0,
                                       double min = -2147483647, double max = 2147483647,
                                       int decimals = 1, bool *ok = 0,
                                       QWidget *parent = 0, const char *name = 0, Qt::WindowFlags flags = 0)

    Call getDouble(\a parent, \a title, \a label, \a value, \a
    min, \a max, \a decimals, \a ok, \a flags).

    The \a name parameter is ignored.
*/

/*!
    \fn QString QInputDialog::getItem(const QString &title, const QString &label, const QStringList &list,
                                      int current = 0, bool editable = true, bool *ok = 0,
                                      QWidget *parent = 0, const char *name = 0, Qt::WindowFlags flags = 0)

    Call getItem(\a parent, \a title, \a label, \a list, \a current,
    \a editable, \a ok, \a flags) instead.

    The \a name parameter is ignored.
*/

/*!
    \fn void QInputDialog::doubleValueChanged(double value)

    This signal is emitted whenever the double value changes in the dialog.
    The current value is specified by \a value.

    This signal is only relevant when the input dialog is used in
    DoubleInput mode.
*/

/*!
    \fn void QInputDialog::doubleValueSelected(double value)

    This signal is emitted whenever the user selects a double value by
    accepting the dialog; for example, by clicking the \gui{OK} button.
    The selected value is specified by \a value.

    This signal is only relevant when the input dialog is used in
    DoubleInput mode.
*/

/*!
    \fn void QInputDialog::intValueChanged(int value)

    This signal is emitted whenever the integer value changes in the dialog.
    The current value is specified by \a value.

    This signal is only relevant when the input dialog is used in
    IntInput mode.
*/

/*!
    \fn void QInputDialog::intValueSelected(int value)

    This signal is emitted whenever the user selects a integer value by
    accepting the dialog; for example, by clicking the \gui{OK} button.
    The selected value is specified by \a value.

    This signal is only relevant when the input dialog is used in
    IntInput mode.
*/

/*!
    \fn void QInputDialog::textValueChanged(const QString &text)

    This signal is emitted whenever the text string changes in the dialog.
    The current string is specified by \a text.

    This signal is only relevant when the input dialog is used in
    TextInput mode.
*/

/*!
    \fn void QInputDialog::textValueSelected(const QString &text)

    This signal is emitted whenever the user selects a text string by
    accepting the dialog; for example, by clicking the \gui{OK} button.
    The selected string is specified by \a text.

    This signal is only relevant when the input dialog is used in
    TextInput mode.
*/

QT_END_NAMESPACE

#include "moc_qinputdialog.cpp"

#endif // QT_NO_INPUTDIALOG
