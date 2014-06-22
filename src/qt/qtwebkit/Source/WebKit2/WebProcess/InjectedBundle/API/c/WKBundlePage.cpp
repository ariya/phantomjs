/*
 * Copyright (C) 2010, 2011, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WKBundlePage.h"
#include "WKBundlePagePrivate.h"

#include "InjectedBundleBackForwardList.h"
#include "InjectedBundleNodeHandle.h"
#include "PageBanner.h"
#include "WKAPICast.h"
#include "WKArray.h"
#include "WKBundleAPICast.h"
#include "WKRetainPtr.h"
#include "WKString.h"
#include "WebContextMenu.h"
#include "WebContextMenuItem.h"
#include "WebFrame.h"
#include "WebFullScreenManager.h"
#include "WebImage.h"
#include "WebPage.h"
#include "WebRenderLayer.h"
#include "WebRenderObject.h"
#include "WebString.h"
#include "WebURL.h"
#include "WebURLRequest.h"

#include <WebCore/AXObjectCache.h>
#include <WebCore/AccessibilityObject.h>
#include <WebCore/Frame.h>
#include <WebCore/KURL.h>
#include <WebCore/Page.h>
#include <wtf/OwnArrayPtr.h>

using namespace WebKit;

WKTypeID WKBundlePageGetTypeID()
{
    return toAPI(WebPage::APIType);
}

void WKBundlePageSetContextMenuClient(WKBundlePageRef pageRef, WKBundlePageContextMenuClient* wkClient)
{
#if ENABLE(CONTEXT_MENUS)
    toImpl(pageRef)->initializeInjectedBundleContextMenuClient(wkClient);
#endif
}

void WKBundlePageSetEditorClient(WKBundlePageRef pageRef, WKBundlePageEditorClient* wkClient)
{
    toImpl(pageRef)->initializeInjectedBundleEditorClient(wkClient);
}

void WKBundlePageSetFormClient(WKBundlePageRef pageRef, WKBundlePageFormClient* wkClient)
{
    toImpl(pageRef)->initializeInjectedBundleFormClient(wkClient);
}

void WKBundlePageSetPageLoaderClient(WKBundlePageRef pageRef, WKBundlePageLoaderClient* wkClient)
{
    toImpl(pageRef)->initializeInjectedBundleLoaderClient(wkClient);
}

void WKBundlePageSetResourceLoadClient(WKBundlePageRef pageRef, WKBundlePageResourceLoadClient* wkClient)
{
    toImpl(pageRef)->initializeInjectedBundleResourceLoadClient(wkClient);
}

void WKBundlePageSetPolicyClient(WKBundlePageRef pageRef, WKBundlePagePolicyClient* wkClient)
{
    toImpl(pageRef)->initializeInjectedBundlePolicyClient(wkClient);
}

void WKBundlePageSetUIClient(WKBundlePageRef pageRef, WKBundlePageUIClient* wkClient)
{
    toImpl(pageRef)->initializeInjectedBundleUIClient(wkClient);
}

void WKBundlePageSetFullScreenClient(WKBundlePageRef pageRef, WKBundlePageFullScreenClient* wkClient)
{
#if defined(ENABLE_FULLSCREEN_API) && ENABLE_FULLSCREEN_API
    toImpl(pageRef)->initializeInjectedBundleFullScreenClient(wkClient);
#endif
}

void WKBundlePageWillEnterFullScreen(WKBundlePageRef pageRef)
{
#if defined(ENABLE_FULLSCREEN_API) && ENABLE_FULLSCREEN_API
    toImpl(pageRef)->fullScreenManager()->willEnterFullScreen();
#endif
}

void WKBundlePageDidEnterFullScreen(WKBundlePageRef pageRef)
{
#if defined(ENABLE_FULLSCREEN_API) && ENABLE_FULLSCREEN_API
    toImpl(pageRef)->fullScreenManager()->didEnterFullScreen();
#endif
}

void WKBundlePageWillExitFullScreen(WKBundlePageRef pageRef)
{
#if defined(ENABLE_FULLSCREEN_API) && ENABLE_FULLSCREEN_API
    toImpl(pageRef)->fullScreenManager()->willExitFullScreen();
#endif
}

void WKBundlePageDidExitFullScreen(WKBundlePageRef pageRef)
{
#if defined(ENABLE_FULLSCREEN_API) && ENABLE_FULLSCREEN_API
    toImpl(pageRef)->fullScreenManager()->didExitFullScreen();
#endif
}

void WKBundlePageSetDiagnosticLoggingClient(WKBundlePageRef pageRef, WKBundlePageDiagnosticLoggingClient* client)
{
    toImpl(pageRef)->initializeInjectedBundleDiagnosticLoggingClient(client);
}

WKBundlePageGroupRef WKBundlePageGetPageGroup(WKBundlePageRef pageRef)
{
    return toAPI(toImpl(pageRef)->pageGroup());
}

WKBundleFrameRef WKBundlePageGetMainFrame(WKBundlePageRef pageRef)
{
    return toAPI(toImpl(pageRef)->mainWebFrame());
}

WKArrayRef WKBundlePageCopyContextMenuItemTitles(WKBundlePageRef pageRef)
{
#if ENABLE(CONTEXT_MENUS)
    WebContextMenu* contextMenu = toImpl(pageRef)->contextMenu();
    const Vector<WebContextMenuItemData> &items = contextMenu->items();
    size_t arrayLength = items.size();
    OwnArrayPtr<WKTypeRef> itemNames = adoptArrayPtr(new WKTypeRef[arrayLength]);
    for (size_t i = 0; i < arrayLength; ++i)
        itemNames[i] = WKStringCreateWithUTF8CString(items[i].title().utf8().data());

    return WKArrayCreateAdoptingValues(itemNames.get(), arrayLength);
#else
    return 0;
#endif
}

WKArrayRef WKBundlePageCopyContextMenuAtPointInWindow(WKBundlePageRef pageRef, WKPoint point)
{
#if ENABLE(CONTEXT_MENUS)
    WebContextMenu* contextMenu = toImpl(pageRef)->contextMenuAtPointInWindow(toIntPoint(point));
    if (!contextMenu)
        return 0;

    const Vector<WebContextMenuItemData>& items = contextMenu->items();
    size_t arrayLength = items.size();

    RefPtr<MutableArray> menuArray = MutableArray::create();
    menuArray->reserveCapacity(arrayLength);
    for (unsigned i = 0; i < arrayLength; ++i)
        menuArray->append(WebContextMenuItem::create(items[i]).get());
    
    return toAPI(menuArray.release().leakRef());
#else
    return 0;
#endif
}

void* WKAccessibilityRootObject(WKBundlePageRef pageRef)
{
#if HAVE(ACCESSIBILITY)
    if (!pageRef)
        return 0;
    
    WebCore::Page* page = toImpl(pageRef)->corePage();
    if (!page)
        return 0;
    
    WebCore::Frame* core = page->mainFrame();
    if (!core || !core->document())
        return 0;
    
    WebCore::AXObjectCache::enableAccessibility();

    WebCore::AccessibilityObject* root = core->document()->axObjectCache()->rootObject();
    if (!root)
        return 0;
    
    return root->wrapper();
#else
    UNUSED_PARAM(pageRef);
    return 0;
#endif
}

void* WKAccessibilityFocusedObject(WKBundlePageRef pageRef)
{
#if HAVE(ACCESSIBILITY)
    if (!pageRef)
        return 0;
    
    WebCore::Page* page = toImpl(pageRef)->corePage();
    if (!page)
        return 0;

    WebCore::AXObjectCache::enableAccessibility();

    WebCore::AccessibilityObject* focusedObject = WebCore::AXObjectCache::focusedUIElementForPage(page);
    if (!focusedObject)
        return 0;
    
    return focusedObject->wrapper();
#else
    UNUSED_PARAM(pageRef);
    return 0;
#endif
}

void WKBundlePageStopLoading(WKBundlePageRef pageRef)
{
    toImpl(pageRef)->stopLoading();
}

void WKBundlePageSetDefersLoading(WKBundlePageRef pageRef, bool defersLoading)
{
    toImpl(pageRef)->setDefersLoading(defersLoading);
}

WKStringRef WKBundlePageCopyRenderTreeExternalRepresentation(WKBundlePageRef pageRef)
{
    return toCopiedAPI(toImpl(pageRef)->renderTreeExternalRepresentation());
}

WKStringRef WKBundlePageCopyRenderTreeExternalRepresentationForPrinting(WKBundlePageRef pageRef)
{
    return toCopiedAPI(toImpl(pageRef)->renderTreeExternalRepresentationForPrinting());
}

void WKBundlePageExecuteEditingCommand(WKBundlePageRef pageRef, WKStringRef name, WKStringRef argument)
{
    toImpl(pageRef)->executeEditingCommand(toWTFString(name), toWTFString(argument));
}

bool WKBundlePageIsEditingCommandEnabled(WKBundlePageRef pageRef, WKStringRef name)
{
    return toImpl(pageRef)->isEditingCommandEnabled(toWTFString(name));
}

void WKBundlePageClearMainFrameName(WKBundlePageRef pageRef)
{
    toImpl(pageRef)->clearMainFrameName();
}

void WKBundlePageClose(WKBundlePageRef pageRef)
{
    toImpl(pageRef)->sendClose();
}

double WKBundlePageGetTextZoomFactor(WKBundlePageRef pageRef)
{
    return toImpl(pageRef)->textZoomFactor();
}

void WKBundlePageSetTextZoomFactor(WKBundlePageRef pageRef, double zoomFactor)
{
    toImpl(pageRef)->setTextZoomFactor(zoomFactor);
}

double WKBundlePageGetPageZoomFactor(WKBundlePageRef pageRef)
{
    return toImpl(pageRef)->pageZoomFactor();
}

void WKBundlePageSetPageZoomFactor(WKBundlePageRef pageRef, double zoomFactor)
{
    toImpl(pageRef)->setPageZoomFactor(zoomFactor);
}

void WKBundlePageSetScaleAtOrigin(WKBundlePageRef pageRef, double scale, WKPoint origin)
{
    toImpl(pageRef)->scalePage(scale, toIntPoint(origin));
}

WKBundleBackForwardListRef WKBundlePageGetBackForwardList(WKBundlePageRef pageRef)
{
    return toAPI(toImpl(pageRef)->backForwardList());
}

void WKBundlePageSetUnderlayPage(WKBundlePageRef pageRef, WKBundlePageRef pageUnderlayRef)
{
    toImpl(pageRef)->setUnderlayPage(toImpl(pageUnderlayRef));
}

void WKBundlePageInstallPageOverlay(WKBundlePageRef pageRef, WKBundlePageOverlayRef pageOverlayRef)
{
    toImpl(pageRef)->installPageOverlay(toImpl(pageOverlayRef));
}

void WKBundlePageUninstallPageOverlay(WKBundlePageRef pageRef, WKBundlePageOverlayRef pageOverlayRef)
{
    toImpl(pageRef)->uninstallPageOverlay(toImpl(pageOverlayRef));
}

void WKBundlePageInstallPageOverlayWithAnimation(WKBundlePageRef pageRef, WKBundlePageOverlayRef pageOverlayRef)
{
    toImpl(pageRef)->installPageOverlay(toImpl(pageOverlayRef), true);
}

void WKBundlePageUninstallPageOverlayWithAnimation(WKBundlePageRef pageRef, WKBundlePageOverlayRef pageOverlayRef)
{
    toImpl(pageRef)->uninstallPageOverlay(toImpl(pageOverlayRef), true);
}

void WKBundlePageSetTopOverhangImage(WKBundlePageRef pageRef, WKImageRef imageRef)
{
#if PLATFORM(MAC)
    toImpl(pageRef)->setTopOverhangImage(toImpl(imageRef));
#else
    UNUSED_PARAM(pageRef);
    UNUSED_PARAM(imageRef);
#endif
}

void WKBundlePageSetBottomOverhangImage(WKBundlePageRef pageRef, WKImageRef imageRef)
{
#if PLATFORM(MAC)
    toImpl(pageRef)->setBottomOverhangImage(toImpl(imageRef));
#else
    UNUSED_PARAM(pageRef);
    UNUSED_PARAM(imageRef);
#endif
}

void WKBundlePageSetHeaderBanner(WKBundlePageRef pageRef, WKBundlePageBannerRef bannerRef)
{
    toImpl(pageRef)->setHeaderPageBanner(toImpl(bannerRef));
}

void WKBundlePageSetFooterBanner(WKBundlePageRef pageRef, WKBundlePageBannerRef bannerRef)
{
    toImpl(pageRef)->setFooterPageBanner(toImpl(bannerRef));
}

bool WKBundlePageHasLocalDataForURL(WKBundlePageRef pageRef, WKURLRef urlRef)
{
    return toImpl(pageRef)->hasLocalDataForURL(WebCore::KURL(WebCore::KURL(), toWTFString(urlRef)));
}

bool WKBundlePageCanHandleRequest(WKURLRequestRef requestRef)
{
    return WebPage::canHandleRequest(toImpl(requestRef)->resourceRequest());
}

bool WKBundlePageFindString(WKBundlePageRef pageRef, WKStringRef target, WKFindOptions findOptions)
{
    return toImpl(pageRef)->findStringFromInjectedBundle(toWTFString(target), toFindOptions(findOptions));
}

WKImageRef WKBundlePageCreateSnapshotWithOptions(WKBundlePageRef pageRef, WKRect rect, WKSnapshotOptions options)
{
    RefPtr<WebImage> webImage = toImpl(pageRef)->scaledSnapshotWithOptions(toIntRect(rect), 1, toSnapshotOptions(options));
    return toAPI(webImage.release().leakRef());
}

WKImageRef WKBundlePageCreateSnapshotInViewCoordinates(WKBundlePageRef pageRef, WKRect rect, WKImageOptions options)
{
    SnapshotOptions snapshotOptions = snapshotOptionsFromImageOptions(options);
    snapshotOptions |= SnapshotOptionsInViewCoordinates;
    RefPtr<WebImage> webImage = toImpl(pageRef)->scaledSnapshotWithOptions(toIntRect(rect), 1, snapshotOptions);
    return toAPI(webImage.release().leakRef());
}

WKImageRef WKBundlePageCreateSnapshotInDocumentCoordinates(WKBundlePageRef pageRef, WKRect rect, WKImageOptions options)
{
    RefPtr<WebImage> webImage = toImpl(pageRef)->scaledSnapshotWithOptions(toIntRect(rect), 1, snapshotOptionsFromImageOptions(options));
    return toAPI(webImage.release().leakRef());
}

WKImageRef WKBundlePageCreateScaledSnapshotInDocumentCoordinates(WKBundlePageRef pageRef, WKRect rect, double scaleFactor, WKImageOptions options)
{
    RefPtr<WebImage> webImage = toImpl(pageRef)->scaledSnapshotWithOptions(toIntRect(rect), scaleFactor, snapshotOptionsFromImageOptions(options));
    return toAPI(webImage.release().leakRef());
}

double WKBundlePageGetBackingScaleFactor(WKBundlePageRef pageRef)
{
    return toImpl(pageRef)->deviceScaleFactor();
}

void WKBundlePageListenForLayoutMilestones(WKBundlePageRef pageRef, WKLayoutMilestones milestones)
{
    toImpl(pageRef)->listenForLayoutMilestones(toLayoutMilestones(milestones));
}

WKBundleInspectorRef WKBundlePageGetInspector(WKBundlePageRef pageRef)
{
#if ENABLE(INSPECTOR)
    return toAPI(toImpl(pageRef)->inspector());
#else
    UNUSED_PARAM(pageRef);
    return 0;
#endif
}

void WKBundlePageForceRepaint(WKBundlePageRef page)
{
    toImpl(page)->forceRepaintWithoutCallback();
}

void WKBundlePageSimulateMouseDown(WKBundlePageRef page, int button, WKPoint position, int clickCount, WKEventModifiers modifiers, double time)
{
    toImpl(page)->simulateMouseDown(button, toIntPoint(position), clickCount, modifiers, time);
}

void WKBundlePageSimulateMouseUp(WKBundlePageRef page, int button, WKPoint position, int clickCount, WKEventModifiers modifiers, double time)
{
    toImpl(page)->simulateMouseUp(button, toIntPoint(position), clickCount, modifiers, time);
}

void WKBundlePageSimulateMouseMotion(WKBundlePageRef page, WKPoint position, double time)
{
    toImpl(page)->simulateMouseMotion(toIntPoint(position), time);
}

uint64_t WKBundlePageGetRenderTreeSize(WKBundlePageRef pageRef)
{
    return toImpl(pageRef)->renderTreeSize();
}

WKRenderObjectRef WKBundlePageCopyRenderTree(WKBundlePageRef pageRef)
{
    return toAPI(WebRenderObject::create(toImpl(pageRef)).leakRef());
}

WKRenderLayerRef WKBundlePageCopyRenderLayerTree(WKBundlePageRef pageRef)
{
    return toAPI(WebRenderLayer::create(toImpl(pageRef)).leakRef());
}

void WKBundlePageSetPaintedObjectsCounterThreshold(WKBundlePageRef, uint64_t)
{
    // FIXME: This function is only still here to keep open source Mac builds building.
    // We should remove it as soon as we can.
}

void WKBundlePageSetTracksRepaints(WKBundlePageRef pageRef, bool trackRepaints)
{
    toImpl(pageRef)->setTracksRepaints(trackRepaints);
}

bool WKBundlePageIsTrackingRepaints(WKBundlePageRef pageRef)
{
    return toImpl(pageRef)->isTrackingRepaints();
}

void WKBundlePageResetTrackedRepaints(WKBundlePageRef pageRef)
{
    toImpl(pageRef)->resetTrackedRepaints();
}

WKArrayRef WKBundlePageCopyTrackedRepaintRects(WKBundlePageRef pageRef)
{
    return toAPI(toImpl(pageRef)->trackedRepaintRects().leakRef());
}

void WKBundlePageSetComposition(WKBundlePageRef pageRef, WKStringRef text, int from, int length)
{
    toImpl(pageRef)->setCompositionForTesting(toWTFString(text), from, length);
}

bool WKBundlePageHasComposition(WKBundlePageRef pageRef)
{
    return toImpl(pageRef)->hasCompositionForTesting();
}

void WKBundlePageConfirmComposition(WKBundlePageRef pageRef)
{
    toImpl(pageRef)->confirmCompositionForTesting(String());
}

void WKBundlePageConfirmCompositionWithText(WKBundlePageRef pageRef, WKStringRef text)
{
    toImpl(pageRef)->confirmCompositionForTesting(toWTFString(text));
}

bool WKBundlePageCanShowMIMEType(WKBundlePageRef pageRef, WKStringRef mimeTypeRef)
{
    return toImpl(pageRef)->canShowMIMEType(toWTFString(mimeTypeRef));
}

WKRenderingSuppressionToken WKBundlePageExtendIncrementalRenderingSuppression(WKBundlePageRef pageRef)
{
    return toImpl(pageRef)->extendIncrementalRenderingSuppression();
}

void WKBundlePageStopExtendingIncrementalRenderingSuppression(WKBundlePageRef pageRef, WKRenderingSuppressionToken token)
{
    toImpl(pageRef)->stopExtendingIncrementalRenderingSuppression(token);
}
