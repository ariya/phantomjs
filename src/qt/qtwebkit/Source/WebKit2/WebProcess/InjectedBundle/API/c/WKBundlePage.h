/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef WKBundlePage_h
#define WKBundlePage_h

#include <JavaScriptCore/JavaScript.h>
#include <WebKit2/WKBase.h>
#include <WebKit2/WKEvent.h>
#include <WebKit2/WKFindOptions.h>
#include <WebKit2/WKImage.h>
#include <WebKit2/WKPageLoadTypes.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    kWKInsertActionTyped = 0,
    kWKInsertActionPasted = 1,
    kWKInsertActionDropped = 2
};
typedef uint32_t WKInsertActionType;

enum {
    kWKAffinityUpstream,
    kWKAffinityDownstream
};
typedef uint32_t WKAffinityType;

enum {
    WKInputFieldActionTypeMoveUp,
    WKInputFieldActionTypeMoveDown,
    WKInputFieldActionTypeCancel,
    WKInputFieldActionTypeInsertTab,
    WKInputFieldActionTypeInsertBacktab,
    WKInputFieldActionTypeInsertNewline,
    WKInputFieldActionTypeInsertDelete
};
typedef uint32_t WKInputFieldActionType;

enum {
    WKFullScreenNoKeyboard,
    WKFullScreenKeyboard,
};
typedef uint32_t WKFullScreenKeyboardRequestType;

enum {
    WKScrollDirectionLeft,
    WKScrollDirectionRight
};
typedef uint32_t WKScrollDirection;

// Loader Client
typedef void (*WKBundlePageDidStartProvisionalLoadForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidReceiveServerRedirectForProvisionalLoadForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidFailProvisionalLoadWithErrorForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKErrorRef error, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidCommitLoadForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidDocumentFinishLoadForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidFinishLoadForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidFinishDocumentLoadForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidFinishProgressCallback)(WKBundlePageRef page, const void *clientInfo);
typedef void (*WKBundlePageDidFailLoadWithErrorForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKErrorRef error, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidSameDocumentNavigationForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKSameDocumentNavigationType type, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidReceiveTitleForFrameCallback)(WKBundlePageRef page, WKStringRef title, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidRemoveFrameFromHierarchyCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidDisplayInsecureContentForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidRunInsecureContentForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidDetectXSSForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidFirstLayoutForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidFirstVisuallyNonEmptyLayoutForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidNewFirstVisuallyNonEmptyLayoutCallback)(WKBundlePageRef page, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidLayoutForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, const void* clientInfo);
typedef void (*WKBundlePageDidClearWindowObjectForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKBundleScriptWorldRef world, const void *clientInfo);
typedef void (*WKBundlePageDidCancelClientRedirectForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, const void *clientInfo);
typedef void (*WKBundlePageWillPerformClientRedirectForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKURLRef url, double delay, double date, const void *clientInfo);
typedef void (*WKBundlePageDidHandleOnloadEventsForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef frame, const void *clientInfo);
typedef bool (*WKBundlePageShouldGoToBackForwardListItemCallback)(WKBundlePageRef page, WKBundleBackForwardListItemRef item, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageGlobalObjectIsAvailableForFrameCallback)(WKBundlePageRef page, WKBundleFrameRef, WKBundleScriptWorldRef, const void* clientInfo);
typedef void (*WKBundlePageWillDisconnectDOMWindowExtensionFromGlobalObjectCallback)(WKBundlePageRef page, WKBundleDOMWindowExtensionRef, const void* clientInfo);
typedef void (*WKBundlePageDidReconnectDOMWindowExtensionToGlobalObjectCallback)(WKBundlePageRef page, WKBundleDOMWindowExtensionRef, const void* clientInfo);
typedef void (*WKBundlePageWillDestroyGlobalObjectForDOMWindowExtensionCallback)(WKBundlePageRef page, WKBundleDOMWindowExtensionRef, const void* clientInfo);
typedef bool (*WKBundlePageShouldForceUniversalAccessFromLocalURLCallback)(WKBundlePageRef, WKStringRef url, const void* clientInfo);
typedef void (*WKBundlePageDidLayoutCallback)(WKBundlePageRef page, WKLayoutMilestones milestones, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageFeaturesUsedInPageCallback)(WKBundlePageRef page, WKArrayRef featureStrings, const void *clientInfo);
typedef void (*WKBundlePageWillLoadURLRequestCallback)(WKBundlePageRef page, WKURLRequestRef request, WKTypeRef userData, const void *clientInfo);
typedef void (*WKBundlePageWillLoadDataRequestCallback)(WKBundlePageRef page, WKURLRequestRef request, WKDataRef data, WKStringRef MIMEType, WKStringRef encodingName, WKURLRef unreachableURL, WKTypeRef userData, const void *clientInfo);

struct WKBundlePageLoaderClient {
    int                                                                     version;
    const void *                                                            clientInfo;

    // Version 0.
    WKBundlePageDidStartProvisionalLoadForFrameCallback                     didStartProvisionalLoadForFrame;
    WKBundlePageDidReceiveServerRedirectForProvisionalLoadForFrameCallback  didReceiveServerRedirectForProvisionalLoadForFrame;
    WKBundlePageDidFailProvisionalLoadWithErrorForFrameCallback             didFailProvisionalLoadWithErrorForFrame;
    WKBundlePageDidCommitLoadForFrameCallback                               didCommitLoadForFrame;
    WKBundlePageDidFinishDocumentLoadForFrameCallback                       didFinishDocumentLoadForFrame;
    WKBundlePageDidFinishLoadForFrameCallback                               didFinishLoadForFrame;
    WKBundlePageDidFailLoadWithErrorForFrameCallback                        didFailLoadWithErrorForFrame;
    WKBundlePageDidSameDocumentNavigationForFrameCallback                   didSameDocumentNavigationForFrame;
    WKBundlePageDidReceiveTitleForFrameCallback                             didReceiveTitleForFrame;
    WKBundlePageDidFirstLayoutForFrameCallback                              didFirstLayoutForFrame;
    WKBundlePageDidFirstVisuallyNonEmptyLayoutForFrameCallback              didFirstVisuallyNonEmptyLayoutForFrame;
    WKBundlePageDidRemoveFrameFromHierarchyCallback                         didRemoveFrameFromHierarchy;
    WKBundlePageDidDisplayInsecureContentForFrameCallback                   didDisplayInsecureContentForFrame;
    WKBundlePageDidRunInsecureContentForFrameCallback                       didRunInsecureContentForFrame;
    WKBundlePageDidClearWindowObjectForFrameCallback                        didClearWindowObjectForFrame;
    WKBundlePageDidCancelClientRedirectForFrameCallback                     didCancelClientRedirectForFrame;
    WKBundlePageWillPerformClientRedirectForFrameCallback                   willPerformClientRedirectForFrame;
    WKBundlePageDidHandleOnloadEventsForFrameCallback                       didHandleOnloadEventsForFrame;

    // Version 1.
    WKBundlePageDidLayoutForFrameCallback                                   didLayoutForFrame;
    WKBundlePageDidNewFirstVisuallyNonEmptyLayoutCallback                   didNewFirstVisuallyNonEmptyLayout;
    WKBundlePageDidDetectXSSForFrameCallback                                didDetectXSSForFrame;
    WKBundlePageShouldGoToBackForwardListItemCallback                       shouldGoToBackForwardListItem;
    WKBundlePageGlobalObjectIsAvailableForFrameCallback                     globalObjectIsAvailableForFrame;
    WKBundlePageWillDisconnectDOMWindowExtensionFromGlobalObjectCallback    willDisconnectDOMWindowExtensionFromGlobalObject;
    WKBundlePageDidReconnectDOMWindowExtensionToGlobalObjectCallback        didReconnectDOMWindowExtensionToGlobalObject;
    WKBundlePageWillDestroyGlobalObjectForDOMWindowExtensionCallback        willDestroyGlobalObjectForDOMWindowExtension;
    
    // Version 2
    WKBundlePageDidFinishProgressCallback                                   didFinishProgress;
    WKBundlePageShouldForceUniversalAccessFromLocalURLCallback              shouldForceUniversalAccessFromLocalURL;

    // Version 3
    void *                                                                  didReceiveIntentForFrame_unavailable;
    void *                                                                  registerIntentServiceForFrame_unavailable;

    // Version 4
    WKBundlePageDidLayoutCallback                                           didLayout;

    // Version 5
    WKBundlePageFeaturesUsedInPageCallback                                  featuresUsedInPage;
    
    // Version 6
    WKBundlePageWillLoadURLRequestCallback                                  willLoadURLRequest;
    WKBundlePageWillLoadDataRequestCallback                                 willLoadDataRequest;
};
typedef struct WKBundlePageLoaderClient WKBundlePageLoaderClient;

enum { kWKBundlePageLoaderClientCurrentVersion = 6 };

enum {
    WKBundlePagePolicyActionPassThrough,
    WKBundlePagePolicyActionUse
};
typedef uint32_t WKBundlePagePolicyAction;

// Policy Client
typedef WKBundlePagePolicyAction (*WKBundlePageDecidePolicyForNavigationActionCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKBundleNavigationActionRef navigationAction, WKURLRequestRef request, WKTypeRef* userData, const void* clientInfo);
typedef WKBundlePagePolicyAction (*WKBundlePageDecidePolicyForNewWindowActionCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKBundleNavigationActionRef navigationAction, WKURLRequestRef request, WKStringRef frameName, WKTypeRef* userData, const void* clientInfo);
typedef WKBundlePagePolicyAction (*WKBundlePageDecidePolicyForResponseCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKURLResponseRef response, WKURLRequestRef request, WKTypeRef* userData, const void* clientInfo);
typedef void (*WKBundlePageUnableToImplementPolicyCallback)(WKBundlePageRef page, WKBundleFrameRef frame, WKErrorRef error, WKTypeRef* userData, const void* clientInfo);

struct WKBundlePagePolicyClient {
    int                                                                 version;
    const void *                                                        clientInfo;
    WKBundlePageDecidePolicyForNavigationActionCallback                 decidePolicyForNavigationAction;
    WKBundlePageDecidePolicyForNewWindowActionCallback                  decidePolicyForNewWindowAction;
    WKBundlePageDecidePolicyForResponseCallback                         decidePolicyForResponse;
    WKBundlePageUnableToImplementPolicyCallback                         unableToImplementPolicy;
};
typedef struct WKBundlePagePolicyClient WKBundlePagePolicyClient;

enum { kWKBundlePagePolicyClientCurrentVersion = 0 };

// Resource Load Client
typedef void (*WKBundlePageDidInitiateLoadForResourceCallback)(WKBundlePageRef, WKBundleFrameRef, uint64_t resourceIdentifier, WKURLRequestRef, bool pageIsProvisionallyLoading, const void* clientInfo);
typedef WKURLRequestRef (*WKBundlePageWillSendRequestForFrameCallback)(WKBundlePageRef, WKBundleFrameRef, uint64_t resourceIdentifier, WKURLRequestRef, WKURLResponseRef redirectResponse, const void *clientInfo);
typedef void (*WKBundlePageDidReceiveResponseForResourceCallback)(WKBundlePageRef, WKBundleFrameRef, uint64_t resourceIdentifier, WKURLResponseRef, const void* clientInfo);
typedef void (*WKBundlePageDidReceiveContentLengthForResourceCallback)(WKBundlePageRef, WKBundleFrameRef, uint64_t resourceIdentifier, uint64_t contentLength, const void* clientInfo);
typedef void (*WKBundlePageDidFinishLoadForResourceCallback)(WKBundlePageRef, WKBundleFrameRef, uint64_t resourceIdentifier, const void* clientInfo);
typedef void (*WKBundlePageDidFailLoadForResourceCallback)(WKBundlePageRef, WKBundleFrameRef, uint64_t resourceIdentifier, WKErrorRef, const void* clientInfo);
typedef bool (*WKBundlePageShouldCacheResponseCallback)(WKBundlePageRef, WKBundleFrameRef, uint64_t resourceIdentifier, const void* clientInfo);
typedef bool (*WKBundlePageShouldUseCredentialStorageCallback)(WKBundlePageRef, WKBundleFrameRef, uint64_t resourceIdentifier, const void* clientInfo);

struct WKBundlePageResourceLoadClient {
    int                                                                 version;
    const void *                                                        clientInfo;

    // Version 0.
    WKBundlePageDidInitiateLoadForResourceCallback                      didInitiateLoadForResource;

    // willSendRequestForFrame is supposed to return a retained reference to the URL request.
    WKBundlePageWillSendRequestForFrameCallback                         willSendRequestForFrame;

    WKBundlePageDidReceiveResponseForResourceCallback                   didReceiveResponseForResource;
    WKBundlePageDidReceiveContentLengthForResourceCallback              didReceiveContentLengthForResource;
    WKBundlePageDidFinishLoadForResourceCallback                        didFinishLoadForResource;
    WKBundlePageDidFailLoadForResourceCallback                          didFailLoadForResource;

    // Version 1.
    WKBundlePageShouldCacheResponseCallback                             shouldCacheResponse;
    WKBundlePageShouldUseCredentialStorageCallback                      shouldUseCredentialStorage;
};
typedef struct WKBundlePageResourceLoadClient WKBundlePageResourceLoadClient;

enum { kWKBundlePageResourceLoadClientCurrentVersion = 1 };

enum {
    WKBundlePageUIElementVisibilityUnknown,
    WKBundlePageUIElementVisible,
    WKBundlePageUIElementHidden
};
typedef uint32_t WKBundlePageUIElementVisibility;

enum {
    WKBundlePageLabelSizeSmall,
    WKBundlePageLabelSizeLarge,
};
typedef uint32_t WKBundlePageLabelSize;

// UI Client
typedef void (*WKBundlePageWillAddMessageToConsoleCallback)(WKBundlePageRef page, WKStringRef message, uint32_t lineNumber, const void *clientInfo);
typedef void (*WKBundlePageWillSetStatusbarTextCallback)(WKBundlePageRef page, WKStringRef statusbarText, const void *clientInfo);
typedef void (*WKBundlePageWillRunJavaScriptAlertCallback)(WKBundlePageRef page, WKStringRef alertText, WKBundleFrameRef frame, const void *clientInfo);
typedef void (*WKBundlePageWillRunJavaScriptConfirmCallback)(WKBundlePageRef page, WKStringRef message, WKBundleFrameRef frame, const void *clientInfo);
typedef void (*WKBundlePageWillRunJavaScriptPromptCallback)(WKBundlePageRef page, WKStringRef message, WKStringRef defaultValue, WKBundleFrameRef frame, const void *clientInfo);
typedef void (*WKBundlePageMouseDidMoveOverElementCallback)(WKBundlePageRef page, WKBundleHitTestResultRef hitTestResult, WKEventModifiers modifiers, WKTypeRef* userData, const void *clientInfo);
typedef void (*WKBundlePageDidScrollCallback)(WKBundlePageRef page, const void *clientInfo);
typedef void (*WKBundlePagePaintCustomOverhangAreaCallback)(WKBundlePageRef page, WKGraphicsContextRef graphicsContext, WKRect horizontalOverhang, WKRect verticalOverhang, WKRect dirtyRect, const void* clientInfo);
typedef WKStringRef (*WKBundlePageGenerateFileForUploadCallback)(WKBundlePageRef page, WKStringRef originalFilePath, const void* clientInfo);
typedef bool (*WKBundlePageShouldRubberBandInDirectionCallback)(WKBundlePageRef page, WKScrollDirection scrollDirection, const void* clientInfo);
typedef WKBundlePageUIElementVisibility (*WKBundlePageStatusBarIsVisibleCallback)(WKBundlePageRef page, const void *clientInfo);
typedef WKBundlePageUIElementVisibility (*WKBundlePageMenuBarIsVisibleCallback)(WKBundlePageRef page, const void *clientInfo);
typedef WKBundlePageUIElementVisibility (*WKBundlePageToolbarsAreVisibleCallback)(WKBundlePageRef page, const void *clientInfo);
typedef void (*WKBundlePageReachedAppCacheOriginQuotaCallback)(WKBundlePageRef page, WKSecurityOriginRef origin, int64_t totalBytesNeeded, const void *clientInfo);
typedef uint64_t (*WKBundlePageExceededDatabaseQuotaCallback)(WKBundlePageRef page, WKSecurityOriginRef origin, WKStringRef databaseName, WKStringRef databaseDisplayName, uint64_t currentQuotaBytes, uint64_t currentOriginUsageBytes, uint64_t currentDatabaseUsageBytes, uint64_t expectedUsageBytes, const void *clientInfo);
typedef WKStringRef (*WKBundlePagePlugInCreateStartLabelTitleCallback)(WKStringRef mimeType, const void *clientInfo);
typedef WKStringRef (*WKBundlePagePlugInCreateStartLabelSubtitleCallback)(WKStringRef mimeType, const void *clientInfo);
typedef WKStringRef (*WKBundlePagePlugInCreateExtraStyleSheetCallback)(const void *clientInfo);
typedef WKStringRef (*WKBundlePagePlugInCreateExtraScriptCallback)(const void *clientInfo);

struct WKBundlePageUIClient {
    int                                                                 version;
    const void *                                                        clientInfo;
    WKBundlePageWillAddMessageToConsoleCallback                         willAddMessageToConsole;
    WKBundlePageWillSetStatusbarTextCallback                            willSetStatusbarText;
    WKBundlePageWillRunJavaScriptAlertCallback                          willRunJavaScriptAlert;
    WKBundlePageWillRunJavaScriptConfirmCallback                        willRunJavaScriptConfirm;
    WKBundlePageWillRunJavaScriptPromptCallback                         willRunJavaScriptPrompt;
    WKBundlePageMouseDidMoveOverElementCallback                         mouseDidMoveOverElement;
    WKBundlePageDidScrollCallback                                       pageDidScroll;
    WKBundlePagePaintCustomOverhangAreaCallback                         paintCustomOverhangArea;
    WKBundlePageGenerateFileForUploadCallback                           shouldGenerateFileForUpload;
    WKBundlePageGenerateFileForUploadCallback                           generateFileForUpload;
    WKBundlePageShouldRubberBandInDirectionCallback                     shouldRubberBandInDirection;
    WKBundlePageStatusBarIsVisibleCallback                              statusBarIsVisible;
    WKBundlePageMenuBarIsVisibleCallback                                menuBarIsVisible;
    WKBundlePageToolbarsAreVisibleCallback                              toolbarsAreVisible;

    // Version 1.
    WKBundlePageReachedAppCacheOriginQuotaCallback                      didReachApplicationCacheOriginQuota;

    // Version 2.
    WKBundlePageExceededDatabaseQuotaCallback                           didExceedDatabaseQuota;
    WKBundlePagePlugInCreateStartLabelTitleCallback                     createPlugInStartLabelTitle;
    WKBundlePagePlugInCreateStartLabelSubtitleCallback                  createPlugInStartLabelSubtitle;
    WKBundlePagePlugInCreateExtraStyleSheetCallback                     createPlugInExtraStyleSheet;
    WKBundlePagePlugInCreateExtraScriptCallback                         createPlugInExtraScript;
};
typedef struct WKBundlePageUIClient WKBundlePageUIClient;

enum { kWKBundlePageUIClientCurrentVersion = 2 };

// Editor client
typedef bool (*WKBundlePageShouldBeginEditingCallback)(WKBundlePageRef page, WKBundleRangeHandleRef range, const void* clientInfo);
typedef bool (*WKBundlePageShouldEndEditingCallback)(WKBundlePageRef page, WKBundleRangeHandleRef range, const void* clientInfo);
typedef bool (*WKBundlePageShouldInsertNodeCallback)(WKBundlePageRef page, WKBundleNodeHandleRef node, WKBundleRangeHandleRef rangeToReplace, WKInsertActionType action, const void* clientInfo);
typedef bool (*WKBundlePageShouldInsertTextCallback)(WKBundlePageRef page, WKStringRef string, WKBundleRangeHandleRef rangeToReplace, WKInsertActionType action, const void* clientInfo);
typedef bool (*WKBundlePageShouldDeleteRangeCallback)(WKBundlePageRef page, WKBundleRangeHandleRef range, const void* clientInfo);
typedef bool (*WKBundlePageShouldChangeSelectedRange)(WKBundlePageRef page, WKBundleRangeHandleRef fromRange, WKBundleRangeHandleRef toRange, WKAffinityType affinity, bool stillSelecting, const void* clientInfo);
typedef bool (*WKBundlePageShouldApplyStyle)(WKBundlePageRef page, WKBundleCSSStyleDeclarationRef style, WKBundleRangeHandleRef range, const void* clientInfo);
typedef void (*WKBundlePageEditingNotification)(WKBundlePageRef page, WKStringRef notificationName, const void* clientInfo);
typedef void (*WKBundlePageWillWriteToPasteboard)(WKBundlePageRef page, WKBundleRangeHandleRef range,  const void* clientInfo);
typedef void (*WKBundlePageGetPasteboardDataForRange)(WKBundlePageRef page, WKBundleRangeHandleRef range, WKArrayRef* pasteboardTypes, WKArrayRef* pasteboardData, const void* clientInfo);
typedef void (*WKBundlePageDidWriteToPasteboard)(WKBundlePageRef page, const void* clientInfo);

struct WKBundlePageEditorClient {
    int                                                                 version;
    const void *                                                        clientInfo;
    WKBundlePageShouldBeginEditingCallback                              shouldBeginEditing;
    WKBundlePageShouldEndEditingCallback                                shouldEndEditing;
    WKBundlePageShouldInsertNodeCallback                                shouldInsertNode;
    WKBundlePageShouldInsertTextCallback                                shouldInsertText;
    WKBundlePageShouldDeleteRangeCallback                               shouldDeleteRange;
    WKBundlePageShouldChangeSelectedRange                               shouldChangeSelectedRange;
    WKBundlePageShouldApplyStyle                                        shouldApplyStyle;
    WKBundlePageEditingNotification                                     didBeginEditing;
    WKBundlePageEditingNotification                                     didEndEditing;
    WKBundlePageEditingNotification                                     didChange;
    WKBundlePageEditingNotification                                     didChangeSelection;
    // Version 1.
    WKBundlePageWillWriteToPasteboard                                   willWriteToPasteboard;
    WKBundlePageGetPasteboardDataForRange                               getPasteboardDataForRange;
    WKBundlePageDidWriteToPasteboard                                    didWriteToPasteboard;
};
typedef struct WKBundlePageEditorClient WKBundlePageEditorClient;

enum { kWKBundlePageEditorClientCurrentVersion = 1 };

// Form client
typedef void (*WKBundlePageTextFieldDidBeginEditingCallback)(WKBundlePageRef page, WKBundleNodeHandleRef htmlInputElementHandle, WKBundleFrameRef frame, const void* clientInfo);
typedef void (*WKBundlePageTextFieldDidEndEditingCallback)(WKBundlePageRef page, WKBundleNodeHandleRef htmlInputElementHandle, WKBundleFrameRef frame, const void* clientInfo);
typedef void (*WKBundlePageTextDidChangeInTextFieldCallback)(WKBundlePageRef page, WKBundleNodeHandleRef htmlInputElementHandle, WKBundleFrameRef frame, const void* clientInfo);
typedef void (*WKBundlePageTextDidChangeInTextAreaCallback)(WKBundlePageRef page, WKBundleNodeHandleRef htmlTextAreaElementHandle, WKBundleFrameRef frame, const void* clientInfo);
typedef bool (*WKBundlePageShouldPerformActionInTextFieldCallback)(WKBundlePageRef page, WKBundleNodeHandleRef htmlInputElementHandle, WKInputFieldActionType actionType, WKBundleFrameRef frame, const void* clientInfo);
typedef void (*WKBundlePageWillSubmitFormCallback)(WKBundlePageRef page, WKBundleNodeHandleRef htmlFormElementHandle, WKBundleFrameRef frame, WKBundleFrameRef sourceFrame, WKDictionaryRef values, WKTypeRef* userData, const void* clientInfo);
typedef void (*WKBundlePageWillSendSubmitEventCallback)(WKBundlePageRef page, WKBundleNodeHandleRef htmlFormElementHandle, WKBundleFrameRef frame, WKBundleFrameRef sourceFrame, WKDictionaryRef values, const void* clientInfo);
typedef void (*WKBundlePageDidFocusTextFieldCallback)(WKBundlePageRef page, WKBundleNodeHandleRef htmlInputElementHandle, WKBundleFrameRef frame, const void* clientInfo);
typedef bool (*WKBundlePageShouldNotifyOnFormChangesCallback)(WKBundlePageRef page, const void* clientInfo);
typedef void (*WKBundlePageDidAssociateFormControlsCallback)(WKBundlePageRef page, WKArrayRef elementHandles, const void* clientInfo);

struct WKBundlePageFormClient {
    int                                                                 version;
    const void *                                                        clientInfo;
    
    // Version 0.
    WKBundlePageTextFieldDidBeginEditingCallback                        textFieldDidBeginEditing;
    WKBundlePageTextFieldDidEndEditingCallback                          textFieldDidEndEditing;
    WKBundlePageTextDidChangeInTextFieldCallback                        textDidChangeInTextField;
    WKBundlePageTextDidChangeInTextAreaCallback                         textDidChangeInTextArea;
    WKBundlePageShouldPerformActionInTextFieldCallback                  shouldPerformActionInTextField;
    WKBundlePageWillSubmitFormCallback                                  willSubmitForm;
    
    // Version 1.
    WKBundlePageWillSendSubmitEventCallback                             willSendSubmitEvent;

    // version 2.
    WKBundlePageDidFocusTextFieldCallback                               didFocusTextField;
    WKBundlePageShouldNotifyOnFormChangesCallback                       shouldNotifyOnFormChanges;
    WKBundlePageDidAssociateFormControlsCallback                        didAssociateFormControls;
};
typedef struct WKBundlePageFormClient WKBundlePageFormClient;

enum { kWKBundlePageFormClientCurrentVersion = 2 };

// ContextMenu client
typedef void (*WKBundlePageGetContextMenuFromDefaultContextMenuCallback)(WKBundlePageRef page, WKBundleHitTestResultRef hitTestResult, WKArrayRef defaultMenu, WKArrayRef* newMenu, WKTypeRef* userData, const void* clientInfo);

struct WKBundlePageContextMenuClient {
    int                                                                 version;
    const void *                                                        clientInfo;
    WKBundlePageGetContextMenuFromDefaultContextMenuCallback            getContextMenuFromDefaultMenu;
};
typedef struct WKBundlePageContextMenuClient WKBundlePageContextMenuClient;

enum { kWKBundlePageContextMenuClientCurrentVersion = 0 };

// Full Screen client
typedef bool (*WKBundlePageSupportsFullScreen)(WKBundlePageRef page, WKFullScreenKeyboardRequestType requestType);
typedef void (*WKBundlePageEnterFullScreenForElement)(WKBundlePageRef page, WKBundleNodeHandleRef element);
typedef void (*WKBundlePageExitFullScreenForElement)(WKBundlePageRef page, WKBundleNodeHandleRef element);
typedef void (*WKBundlePageBeganEnterFullScreen)(WKBundlePageRef page, WKRect initialFrame, WKRect finalFrame);
typedef void (*WKBundlePageBeganExitFullScreen)(WKBundlePageRef page, WKRect initialFrame, WKRect finalFrame);
typedef void (*WKBundlePageCloseFullScreen)(WKBundlePageRef page);

struct WKBundlePageFullScreenClient {
    int                                                                 version;
    const void *                                                        clientInfo;

    // Version 0:
    WKBundlePageSupportsFullScreen                                      supportsFullScreen;
    WKBundlePageEnterFullScreenForElement                               enterFullScreenForElement;
    WKBundlePageExitFullScreenForElement                                exitFullScreenForElement;

    // Version 1:
    WKBundlePageBeganEnterFullScreen                                    beganEnterFullScreen;
    WKBundlePageBeganExitFullScreen                                     beganExitFullScreen;
    WKBundlePageCloseFullScreen                                         closeFullScreen;
};
typedef struct WKBundlePageFullScreenClient WKBundlePageFullScreenClient;

enum { kWKBundlePageFullScreenClientCurrentVersion = 1 };

// MessageTrace client
typedef void (*WKBundlePageDiagnosticLoggingCallback)(WKBundlePageRef page, WKStringRef message, WKStringRef description, WKStringRef success, const void* clientInfo);

struct WKBundlePageDiagnosticLoggingClient {
    int                                                                 version;
    const void *                                                        clientInfo;
    WKBundlePageDiagnosticLoggingCallback                               logDiagnosticMessage;
};
typedef struct WKBundlePageDiagnosticLoggingClient WKBundlePageDiagnosticLoggingClient;

enum { kWKBundlePageDiagnosticLoggingClientCurrentVersion = 0 };

WK_EXPORT void WKBundlePageWillEnterFullScreen(WKBundlePageRef page);
WK_EXPORT void WKBundlePageDidEnterFullScreen(WKBundlePageRef page);
WK_EXPORT void WKBundlePageWillExitFullScreen(WKBundlePageRef page);
WK_EXPORT void WKBundlePageDidExitFullScreen(WKBundlePageRef page);

WK_EXPORT WKTypeID WKBundlePageGetTypeID();

WK_EXPORT void WKBundlePageSetContextMenuClient(WKBundlePageRef page, WKBundlePageContextMenuClient* client);
WK_EXPORT void WKBundlePageSetEditorClient(WKBundlePageRef page, WKBundlePageEditorClient* client);
WK_EXPORT void WKBundlePageSetFormClient(WKBundlePageRef page, WKBundlePageFormClient* client);
WK_EXPORT void WKBundlePageSetPageLoaderClient(WKBundlePageRef page, WKBundlePageLoaderClient* client);
WK_EXPORT void WKBundlePageSetResourceLoadClient(WKBundlePageRef page, WKBundlePageResourceLoadClient* client);
WK_EXPORT void WKBundlePageSetPolicyClient(WKBundlePageRef page, WKBundlePagePolicyClient* client);
WK_EXPORT void WKBundlePageSetUIClient(WKBundlePageRef page, WKBundlePageUIClient* client);
WK_EXPORT void WKBundlePageSetFullScreenClient(WKBundlePageRef page, WKBundlePageFullScreenClient* client);
WK_EXPORT void WKBundlePageSetDiagnosticLoggingClient(WKBundlePageRef page, WKBundlePageDiagnosticLoggingClient* client);

WK_EXPORT WKBundlePageGroupRef WKBundlePageGetPageGroup(WKBundlePageRef page);
WK_EXPORT WKBundleFrameRef WKBundlePageGetMainFrame(WKBundlePageRef page);

WK_EXPORT WKBundleBackForwardListRef WKBundlePageGetBackForwardList(WKBundlePageRef page);

WK_EXPORT void WKBundlePageSetUnderlayPage(WKBundlePageRef page, WKBundlePageRef pageUnderlay);

WK_EXPORT void WKBundlePageInstallPageOverlay(WKBundlePageRef page, WKBundlePageOverlayRef pageOverlay);
WK_EXPORT void WKBundlePageUninstallPageOverlay(WKBundlePageRef page, WKBundlePageOverlayRef pageOverlay);

WK_EXPORT void WKBundlePageInstallPageOverlayWithAnimation(WKBundlePageRef page, WKBundlePageOverlayRef pageOverlay);
WK_EXPORT void WKBundlePageUninstallPageOverlayWithAnimation(WKBundlePageRef page, WKBundlePageOverlayRef pageOverlay);

WK_EXPORT void WKBundlePageSetTopOverhangImage(WKBundlePageRef page, WKImageRef image);
WK_EXPORT void WKBundlePageSetBottomOverhangImage(WKBundlePageRef page, WKImageRef image);

WK_EXPORT void WKBundlePageSetHeaderBanner(WKBundlePageRef page, WKBundlePageBannerRef banner);
WK_EXPORT void WKBundlePageSetFooterBanner(WKBundlePageRef page, WKBundlePageBannerRef banner);

WK_EXPORT bool WKBundlePageHasLocalDataForURL(WKBundlePageRef page, WKURLRef url);
WK_EXPORT bool WKBundlePageCanHandleRequest(WKURLRequestRef request);

WK_EXPORT bool WKBundlePageFindString(WKBundlePageRef page, WKStringRef target, WKFindOptions findOptions);

WK_EXPORT WKImageRef WKBundlePageCreateSnapshotWithOptions(WKBundlePageRef page, WKRect rect, WKSnapshotOptions options);

// We should deprecate these functions in favor of just using WKBundlePageCreateSnapshotWithOptions.
WK_EXPORT WKImageRef WKBundlePageCreateSnapshotInViewCoordinates(WKBundlePageRef page, WKRect rect, WKImageOptions options);
WK_EXPORT WKImageRef WKBundlePageCreateSnapshotInDocumentCoordinates(WKBundlePageRef page, WKRect rect, WKImageOptions options);

// We should keep this function since it allows passing a scale factor, but we should re-name it to
// WKBundlePageCreateScaledSnapshotWithOptions.
WK_EXPORT WKImageRef WKBundlePageCreateScaledSnapshotInDocumentCoordinates(WKBundlePageRef page, WKRect rect, double scaleFactor, WKImageOptions options);

WK_EXPORT double WKBundlePageGetBackingScaleFactor(WKBundlePageRef page);

WK_EXPORT void WKBundlePageListenForLayoutMilestones(WKBundlePageRef page, WKLayoutMilestones milestones);

WK_EXPORT WKBundleInspectorRef WKBundlePageGetInspector(WKBundlePageRef page);

#ifdef __cplusplus
}
#endif

#endif /* WKBundlePage_h */
