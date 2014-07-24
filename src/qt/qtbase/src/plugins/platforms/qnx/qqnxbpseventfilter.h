/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QQNXBPSEVENTFILTER_H
#define QQNXBPSEVENTFILTER_H

#include <QObject>
#include <QHash>
#include <QAbstractNativeEventFilter>

#include <bps/dialog.h>

struct bps_event_t;

QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;
class QQnxNavigatorEventHandler;
class QQnxFileDialogHelper;
class QQnxScreen;
class QQnxScreenEventHandler;
class QQnxVirtualKeyboardBps;

class QQnxBpsEventFilter : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    QQnxBpsEventFilter(QQnxNavigatorEventHandler *navigatorEventHandler,
                       QQnxScreenEventHandler *screenEventHandler,
                       QQnxVirtualKeyboardBps *virtualKeyboard, QObject *parent = 0);
    ~QQnxBpsEventFilter();

    void installOnEventDispatcher(QAbstractEventDispatcher *dispatcher);

    void registerForScreenEvents(QQnxScreen *screen);
    void unregisterForScreenEvents(QQnxScreen *screen);

#ifdef Q_OS_BLACKBERRY_TABLET
    void registerForDialogEvents(QQnxFileDialogHelper *dialog);
    void unregisterForDialogEvents(QQnxFileDialogHelper *dialog);
#endif

private:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) Q_DECL_OVERRIDE;

    bool handleNavigatorEvent(bps_event_t *event);

private:
    QQnxNavigatorEventHandler *m_navigatorEventHandler;
    QQnxScreenEventHandler *m_screenEventHandler;
    QQnxVirtualKeyboardBps *m_virtualKeyboard;
    QHash<dialog_instance_t, QQnxFileDialogHelper*> m_dialogMapper;
};

QT_END_NAMESPACE

#endif // QQNXBPSEVENTFILTER_H
