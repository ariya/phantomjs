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

#include "qopenwfddevice.h"

#include "qopenwfdport.h"
#include "qopenwfdscreen.h"

#include <QtCore/QDebug>

#include <WF/wfdext.h>
#include <gbm.h>

QOpenWFDDevice::QOpenWFDDevice(QOpenWFDIntegration *integration, WFDint device_enumeration)
    : mIntegration(integration)
    , mDeviceEnum(device_enumeration)
    , mCommitedDevice(false)
    , mWaitingForBindSourceEvent(false)
{
    mDevice = wfdCreateDevice(WFD_DEFAULT_DEVICE_ID,WFD_NONE);
    if (mDevice == WFD_INVALID_HANDLE)
        qDebug() << "failed to create device";

    mEvent = wfdCreateEvent(mDevice,0);
    if (mEvent == WFD_INVALID_HANDLE)
        qDebug() << "failed to create event handle";

    //initialize pipelines for device.
    wfdEnumeratePipelines(mDevice,WFD_NONE,0,WFD_NONE);

    initializeGbmAndEgl();

    WFDint numberOfPorts = wfdEnumeratePorts(mDevice,0,0,0);
    WFDint port_enumerations[numberOfPorts];
    WFDint actualNumberOfPorts = wfdEnumeratePorts(mDevice,port_enumerations,numberOfPorts,WFD_NONE);
    Q_ASSERT(actualNumberOfPorts == numberOfPorts);

    for (int i = 0; i < actualNumberOfPorts; i++)
    {
        QOpenWFDPort *port = new QOpenWFDPort(this,port_enumerations[i]);
        if (port->attached()) {
            mPorts.append(port);
        } else {
            delete port;
        }
    }

    int fd = wfdDeviceEventGetFD(mDevice,mEvent);
    mEventSocketNotifier = new QSocketNotifier(fd,QSocketNotifier::Read,this);
    connect(mEventSocketNotifier,SIGNAL(activated(int)),SLOT(readEvents()));

    mCommitedDevice = true;
    commit(WFD_COMMIT_ENTIRE_DEVICE, handle());
}

QOpenWFDDevice::~QOpenWFDDevice()
{
    delete mEventSocketNotifier;
    wfdDestroyEvent(mDevice,mEvent);

    for (int i = 0; i < mPorts.size(); i++) {
        //probably don't need to remove them from the list
        QList <WFDint> keys = mUsedPipelines.keys(mPorts.at(i));
        for (int keyIndex = 0; keyIndex < keys.size(); keyIndex++) {
            mUsedPipelines.remove(keys.at(keyIndex));
        }
        //but we have to delete them :)
        delete mPorts[i];
    }

    eglDestroyContext(mEglDisplay,mEglContext);
    eglTerminate(mEglDisplay);

    gbm_device_destroy(mGbmDevice);

    wfdDestroyDevice(mDevice);
}

WFDDevice QOpenWFDDevice::handle() const
{
    return mDevice;
}

QOpenWFDIntegration * QOpenWFDDevice::integration() const
{
    return mIntegration;
}

bool QOpenWFDDevice::isPipelineUsed(WFDint pipelineId)
{
    return mUsedPipelines.contains(pipelineId);
}

void QOpenWFDDevice::addToUsedPipelineSet(WFDint pipelineId,QOpenWFDPort *port)
{
    mUsedPipelines.insert(pipelineId,port);
}

void QOpenWFDDevice::removeFromUsedPipelineSet(WFDint pipelineId)
{
    mUsedPipelines.remove(pipelineId);
}

gbm_device * QOpenWFDDevice::gbmDevice() const

{
    return mGbmDevice;
}

EGLDisplay QOpenWFDDevice::eglDisplay() const
{
    return mEglDisplay;
}

EGLContext QOpenWFDDevice::eglContext() const
{
    return mEglContext;
}

void QOpenWFDDevice::commit(WFDCommitType type, WFDHandle handle)
{
    if (mCommitedDevice) {
        wfdDeviceCommit(mDevice,type,handle);
    }
}

void QOpenWFDDevice::waitForPipelineBindSourceCompleteEvent()
{
    mWaitingForBindSourceEvent = true;

    while (mWaitingForBindSourceEvent) {
        readEvents(WFD_FOREVER);
    }
}

void QOpenWFDDevice::readEvents(WFDtime wait)
{
    WFDEventType type = wfdDeviceEventWait(mDevice,mEvent,wait);

    if (type == WFD_EVENT_NONE || type == WFD_EVENT_DESTROYED) {
        return;
    }
    switch (type) {
    case WFD_EVENT_INVALID:
    case WFD_EVENT_NONE:
        return;
    case WFD_EVENT_DESTROYED:
        qDebug() << "Event or Device destoryed!";
        return;
    case WFD_EVENT_PORT_ATTACH_DETACH:
        handlePortAttachDetach();
        break;
    case WFD_EVENT_PORT_PROTECTION_FAILURE:
        qDebug() << "Port protection event handling not implemented";
        break;
    case WFD_EVENT_PIPELINE_BIND_SOURCE_COMPLETE:
        handlePipelineBindSourceComplete();
        break;
    case WFD_EVENT_PIPELINE_BIND_MASK_COMPLETE:
        qDebug() << "Pipeline bind mask event handling not implemented";
        break;
    default:
        qDebug() << "Not recognised event type";
        break;
    }


}

void QOpenWFDDevice::initializeGbmAndEgl()
{

    qDebug() << "initializing GBM and EGL";
    int fd = wfdGetDeviceAttribi(mDevice,WFD_DEVICE_ID);
    if (fd < 0) {
        qDebug() << "failed to get WFD_DEVICE_ID";
    }

    mGbmDevice = gbm_create_device(fd);

    setenv("EGL_PLATFORM", "drm",1);

    mEglDisplay = eglGetDisplay(mGbmDevice);

    EGLint minor, major;

    if (!eglInitialize(mEglDisplay,&major,&minor)) {
        qDebug() << "failed to initialize egl";
    }

    QByteArray eglExtensions = eglQueryString(mEglDisplay, EGL_EXTENSIONS);
    if (!eglExtensions.contains("EGL_KHR_surfaceless_opengl")) {
        qDebug() << "This egl implementation does not have the required EGL extension EGL_KHR_surfaceless_opengl";
    }

    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    mEglContext = eglCreateContext(mEglDisplay,NULL,EGL_NO_CONTEXT,contextAttribs);
    if (mEglContext == EGL_NO_CONTEXT) {
        qDebug() << "Failed to create EGL context";
    }

    eglCreateImage = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
    if (!eglCreateImage) {
        qWarning("failed to load extension eglCreateImageKHR");
    }

    eglDestroyImage = (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");
    if (!eglDestroyImage) {
        qWarning("failed to load extension eglDestoryImageKHR");
    }

    glEglImageTargetRenderBufferStorage = (PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC) eglGetProcAddress("glEGLImageTargetRenderbufferStorageOES");
    if (!glEglImageTargetRenderBufferStorage) {
        qWarning("failed to load extension glEGLImageTargetRenderbufferStorageOES");
    }
}

void QOpenWFDDevice::handlePortAttachDetach()
{
    WFDint id = wfdGetEventAttribi(mDevice,mEvent,WFD_EVENT_PORT_ATTACH_PORT_ID);
    if (id == WFD_INVALID_PORT_ID)
        return;

    WFDint attachState = wfdGetEventAttribi(mDevice,mEvent,WFD_EVENT_PORT_ATTACH_STATE);
    if (attachState == WFD_TRUE) {
        int indexToAdd = -1;
        for (int i = 0; i < mPorts.size(); i++) {
            if (mPorts.at(i)->portId() == id) {
                indexToAdd = i;
                qDebug() << "found index to attach";
                break;
            }
        }
        if (indexToAdd >= 0) {
            mPorts[indexToAdd]->attach();
        } else {
            mPorts.append(new QOpenWFDPort(this,id));
        }

    } else {
        int indexToDelete = -1;
        for (int i = 0; i < mPorts.size(); i++) {
            if (mPorts.at(i)->portId() == id) {
                indexToDelete = i;
                break;
            }
        }
        if (indexToDelete >= 0) {
            QOpenWFDPort *portToDelete = mPorts.at(indexToDelete);
            mPorts.removeAt(indexToDelete);
            delete portToDelete;
        }
    }
}

void QOpenWFDDevice::handlePipelineBindSourceComplete()
{
    mWaitingForBindSourceEvent = false;

    WFDint overflow = wfdGetEventAttribi(mDevice,mEvent, WFD_EVENT_PIPELINE_BIND_QUEUE_OVERFLOW);
    if (overflow == WFD_TRUE) {
        qDebug() << "PIPELINE_BIND_QUEUE_OVERFLOW event occurred";
    }

    WFDint pipelineId = wfdGetEventAttribi(mDevice,mEvent,WFD_EVENT_PIPELINE_BIND_PIPELINE_ID);
    for (int i = 0; i < mPorts.size(); i++) {
        if (pipelineId != WFD_INVALID_PIPELINE_ID && mUsedPipelines.contains(pipelineId)) {
            QOpenWFDPort *port = mUsedPipelines.value(pipelineId);
            port->screen()->pipelineBindSourceComplete();
            break;
        }
    }
}
