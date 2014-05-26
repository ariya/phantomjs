/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QCOLORDIALOG_H
#define QCOLORDIALOG_H

#include <QtGui/qdialog.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_COLORDIALOG

class QColorDialogPrivate;

class Q_GUI_EXPORT QColorDialog : public QDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QColorDialog)
    Q_ENUMS(ColorDialogOption)
    Q_PROPERTY(QColor currentColor READ currentColor WRITE setCurrentColor
               NOTIFY currentColorChanged)
    Q_PROPERTY(ColorDialogOptions options READ options WRITE setOptions)

public:
    enum ColorDialogOption {
        ShowAlphaChannel    = 0x00000001,
        NoButtons           = 0x00000002,
        DontUseNativeDialog = 0x00000004
    };

    Q_DECLARE_FLAGS(ColorDialogOptions, ColorDialogOption)

    explicit QColorDialog(QWidget *parent = 0);
    explicit QColorDialog(const QColor &initial, QWidget *parent = 0);
    ~QColorDialog();

    void setCurrentColor(const QColor &color);
    QColor currentColor() const;

    QColor selectedColor() const;

    void setOption(ColorDialogOption option, bool on = true);
    bool testOption(ColorDialogOption option) const;
    void setOptions(ColorDialogOptions options);
    ColorDialogOptions options() const;

#ifdef Q_NO_USING_KEYWORD
    void open() { QDialog::open(); }
#else
    using QDialog::open;
#endif
    void open(QObject *receiver, const char *member);

    void setVisible(bool visible);

    // ### Qt 5: merge overloads with title = QString()
    static QColor getColor(const QColor &initial, QWidget *parent, const QString &title,
                           ColorDialogOptions options = 0);
    static QColor getColor(const QColor &initial = Qt::white, QWidget *parent = 0);

    // obsolete
    static QRgb getRgba(QRgb rgba = 0xffffffff, bool *ok = 0, QWidget *parent = 0);

    // ### Qt 5: use QColor in signatures
    static int customCount();
    static QRgb customColor(int index);
    static void setCustomColor(int index, QRgb color);
    static void setStandardColor(int index, QRgb color);

#ifdef QT3_SUPPORT
    static QColor getColor(const QColor &init, QWidget *parent, const char *name)
        { Q_UNUSED(name); return getColor(init, parent); }
    static QRgb getRgba(QRgb rgba, bool *ok, QWidget *parent, const char *name)
        { Q_UNUSED(name); return getRgba(rgba, ok, parent); }
#endif

Q_SIGNALS:
    void currentColorChanged(const QColor &color);
    void colorSelected(const QColor &color);

protected:
    void changeEvent(QEvent *event);
    void done(int result);

private:
    Q_DISABLE_COPY(QColorDialog)

    Q_PRIVATE_SLOT(d_func(), void _q_addCustom())
    Q_PRIVATE_SLOT(d_func(), void _q_newHsv(int h, int s, int v))
    Q_PRIVATE_SLOT(d_func(), void _q_newColorTypedIn(QRgb rgb))
    Q_PRIVATE_SLOT(d_func(), void _q_newCustom(int, int))
    Q_PRIVATE_SLOT(d_func(), void _q_newStandard(int, int))
#if defined(Q_WS_MAC)
    Q_PRIVATE_SLOT(d_func(), void _q_macRunNativeAppModalPanel())
#endif

    friend class QColorShower;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QColorDialog::ColorDialogOptions)

#endif // QT_NO_COLORDIALOG

QT_END_NAMESPACE

QT_END_HEADER

#endif // QCOLORDIALOG_H
