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

static OSStatus applicationEventHandler(EventHandlerCallRef next_ref, EventRef event, void *data);
static EventHandlerUPP applicationEventHandlerUPP = 0;
static EventTypeSpec application_events[] = {
    { kEventClassAccessibility,  kEventAccessibleGetChildAtPoint },
    { kEventClassAccessibility,  kEventAccessibleGetNamedAttribute }
};

static CFStringRef kObjectQtAccessibility = CFSTR("com.trolltech.qt.accessibility");
static EventHandlerUPP objectCreateEventHandlerUPP = 0;
static EventTypeSpec objectCreateEvents[] = {
    { kEventClassHIObject,  kEventHIObjectConstruct },
    { kEventClassHIObject,  kEventHIObjectInitialize },
    { kEventClassHIObject,  kEventHIObjectDestruct },
    { kEventClassHIObject,  kEventHIObjectPrintDebugInfo }
};

static OSStatus accessibilityEventHandler(EventHandlerCallRef next_ref, EventRef event, void *data);
static EventHandlerUPP accessibilityEventHandlerUPP = 0;
static EventTypeSpec accessibilityEvents[] = {
    { kEventClassAccessibility,  kEventAccessibleGetChildAtPoint },
    { kEventClassAccessibility,  kEventAccessibleGetFocusedChild },
    { kEventClassAccessibility,  kEventAccessibleGetAllAttributeNames },
    { kEventClassAccessibility,  kEventAccessibleGetNamedAttribute },
    { kEventClassAccessibility,  kEventAccessibleSetNamedAttribute },
    { kEventClassAccessibility,  kEventAccessibleIsNamedAttributeSettable },
    { kEventClassAccessibility,  kEventAccessibleGetAllActionNames },
    { kEventClassAccessibility,  kEventAccessiblePerformNamedAction },
    { kEventClassAccessibility,  kEventAccessibleGetNamedActionDescription }
};

static void installAcessibilityEventHandler(HIObjectRef hiObject)
{
    if (!accessibilityEventHandlerUPP)
        accessibilityEventHandlerUPP = NewEventHandlerUPP(accessibilityEventHandler);

    InstallHIObjectEventHandler(hiObject, accessibilityEventHandlerUPP,
                                        GetEventTypeCount(accessibilityEvents),
                                        accessibilityEvents, 0, 0);
}

static OSStatus objectCreateEventHandler(EventHandlerCallRef next_ref, EventRef event, void *data)
{
    Q_UNUSED(data)
    Q_UNUSED(event)
    Q_UNUSED(next_ref)
    return noErr;
}

static void registerQtAccessibilityHIObjectSubclass()
{
    if (!objectCreateEventHandlerUPP)
        objectCreateEventHandlerUPP = NewEventHandlerUPP(objectCreateEventHandler);
    OSStatus err = HIObjectRegisterSubclass(kObjectQtAccessibility, 0, 0, objectCreateEventHandlerUPP,
                                         GetEventTypeCount(objectCreateEvents), objectCreateEvents, 0, 0);
    if (err && err != hiObjectClassExistsErr)
        qWarning("qaccessible_mac internal error: Could not register accessibility HIObject subclass");
}

static void installApplicationEventhandler()
{
    if (!applicationEventHandlerUPP)
        applicationEventHandlerUPP = NewEventHandlerUPP(applicationEventHandler);

    OSStatus err = InstallApplicationEventHandler(applicationEventHandlerUPP,
                            GetEventTypeCount(application_events), application_events,
                            0, 0);

    if (err && err != eventHandlerAlreadyInstalledErr)
        qWarning("qaccessible_mac internal error: Could not install application accessibility event handler");
}

static void removeEventhandler(EventHandlerUPP eventHandler)
{
    if (eventHandler) {
        DisposeEventHandlerUPP(eventHandler);
        eventHandler = 0;
    }
}
