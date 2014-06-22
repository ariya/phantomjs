/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

//! [0]
QGraphicsScene scene;
scene.addText("Hello, world!");

QGraphicsView view(&scene);
view.show();
//! [0]


//! [1]
QGraphicsScene scene;
scene.addItem(...
...
QPrinter printer(QPrinter::HighResolution);
printer.setPaperSize(QPrinter::A4);

QPainter painter(&printer);
scene.render(&painter);
//! [1]


//! [2]
QSizeF segmentSize = sceneRect().size() / pow(2, depth - 1);
//! [2]


//! [3]
QGraphicsScene scene;
QGraphicsView view(&scene);
view.show();

// a blue background
scene.setBackgroundBrush(Qt::blue);

// a gradient background
QRadialGradient gradient(0, 0, 10);
gradient.setSpread(QGradient::RepeatSpread);
scene.setBackgroundBrush(gradient);
//! [3]


//! [4]
QGraphicsScene scene;
QGraphicsView view(&scene);
view.show();

// a white semi-transparent foreground
scene.setForegroundBrush(QColor(255, 255, 255, 127));

// a grid foreground
scene.setForegroundBrush(QBrush(Qt::lightGray, Qt::CrossPattern));
//! [4]


//! [5]
QRectF TileScene::rectForTile(int x, int y) const
{
    // Return the rectangle for the tile at position (x, y).
    return QRectF(x * tileWidth, y * tileHeight, tileWidth, tileHeight);
}

void TileScene::setTile(int x, int y, const QPixmap &pixmap)
{
    // Sets or replaces the tile at position (x, y) with pixmap.
    if (x >= 0 && x < numTilesH && y >= 0 && y < numTilesV) {
        tiles[y][x] = pixmap;
        invalidate(rectForTile(x, y), BackgroundLayer);
    }
}

void TileScene::drawBackground(QPainter *painter, const QRectF &exposed)
{
    // Draws all tiles that intersect the exposed area.
    for (int y = 0; y < numTilesV; ++y) {
        for (int x = 0; x < numTilesH; ++x) {
            QRectF rect = rectForTile(x, y);
            if (exposed.intersects(rect))
                painter->drawPixmap(rect.topLeft(), tiles[y][x]);
        }
    }
}
//! [5]
