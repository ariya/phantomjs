/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QINPUTDIALOG_H
#define QINPUTDIALOG_H

#include <QtWidgets/qdialog.h>
#include <QtCore/qstring.h>
#include <QtWidgets/qlineedit.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_INPUTDIALOG

class QInputDialogPrivate;

class Q_WIDGETS_EXPORT QInputDialog : public QDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QInputDialog)
//  Q_ENUMS(InputMode InputDialogOption)
    QDOC_PROPERTY(InputMode inputMode READ inputMode WRITE setInputMode)
    QDOC_PROPERTY(QString labelText READ labelText WRITE setLabelText)
    QDOC_PROPERTY(InputDialogOptions options READ options WRITE setOptions)
    QDOC_PROPERTY(QString textValue READ textValue WRITE setTextValue NOTIFY textValueChanged)
    QDOC_PROPERTY(int intValue READ intValue WRITE setIntValue NOTIFY intValueChanged)
    QDOC_PROPERTY(int doubleValue READ doubleValue WRITE setDoubleValue NOTIFY doubleValueChanged)
    QDOC_PROPERTY(QLineEdit::EchoMode textEchoMode READ textEchoMode WRITE setTextEchoMode)
    QDOC_PROPERTY(bool comboBoxEditable READ isComboBoxEditable WRITE setComboBoxEditable)
    QDOC_PROPERTY(QStringList comboBoxItems READ comboBoxItems WRITE setComboBoxItems)
    QDOC_PROPERTY(int intMinimum READ intMinimum WRITE setIntMinimum)
    QDOC_PROPERTY(int intMaximum READ intMaximum WRITE setIntMaximum)
    QDOC_PROPERTY(int intStep READ intStep WRITE setIntStep)
    QDOC_PROPERTY(double doubleMinimum READ doubleMinimum WRITE setDoubleMinimum)
    QDOC_PROPERTY(double doubleMaximum READ doubleMaximum WRITE setDoubleMaximum)
    QDOC_PROPERTY(int doubleDecimals READ doubleDecimals WRITE setDoubleDecimals)
    QDOC_PROPERTY(QString okButtonText READ okButtonText WRITE setOkButtonText)
    QDOC_PROPERTY(QString cancelButtonText READ cancelButtonText WRITE setCancelButtonText)

public:
    enum InputDialogOption {
        NoButtons                    = 0x00000001,
        UseListViewForComboBoxItems  = 0x00000002,
        UsePlainTextEditForTextInput = 0x00000004
    };

    Q_DECLARE_FLAGS(InputDialogOptions, InputDialogOption)

    enum InputMode {
        TextInput,
        IntInput,
        DoubleInput
    };

    QInputDialog(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~QInputDialog();

    void setInputMode(InputMode mode);
    InputMode inputMode() const;

    void setLabelText(const QString &text);
    QString labelText() const;

    void setOption(InputDialogOption option, bool on = true);
    bool testOption(InputDialogOption option) const;
    void setOptions(InputDialogOptions options);
    InputDialogOptions options() const;

    void setTextValue(const QString &text);
    QString textValue() const;

    void setTextEchoMode(QLineEdit::EchoMode mode);
    QLineEdit::EchoMode textEchoMode() const;

    void setComboBoxEditable(bool editable);
    bool isComboBoxEditable() const;

    void setComboBoxItems(const QStringList &items);
    QStringList comboBoxItems() const;

    void setIntValue(int value);
    int intValue() const;

    void setIntMinimum(int min);
    int intMinimum() const;

    void setIntMaximum(int max);
    int intMaximum() const;

    void setIntRange(int min, int max);

    void setIntStep(int step);
    int intStep() const;

    void setDoubleValue(double value);
    double doubleValue() const;

    void setDoubleMinimum(double min);
    double doubleMinimum() const;

    void setDoubleMaximum(double max);
    double doubleMaximum() const;

    void setDoubleRange(double min, double max);

    void setDoubleDecimals(int decimals);
    int doubleDecimals() const;

    void setOkButtonText(const QString &text);
    QString okButtonText() const;

    void setCancelButtonText(const QString &text);
    QString cancelButtonText() const;

#ifdef Q_NO_USING_KEYWORD
#ifndef Q_QDOC
    void open() { QDialog::open(); }
#endif
#else
    using QDialog::open;
#endif
    void open(QObject *receiver, const char *member);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    void setVisible(bool visible);

    static QString getText(QWidget *parent, const QString &title, const QString &label,
                           QLineEdit::EchoMode echo = QLineEdit::Normal,
                           const QString &text = QString(), bool *ok = 0, Qt::WindowFlags flags = 0,
                           Qt::InputMethodHints inputMethodHints = Qt::ImhNone);
    static QString getMultiLineText(QWidget *parent, const QString &title, const QString &label,
                           const QString &text = QString(), bool *ok = 0, Qt::WindowFlags flags = 0,
                           Qt::InputMethodHints inputMethodHints = Qt::ImhNone);
    static QString getItem(QWidget *parent, const QString &title, const QString &label,
                           const QStringList &items, int current = 0, bool editable = true,
                           bool *ok = 0, Qt::WindowFlags flags = 0,
                           Qt::InputMethodHints inputMethodHints = Qt::ImhNone);

    static int getInt(QWidget *parent, const QString &title, const QString &label, int value = 0,
                      int minValue = -2147483647, int maxValue = 2147483647,
                      int step = 1, bool *ok = 0, Qt::WindowFlags flags = 0);
    static double getDouble(QWidget *parent, const QString &title, const QString &label, double value = 0,
                            double minValue = -2147483647, double maxValue = 2147483647,
                            int decimals = 1, bool *ok = 0, Qt::WindowFlags flags = 0);

#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED static inline int getInteger(QWidget *parent, const QString &title, const QString &label, int value = 0,
                          int minValue = -2147483647, int maxValue = 2147483647,
                          int step = 1, bool *ok = 0, Qt::WindowFlags flags = 0)
    {
        return getInt(parent, title, label, value, minValue, maxValue, step, ok, flags);
    }
#endif

Q_SIGNALS:
    // ### emit signals!
    void textValueChanged(const QString &text);
    void textValueSelected(const QString &text);
    void intValueChanged(int value);
    void intValueSelected(int value);
    void doubleValueChanged(double value);
    void doubleValueSelected(double value);

public:
    void done(int result);

private:
    Q_DISABLE_COPY(QInputDialog)
    Q_PRIVATE_SLOT(d_func(), void _q_textChanged(const QString&))
    Q_PRIVATE_SLOT(d_func(), void _q_plainTextEditTextChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_currentRowChanged(const QModelIndex&, const QModelIndex&))
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QInputDialog::InputDialogOptions)

#endif // QT_NO_INPUTDIALOG

QT_END_NAMESPACE

#endif // QINPUTDIALOG_H
