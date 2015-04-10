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

#include "qxlibeglintegration_p.h"

VisualID QXlibEglIntegration::getCompatibleVisualId(Display *display, EGLDisplay eglDisplay, EGLConfig config)
{
    VisualID    visualId = 0;
    EGLint      eglValue = 0;

    EGLint configRedSize = 0;
    eglGetConfigAttrib(eglDisplay, config, EGL_RED_SIZE, &configRedSize);

    EGLint configGreenSize = 0;
    eglGetConfigAttrib(eglDisplay, config, EGL_GREEN_SIZE, &configGreenSize);

    EGLint configBlueSize = 0;
    eglGetConfigAttrib(eglDisplay, config, EGL_BLUE_SIZE, &configBlueSize);

    EGLint configAlphaSize = 0;
    eglGetConfigAttrib(eglDisplay, config, EGL_ALPHA_SIZE, &configAlphaSize);

    eglGetConfigAttrib(eglDisplay, config, EGL_CONFIG_ID, &eglValue);
    int configId = eglValue;

    // See if EGL provided a valid VisualID:
    eglGetConfigAttrib(eglDisplay, config, EGL_NATIVE_VISUAL_ID, &eglValue);
    visualId = (VisualID)eglValue;
    if (visualId) {
        // EGL has suggested a visual id, so get the rest of the visual info for that id:
        XVisualInfo visualInfoTemplate;
        memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
        visualInfoTemplate.visualid = visualId;

        XVisualInfo *chosenVisualInfo;
        int matchingCount = 0;
        chosenVisualInfo = XGetVisualInfo(display, VisualIDMask, &visualInfoTemplate, &matchingCount);
        if (chosenVisualInfo) {
            // Skip size checks if implementation supports non-matching visual
            // and config (http://bugreports.qt-project.org/browse/QTBUG-9444).
            if (q_hasEglExtension(eglDisplay,"EGL_NV_post_convert_rounding")) {
                XFree(chosenVisualInfo);
                return visualId;
            }

            int visualRedSize = qPopulationCount(chosenVisualInfo->red_mask);
            int visualGreenSize = qPopulationCount(chosenVisualInfo->green_mask);
            int visualBlueSize = qPopulationCount(chosenVisualInfo->blue_mask);
            int visualAlphaSize = -1; // Need XRender to tell us the alpha channel size

            bool visualMatchesConfig = false;
            if ( visualRedSize == configRedSize &&
                 visualGreenSize == configGreenSize &&
                 visualBlueSize == configBlueSize )
            {
                // We need XRender to check the alpha channel size of the visual. If we don't have
                // the alpha size, we don't check it against the EGL config's alpha size.
                if (visualAlphaSize >= 0)
                    visualMatchesConfig = visualAlphaSize == configAlphaSize;
                else
                    visualMatchesConfig = true;
            }

            if (!visualMatchesConfig) {
                if (visualAlphaSize >= 0) {
                    qWarning("Warning: EGL suggested using X Visual ID %d (ARGB%d%d%d%d) for EGL config %d (ARGB%d%d%d%d), but this is incompatable",
                             (int)visualId, visualAlphaSize, visualRedSize, visualGreenSize, visualBlueSize,
                             configId, configAlphaSize, configRedSize, configGreenSize, configBlueSize);
                } else {
                    qWarning("Warning: EGL suggested using X Visual ID %d (RGB%d%d%d) for EGL config %d (RGB%d%d%d), but this is incompatable",
                             (int)visualId, visualRedSize, visualGreenSize, visualBlueSize,
                             configId, configRedSize, configGreenSize, configBlueSize);
                }
                visualId = 0;
            }
        } else {
            qWarning("Warning: EGL suggested using X Visual ID %d for EGL config %d, but that isn't a valid ID",
                     (int)visualId, configId);
            visualId = 0;
        }
        XFree(chosenVisualInfo);
    }
#ifdef QT_DEBUG_X11_VISUAL_SELECTION
    else
        qDebug("EGL did not suggest a VisualID (EGL_NATIVE_VISUAL_ID was zero) for EGLConfig %d", configId);
#endif

    if (visualId) {
#ifdef QT_DEBUG_X11_VISUAL_SELECTION
        if (configAlphaSize > 0)
            qDebug("Using ARGB Visual ID %d provided by EGL for config %d", (int)visualId, configId);
        else
            qDebug("Using Opaque Visual ID %d provided by EGL for config %d", (int)visualId, configId);
#endif
        return visualId;
    }

    // Finally, try to
    // use XGetVisualInfo and only use the bit depths to match on:
    if (!visualId) {
        XVisualInfo visualInfoTemplate;
        memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
        XVisualInfo *matchingVisuals;
        int matchingCount = 0;

        visualInfoTemplate.depth = configRedSize + configGreenSize + configBlueSize + configAlphaSize;
        matchingVisuals = XGetVisualInfo(display,
                                         VisualDepthMask,
                                         &visualInfoTemplate,
                                         &matchingCount);
        if (!matchingVisuals) {
            // Try again without taking the alpha channel into account:
            visualInfoTemplate.depth = configRedSize + configGreenSize + configBlueSize;
            matchingVisuals = XGetVisualInfo(display,
                                             VisualDepthMask,
                                             &visualInfoTemplate,
                                             &matchingCount);
        }

        if (matchingVisuals) {
            visualId = matchingVisuals[0].visualid;
            XFree(matchingVisuals);
        }
    }

    if (visualId) {
#ifdef QT_DEBUG_X11_VISUAL_SELECTION
        qDebug("Using Visual ID %d provided by XGetVisualInfo for EGL config %d", (int)visualId, configId);
#endif
        return visualId;
    }

    qWarning("Unable to find an X11 visual which matches EGL config %d", configId);
    return (VisualID)0;
}
