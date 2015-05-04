/****************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB).
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

#ifndef QOPENGLTIMERQUERY_H
#define QOPENGLTIMERQUERY_H

#include <QtCore/qglobal.h>

#if !defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_2)

#include <QtCore/QObject>
#include <QtGui/qopengl.h>

QT_BEGIN_NAMESPACE

class QOpenGLTimerQueryPrivate;

class Q_GUI_EXPORT QOpenGLTimerQuery : public QObject
{
    Q_OBJECT

public:
    explicit QOpenGLTimerQuery(QObject *parent = 0);
    ~QOpenGLTimerQuery();

    bool create();
    void destroy();
    bool isCreated() const;
    GLuint objectId() const;

    void begin();
    void end();
    GLuint64 waitForTimestamp() const;
    void recordTimestamp();
    bool isResultAvailable() const;
    GLuint64 waitForResult() const;

private:
    Q_DECLARE_PRIVATE(QOpenGLTimerQuery)
    Q_DISABLE_COPY(QOpenGLTimerQuery)
};


class QOpenGLTimeMonitorPrivate;

class Q_GUI_EXPORT QOpenGLTimeMonitor : public QObject
{
    Q_OBJECT

public:
    explicit QOpenGLTimeMonitor(QObject *parent = 0);
    ~QOpenGLTimeMonitor();

    void setSampleCount(int sampleCount);
    int sampleCount() const;

    bool create();
    void destroy();
    bool isCreated() const;
    QVector<GLuint> objectIds() const;

    int recordSample();

    bool isResultAvailable() const;

    QVector<GLuint64> waitForSamples() const;
    QVector<GLuint64> waitForIntervals() const;

    void reset();

private:
    Q_DECLARE_PRIVATE(QOpenGLTimeMonitor)
    Q_DISABLE_COPY(QOpenGLTimeMonitor)
};

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QOPENGLTIMERQUERY_H
