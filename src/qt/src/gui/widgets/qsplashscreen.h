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

#ifndef QSPLASHSCREEN_H
#define QSPLASHSCREEN_H

#include <QtGui/qpixmap.h>
#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_SPLASHSCREEN
class QSplashScreenPrivate;

class Q_GUI_EXPORT QSplashScreen : public QWidget
{
    Q_OBJECT
public:
    explicit QSplashScreen(const QPixmap &pixmap = QPixmap(), Qt::WindowFlags f = 0);
    QSplashScreen(QWidget *parent, const QPixmap &pixmap = QPixmap(), Qt::WindowFlags f = 0);
    virtual ~QSplashScreen();

    void setPixmap(const QPixmap &pixmap);
    const QPixmap pixmap() const;
    void finish(QWidget *w);
    void repaint();

public Q_SLOTS:
    void showMessage(const QString &message, int alignment = Qt::AlignLeft,
                  const QColor &color = Qt::black);
    void clearMessage();
#ifdef QT3_SUPPORT
    inline QT_MOC_COMPAT void message(const QString &str, int alignment = Qt::AlignLeft,
        const QColor &color = Qt::black) { showMessage(str, alignment, color); }
    inline QT_MOC_COMPAT void clear() { clearMessage(); }
#endif

Q_SIGNALS:
    void messageChanged(const QString &message);

protected:
    bool event(QEvent *e);
    virtual void drawContents(QPainter *painter);
    void mousePressEvent(QMouseEvent *);

private:
    Q_DISABLE_COPY(QSplashScreen)
    Q_DECLARE_PRIVATE(QSplashScreen)
};

#endif // QT_NO_SPLASHSCREEN

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSPLASHSCREEN_H
