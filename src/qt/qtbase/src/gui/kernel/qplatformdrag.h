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

#ifndef QPLATFORMDRAG_H
#define QPLATFORMDRAG_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtCore/qglobal.h>
#include <QtGui/QPixmap>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DRAGANDDROP

class QMimeData;
class QMouseEvent;
class QDrag;
class QObject;
class QEvent;
class QPlatformDragPrivate;

class Q_GUI_EXPORT QPlatformDropQtResponse
{
public:
    QPlatformDropQtResponse(bool accepted, Qt::DropAction acceptedAction);
    bool isAccepted() const;
    Qt::DropAction acceptedAction() const;

private:
    bool m_accepted;
    Qt::DropAction m_accepted_action;

};

class Q_GUI_EXPORT QPlatformDragQtResponse : public QPlatformDropQtResponse
{
public:
    QPlatformDragQtResponse(bool accepted, Qt::DropAction acceptedAction, QRect answerRect);

    QRect answerRect() const;

private:
    QRect m_answer_rect;
};

class Q_GUI_EXPORT QPlatformDrag
{
    Q_DECLARE_PRIVATE(QPlatformDrag)
public:
    QPlatformDrag();
    virtual ~QPlatformDrag();

    QDrag *currentDrag() const;
    virtual QMimeData *platformDropData() = 0;

    virtual Qt::DropAction drag(QDrag *m_drag) = 0;
    void updateAction(Qt::DropAction action);

    virtual Qt::DropAction defaultAction(Qt::DropActions possibleActions, Qt::KeyboardModifiers modifiers) const;

    static QPixmap defaultPixmap();

private:
    QPlatformDragPrivate *d_ptr;

    Q_DISABLE_COPY(QPlatformDrag)
};

#endif // QT_NO_DRAGANDDROP

QT_END_NAMESPACE

#endif
