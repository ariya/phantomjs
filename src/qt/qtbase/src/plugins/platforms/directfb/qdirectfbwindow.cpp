/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qdirectfbwindow.h"
#include "qdirectfbbackingstore.h"
#include "qdirectfbinput.h"
#include "qdirectfbscreen.h"

#include <qpa/qwindowsysteminterface.h>

#include <directfb.h>

QT_BEGIN_NAMESPACE

QDirectFbWindow::QDirectFbWindow(QWindow *tlw, QDirectFbInput *inputhandler)
    : QPlatformWindow(tlw), m_inputHandler(inputhandler)
{
}

void QDirectFbWindow::createDirectFBWindow()
{
    Q_ASSERT(!m_dfbWindow.data());

    DFBDisplayLayerConfig layerConfig;
    IDirectFBDisplayLayer *layer;

    layer = toDfbScreen(window())->dfbLayer();
    layer->GetConfiguration(layer, &layerConfig);

    DFBWindowDescription description;
    memset(&description,0,sizeof(DFBWindowDescription));

    if (window()->type() == Qt::Desktop) {
        QRect fullscreenRect(QPoint(), screen()->availableGeometry().size());
        window()->setGeometry(fullscreenRect);

        DFBResult result = layer->CreateWindow(layer, &description, m_dfbWindow.outPtr());
        if (result != DFB_OK)
            DirectFBError("QDirectFbWindow: failed to create window", result);

    } else {
        description.flags = DFBWindowDescriptionFlags(DWDESC_WIDTH|DWDESC_HEIGHT|DWDESC_POSX|DWDESC_POSY|DWDESC_SURFACE_CAPS
                                                      |DWDESC_OPTIONS
                                                      |DWDESC_CAPS);
        description.width = qMax(1, window()->width());
        description.height = qMax(1, window()->height());
        description.posx = window()->x();
        description.posy = window()->y();

        if (layerConfig.surface_caps & DSCAPS_PREMULTIPLIED)
            description.surface_caps = DSCAPS_PREMULTIPLIED;
        description.pixelformat = layerConfig.pixelformat;

        description.options = DFBWindowOptions(DWOP_ALPHACHANNEL);
        description.caps = DFBWindowCapabilities(DWCAPS_DOUBLEBUFFER|DWCAPS_ALPHACHANNEL);


        DFBResult result = layer->CreateWindow(layer, &description, m_dfbWindow.outPtr());
        if (result != DFB_OK)
            DirectFBError("QDirectFbWindow: failed to create window", result);

        m_dfbWindow->SetOpacity(m_dfbWindow.data(), 0xff);
        m_inputHandler->addWindow(m_dfbWindow.data(), window());
    }
}

QDirectFbWindow::~QDirectFbWindow()
{
    m_inputHandler->removeWindow(m_dfbWindow.data());
    m_dfbWindow->Destroy(m_dfbWindow.data());
}

void QDirectFbWindow::setGeometry(const QRect &rect)
{
    QPlatformWindow::setGeometry(rect);
    m_dfbWindow->SetBounds(m_dfbWindow.data(), rect.x(),rect.y(),
                           rect.width(), rect.height());
}

void QDirectFbWindow::setOpacity(qreal level)
{
    const quint8 windowOpacity = quint8(level * 0xff);
    m_dfbWindow->SetOpacity(m_dfbWindow.data(), windowOpacity);
}

void QDirectFbWindow::setVisible(bool visible)
{
    if (window()->type() != Qt::Desktop) {
        if (visible) {
            int x = geometry().x();
            int y = geometry().y();
            m_dfbWindow->MoveTo(m_dfbWindow.data(), x, y);
        } else {
            QDirectFBPointer<IDirectFBDisplayLayer> displayLayer;
            QDirectFbConvenience::dfbInterface()->GetDisplayLayer(QDirectFbConvenience::dfbInterface(), DLID_PRIMARY, displayLayer.outPtr());

            DFBDisplayLayerConfig config;
            displayLayer->GetConfiguration(displayLayer.data(), &config);
            m_dfbWindow->MoveTo(m_dfbWindow.data(), config. width + 1, config.height + 1);
        }

        if (window()->isTopLevel() && visible)
            QPlatformWindow::setVisible(visible);
    }
}

void QDirectFbWindow::setWindowFlags(Qt::WindowFlags flags)
{
    switch (flags & Qt::WindowType_Mask) {
    case Qt::ToolTip: {
        DFBWindowOptions options;
        m_dfbWindow->GetOptions(m_dfbWindow.data(), &options);
        options = DFBWindowOptions(options | DWOP_GHOST);
        m_dfbWindow->SetOptions(m_dfbWindow.data(), options);
        break; }
    default:
        break;
    }

    m_dfbWindow->SetStackingClass(m_dfbWindow.data(), flags & Qt::WindowStaysOnTopHint ? DWSC_UPPER : DWSC_MIDDLE);
}

void QDirectFbWindow::raise()
{
    if (window()->type() != Qt::Desktop)
        m_dfbWindow->RaiseToTop(m_dfbWindow.data());
}

void QDirectFbWindow::lower()
{
    if (window()->type() != Qt::Desktop)
        m_dfbWindow->LowerToBottom(m_dfbWindow.data());
}

WId QDirectFbWindow::winId() const
{
    DFBWindowID id;
    m_dfbWindow->GetID(m_dfbWindow.data(), &id);
    return WId(id);
}

bool QDirectFbWindow::setKeyboardGrabEnabled(bool grab)
{
    DFBResult res;

    if (grab)
        res = m_dfbWindow->GrabKeyboard(m_dfbWindow.data());
    else
        res = m_dfbWindow->UngrabKeyboard(m_dfbWindow.data());

    return res == DFB_OK;
}

bool QDirectFbWindow::setMouseGrabEnabled(bool grab)
{
    DFBResult res;

    if (grab)
        res = m_dfbWindow->GrabPointer(m_dfbWindow.data());
    else
        res = m_dfbWindow->UngrabPointer(m_dfbWindow.data());

    return res == DFB_OK;
}

IDirectFBWindow *QDirectFbWindow::dfbWindow() const
{
    return m_dfbWindow.data();
}

IDirectFBSurface *QDirectFbWindow::dfbSurface()
{
    if (!m_dfbSurface) {
        DFBResult res = m_dfbWindow->GetSurface(m_dfbWindow.data(), m_dfbSurface.outPtr());
        if (res != DFB_OK)
            DirectFBError(QDFB_PRETTY, res);
    }

    return m_dfbSurface.data();
}

QT_END_NAMESPACE
