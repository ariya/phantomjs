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

#include "screenreader.h"
#include "optionswidget.h"
#include "accessibilityscenemanager.h"
#include <QtGui>

ScreenReader::ScreenReader(QObject *parent) :
    QObject(parent)
{
    m_selectedInterface = 0;
    m_rootInterface = 0;
    bool activateCalled = false;
}

ScreenReader::~ScreenReader()
{
}

void ScreenReader::setRootObject(QObject *rootObject)
{
    m_rootInterface = QAccessible::queryAccessibleInterface(rootObject);
}

void ScreenReader::setOptionsWidget(OptionsWidget *optionsWidget)
{
    m_optionsWidget = optionsWidget;
}

void ScreenReader::touchPoint(const QPoint &point)
{
    qDebug() << "touch" << point;
    // Wait and see if this touch is the start of a double-tap
    // (activate will then be called and cancel the touch processing)
    m_activateCalled = false;
    m_currentTouchPoint = point;
    QTimer::singleShot(200, this, SLOT(processTouchPoint()));
}

void ScreenReader::processTouchPoint()
{
    if (m_activateCalled) {
        return;
    }

    if (m_rootInterface == 0) {
        return;
    }

    QAccessibleInterface * currentInterface = m_rootInterface;

    int hit = -2;
    int guardCounter = 0;
    const int guardMax = 40;
    while (currentInterface != 0) {
        ++guardCounter;
        if (guardCounter > guardMax) {
            qDebug() << "touchPoint exit recursion overflow";
            return; // outside
        }

        QAccessibleInterface * hit = currentInterface->childAt(m_currentTouchPoint.x(), m_currentTouchPoint.y());
        if (!hit)
            break;
        currentInterface = hit;
    }

    m_selectedInterface = currentInterface;
    if (m_selectedInterface->object())
        emit selected(m_selectedInterface->object());
    if (m_optionsWidget->enableTextToSpeach())
        speak(m_selectedInterface->text(QAccessible::Name)
              /*+ "," + translateRole(m_selectedInterface->role(0)) */);

//    qDebug() << "touchPoint exit found" << m_selectedInterface->text(QAccessible::Name, 0) << m_selectedInterface->object() << m_selectedInterface->rect(0);
}

void ScreenReader::activate()
{
    qDebug() << "ScreenReader::activate";
    m_activateCalled = true;
    if (m_selectedInterface) {
        m_selectedInterface->actionInterface()->doAction(QAccessibleActionInterface::pressAction());
    }
}

void ScreenReader::speak(const QString &text, const QString &/*voice*/)
{
    QFile f("festivalspeachhack");
    f.open(QIODevice::WriteOnly);
    f.write(text.toLocal8Bit());
    f.close();

    QProcess *process = new QProcess;
    process->start("/usr/bin/festival", QStringList() << "--tts" << "festivalspeachhack");
}


