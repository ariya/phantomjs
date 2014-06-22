/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QWINDOWSDIALOGHELPER_H
#define QWINDOWSDIALOGHELPER_H

#include "qtwindows_additional.h"
#include <qpa/qplatformdialoghelper.h>
#include <qpa/qplatformtheme.h>
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE

class QFileDialog;
class QDialog;
class QThread;
class QWindowsNativeDialogBase;

namespace QWindowsDialogs
{
    void eatMouseMove();

    bool useHelper(QPlatformTheme::DialogType type);
    QPlatformDialogHelper *createHelper(QPlatformTheme::DialogType type);
} // namespace QWindowsDialogs

template <class BaseClass>
class QWindowsDialogHelperBase : public BaseClass
{
    Q_DISABLE_COPY(QWindowsDialogHelperBase)
public:
    typedef QSharedPointer<QWindowsNativeDialogBase> QWindowsNativeDialogBasePtr;
    ~QWindowsDialogHelperBase() { cleanupThread(); }

    virtual void exec();
    virtual bool show(Qt::WindowFlags windowFlags,
                          Qt::WindowModality windowModality,
                          QWindow *parent);
    virtual void hide();

    virtual bool supportsNonModalDialog(const QWindow * /* parent */ = 0) const { return true; }

protected:
    QWindowsDialogHelperBase();
    QWindowsNativeDialogBase *nativeDialog() const;
    inline bool hasNativeDialog() const { return m_nativeDialog; }
    void timerEvent(QTimerEvent *);

private:
    virtual QWindowsNativeDialogBase *createNativeDialog() = 0;
    inline QWindowsNativeDialogBase *ensureNativeDialog();
    inline void startDialogThread();
    inline void stopTimer();
    void cleanupThread();

    QWindowsNativeDialogBasePtr m_nativeDialog;
    HWND m_ownerWindow;
    int m_timerId;
    QThread *m_thread;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIALOGHELPER_H
