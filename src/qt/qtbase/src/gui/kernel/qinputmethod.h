/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QINPUTMETHOD_H
#define QINPUTMETHOD_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QInputMethodPrivate;
class QWindow;
class QRectF;
class QTransform;
class QInputMethodQueryEvent;

class Q_GUI_EXPORT QInputMethod : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QInputMethod)
    Q_PROPERTY(QRectF cursorRectangle READ cursorRectangle NOTIFY cursorRectangleChanged)
    Q_PROPERTY(QRectF keyboardRectangle READ keyboardRectangle NOTIFY keyboardRectangleChanged)
    Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged)
    Q_PROPERTY(bool animating READ isAnimating NOTIFY animatingChanged)
    Q_PROPERTY(QLocale locale READ locale NOTIFY localeChanged)
    Q_PROPERTY(Qt::LayoutDirection inputDirection READ inputDirection NOTIFY inputDirectionChanged)

    Q_ENUMS(Action)
public:
    QTransform inputItemTransform() const;
    void setInputItemTransform(const QTransform &transform);

    QRectF inputItemRectangle() const;
    void setInputItemRectangle(const QRectF &rect);

    // in window coordinates
    QRectF cursorRectangle() const; // ### what if we have rotations for the item?

    // keyboard geometry in window coords
    QRectF keyboardRectangle() const;

    enum Action {
        Click,
        ContextMenu
    };

    bool isVisible() const;
    void setVisible(bool visible);

    bool isAnimating() const;

    QLocale locale() const;
    Qt::LayoutDirection inputDirection() const;

    static QVariant queryFocusObject(Qt::InputMethodQuery query, QVariant argument);

public Q_SLOTS:
    void show();
    void hide();

    void update(Qt::InputMethodQueries queries);
    void reset();
    void commit();

    void invokeAction(Action a, int cursorPosition);

Q_SIGNALS:
    void cursorRectangleChanged();
    void keyboardRectangleChanged();
    void visibleChanged();
    void animatingChanged();
    void localeChanged();
    void inputDirectionChanged(Qt::LayoutDirection newDirection);

private:
    friend class QGuiApplication;
    friend class QGuiApplicationPrivate;
    friend class QPlatformInputContext;
    QInputMethod();
    ~QInputMethod();
};

QT_END_NAMESPACE

#endif
