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

/*
viewer.cpp

Provides a main window for displaying a user-specified original image
with three color separations in a grid layout.

A main menu provides entries for selecting files, and adjusting the
brightness of the separations.
*/

#include <QtGui>

#include "finalwidget.h"
#include "screenwidget.h"
#include "viewer.h"

/*
    Constructor: initializes a default value for the brightness, creates
    the main menu entries, and constructs a central widget that contains
    enough space for images to be displayed.
*/

Viewer::Viewer()
{
    setWindowTitle(tr("QImage Color Separations"));

    brightness = 255;

    createMenus();
    setCentralWidget(createCentralWidget());
}

/*
    Creates a main menu with two entries: a File menu, to allow the image
    to be selected, and a Brightness menu to allow the brightness of the
    separations to be changed.

    Initially, the Brightness menu items are disabled, but the first entry in
    the menu is checked to reflect the default brightness.
*/

void Viewer::createMenus()
{
    fileMenu = new QMenu(tr("&File"), this);
    brightnessMenu = new QMenu(tr("&Brightness"), this);

    QAction *openAction = fileMenu->addAction(tr("&Open..."));
    openAction->setShortcut(QKeySequence("Ctrl+O"));
    saveAction = fileMenu->addAction(tr("&Save..."));
    saveAction->setShortcut(QKeySequence("Ctrl+S"));
    saveAction->setEnabled(false);
    QAction *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));

    QAction *noBrightness = brightnessMenu->addAction(tr("&0%"));
    noBrightness->setCheckable(true);
    QAction *quarterBrightness = brightnessMenu->addAction(tr("&25%"));
    quarterBrightness->setCheckable(true);
    QAction *halfBrightness = brightnessMenu->addAction(tr("&50%"));
    halfBrightness->setCheckable(true);
    QAction *threeQuartersBrightness = brightnessMenu->addAction(tr("&75%"));
    threeQuartersBrightness->setCheckable(true);
    QAction *fullBrightness = brightnessMenu->addAction(tr("&100%"));
    fullBrightness->setCheckable(true);

    menuMap[noBrightness] = None;
    menuMap[quarterBrightness] = Quarter;
    menuMap[halfBrightness] = Half;
    menuMap[threeQuartersBrightness] = ThreeQuarters;
    menuMap[fullBrightness] = Full;

    currentBrightness = fullBrightness;
    currentBrightness->setChecked(true);
    brightnessMenu->setEnabled(false);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(brightnessMenu);

    connect(openAction, SIGNAL(triggered()), this, SLOT(chooseFile()));
    connect(saveAction, SIGNAL(triggered()), this, SLOT(saveImage()));
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(brightnessMenu, SIGNAL(triggered(QAction*)), this,
            SLOT(setBrightness(QAction*)));
}

/*
    Constructs a central widget for the window consisting of a two-by-two
    grid of labels, each of which will contain an image. We restrict the
    size of the labels to 256 pixels, and ensure that the window cannot
    be resized.
*/

QFrame* Viewer::createCentralWidget()
{
    QFrame* frame = new QFrame(this);
    grid = new QGridLayout(frame);
    grid->setSpacing(8);
    grid->setMargin(4);

    layout()->setSizeConstraint(QLayout::SetFixedSize);

    QSize labelSize(256, 256);

    finalWidget = new FinalWidget(frame, tr("Final image"), labelSize);

    cyanWidget = new ScreenWidget(frame, Qt::cyan, tr("Cyan"),
                                  ScreenWidget::Cyan, labelSize);
    magentaWidget = new ScreenWidget(frame, Qt::magenta, tr("Magenta"),
                                     ScreenWidget::Magenta, labelSize);
    yellowWidget = new ScreenWidget(frame, Qt::yellow, tr("Yellow"),
                                    ScreenWidget::Yellow, labelSize);

    connect(cyanWidget, SIGNAL(imageChanged()), this, SLOT(createImage()));
    connect(magentaWidget, SIGNAL(imageChanged()), this, SLOT(createImage()));
    connect(yellowWidget, SIGNAL(imageChanged()), this, SLOT(createImage()));

    grid->addWidget(finalWidget, 0, 0, Qt::AlignTop | Qt::AlignHCenter);
    grid->addWidget(cyanWidget, 0, 1, Qt::AlignTop | Qt::AlignHCenter);
    grid->addWidget(magentaWidget, 1, 0, Qt::AlignTop | Qt::AlignHCenter);
    grid->addWidget(yellowWidget, 1, 1, Qt::AlignTop | Qt::AlignHCenter);

    return frame;
}

/*
    Provides a dialog window to allow the user to specify an image file.
    If a file is selected, the appropriate function is called to process
    and display it.
*/

void Viewer::chooseFile()
{
    QString imageFile = QFileDialog::getOpenFileName(this,
        tr("Choose an image file to open"), path, tr("Images (*.*)"));

    if (!imageFile.isEmpty()) {
        openImageFile(imageFile);
        path = imageFile;
    }
}

/*
    Changes the value of the brightness according to the entry selected in the
    Brightness menu. The selected entry is checked, and the previously selected
    entry is unchecked.

    The color separations are updated to use the new value for the brightness.
*/

void Viewer::setBrightness(QAction *action)
{
    if (!menuMap.contains(action) || scaledImage.isNull())
        return;

    Brightness amount = menuMap[action];

    switch (amount) {
        case None:
            brightness = 0; break;
        case Quarter:
            brightness = 64; break;
        case Half:
            brightness = 128; break;
        case ThreeQuarters:
            brightness = 191; break;
        case Full:
            brightness = 255; break;
        default: return;
    }

    currentBrightness->setChecked(false);
    currentBrightness = action;
    currentBrightness->setChecked(true);

    createImage();
}

/*
    Load the image from the file given, and create four pixmaps based
    on the original image.

    The window caption is set, and the Brightness menu enabled if the image file
    can be loaded.
*/

void Viewer::openImageFile(QString &imageFile)
{
    QImage originalImage;

    if (originalImage.load(imageFile)) {
        setWindowTitle(imageFile);
        //menuBar()->setItemEnabled(brightnessMenuId, true);
        saveAction->setEnabled(true);
        brightnessMenu->setEnabled(true);

        /* Note: the ScaleMin value may be different for Qt 4. */
        scaledImage = originalImage.scaled(256, 256, Qt::KeepAspectRatio);

        cyanWidget->setImage(scaledImage);
        magentaWidget->setImage(scaledImage);
        yellowWidget->setImage(scaledImage);
        createImage();
    }
    else
        (void) QMessageBox::warning(this, tr("Cannot open file"),
            tr("The selected file could not be opened."),
            QMessageBox::Cancel, QMessageBox::NoButton, QMessageBox::NoButton);
}

/*
    Creates an image by combining the contents of the three screens
    to present a page preview.

    The image associated with each screen is separated into cyan,
    magenta, and yellow components. We add up the values for each
    component from the three screen images, and subtract the totals
    from the maximum value for each corresponding primary color.
*/

void Viewer::createImage()
{
    QImage newImage = scaledImage.copy();

    QImage *image1 = cyanWidget->image();
    QImage *image2 = magentaWidget->image();
    QImage *image3 = yellowWidget->image();
    int darkness = 255 - brightness;

    for (int y = 0; y < newImage.height(); ++y) {
        for (int x = 0; x < newImage.width(); ++x) {

            // Create three screens, using the quantities of the source
            // CMY components to determine how much of each of the
            // inks are to be put on each screen.
            QRgb p1(image1->pixel(x, y));
            float cyan1 = 255 - qRed(p1);
            float magenta1 = 255 - qGreen(p1);
            float yellow1 = 255 - qBlue(p1);

            QRgb p2(image2->pixel(x, y));
            float cyan2 = 255 - qRed(p2);
            float magenta2 = 255 - qGreen(p2);
            float yellow2 = 255 - qBlue(p2);

            QRgb p3(image3->pixel(x, y));
            float cyan3 = 255 - qRed(p3);
            float magenta3 = 255 - qGreen(p3);
            float yellow3 = 255 - qBlue(p3);

            QColor newColor(
                qMax(255 - int(cyan1+cyan2+cyan3) - darkness, 0),
                qMax(255 - int(magenta1+magenta2+magenta3) - darkness, 0),
                qMax(255 - int(yellow1+yellow2+yellow3) - darkness, 0));

            newImage.setPixel(x, y, newColor.rgb());
        }
    }

    finalWidget->setPixmap(QPixmap::fromImage(newImage));
}

/*
    Provides a dialog window to allow the user to save the image file.
*/

void Viewer::saveImage()
{
    QString imageFile = QFileDialog::getSaveFileName(this,
        tr("Choose a filename to save the image"), "", tr("Images (*.png)"));

    QFileInfo info(imageFile);

    if (!info.baseName().isEmpty()) {
        QString newImageFile = QFileInfo(info.absoluteDir(),
            info.baseName() + ".png").absoluteFilePath();

        if (!finalWidget->pixmap()->save(newImageFile, "PNG"))
            (void) QMessageBox::warning(this, tr("Cannot save file"),
                tr("The file could not be saved."),
                QMessageBox::Cancel, QMessageBox::NoButton,
                QMessageBox::NoButton);
    }
    else
        (void) QMessageBox::warning(this, tr("Cannot save file"),
            tr("Please enter a valid filename."),
            QMessageBox::Cancel, QMessageBox::NoButton,
            QMessageBox::NoButton);
}
