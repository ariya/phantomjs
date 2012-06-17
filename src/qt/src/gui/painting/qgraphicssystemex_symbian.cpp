/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgraphicssystemex_symbian_p.h"
#include "private/qwidget_p.h"
#include "private/qbackingstore_p.h"
#include "private/qapplication_p.h"

#include <e32property.h>

#if defined(Q_SYMBIAN_SUPPORTS_SURFACES) && !defined (QT_NO_EGL)
#include "private/qegl_p.h"
#endif

QT_BEGIN_NAMESPACE

static bool bcm2727Initialized = false;
static bool bcm2727 = false;

#if defined(Q_SYMBIAN_SUPPORTS_SURFACES) && !defined (QT_NO_EGL)
typedef EGLBoolean (*NOK_resource_profiling)(EGLDisplay, EGLint, EGLint*, EGLint, EGLint*);
#define EGL_PROF_TOTAL_MEMORY_NOK 0x3070
#endif

// Detect if Qt is running on BCM2727 chip.
// BCM2727 is a special case on Symbian because
// it has only 32MB GPU memory which exposes
// significant limitations to hw accelerated UI.
bool QSymbianGraphicsSystemEx::hasBCM2727()
{
    if (bcm2727Initialized)
        return bcm2727;

#if defined(Q_SYMBIAN_SUPPORTS_SURFACES) && !defined (QT_NO_EGL)
    EGLDisplay display = QEgl::display();
#if 1
    // Hacky but fast ~0ms.
    const char* vendor = eglQueryString(display, EGL_VENDOR);
    if (vendor && strstr(vendor, "Broadcom")) {
        const TUid KIvePropertyCat = {0x2726beef};
        enum TIvePropertyChipType {
            EVCBCM2727B1 = 0x00000000,
            EVCBCM2763A0 = 0x04000100,
            EVCBCM2763B0 = 0x04000102,
            EVCBCM2763C0 = 0x04000103,
            EVCBCM2763C1 = 0x04000104,
            EVCBCMUnknown = 0x7fffffff
        };

        // Broadcom driver publishes KIvePropertyCat PS key on
        // devices which are running on BCM2727 chip and post Anna Symbian.
        TInt chipType = EVCBCMUnknown;
        if (RProperty::Get(KIvePropertyCat, 0, chipType) == KErrNone) {
            if (chipType == EVCBCM2727B1)
                bcm2727 = true;
        } else if (QSysInfo::symbianVersion() <= QSysInfo::SV_SF_3) {
            // Device is running on Symbian Anna or older Symbian^3 in which
            // KIvePropertyCat is not published. These ones are always 32MB devices.
            bcm2727 = true;
        } else {
            // We have some other Broadcom chip on post Anna Symbian.
            // Should have > 32MB GPU memory.
        }
    }
#else
    // Fool proof but takes 15-20ms and we don't want this delay on app startup...

    // All devices with <= 32MB GPU memory should be
    // dealed in similar manner to BCM2727
    // So let's query max GPU memory amount.
    NOK_resource_profiling eglQueryProfilingData = (NOK_resource_profiling)eglGetProcAddress("eglQueryProfilingDataNOK");
    if (eglQueryProfilingData) {
        EGLint dataCount;
        eglQueryProfilingData(display,
                               EGL_PROF_QUERY_GLOBAL_BIT_NOK |
                               EGL_PROF_QUERY_MEMORY_USAGE_BIT_NOK,
                               NULL,
                               0,
                               (EGLint*)&dataCount);

        // Allocate room for the profiling data
        EGLint* profData = (EGLint*)malloc(dataCount * sizeof(EGLint));
        memset(profData,0,dataCount * sizeof(EGLint));

        // Retrieve the profiling data
        eglQueryProfilingData(display,
                              EGL_PROF_QUERY_GLOBAL_BIT_NOK |
                              EGL_PROF_QUERY_MEMORY_USAGE_BIT_NOK,
                              profData,
                              dataCount,
                              (EGLint*)&dataCount);

        int totalMemory;
        EGLint i = 0;
        while (profData && i < dataCount) {
            switch (profData[i++]) {
                case EGL_PROF_TOTAL_MEMORY_NOK:
                    totalMemory = profData[i++];
                    break;
                default:
                    i++;
            }
        }

        // ok, hasBCM2727() naming is a bit misleading but Qt must
        // behave the same on all chips like BCM2727 (<= 32MB GPU memory)
        // and our code (and others) are already using this function.
        if (totalMemory <= 33554432)
            bcm2727 = true;
    }
#endif
#endif // Q_SYMBIAN_SUPPORTS_SURFACES

    bcm2727Initialized = true;

    return bcm2727;
}

void QSymbianGraphicsSystemEx::releaseCachedGpuResources()
{
    // Do nothing here

    // This virtual function should be implemented in graphics system specific
    // plugin
}

void QSymbianGraphicsSystemEx::releaseAllGpuResources()
{
    releaseCachedGpuResources();

    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        if (QTLWExtra *topExtra = qt_widget_private(widget)->maybeTopData())
            topExtra->backingStore.destroy();
    }
}

void QSymbianGraphicsSystemEx::forceToRaster(QWidget *window)
{
    if (window && window->isWindow()) {
        qt_widget_private(window)->createTLExtra();
        if (QTLWExtra *topExtra = qt_widget_private(window)->maybeTopData()) {
            topExtra->forcedToRaster = 1;
            if (topExtra->backingStore.data()) {
                topExtra->backingStore.create(window);
                topExtra->backingStore.registerWidget(window);
            }
        }
    }
}

QT_END_NAMESPACE
