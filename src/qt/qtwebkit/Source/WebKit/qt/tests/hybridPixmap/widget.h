/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef widget_h
#define widget_h

#include <QImage>
#include <QPixmap>
#include <QWidget>
#include "qwebview.h"

typedef QWebView WebView;

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap)
    Q_PROPERTY(QImage image READ image WRITE setImage)

public:
    Widget(QWidget* parent = 0);
    ~Widget();
    void setPixmap(const QPixmap&);
    QPixmap pixmap() const;
    void setImage(const QImage&);
    QImage image() const;

private Q_SLOTS:
    void refreshJS();

public Q_SLOTS:
    void completeTest();
    void start();
    void compare(const QVariant& a, const QVariant& b);
    void imageSlot(const QImage&);
    void pixmapSlot(const QPixmap&);
    void randomSlot(const QPixmap&);
    QImage abcImage(int format);

Q_SIGNALS:
    void testComplete();
    void imageSignal(const QImage&);
    void pixmapSignal(const QPixmap&);

protected:
    void changeEvent(QEvent* e);

private:
    Ui::Widget* ui;
    QImage abcFilledImage;
};

#endif // widget_h
