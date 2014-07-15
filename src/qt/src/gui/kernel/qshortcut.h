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
