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

#ifndef QPLATFORMINPUTCONTEXT_H
#define QPLATFORMINPUTCONTEXT_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qinputmethod.h>

QT_BEGIN_NAMESPACE

class QPlatformInputContextPrivate;

class Q_GUI_EXPORT QPlatformInputContext : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPlatformInputContext)

public:
    QPlatformInputContext();
    virtual ~QPlatformInputContext();

    virtual bool isValid() const;

    virtual void reset();
    virtual void commit();
    virtual void update(Qt::InputMethodQueries);
    virtual void invokeAction(QInputMethod::Action, int cursorPosition);
    virtual bool filterEvent(const QEvent *event);
    virtual QRectF keyboardRect() const;
    void emitKeyboardRectChanged();

    virtual bool isAnimating() const;
    void emitAnimatingChanged();

    virtual void showInputPanel();
    virtual void hideInputPanel();
    virtual bool isInputPanelVisible() const;
    void emitInputPanelVisibleChanged();

    virtual QLocale locale() const;
    void emitLocaleChanged();
    virtual Qt::LayoutDirection inputDirection() const;
    void emitInputDirectionChanged(Qt::LayoutDirection newDirection);

    virtual void setFocusObject(QObject *object);
    bool inputMethodAccepted() const;

private:
    friend class QGuiApplication;
    friend class QGuiApplicationPrivate;
    friend class QInputMethod;
};

QT_END_NAMESPACE

#endif // QPLATFORMINPUTCONTEXT_H
