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

#include <private/qmainwindowlayout_p.h>
#include <qtoolbar.h>
#include <private/qtoolbarlayout_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#include <private/qtoolbar_p.h>

#ifndef QT_MAC_USE_COCOA
#include <Carbon/Carbon.h>
#else
#include <private/qcocoatoolbardelegate_mac_p.h>
#import  <private/qcocoawindowdelegate_mac_p.h>
#endif

QT_BEGIN_NAMESPACE
#ifdef QT_NAMESPACE

// namespace up the stuff
#define SS(x) #x
#define S0(x) SS(x)
#define S "com.trolltech.qt-" S0(QT_NAMESPACE) ".qmainwindow.qtoolbarInHIToolbar"
#define SToolbar "com.trolltech.qt-" S0(QT_NAMESPACE) ".hitoolbar-qtoolbar"
#define SNSToolbar "com.trolltech.qt-" S0(QT_NAMESPACE) ".qtoolbarInNSToolbar"
#define MacToolbar "com.trolltech.qt-" S0(QT_NAMESPACE) ".qmainwindow.mactoolbar"

#ifndef QT_MAC_USE_COCOA
static CFStringRef kQToolBarHIToolbarItemClassID = CFSTR(S);
static CFStringRef kQToolBarHIToolbarIdentifier = CFSTR(SToolbar);
#else
static NSString *kQToolBarNSToolbarIdentifier = @SNSToolbar;
#endif
static CFStringRef kQMainWindowMacToolbarID = CFSTR(MacToolbar);
#undef SS
#undef S0
#undef S
#undef SToolbar
#undef SNSToolbar
#undef MacToolbar

#else
#ifndef QT_MAC_USE_COCOA
static CFStringRef kQToolBarHIToolbarItemClassID = CFSTR("com.trolltech.qt.qmainwindow.qtoolbarInHIToolbar");
static CFStringRef kQToolBarHIToolbarIdentifier = CFSTR("com.trolltech.qt.hitoolbar-qtoolbar");
#else
static NSString *kQToolBarNSToolbarIdentifier = @"com.trolltech.qt.qmainwindow.qtoolbarInNSToolbar";
#endif
static CFStringRef kQMainWindowMacToolbarID = CFSTR("com.trolltech.qt.qmainwindow.mactoolbar");
#endif // QT_NAMESPACE

#ifndef QT_MAC_USE_COCOA

static const int kEventParamQToolBar = 'QTBR';
static const int kEventParamQMainWindowLayout = 'QMWL';

const EventTypeSpec qtoolbarEvents[] =
{
    { kEventClassHIObject, kEventHIObjectConstruct },
    { kEventClassHIObject, kEventHIObjectDestruct },
    { kEventClassHIObject, kEventHIObjectInitialize },
    { kEventClassToolbarItem, kEventToolbarItemCreateCustomView }
};

struct QToolBarInHIToolbarInfo
{
    QToolBarInHIToolbarInfo(HIToolbarItemRef item)
        : toolbarItem(item), mainWindowLayout(0)
    {}
    HIToolbarItemRef toolbarItem;
    QMainWindowLayout *mainWindowLayout;
};

OSStatus QMainWindowLayout::qtoolbarInHIToolbarHandler(EventHandlerCallRef inCallRef,
                                                       EventRef event, void *data)
{
    OSStatus result = eventNotHandledErr;
    QToolBarInHIToolbarInfo *object = static_cast<QToolBarInHIToolbarInfo *>(data);

    switch (GetEventClass(event)) {
        case kEventClassHIObject:
            switch (GetEventKind(event)) {
                case kEventHIObjectConstruct:
                {
                    HIObjectRef toolbarItem;
                    GetEventParameter(event, kEventParamHIObjectInstance, typeHIObjectRef,
                                      0, sizeof( HIObjectRef ), 0, &toolbarItem);

                    QToolBarInHIToolbarInfo *item = new QToolBarInHIToolbarInfo(toolbarItem);
                    SetEventParameter(event, kEventParamHIObjectInstance, typeVoidPtr,
                                      sizeof(void *), &item);
                    result = noErr;
                }
                    break;
                case kEventHIObjectInitialize:
                    result = CallNextEventHandler(inCallRef, event);
                    if (result == noErr) {
                        QToolBar *toolbar = 0;
                        QMainWindowLayout *layout = 0;
                        GetEventParameter(event, kEventParamQToolBar, typeVoidPtr,
                                          0, sizeof(void *), 0, &toolbar);
                        GetEventParameter(event, kEventParamQMainWindowLayout, typeVoidPtr,
                                          0, sizeof(void *), 0, &layout);
                        object->mainWindowLayout = layout;
                        object->mainWindowLayout->unifiedToolbarHash.insert(object->toolbarItem, toolbar);
                        HIToolbarItemChangeAttributes(object->toolbarItem,
                                                      kHIToolbarItemLabelDisabled, 0);
                    }
                    break;

                case kEventHIObjectDestruct:
                    delete object;
                    result = noErr;
                    break;
            }
            break;

        case kEventClassToolbarItem:
            switch (GetEventKind(event))
        {
            case kEventToolbarItemCreateCustomView:
            {
                QToolBar *toolbar
                = object->mainWindowLayout->unifiedToolbarHash.value(object->toolbarItem);
                if (toolbar) {
                    HIViewRef hiview = HIViewRef(toolbar->winId());
                    SetEventParameter(event, kEventParamControlRef, typeControlRef,
                                      sizeof(HIViewRef), &hiview);
                    result = noErr;
                }
            }
                break;
        }
            break;
    }
    return result;
}

void QMainWindowLayout::qtMacHIToolbarRegisterQToolBarInHIToolborItemClass()
{
    static bool registered = false;

    if (!registered) {
        HIObjectRegisterSubclass( kQToolBarHIToolbarItemClassID,
                                 kHIToolbarItemClassID, 0, QMainWindowLayout::qtoolbarInHIToolbarHandler,
                                 GetEventTypeCount(qtoolbarEvents), qtoolbarEvents, 0, 0 );
        registered = true;
    }
}

static void GetToolbarAllowedItems(CFMutableArrayRef array)
{
    CFArrayAppendValue(array, kQToolBarHIToolbarIdentifier);
}

HIToolbarItemRef QMainWindowLayout::createQToolBarInHIToolbarItem(QToolBar *toolbar,
                                                                  QMainWindowLayout *layout)
{
    QMainWindowLayout::qtMacHIToolbarRegisterQToolBarInHIToolborItemClass();

    EventRef event;
    HIToolbarItemRef result = 0;

    CFStringRef identifier = kQToolBarHIToolbarIdentifier;
    UInt32 options = kHIToolbarItemAllowDuplicates;

    CreateEvent(0, kEventClassHIObject, kEventHIObjectInitialize,
                GetCurrentEventTime(), 0, &event);
    SetEventParameter(event, kEventParamToolbarItemIdentifier, typeCFStringRef,
                      sizeof(CFStringRef), &identifier);
    SetEventParameter(event, kEventParamAttributes, typeUInt32, sizeof(UInt32), &options);
    SetEventParameter(event, kEventParamQToolBar, typeVoidPtr, sizeof(void *), &toolbar);
    SetEventParameter(event, kEventParamQMainWindowLayout, typeVoidPtr, sizeof(void *), &layout);

    HIObjectCreate(kQToolBarHIToolbarItemClassID, event,
                   static_cast<HIObjectRef *>(&result));

    ReleaseEvent(event);
    return result;

}

HIToolbarItemRef QMainWindowLayout::CreateToolbarItemForIdentifier(CFStringRef identifier,
                                                                   CFTypeRef data)
{
    HIToolbarItemRef item = 0;
    if (CFStringCompare(kQToolBarHIToolbarIdentifier, identifier,
                        kCFCompareBackwards) == kCFCompareEqualTo) {
        if (data && CFGetTypeID(data) == CFArrayGetTypeID()) {
            CFArrayRef array = static_cast<CFArrayRef>(data);
            QToolBar *toolbar = static_cast<QToolBar *>(const_cast<void *>(CFArrayGetValueAtIndex(array, 0)));
            QMainWindowLayout *layout = static_cast<QMainWindowLayout *>(const_cast<void *>(CFArrayGetValueAtIndex(array, 1)));
            item = createQToolBarInHIToolbarItem(toolbar, layout);
        }
    }
    return item;
}

static const EventTypeSpec kToolbarEvents[] = {
{ kEventClassToolbar, kEventToolbarGetDefaultIdentifiers },
{ kEventClassToolbar, kEventToolbarGetAllowedIdentifiers },
{ kEventClassToolbar, kEventToolbarCreateItemWithIdentifier },
{ kEventClassToolbar, kEventToolbarItemAdded },
{ kEventClassToolbar, kEventToolbarItemRemoved }
};

OSStatus QMainWindowLayout::qtmacToolbarDelegate(EventHandlerCallRef, EventRef event, void *data)
{
    QMainWindowLayout *mainWindowLayout = static_cast<QMainWindowLayout *>(data);
    OSStatus            result = eventNotHandledErr;
    CFMutableArrayRef   array;
    CFStringRef         identifier;
    switch (GetEventKind(event)) {
        case kEventToolbarGetDefaultIdentifiers:
        case kEventToolbarGetAllowedIdentifiers:
            GetEventParameter(event, kEventParamMutableArray, typeCFMutableArrayRef, 0,
                              sizeof(CFMutableArrayRef), 0, &array);
            GetToolbarAllowedItems(array);
            result = noErr;
            break;
        case kEventToolbarCreateItemWithIdentifier: {
            HIToolbarItemRef item;
            CFTypeRef data = 0;
            OSStatus err = GetEventParameter(event, kEventParamToolbarItemIdentifier, typeCFStringRef,
                                             0, sizeof(CFStringRef), 0, &identifier);
            err = GetEventParameter(event, kEventParamToolbarItemConfigData, typeCFTypeRef,
                                    0, sizeof(CFTypeRef), 0, &data);
            item = CreateToolbarItemForIdentifier(identifier, data);
            if (item) {
                result = SetEventParameter(event, kEventParamToolbarItem, typeHIToolbarItemRef,
                                           sizeof(HIToolbarItemRef), &item );
            }
            break;
        }
        case kEventToolbarItemAdded: {
            // Double check that our "view" of the toolbar is similar.
            HIToolbarItemRef item;
            CFIndex index;
            if (GetEventParameter(event, kEventParamToolbarItem, typeHIToolbarItemRef,
                                  0, sizeof(HIToolbarItemRef), 0, &item) == noErr
                && GetEventParameter(event, kEventParamIndex, typeCFIndex, 0,
                                     sizeof(CFIndex), 0, &index) == noErr) {
                CFRetain(item); // We will watch this until it's removed from the list (or bust).
                mainWindowLayout->toolbarItemsCopy.insert(index, item);
                QToolBar *toolbar = mainWindowLayout->unifiedToolbarHash.value(item);
                if (toolbar) {
                    int toolbarIndex = mainWindowLayout->qtoolbarsInUnifiedToolbarList.indexOf(toolbar);
                    if (index != toolbarIndex) {
                        // Dang, we must be out of sync, rebuild it from the "toolbarItemsCopy"
                        mainWindowLayout->qtoolbarsInUnifiedToolbarList.clear();
                        for (int i = 0; i < mainWindowLayout->toolbarItemsCopy.size(); ++i) {
                            // This will either append the correct toolbar or an
                            // null toolbar. This is fine because this list
                            // is really only kept to make sure that things are but in the right order.
                            mainWindowLayout->qtoolbarsInUnifiedToolbarList.append(
                                                                                   mainWindowLayout->unifiedToolbarHash.value(mainWindowLayout->
                                                                                                                              toolbarItemsCopy.at(i)));
                        }
                    }
                }
            }
            break;
        }
        case kEventToolbarItemRemoved: {
            HIToolbarItemRef item;
            if (GetEventParameter(event, kEventParamToolbarItem, typeHIToolbarItemRef,
                                  0, sizeof(HIToolbarItemRef), 0, &item) == noErr) {
                mainWindowLayout->unifiedToolbarHash.remove(item);
                for (int i = 0; i < mainWindowLayout->toolbarItemsCopy.size(); ++i) {
                    if (mainWindowLayout->toolbarItemsCopy.at(i) == item) {
                        // I know about it, so release it.
                        mainWindowLayout->toolbarItemsCopy.removeAt(i);
                        mainWindowLayout->qtoolbarsInUnifiedToolbarList.removeAt(i);
                        CFRelease(item);
                        break;
                    }
                }
            }
            break;
        }
    }
    return result;
}
#endif // ! QT_MAC_USE_COCOA

#ifndef kWindowUnifiedTitleAndToolbarAttribute
#define kWindowUnifiedTitleAndToolbarAttribute (1 << 7)
#endif

void QMainWindowLayout::updateHIToolBarStatus()
{
    bool useMacToolbar = layoutState.mainWindow->unifiedTitleAndToolBarOnMac();
#ifndef QT_MAC_USE_COCOA
    if (useMacToolbar) {
        ChangeWindowAttributes(qt_mac_window_for(layoutState.mainWindow),
                               kWindowUnifiedTitleAndToolbarAttribute, 0);
    } else {
        ChangeWindowAttributes(qt_mac_window_for(layoutState.mainWindow),
                               0, kWindowUnifiedTitleAndToolbarAttribute);
    }
#endif

    layoutState.mainWindow->setUpdatesEnabled(false);  // reduces a little bit of flicker, not all though
#if defined(QT_MAC_USE_COCOA)
    QMacCocoaAutoReleasePool pool;
    NSView *cView = [qt_mac_window_for(layoutState.mainWindow) contentView];
    if (useMacToolbar) {
        [cView setPostsFrameChangedNotifications:YES];
        [[NSNotificationCenter defaultCenter] addObserver: [QT_MANGLE_NAMESPACE(QCocoaWindowDelegate) sharedDelegate]
                                                 selector: @selector(syncContentViewFrame:)
                                                     name: NSViewFrameDidChangeNotification
                                                   object: cView];
    }
#endif
    if (!useMacToolbar) {
        macWindowToolbarShow(layoutState.mainWindow, false);
        // Move everything out of the HIToolbar into the main toolbar.
        while (!qtoolbarsInUnifiedToolbarList.isEmpty()) {
            // Should shrink the list by one every time.
            QToolBar *toolbar = qtoolbarsInUnifiedToolbarList.first();
#if defined(QT_MAC_USE_COCOA)
            unifiedSurface->removeToolbar(toolbar);
#endif
            layoutState.mainWindow->addToolBar(Qt::TopToolBarArea, toolbar);
        }
        macWindowToolbarSet(qt_mac_window_for(layoutState.mainWindow), 0);
    } else {
        QList<QToolBar *> toolbars = layoutState.mainWindow->findChildren<QToolBar *>();
        for (int i = 0; i < toolbars.size(); ++i) {
            QToolBar *toolbar = toolbars.at(i);
            if (toolBarArea(toolbar) == Qt::TopToolBarArea) {
                // Do this here, because we are in an in-between state.
                removeWidget(toolbar);
                layoutState.mainWindow->addToolBar(Qt::TopToolBarArea, toolbar);
            }
        }
        syncUnifiedToolbarVisibility();
    }
#if defined(QT_MAC_USE_COCOA)
    if (!useMacToolbar) {
        [cView setPostsFrameChangedNotifications:NO];
        [[NSNotificationCenter defaultCenter] removeObserver: [QT_MANGLE_NAMESPACE(QCocoaWindowDelegate) sharedDelegate]
                                                        name: NSViewFrameDidChangeNotification
                                                      object: cView];
    }
#endif
    layoutState.mainWindow->setUpdatesEnabled(true);
}

void QMainWindowLayout::insertIntoMacToolbar(QToolBar *before, QToolBar *toolbar)
{
    // This layering could go on to one more level, but I decided to stop here.
    // The HIToolbar and NSToolbar APIs are fairly similar as you will see.
    if (toolbar == 0)
        return;

#if defined(QT_MAC_USE_COCOA)
    // toolbar will now become native (if not already) since we need
    // an nsview for it inside the corresponding NSToolbarItem.
    // Setting isInUnifiedToolbar will (among other things) stop alien
    // siblings from becoming native when this happends since the toolbar
    // will not overlap with other children of the QMainWindow. NB: Switching
    // unified toolbar off after this stage is not supported, as this means
    // that either the menubar must be alien again, or the sibling must
    // be backed by an nsview to protect from overlapping issues:
    toolbar->d_func()->isInUnifiedToolbar = true;
#endif

    QToolBarLayout *toolbarLayout = static_cast<QToolBarLayout *>(toolbar->layout());
    toolbarSaveState.insert(toolbar, ToolBarSaveState(toolbar->isMovable(), toolbar->maximumSize()));

    if (toolbarLayout->hasExpandFlag() == false)
        toolbar->setMaximumSize(toolbar->sizeHint());

    toolbar->setMovable(false);
    toolbarLayout->setUsePopupMenu(true);
    // Make the toolbar a child of the mainwindow to avoid creating a window.
    toolbar->setParent(layoutState.mainWindow);

    toolbar->winId();  // Now create the OSViewRef.
    layoutState.mainWindow->createWinId();

    OSWindowRef window = qt_mac_window_for(layoutState.mainWindow);
    int beforeIndex = qtoolbarsInUnifiedToolbarList.indexOf(before);
    if (beforeIndex == -1)
        beforeIndex = qtoolbarsInUnifiedToolbarList.size();

    int toolbarIndex = qtoolbarsInUnifiedToolbarList.indexOf(toolbar);

#ifndef QT_MAC_USE_COCOA
    HIToolbarRef macToolbar = NULL;
    if ((GetWindowToolbar(window, &macToolbar) == noErr) && !macToolbar) {
        HIToolbarCreate(kQMainWindowMacToolbarID,
                        kHIToolbarItemAllowDuplicates, &macToolbar);
        InstallEventHandler(HIObjectGetEventTarget(static_cast<HIToolbarRef>(macToolbar)),
                            QMainWindowLayout::qtmacToolbarDelegate, GetEventTypeCount(kToolbarEvents),
                            kToolbarEvents, this, 0);
        HIToolbarSetDisplaySize(macToolbar, kHIToolbarDisplaySizeNormal);
        HIToolbarSetDisplayMode(macToolbar, kHIToolbarDisplayModeIconOnly);
        macWindowToolbarSet(window, macToolbar);
        if (layoutState.mainWindow->isVisible())
            macWindowToolbarShow(layoutState.mainWindow, true);
        CFRelease(macToolbar);
    }
#else
    QMacCocoaAutoReleasePool pool;
    NSToolbar *macToolbar = [window toolbar];
    if (macToolbar == nil) {
        macToolbar = [[NSToolbar alloc] initWithIdentifier:(NSString *)kQMainWindowMacToolbarID];
        [macToolbar setDisplayMode:NSToolbarDisplayModeIconOnly];
        [macToolbar setSizeMode:NSToolbarSizeModeRegular];
        [macToolbar setDelegate:[[QT_MANGLE_NAMESPACE(QCocoaToolBarDelegate) alloc] initWithMainWindowLayout:this]];
        [window setToolbar:macToolbar];
        [macToolbar release];
    }
#endif
    if (toolbarIndex != -1) {
        qtoolbarsInUnifiedToolbarList.removeAt(toolbarIndex);
#ifndef QT_MAC_USE_COCOA
        HIToolbarRemoveItemAtIndex(macToolbar, toolbarIndex);
#else
        [macToolbar removeItemAtIndex:toolbarIndex];
#endif
    }
    qtoolbarsInUnifiedToolbarList.insert(beforeIndex, toolbar);

    // Adding to the unified toolbar surface for the raster engine.
    if (layoutState.mainWindow->windowSurface()) {
        QPoint offset(0, 0);
        for (int i = 0; i < beforeIndex; ++i) {
            offset.setX(offset.x() + qtoolbarsInUnifiedToolbarList.at(i)->size().width());
        }
#ifdef QT_MAC_USE_COCOA
        unifiedSurface->insertToolbar(toolbar, offset);
#endif // QT_MAC_USE_COCOA
    }

#ifndef QT_MAC_USE_COCOA
    QCFType<HIToolbarItemRef> outItem;
    const QObject *stupidArray[] = { toolbar, this };
    QCFType<CFArrayRef> array = CFArrayCreate(0, reinterpret_cast<const void **>(&stupidArray),
                                              2, 0);
    HIToolbarCreateItemWithIdentifier(macToolbar, kQToolBarHIToolbarIdentifier,
                                      array, &outItem);
    HIToolbarInsertItemAtIndex(macToolbar, outItem, beforeIndex);
#else
    NSString *toolbarID = kQToolBarNSToolbarIdentifier;
    toolbarID = [toolbarID stringByAppendingFormat:@"%p", toolbar];
    cocoaItemIDToToolbarHash.insert(qt_mac_NSStringToQString(toolbarID), toolbar);
    [macToolbar insertItemWithItemIdentifier:toolbarID atIndex:beforeIndex];
#endif
}

#ifdef QT_MAC_USE_COCOA
void QMainWindowLayout::updateUnifiedToolbarOffset()
{
    QPoint offset(0, 0);

    for (int i = 1; i < qtoolbarsInUnifiedToolbarList.length(); ++i) {
        offset.setX(offset.x() + qtoolbarsInUnifiedToolbarList.at(i - 1)->size().width());
        qtoolbarsInUnifiedToolbarList.at(i)->d_func()->toolbar_offset = offset;
    }
}
#endif // QT_MAC_USE_COCOA


void QMainWindowLayout::removeFromMacToolbar(QToolBar *toolbar)
{
    QHash<void *, QToolBar *>::iterator it = unifiedToolbarHash.begin();
    while (it != unifiedToolbarHash.end()) {
        if (it.value() == toolbar) {
            // Rescue our HIView and set it on the mainWindow again.
            bool saveVisible = !toolbar->isHidden();
            toolbar->setParent(0);
            toolbar->setParent(parentWidget());
            toolbar->setVisible(saveVisible);
            ToolBarSaveState saveState = toolbarSaveState.value(toolbar);
            static_cast<QToolBarLayout *>(toolbar->layout())->setUsePopupMenu(false);
            toolbar->setMovable(saveState.movable);
            toolbar->setMaximumSize(saveState.maximumSize);
            toolbarSaveState.remove(toolbar);
#ifndef QT_MAC_USE_COCOA
            HIToolbarItemRef item = static_cast<HIToolbarItemRef>(it.key());
            HIToolbarRemoveItemAtIndex(HIToolbarItemGetToolbar(item),
                                       toolbarItemsCopy.indexOf(item));
#else
            NSToolbarItem *item = static_cast<NSToolbarItem *>(it.key());
            [[qt_mac_window_for(layoutState.mainWindow->window()) toolbar]
                removeItemAtIndex:toolbarItemsCopy.indexOf(item)];
             unifiedToolbarHash.remove(item);
             qtoolbarsInUnifiedToolbarList.removeAll(toolbar);
#endif
            break;
        }
        ++it;
    }
}

void QMainWindowLayout::cleanUpMacToolbarItems()
{
#ifdef QT_MAC_USE_COCOA
    QMacCocoaAutoReleasePool pool;
#endif
    for (int i = 0; i < toolbarItemsCopy.size(); ++i) {
#ifdef QT_MAC_USE_COCOA
        NSToolbarItem *item = static_cast<NSToolbarItem *>(toolbarItemsCopy.at(i));
        [item setView:0];
#endif
        CFRelease(toolbarItemsCopy.at(i));
    }
    toolbarItemsCopy.clear();
    unifiedToolbarHash.clear();

#ifdef QT_MAC_USE_COCOA
    OSWindowRef window = qt_mac_window_for(layoutState.mainWindow);
    NSToolbar *macToolbar = [window toolbar];
    if (macToolbar) {
      [[macToolbar delegate] release];
      [macToolbar setDelegate:nil];
    }
#endif
}

void QMainWindowLayout::fixSizeInUnifiedToolbar(QToolBar *tb) const
{
#ifdef QT_MAC_USE_COCOA
    QHash<void *, QToolBar *>::const_iterator it = unifiedToolbarHash.constBegin();
    NSToolbarItem *item = nil;
    while (it != unifiedToolbarHash.constEnd()) {
        if (tb == it.value()) {
            item = static_cast<NSToolbarItem *>(it.key());
            break;
        }
        ++it;
    }
    if (item) {
        QMacCocoaAutoReleasePool pool;
        QWidgetItem layoutItem(tb);
        QSize size = layoutItem.maximumSize();
        NSSize nssize = NSMakeSize(size.width(), size.height());
        [item setMaxSize:nssize];
        size = layoutItem.minimumSize();
        nssize.width = size.width();
        nssize.height = size.height();
        [item setMinSize:nssize];
    }
#else
    Q_UNUSED(tb);
#endif
}

void QMainWindowLayout::syncUnifiedToolbarVisibility()
{
    if (blockVisiblityCheck)
        return;

    Q_ASSERT(layoutState.mainWindow->unifiedTitleAndToolBarOnMac());
    bool show = false;
    const int ToolBarCount = qtoolbarsInUnifiedToolbarList.count();
    for (int i = 0; i < ToolBarCount; ++i) {
        if (qtoolbarsInUnifiedToolbarList.at(i)->isVisible()) {
            show = true;
            break;
        }
    }
    macWindowToolbarShow(layoutState.mainWindow, show);
}

QT_END_NAMESPACE
