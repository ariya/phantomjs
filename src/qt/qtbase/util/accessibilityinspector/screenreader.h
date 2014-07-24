/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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


#ifndef SCREENREADER_H
#define SCREENREADER_H

#include <QObject>
#include <QAccessible>
#include <QAccessibleBridge>

/*
    A Simple screen reader for touch-based user interfaces.

    Requires a text-to-speach backend. Currently implemented
    using festival on unix.
*/
class OptionsWidget;
class ScreenReader : public QObject
{
    Q_OBJECT
public:
    explicit ScreenReader(QObject *parent = 0);
    ~ScreenReader();

    void setRootObject(QObject *rootObject);
    void setOptionsWidget(OptionsWidget *optionsWidget);
public slots:
    void touchPoint(const QPoint &point);
    void activate();
protected slots:
    void processTouchPoint();
signals:
    void selected(QObject *object);

protected:
    void speak(const QString &text, const QString &voice = QString());
private:
    QAccessibleInterface *m_selectedInterface;
    QAccessibleInterface *m_rootInterface;
    OptionsWidget *m_optionsWidget;
    QPoint m_currentTouchPoint;
    bool m_activateCalled;
};

#endif // SCREENREADER_H
