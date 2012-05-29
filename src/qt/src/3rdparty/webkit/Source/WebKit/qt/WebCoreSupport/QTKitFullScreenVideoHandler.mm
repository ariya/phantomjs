/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

#include "QTKitFullScreenVideoHandler.h"

#import <Cocoa/Cocoa.h>

#include "HTMLVideoElement.h"
#include "WebVideoFullscreenController.h"

using namespace WebCore;

class QTKitFullScreenVideoHandler::QTKitFullScreenVideoHandlerPrivate {
public :
    WebVideoFullscreenController* m_FullScreenController;
};

QTKitFullScreenVideoHandler::QTKitFullScreenVideoHandler()
    : privateData (adoptPtr(new QTKitFullScreenVideoHandlerPrivate))
{
    privateData->m_FullScreenController = nil;
}

QTKitFullScreenVideoHandler::~QTKitFullScreenVideoHandler()
{
    exitFullScreen();
}

void QTKitFullScreenVideoHandler::enterFullScreen(HTMLVideoElement* videoElement)
{
    if (privateData->m_FullScreenController) {
        // First exit fullscreen for the old mediaElement.
        exitFullScreen();
        ASSERT(!privateData->m_FullScreenController);
    }
    if (!privateData->m_FullScreenController) {
        privateData->m_FullScreenController = [[WebVideoFullscreenController alloc] init];
        [privateData->m_FullScreenController setMediaElement:videoElement];
        NSScreen* currentScreen = [NSScreen mainScreen];
        [privateData->m_FullScreenController enterFullscreen:currentScreen];
    }
}

void QTKitFullScreenVideoHandler::exitFullScreen()
{
    [privateData->m_FullScreenController exitFullscreen];
    [privateData->m_FullScreenController release];
    privateData->m_FullScreenController = nil;
}
