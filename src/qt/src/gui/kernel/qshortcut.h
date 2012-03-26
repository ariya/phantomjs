/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QSHORTCUT_H
#define QSHORTCUT_H

#include <QtGui/qwidget.h>
#include <QtGui/qkeysequence.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_SHORTCUT

class QShortcutPrivate;
class Q_GUI_EXPORT QShortcut : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QShortcut)
    Q_PROPERTY(QKeySequence key READ key WRITE setKey)
    Q_PROPERTY(QString whatsThis READ whatsThis WRITE setWhatsThis)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
    Q_PROPERTY(bool autoRepeat READ autoRepeat WRITE setAutoRepeat)
    Q_PROPERTY(Qt::ShortcutContext context READ context WRITE setContext)
public:
    explicit QShortcut(QWidget *parent);
    QShortcut(const QKeySequence& key, QWidget *parent,
              const char *member = 0, const char *ambiguousMember = 0,
              Qt::ShortcutContext context = Qt::WindowShortcut);
    ~QShortcut();

    void setKey(const QKeySequence& key);
    QKeySequence key() const;

    void setEnabled(bool enable);
    bool isEnabled() const;

    void setContext(Qt::ShortcutContext context);
    Qt::ShortcutContext context();

    void setWhatsThis(const QString &text);
    QString whatsThis() const;

    void setAutoRepeat(bool on);
    bool autoRepeat() const;

    int id() const;

    inline QWidget *parentWidget() const
    { return static_cast<QWidget *>(QObject::parent()); }

Q_SIGNALS:
    void activated();
    void activatedAmbiguously();

protected:
    bool event(QEvent *e);
};

#endif // QT_NO_SHORTCUT

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSHORTCUT_H
