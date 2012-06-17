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

#ifndef QUNDOSTACK_H
#define QUNDOSTACK_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QAction;
class QUndoCommandPrivate;
class QUndoStackPrivate;

#ifndef QT_NO_UNDOCOMMAND

class Q_GUI_EXPORT QUndoCommand
{
    QUndoCommandPrivate *d;

public:
    explicit QUndoCommand(QUndoCommand *parent = 0);
    explicit QUndoCommand(const QString &text, QUndoCommand *parent = 0);
    virtual ~QUndoCommand();

    virtual void undo();
    virtual void redo();

    QString text() const;
    QString actionText() const;
    void setText(const QString &text);

    virtual int id() const;
    virtual bool mergeWith(const QUndoCommand *other);

    int childCount() const;
    const QUndoCommand *child(int index) const;

private:
    Q_DISABLE_COPY(QUndoCommand)
    friend class QUndoStack;
};

#endif // QT_NO_UNDOCOMMAND

#ifndef QT_NO_UNDOSTACK

class Q_GUI_EXPORT QUndoStack : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QUndoStack)
    Q_PROPERTY(bool active READ isActive WRITE setActive)
    Q_PROPERTY(int undoLimit READ undoLimit WRITE setUndoLimit)

public:
    explicit QUndoStack(QObject *parent = 0);
    ~QUndoStack();
    void clear();

    void push(QUndoCommand *cmd);

    bool canUndo() const;
    bool canRedo() const;
    QString undoText() const;
    QString redoText() const;

    int count() const;
    int index() const;
    QString text(int idx) const;

#ifndef QT_NO_ACTION
    QAction *createUndoAction(QObject *parent,
                                const QString &prefix = QString()) const;
    QAction *createRedoAction(QObject *parent,
                                const QString &prefix = QString()) const;
#endif // QT_NO_ACTION

    bool isActive() const;
    bool isClean() const;
    int cleanIndex() const;

    void beginMacro(const QString &text);
    void endMacro();

    void setUndoLimit(int limit);
    int undoLimit() const;

    const QUndoCommand *command(int index) const;

public Q_SLOTS:
    void setClean();
    void setIndex(int idx);
    void undo();
    void redo();
    void setActive(bool active = true);

Q_SIGNALS:
    void indexChanged(int idx);
    void cleanChanged(bool clean);
    void canUndoChanged(bool canUndo);
    void canRedoChanged(bool canRedo);
    void undoTextChanged(const QString &undoText);
    void redoTextChanged(const QString &redoText);

private:
    Q_DISABLE_COPY(QUndoStack)
    friend class QUndoGroup;
};

#endif // QT_NO_UNDOSTACK

QT_END_NAMESPACE

QT_END_HEADER

#endif // QUNDOSTACK_H
