/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <QApplication>
#include <QPainter>
#include <QWebFrame>
#include <QWebPage>

//! [0]
class Thumbnailer : public QObject
{
    Q_OBJECT

public:
    Thumbnailer(const QUrl &url);

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void render();

private:
    QWebPage page;

};
//! [0]

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Thumbnailer thumbnail(QUrl("http://qt.nokia.com"));

    QObject::connect(&thumbnail, SIGNAL(finished()),
        &app, SLOT(quit()));

    return app.exec();
}

//! [1]
Thumbnailer::Thumbnailer(const QUrl &url)
{
    page.mainFrame()->load(url);
    connect(&page, SIGNAL(loadFinished(bool)),
        this, SLOT(render()));
}
//! [1]

//! [2]
void Thumbnailer::render()
{
    page.setViewportSize(page.mainFrame()->contentsSize());
    QImage image(page.viewportSize(), QImage::Format_ARGB32);
    QPainter painter(&image);

    page.mainFrame()->render(&painter);
    painter.end();

    QImage thumbnail = image.scaled(400, 400);
    thumbnail.save("thumbnail.png");

    emit finished();
}
//! [2]
#include "main.moc"
