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

#ifndef QFONTDIALOG_H
#define QFONTDIALOG_H

#include <QtGui/qwindowdefs.h>
#include <QtGui/qdialog.h>
#include <QtGui/qfont.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_FONTDIALOG

class QFontDialogPrivate;

class Q_GUI_EXPORT QFontDialog : public QDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QFontDialog)
    Q_ENUMS(FontDialogOption)
    Q_PROPERTY(QFont currentFont READ currentFont WRITE setCurrentFont NOTIFY currentFontChanged)
    Q_PROPERTY(FontDialogOptions options READ options WRITE setOptions)

public:
    enum FontDialogOption {
        NoButtons           = 0x00000001,
        DontUseNativeDialog = 0x00000002
    };

    Q_DECLARE_FLAGS(FontDialogOptions, FontDialogOption)

    explicit QFontDialog(QWidget *parent = 0);
    explicit QFontDialog(const QFont &initial, QWidget *parent = 0);
    ~QFontDialog();

    void setCurrentFont(const QFont &font);
    QFont currentFont() const;

    QFont selectedFont() const;

    void setOption(FontDialogOption option, bool on = true);
    bool testOption(FontDialogOption option) const;
    void setOptions(FontDialogOptions options);
    FontDialogOptions options() const;

#ifdef Q_NO_USING_KEYWORD
#ifndef Q_QDOC
    void open() { QDialog::open(); }
#endif
#else
    using QDialog::open;
#endif
    void open(QObject *receiver, const char *member);

    void setVisible(bool visible);

    // ### Qt 5: merge overloads
    static QFont getFont(bool *ok, const QFont &initial, QWidget *parent, const QString &title,
                         FontDialogOptions options);
    static QFont getFont(bool *ok, const QFont &initial, QWidget *parent, const QString &title);
    static QFont getFont(bool *ok, const QFont &initial, QWidget *parent = 0);
    static QFont getFont(bool *ok, QWidget *parent = 0);

#ifdef QT3_SUPPORT
    static QFont getFont(bool *ok, const QFont &initial, QWidget *parent, const char *name)
        { Q_UNUSED(name); return getFont(ok, initial, parent); }
    static QFont getFont(bool *ok, QWidget *parent, const char *name)
        { Q_UNUSED(name); return getFont(ok, parent); }
#endif

Q_SIGNALS:
    void currentFontChanged(const QFont &font);
    void fontSelected(const QFont &font);

protected:
    void changeEvent(QEvent *event);
    void done(int result);

private:
    // ### Qt 5: make protected
    bool eventFilter(QObject *object, QEvent *event);

    Q_DISABLE_COPY(QFontDialog)

    Q_PRIVATE_SLOT(d_func(), void _q_sizeChanged(const QString &))
    Q_PRIVATE_SLOT(d_func(), void _q_familyHighlighted(int))
    Q_PRIVATE_SLOT(d_func(), void _q_writingSystemHighlighted(int))
    Q_PRIVATE_SLOT(d_func(), void _q_styleHighlighted(int))
    Q_PRIVATE_SLOT(d_func(), void _q_sizeHighlighted(int))
    Q_PRIVATE_SLOT(d_func(), void _q_updateSample())
#if defined(Q_WS_MAC)
    Q_PRIVATE_SLOT(d_func(), void _q_macRunNativeAppModalPanel())
#endif
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFontDialog::FontDialogOptions)

#endif // QT_NO_FONTDIALOG

QT_END_NAMESPACE

QT_END_HEADER

#endif // QFONTDIALOG_H
