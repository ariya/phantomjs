/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc.  All rights reserved.
 * Copyright (C) 2009, 2010, 2011 Appcelerator, Inc. All rights reserved.
 * Copyright (C) 2011 Brent Fulgham. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCfLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABIuLITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebView_H
#define WebView_H

#include "WebKit.h"
#include "WebFrame.h"
#include "WebPreferences.h"
#include <WebCore/COMPtr.h>
#include <WebCore/DragActions.h>
#include <WebCore/IntRect.h>
#include <WebCore/RefCountedGDIHandle.h>
#include <WebCore/SuspendableTimer.h>
#include <WebCore/WindowMessageListener.h>
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>

#if USE(ACCELERATED_COMPOSITING)
#include <WebCore/CACFLayerTreeHostClient.h>
#include <WebCore/GraphicsLayerClient.h>
#endif

#if ENABLE(FULLSCREEN_API)
#include <WebCore/FullScreenControllerClient.h>
#endif

namespace WebCore {
#if USE(ACCELERATED_COMPOSITING)
    class CACFLayerTreeHost;
#endif
    class FullScreenController;
#if PLATFORM(WIN) && USE(AVFOUNDATION)
    struct GraphicsDeviceAdapter;
#endif
}

namespace WebCore {
    class HistoryItem;
}

class FullscreenVideoController;
class WebBackForwardList;
class WebFrame;
class WebInspector;
class WebInspectorClient;

typedef WebCore::RefCountedGDIHandle<HBITMAP> RefCountedHBITMAP;
typedef WebCore::RefCountedGDIHandle<HRGN> RefCountedHRGN;

WebView* kit(WebCore::Page*);
WebCore::Page* core(IWebView*);

interface IDropTargetHelper;

class WebView 
    : public IWebView
    , public IWebViewPrivate
    , public IWebIBActions
    , public IWebViewCSS
    , public IWebViewEditing
    , public IWebViewUndoableEditing
    , public IWebViewEditingActions
    , public IWebNotificationObserver
    , public IDropTarget
    , WebCore::WindowMessageListener
#if USE(ACCELERATED_COMPOSITING)
    , WebCore::GraphicsLayerClient
    , WebCore::CACFLayerTreeHostClient
#endif
#if ENABLE(FULLSCREEN_API)
    , WebCore::FullScreenControllerClient
#endif
{
public:
    static WebView* createInstance();
protected:
    WebView();
    ~WebView();

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebView

    virtual HRESULT STDMETHODCALLTYPE canShowMIMEType( 
        /* [in] */ BSTR mimeType,
        /* [retval][out] */ BOOL *canShow);

    virtual HRESULT STDMETHODCALLTYPE canShowMIMETypeAsHTML( 
        /* [in] */ BSTR mimeType,
        /* [retval][out] */ BOOL *canShow);

    virtual HRESULT STDMETHODCALLTYPE MIMETypesShownAsHTML( 
        /* [retval][out] */ IEnumVARIANT **enumVariant);
    
    virtual HRESULT STDMETHODCALLTYPE setMIMETypesShownAsHTML( 
        /* [size_is][in] */ BSTR *mimeTypes,
        /* [in] */ int cMimeTypes);
    
    virtual HRESULT STDMETHODCALLTYPE URLFromPasteboard( 
        /* [in] */ IDataObject *pasteboard,
        /* [retval][out] */ BSTR *url);
    
    virtual HRESULT STDMETHODCALLTYPE URLTitleFromPasteboard( 
        /* [in] */ IDataObject *pasteboard,
        /* [retval][out] */ BSTR *urlTitle);
    
    virtual HRESULT STDMETHODCALLTYPE initWithFrame( 
        /* [in] */ RECT frame,
        /* [in] */ BSTR frameName,
        /* [in] */ BSTR groupName);

    virtual HRESULT STDMETHODCALLTYPE setAccessibilityDelegate(
        /* [in] */ IAccessibilityDelegate *d);

    virtual HRESULT STDMETHODCALLTYPE accessibilityDelegate(
        /* [out][retval] */ IAccessibilityDelegate **d);

    virtual HRESULT STDMETHODCALLTYPE setUIDelegate( 
        /* [in] */ IWebUIDelegate *d);
    
    virtual HRESULT STDMETHODCALLTYPE uiDelegate( 
        /* [out][retval] */ IWebUIDelegate **d);
    
    virtual HRESULT STDMETHODCALLTYPE setResourceLoadDelegate( 
        /* [in] */ IWebResourceLoadDelegate *d);
    
    virtual HRESULT STDMETHODCALLTYPE resourceLoadDelegate( 
        /* [out][retval] */ IWebResourceLoadDelegate **d);
    
    virtual HRESULT STDMETHODCALLTYPE setDownloadDelegate( 
        /* [in] */ IWebDownloadDelegate *d);
    
    virtual HRESULT STDMETHODCALLTYPE downloadDelegate( 
        /* [out][retval] */ IWebDownloadDelegate **d);
    
    virtual HRESULT STDMETHODCALLTYPE setFrameLoadDelegate( 
        /* [in] */ IWebFrameLoadDelegate *d);
    
    virtual HRESULT STDMETHODCALLTYPE frameLoadDelegate( 
        /* [out][retval] */ IWebFrameLoadDelegate **d);
    
    virtual HRESULT STDMETHODCALLTYPE setPolicyDelegate( 
        /* [in] */ IWebPolicyDelegate *d);
    
    virtual HRESULT STDMETHODCALLTYPE policyDelegate( 
        /* [out][retval] */ IWebPolicyDelegate **d);
    
    virtual HRESULT STDMETHODCALLTYPE mainFrame( 
        /* [out][retval] */ IWebFrame **frame);

    virtual HRESULT STDMETHODCALLTYPE focusedFrame( 
        /* [out][retval] */ IWebFrame **frame);
    
    virtual HRESULT STDMETHODCALLTYPE backForwardList( 
        /* [out][retval] */ IWebBackForwardList **list);
    
    virtual HRESULT STDMETHODCALLTYPE setMaintainsBackForwardList( 
        /* [in] */ BOOL flag);
    
    virtual HRESULT STDMETHODCALLTYPE goBack( 
        /* [retval][out] */ BOOL *succeeded);
    
    virtual HRESULT STDMETHODCALLTYPE goForward( 
        /* [retval][out] */ BOOL *succeeded);
    
    virtual HRESULT STDMETHODCALLTYPE goToBackForwardItem( 
        /* [in] */ IWebHistoryItem *item,
        /* [retval][out] */ BOOL *succeeded);
    
    virtual HRESULT STDMETHODCALLTYPE setTextSizeMultiplier( 
        /* [in] */ float multiplier);
    
    virtual HRESULT STDMETHODCALLTYPE textSizeMultiplier( 
        /* [retval][out] */ float *multiplier);
    
    virtual HRESULT STDMETHODCALLTYPE setApplicationNameForUserAgent( 
        /* [in] */ BSTR applicationName);
    
    virtual HRESULT STDMETHODCALLTYPE applicationNameForUserAgent( 
        /* [retval][out] */ BSTR *applicationName);
    
    virtual HRESULT STDMETHODCALLTYPE setCustomUserAgent( 
        /* [in] */ BSTR userAgentString);
    
    virtual HRESULT STDMETHODCALLTYPE customUserAgent( 
        /* [retval][out] */ BSTR *userAgentString);
    
    virtual HRESULT STDMETHODCALLTYPE userAgentForURL( 
        /* [in] */ BSTR url,
        /* [retval][out] */ BSTR *userAgent);
    
    virtual HRESULT STDMETHODCALLTYPE supportsTextEncoding( 
        /* [retval][out] */ BOOL *supports);
    
    virtual HRESULT STDMETHODCALLTYPE setCustomTextEncodingName( 
        /* [in] */ BSTR encodingName);
    
    virtual HRESULT STDMETHODCALLTYPE customTextEncodingName( 
        /* [retval][out] */ BSTR *encodingName);
    
    virtual HRESULT STDMETHODCALLTYPE setMediaStyle( 
        /* [in] */ BSTR media);
    
    virtual HRESULT STDMETHODCALLTYPE mediaStyle( 
        /* [retval][out] */ BSTR *media);
    
    virtual HRESULT STDMETHODCALLTYPE stringByEvaluatingJavaScriptFromString( 
        /* [in] */ BSTR script,
        /* [retval][out] */ BSTR *result);
    
    virtual HRESULT STDMETHODCALLTYPE windowScriptObject( 
        /* [retval][out] */ IWebScriptObject **webScriptObject);
    
    virtual HRESULT STDMETHODCALLTYPE setPreferences( 
        /* [in] */ IWebPreferences *prefs);
    
    virtual HRESULT STDMETHODCALLTYPE preferences( 
        /* [retval][out] */ IWebPreferences **prefs);
    
    virtual HRESULT STDMETHODCALLTYPE setPreferencesIdentifier( 
        /* [in] */ BSTR anIdentifier);
    
    virtual HRESULT STDMETHODCALLTYPE preferencesIdentifier( 
        /* [retval][out] */ BSTR *anIdentifier);
    
    virtual HRESULT STDMETHODCALLTYPE setHostWindow( 
        /* [in] */ OLE_HANDLE window);
    
    virtual HRESULT STDMETHODCALLTYPE hostWindow( 
        /* [retval][out] */ OLE_HANDLE *window);
    
    virtual HRESULT STDMETHODCALLTYPE searchFor( 
        /* [in] */ BSTR str,
        /* [in] */ BOOL forward,
        /* [in] */ BOOL caseFlag,
        /* [in] */ BOOL wrapFlag,
        /* [retval][out] */ BOOL *found);
    
    virtual HRESULT STDMETHODCALLTYPE registerViewClass( 
        /* [in] */ IWebDocumentView *view,
        /* [in] */ IWebDocumentRepresentation *representation,
        /* [in] */ BSTR forMIMEType);

    virtual HRESULT STDMETHODCALLTYPE setGroupName( 
        /* [in] */ BSTR groupName);
    
    virtual HRESULT STDMETHODCALLTYPE groupName( 
        /* [retval][out] */ BSTR *groupName);
    
    virtual HRESULT STDMETHODCALLTYPE estimatedProgress( 
        /* [retval][out] */ double *estimatedProgress);
    
    virtual HRESULT STDMETHODCALLTYPE isLoading( 
        /* [retval][out] */ BOOL *isLoading);
    
    virtual HRESULT STDMETHODCALLTYPE elementAtPoint( 
        /* [in] */ LPPOINT point,
        /* [retval][out] */ IPropertyBag **elementDictionary);
    
    virtual HRESULT STDMETHODCALLTYPE pasteboardTypesForSelection( 
        /* [retval][out] */ IEnumVARIANT **enumVariant);
    
    virtual HRESULT STDMETHODCALLTYPE writeSelectionWithPasteboardTypes( 
        /* [size_is][in] */ BSTR *types,
        /* [in] */ int cTypes,
        /* [in] */ IDataObject *pasteboard);
    
    virtual HRESULT STDMETHODCALLTYPE pasteboardTypesForElement( 
        /* [in] */ IPropertyBag *elementDictionary,
        /* [retval][out] */ IEnumVARIANT **enumVariant);
    
    virtual HRESULT STDMETHODCALLTYPE writeElement( 
        /* [in] */ IPropertyBag *elementDictionary,
        /* [size_is][in] */ BSTR *withPasteboardTypes,
        /* [in] */ int cWithPasteboardTypes,
        /* [in] */ IDataObject *pasteboard);
    
    virtual HRESULT STDMETHODCALLTYPE selectedText(
        /* [out, retval] */ BSTR* str);

    virtual HRESULT STDMETHODCALLTYPE centerSelectionInVisibleArea(
        /* [in] */ IUnknown* sender);

    virtual HRESULT STDMETHODCALLTYPE moveDragCaretToPoint( 
        /* [in] */ LPPOINT point);
    
    virtual HRESULT STDMETHODCALLTYPE removeDragCaret( void);
    
    virtual HRESULT STDMETHODCALLTYPE setDrawsBackground( 
        /* [in] */ BOOL drawsBackground);
    
    virtual HRESULT STDMETHODCALLTYPE drawsBackground( 
        /* [retval][out] */ BOOL *drawsBackground);
    
    virtual HRESULT STDMETHODCALLTYPE setMainFrameURL( 
        /* [in] */ BSTR urlString);
    
    virtual HRESULT STDMETHODCALLTYPE mainFrameURL( 
        /* [retval][out] */ BSTR *urlString);
    
    virtual HRESULT STDMETHODCALLTYPE mainFrameDocument( 
        /* [retval][out] */ IDOMDocument **document);
    
    virtual HRESULT STDMETHODCALLTYPE mainFrameTitle( 
        /* [retval][out] */ BSTR *title);
    
    virtual HRESULT STDMETHODCALLTYPE mainFrameIcon( 
        /* [retval][out] */ OLE_HANDLE *hBitmap);

    virtual HRESULT STDMETHODCALLTYPE registerURLSchemeAsLocal( 
        /* [in] */ BSTR scheme);

    virtual HRESULT STDMETHODCALLTYPE close();

    // IWebIBActions

    virtual HRESULT STDMETHODCALLTYPE takeStringURLFrom( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE stopLoading( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE reload( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE canGoBack( 
        /* [in] */ IUnknown *sender,
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE goBack( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE canGoForward( 
        /* [in] */ IUnknown *sender,
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE goForward( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE canMakeTextLarger( 
        /* [in] */ IUnknown *sender,
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE makeTextLarger( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE canMakeTextSmaller( 
        /* [in] */ IUnknown *sender,
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE makeTextSmaller( 
        /* [in] */ IUnknown *sender);

    virtual HRESULT STDMETHODCALLTYPE canMakeTextStandardSize( 
        /* [in] */ IUnknown *sender,
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE makeTextStandardSize( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE toggleContinuousSpellChecking( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE toggleSmartInsertDelete( 
        /* [in] */ IUnknown *sender);

    virtual HRESULT STDMETHODCALLTYPE toggleGrammarChecking( 
        /* [in] */ IUnknown *sender);

    virtual HRESULT STDMETHODCALLTYPE reloadFromOrigin( 
        /* [in] */ IUnknown *sender);

    // IWebViewCSS

    virtual HRESULT STDMETHODCALLTYPE computedStyleForElement( 
        /* [in] */ IDOMElement *element,
        /* [in] */ BSTR pseudoElement,
        /* [retval][out] */ IDOMCSSStyleDeclaration **style);

    // IWebViewEditing

    virtual HRESULT STDMETHODCALLTYPE editableDOMRangeForPoint( 
        /* [in] */ LPPOINT point,
        /* [retval][out] */ IDOMRange **range);
    
    virtual HRESULT STDMETHODCALLTYPE setSelectedDOMRange( 
        /* [in] */ IDOMRange *range,
        /* [in] */ WebSelectionAffinity affinity);
    
    virtual HRESULT STDMETHODCALLTYPE selectedDOMRange( 
        /* [retval][out] */ IDOMRange **range);
    
    virtual HRESULT STDMETHODCALLTYPE selectionAffinity( 
        /* [retval][out][retval][out] */ WebSelectionAffinity *affinity);
    
    virtual HRESULT STDMETHODCALLTYPE setEditable( 
        /* [in] */ BOOL flag);
    
    virtual HRESULT STDMETHODCALLTYPE isEditable( 
        /* [retval][out] */ BOOL *isEditable);
    
    virtual HRESULT STDMETHODCALLTYPE setTypingStyle( 
        /* [in] */ IDOMCSSStyleDeclaration *style);
    
    virtual HRESULT STDMETHODCALLTYPE typingStyle( 
        /* [retval][out] */ IDOMCSSStyleDeclaration **style);
    
    virtual HRESULT STDMETHODCALLTYPE setSmartInsertDeleteEnabled( 
        /* [in] */ BOOL flag);
    
    virtual HRESULT STDMETHODCALLTYPE smartInsertDeleteEnabled( 
        /* [in] */ BOOL *enabled);

    virtual HRESULT STDMETHODCALLTYPE setSelectTrailingWhitespaceEnabled( 
        /* [in] */ BOOL flag);
    
    virtual HRESULT STDMETHODCALLTYPE isSelectTrailingWhitespaceEnabled( 
        /* [in] */ BOOL *enabled);

    virtual HRESULT STDMETHODCALLTYPE setContinuousSpellCheckingEnabled( 
        /* [in] */ BOOL flag);
    
    virtual HRESULT STDMETHODCALLTYPE isContinuousSpellCheckingEnabled( 
        /* [retval][out] */ BOOL *enabled);
    
    virtual HRESULT STDMETHODCALLTYPE spellCheckerDocumentTag( 
        /* [retval][out] */ int *tag);
    
    virtual HRESULT STDMETHODCALLTYPE undoManager( 
        /* [retval][out] */ IWebUndoManager **manager);
    
    virtual HRESULT STDMETHODCALLTYPE setEditingDelegate( 
        /* [in] */ IWebEditingDelegate *d);
    
    virtual HRESULT STDMETHODCALLTYPE editingDelegate( 
        /* [retval][out] */ IWebEditingDelegate **d);
    
    virtual HRESULT STDMETHODCALLTYPE styleDeclarationWithText( 
        /* [in] */ BSTR text,
        /* [retval][out] */ IDOMCSSStyleDeclaration **style);
    
    virtual HRESULT STDMETHODCALLTYPE hasSelectedRange( 
        /* [retval][out] */ BOOL *hasSelectedRange);
    
    virtual HRESULT STDMETHODCALLTYPE cutEnabled( 
        /* [retval][out] */ BOOL *enabled);
    
    virtual HRESULT STDMETHODCALLTYPE copyEnabled( 
        /* [retval][out] */ BOOL *enabled);
    
    virtual HRESULT STDMETHODCALLTYPE pasteEnabled( 
        /* [retval][out] */ BOOL *enabled);
    
    virtual HRESULT STDMETHODCALLTYPE deleteEnabled( 
        /* [retval][out] */ BOOL *enabled);

    virtual HRESULT STDMETHODCALLTYPE editingEnabled( 
        /* [retval][out] */ BOOL *enabled);

    virtual HRESULT STDMETHODCALLTYPE isGrammarCheckingEnabled( 
        /* [retval][out] */ BOOL *enabled);
    
    virtual HRESULT STDMETHODCALLTYPE setGrammarCheckingEnabled( 
        BOOL enabled);

    virtual HRESULT STDMETHODCALLTYPE setPageSizeMultiplier( 
        /* [in] */ float multiplier);
    
    virtual HRESULT STDMETHODCALLTYPE pageSizeMultiplier( 
        /* [retval][out] */ float *multiplier);

    virtual HRESULT STDMETHODCALLTYPE canZoomPageIn( 
        /* [in] */ IUnknown *sender,
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE zoomPageIn( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE canZoomPageOut( 
        /* [in] */ IUnknown *sender,
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE zoomPageOut( 
        /* [in] */ IUnknown *sender);

    virtual HRESULT STDMETHODCALLTYPE canResetPageZoom( 
        /* [in] */ IUnknown *sender,
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE resetPageZoom( 
        /* [in] */ IUnknown *sender);

    // IWebViewUndoableEditing

    virtual HRESULT STDMETHODCALLTYPE replaceSelectionWithNode( 
        /* [in] */ IDOMNode *node);
    
    virtual HRESULT STDMETHODCALLTYPE replaceSelectionWithText( 
        /* [in] */ BSTR text);
    
    virtual HRESULT STDMETHODCALLTYPE replaceSelectionWithMarkupString( 
        /* [in] */ BSTR markupString);
    
    virtual HRESULT STDMETHODCALLTYPE replaceSelectionWithArchive( 
        /* [in] */ IWebArchive *archive);
    
    virtual HRESULT STDMETHODCALLTYPE deleteSelection( void);

    virtual HRESULT STDMETHODCALLTYPE clearSelection(void);
    
    virtual HRESULT STDMETHODCALLTYPE applyStyle( 
        /* [in] */ IDOMCSSStyleDeclaration *style);

    // IWebViewEditingActions

    virtual HRESULT STDMETHODCALLTYPE copy( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE cut( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE paste( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE copyURL( 
        /* [in] */ BSTR url);

    virtual HRESULT STDMETHODCALLTYPE copyFont( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE pasteFont( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE delete_( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE pasteAsPlainText( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE pasteAsRichText( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE changeFont( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE changeAttributes( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE changeDocumentBackgroundColor( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE changeColor( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE alignCenter( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE alignJustified( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE alignLeft( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE alignRight( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE checkSpelling( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE showGuessPanel( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE performFindPanelAction( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE startSpeaking( 
        /* [in] */ IUnknown *sender);
    
    virtual HRESULT STDMETHODCALLTYPE stopSpeaking( 
        /* [in] */ IUnknown *sender);

    // IWebNotificationObserver

    virtual HRESULT STDMETHODCALLTYPE onNotify( 
        /* [in] */ IWebNotification *notification);

    // IWebViewPrivate

    virtual HRESULT STDMETHODCALLTYPE MIMETypeForExtension(
        /* [in] */ BSTR extension,
        /* [retval][out] */ BSTR *mimeType);

    virtual HRESULT STDMETHODCALLTYPE setCustomDropTarget(
        /* [in] */ IDropTarget* dt);

    virtual HRESULT STDMETHODCALLTYPE removeCustomDropTarget();

    virtual HRESULT STDMETHODCALLTYPE setInViewSourceMode( 
        /* [in] */ BOOL flag);
    
    virtual HRESULT STDMETHODCALLTYPE inViewSourceMode( 
        /* [retval][out] */ BOOL* flag);

    virtual HRESULT STDMETHODCALLTYPE viewWindow( 
        /* [retval][out] */ OLE_HANDLE *window);

    virtual HRESULT STDMETHODCALLTYPE setFormDelegate( 
        /* [in] */ IWebFormDelegate *formDelegate);

    virtual HRESULT STDMETHODCALLTYPE formDelegate( 
        /* [retval][out] */ IWebFormDelegate **formDelegate);

    virtual HRESULT STDMETHODCALLTYPE setFrameLoadDelegatePrivate( 
        /* [in] */ IWebFrameLoadDelegatePrivate *frameLoadDelegatePrivate);

    virtual HRESULT STDMETHODCALLTYPE frameLoadDelegatePrivate( 
        /* [retval][out] */ IWebFrameLoadDelegatePrivate **frameLoadDelegatePrivate);

    virtual HRESULT STDMETHODCALLTYPE scrollOffset( 
        /* [retval][out] */ LPPOINT offset);

    virtual HRESULT STDMETHODCALLTYPE scrollBy( 
        /* [in] */ LPPOINT offset);

    virtual HRESULT STDMETHODCALLTYPE visibleContentRect( 
        /* [retval][out] */ LPRECT rect);

    virtual HRESULT STDMETHODCALLTYPE updateFocusedAndActiveState();

    virtual HRESULT STDMETHODCALLTYPE executeCoreCommandByName(BSTR name, BSTR value);

    virtual HRESULT STDMETHODCALLTYPE clearMainFrameName();

    virtual HRESULT STDMETHODCALLTYPE markAllMatchesForText(
        BSTR search, BOOL caseSensitive, BOOL highlight, UINT limit, UINT* matches);

    virtual HRESULT STDMETHODCALLTYPE unmarkAllTextMatches();

    virtual HRESULT STDMETHODCALLTYPE rectsForTextMatches(
        IEnumTextMatches** pmatches);

    virtual HRESULT STDMETHODCALLTYPE generateSelectionImage(
        BOOL forceWhiteText, OLE_HANDLE* hBitmap);

    virtual HRESULT STDMETHODCALLTYPE selectionRect(
        RECT* rc);
    
    virtual HRESULT STDMETHODCALLTYPE DragEnter(
        IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

    virtual HRESULT STDMETHODCALLTYPE DragOver(
        DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    
    virtual HRESULT STDMETHODCALLTYPE DragLeave();
    
    virtual HRESULT STDMETHODCALLTYPE Drop(
        IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

    virtual HRESULT STDMETHODCALLTYPE canHandleRequest( 
        IWebURLRequest *request,
        BOOL *result);

    virtual HRESULT STDMETHODCALLTYPE standardUserAgentWithApplicationName( 
        /* [in] */ BSTR applicationName,
        /* [retval][out] */ BSTR *groupName);

    virtual HRESULT STDMETHODCALLTYPE clearFocusNode();

    virtual HRESULT STDMETHODCALLTYPE setInitialFocus(
        /* [in] */ BOOL forward);
    
    virtual HRESULT STDMETHODCALLTYPE setTabKeyCyclesThroughElements( 
        /* [in] */ BOOL cycles);
    
    virtual HRESULT STDMETHODCALLTYPE tabKeyCyclesThroughElements( 
        /* [retval][out] */ BOOL *result);

    virtual HRESULT STDMETHODCALLTYPE setAllowSiteSpecificHacks(
        /* [in] */ BOOL allows);

    virtual HRESULT STDMETHODCALLTYPE addAdditionalPluginDirectory( 
        /* [in] */ BSTR directory);    

    virtual HRESULT STDMETHODCALLTYPE loadBackForwardListFromOtherView( 
        /* [in] */ IWebView *otherView);
        
    virtual HRESULT STDMETHODCALLTYPE inspector(
        /* [retval][out] */ IWebInspector**);

    virtual HRESULT STDMETHODCALLTYPE clearUndoRedoOperations( void);
    virtual HRESULT STDMETHODCALLTYPE shouldClose( 
        /* [retval][out] */ BOOL* result);

    virtual HRESULT STDMETHODCALLTYPE setProhibitsMainFrameScrolling(BOOL);
    virtual HRESULT STDMETHODCALLTYPE setShouldApplyMacFontAscentHack(BOOL);

    virtual HRESULT STDMETHODCALLTYPE windowAncestryDidChange();

    virtual HRESULT STDMETHODCALLTYPE paintDocumentRectToContext(
        /* [in] */ RECT rect,
        /* [in] */ OLE_HANDLE dc);

    virtual HRESULT STDMETHODCALLTYPE paintScrollViewRectToContextAtPoint(
        /* [in] */ RECT rect,
        /* [in] */ POINT pt,
        /* [in] */ OLE_HANDLE dc);

    virtual HRESULT STDMETHODCALLTYPE reportException(
        /* [in] */ JSContextRef context,
        /* [in] */ JSValueRef exception);

    virtual HRESULT STDMETHODCALLTYPE elementFromJS(
        /* [in] */ JSContextRef context,
        /* [in] */ JSValueRef nodeObject,
        /* [retval][out] */ IDOMElement **element);

    virtual HRESULT STDMETHODCALLTYPE setCustomHTMLTokenizerTimeDelay(
        /* [in] */ double timeDelay);

    virtual HRESULT STDMETHODCALLTYPE setCustomHTMLTokenizerChunkSize(
        /* [in] */ int chunkSize);

    virtual HRESULT STDMETHODCALLTYPE backingStore(
        /* [out, retval] */ OLE_HANDLE* hBitmap);

    virtual HRESULT STDMETHODCALLTYPE setTransparent(
        /* [in] */ BOOL transparent);

    virtual HRESULT STDMETHODCALLTYPE transparent(
        /* [out, retval] */ BOOL* transparent);

    virtual HRESULT STDMETHODCALLTYPE setDefersCallbacks(
        /* [in] */ BOOL defersCallbacks);

    virtual HRESULT STDMETHODCALLTYPE defersCallbacks(
        /* [out, retval] */ BOOL* defersCallbacks);

    virtual HRESULT STDMETHODCALLTYPE globalHistoryItem(
        /* [out, retval] */ IWebHistoryItem** item);

    virtual HRESULT STDMETHODCALLTYPE setAlwaysUsesComplexTextCodePath(
        /* [in] */ BOOL complex);

    virtual HRESULT STDMETHODCALLTYPE alwaysUsesComplexTextCodePath(
        /* [out, retval] */ BOOL* complex);

    virtual HRESULT STDMETHODCALLTYPE setCookieEnabled(
        /* [in] */ BOOL enable);

    virtual HRESULT STDMETHODCALLTYPE cookieEnabled(
        /* [out, retval] */ BOOL* enabled);

    virtual HRESULT STDMETHODCALLTYPE setMediaVolume(
        /* [in] */ float volume);

    virtual HRESULT STDMETHODCALLTYPE mediaVolume(
        /* [out, retval] */ float* volume);

    virtual HRESULT STDMETHODCALLTYPE registerEmbeddedViewMIMEType( 
        /* [in] */ BSTR mimeType);

    virtual HRESULT STDMETHODCALLTYPE setMemoryCacheDelegateCallsEnabled( 
        /* [in] */ BOOL enabled);

    virtual HRESULT STDMETHODCALLTYPE setJavaScriptURLsAreAllowed(
        /* [in] */ BOOL areAllowed);

    virtual HRESULT STDMETHODCALLTYPE setCanStartPlugins(
        /* [in] */ BOOL canStartPlugins);

    virtual HRESULT STDMETHODCALLTYPE addUserScriptToGroup(BSTR groupName, IWebScriptWorld*, BSTR source, BSTR url,
                                                           unsigned whitelistCount, BSTR* whitelist, 
                                                           unsigned blacklistCount, BSTR* blacklist,
                                                           WebUserScriptInjectionTime);
    virtual HRESULT STDMETHODCALLTYPE addUserStyleSheetToGroup(BSTR groupName, IWebScriptWorld*, BSTR source, BSTR url,
                                                               unsigned whitelistCount, BSTR* whitelist, 
                                                               unsigned blacklistCount, BSTR* blacklist);
    virtual HRESULT STDMETHODCALLTYPE removeUserScriptFromGroup(BSTR groupName, IWebScriptWorld*, BSTR url);
    virtual HRESULT STDMETHODCALLTYPE removeUserStyleSheetFromGroup(BSTR groupName, IWebScriptWorld*, BSTR url);
    virtual HRESULT STDMETHODCALLTYPE removeUserScriptsFromGroup(BSTR groupName, IWebScriptWorld*);
    virtual HRESULT STDMETHODCALLTYPE removeUserStyleSheetsFromGroup(BSTR groupName, IWebScriptWorld*);
    virtual HRESULT STDMETHODCALLTYPE removeAllUserContentFromGroup(BSTR groupName);

    virtual HRESULT STDMETHODCALLTYPE unused1();
    virtual HRESULT STDMETHODCALLTYPE unused2();

    virtual HRESULT STDMETHODCALLTYPE invalidateBackingStore(const RECT*);

    virtual HRESULT STDMETHODCALLTYPE addOriginAccessWhitelistEntry(BSTR sourceOrigin, BSTR destinationProtocol, BSTR destinationHost, BOOL allowDestinationSubdomains);
    virtual HRESULT STDMETHODCALLTYPE removeOriginAccessWhitelistEntry(BSTR sourceOrigin, BSTR destinationProtocol, BSTR destinationHost, BOOL allowDestinationSubdomains);
    virtual HRESULT STDMETHODCALLTYPE resetOriginAccessWhitelists();

    virtual HRESULT STDMETHODCALLTYPE setHistoryDelegate(IWebHistoryDelegate* historyDelegate);
    virtual HRESULT STDMETHODCALLTYPE historyDelegate(IWebHistoryDelegate** historyDelegate);
    virtual HRESULT STDMETHODCALLTYPE addVisitedLinks(BSTR* visitedURLs, unsigned visitedURLCount);

    virtual HRESULT STDMETHODCALLTYPE unused3();
    virtual HRESULT STDMETHODCALLTYPE unused4();
    virtual HRESULT STDMETHODCALLTYPE unused5();

    virtual HRESULT STDMETHODCALLTYPE setGeolocationProvider(IWebGeolocationProvider* locationProvider);
    virtual HRESULT STDMETHODCALLTYPE geolocationProvider(IWebGeolocationProvider** locationProvider);
    virtual HRESULT STDMETHODCALLTYPE geolocationDidChangePosition(IWebGeolocationPosition* position);
    virtual HRESULT STDMETHODCALLTYPE geolocationDidFailWithError(IWebError* error);

    virtual HRESULT STDMETHODCALLTYPE setDomainRelaxationForbiddenForURLScheme(BOOL forbidden, BSTR scheme);
    virtual HRESULT STDMETHODCALLTYPE registerURLSchemeAsSecure(BSTR);
    virtual HRESULT STDMETHODCALLTYPE registerURLSchemeAsAllowingLocalStorageAccessInPrivateBrowsing(BSTR);
    virtual HRESULT STDMETHODCALLTYPE registerURLSchemeAsAllowingDatabaseAccessInPrivateBrowsing(BSTR);

    virtual HRESULT STDMETHODCALLTYPE nextDisplayIsSynchronous();

    virtual HRESULT STDMETHODCALLTYPE defaultMinimumTimerInterval(
        /* [retval][out] */ double *interval);

    virtual HRESULT STDMETHODCALLTYPE setMinimumTimerInterval(
        /* [in] */ double);

    virtual HRESULT STDMETHODCALLTYPE httpPipeliningEnabled(
        /* [out, retval] */ BOOL* enabled);

    virtual HRESULT STDMETHODCALLTYPE setHTTPPipeliningEnabled(
        /* [in] */ BOOL);

    virtual HRESULT STDMETHODCALLTYPE setUsesLayeredWindow(BOOL);
    virtual HRESULT STDMETHODCALLTYPE usesLayeredWindow(BOOL*);

    // WebView
    bool shouldUseEmbeddedView(const WTF::String& mimeType) const;

    WebCore::Page* page();
    bool handleMouseEvent(UINT, WPARAM, LPARAM);
    void setMouseActivated(bool flag) { m_mouseActivated = flag; }
    bool handleContextMenuEvent(WPARAM, LPARAM);
    bool onMeasureItem(WPARAM, LPARAM);
    bool onDrawItem(WPARAM, LPARAM);
    bool onInitMenuPopup(WPARAM, LPARAM);
    bool onUninitMenuPopup(WPARAM, LPARAM);
    void performContextMenuAction(WPARAM, LPARAM, bool byPosition);
    bool mouseWheel(WPARAM, LPARAM, bool isMouseHWheel);
    bool verticalScroll(WPARAM, LPARAM);
    bool horizontalScroll(WPARAM, LPARAM);
    bool gesture(WPARAM, LPARAM);
    bool gestureNotify(WPARAM, LPARAM);
    bool execCommand(WPARAM wParam, LPARAM lParam);
    bool keyDown(WPARAM, LPARAM, bool systemKeyDown = false);
    bool keyUp(WPARAM, LPARAM, bool systemKeyDown = false);
    bool keyPress(WPARAM, LPARAM, bool systemKeyDown = false);
    void paint(HDC, LPARAM);
    void paintIntoWindow(HDC bitmapDC, HDC windowDC, const WebCore::IntRect& dirtyRect);
    bool ensureBackingStore();
    void addToDirtyRegion(const WebCore::IntRect&);
    void addToDirtyRegion(HRGN);
    void scrollBackingStore(WebCore::FrameView*, int dx, int dy, const WebCore::IntRect& scrollViewRect, const WebCore::IntRect& clipRect);
    void deleteBackingStore();
    void repaint(const WebCore::IntRect&, bool contentChanged, bool immediate = false, bool repaintContentOnly = false);
    void frameRect(RECT* rect);
    void closeWindow();
    void closeWindowSoon();
    void closeWindowTimerFired();
    bool didClose() const { return m_didClose; }

    bool transparent() const { return m_transparent; }
    bool usesLayeredWindow() const { return m_usesLayeredWindow; }

    bool onIMEStartComposition();
    bool onIMEComposition(LPARAM);
    bool onIMEEndComposition();
    bool onIMEChar(WPARAM, LPARAM);
    bool onIMENotify(WPARAM, LPARAM, LRESULT*);
    LRESULT onIMERequest(WPARAM, LPARAM);
    bool onIMESelect(WPARAM, LPARAM);
    bool onIMESetContext(WPARAM, LPARAM);
    void selectionChanged();
    void resetIME(WebCore::Frame*);
    void setInputMethodState(bool);

    HRESULT registerDragDrop();
    HRESULT revokeDragDrop();

    // Convenient to be able to violate the rules of COM here for easy movement to the frame.
    WebFrame* topLevelFrame() const { return m_mainFrame; }
    const WTF::String& userAgentForKURL(const WebCore::KURL& url);

    static bool canHandleRequest(const WebCore::ResourceRequest&);

    static WTF::String standardUserAgentWithApplicationName(const WTF::String&);

    void setIsBeingDestroyed();
    bool isBeingDestroyed() const { return m_isBeingDestroyed; }

    const char* interpretKeyEvent(const WebCore::KeyboardEvent*);
    bool handleEditingKeyboardEvent(WebCore::KeyboardEvent*);

    bool isPainting() const { return m_paintCount > 0; }

    void setToolTip(const WTF::String&);

    void registerForIconNotification(bool listen);
    void dispatchDidReceiveIconFromWebFrame(WebFrame*);

    HRESULT notifyDidAddIcon(IWebNotification*);
    HRESULT notifyPreferencesChanged(IWebNotification*);

    static void setCacheModel(WebCacheModel);
    static WebCacheModel cacheModel();
    static bool didSetCacheModel();
    static WebCacheModel maxCacheModelInAnyInstance();

    void updateActiveStateSoon() const;
    void deleteBackingStoreSoon();
    void cancelDeleteBackingStoreSoon();

    HWND topLevelParent() const { return m_topLevelParent; }
    HWND viewWindow() const { return m_viewWindow; }

    void updateActiveState();

    bool onGetObject(WPARAM, LPARAM, LRESULT&) const;
    static STDMETHODIMP AccessibleObjectFromWindow(HWND, DWORD objectID, REFIID, void** ppObject);

    void downloadURL(const WebCore::KURL&);

#if USE(ACCELERATED_COMPOSITING)
    void flushPendingGraphicsLayerChangesSoon();
    void setRootChildLayer(WebCore::GraphicsLayer*);
#endif

#if PLATFORM(WIN) && USE(AVFOUNDATION)
    WebCore::GraphicsDeviceAdapter* graphicsDeviceAdapter() const;
#endif

    void enterFullscreenForNode(WebCore::Node*);
    void exitFullscreen();

    void setLastCursor(HCURSOR cursor) { m_lastSetCursor = cursor; }

    void setGlobalHistoryItem(WebCore::HistoryItem*);

#if ENABLE(FULLSCREEN_API)
    bool supportsFullScreenForElement(const WebCore::Element*, bool withKeyboard) const;
    bool isFullScreen() const;
    WebCore::FullScreenController* fullScreenController();
    void setFullScreenElement(PassRefPtr<WebCore::Element>);
    WebCore::Element* fullScreenElement() const { return m_fullScreenElement.get(); }
#endif

    // Used by TextInputController in DumpRenderTree

    HRESULT STDMETHODCALLTYPE setCompositionForTesting(
        /* [in] */ BSTR composition, 
        /* [in] */ UINT from, 
        /* [in] */ UINT length);

    HRESULT STDMETHODCALLTYPE hasCompositionForTesting(/* [out, retval] */ BOOL* result);

    HRESULT STDMETHODCALLTYPE confirmCompositionForTesting(/* [in] */ BSTR composition);

    HRESULT STDMETHODCALLTYPE compositionRangeForTesting(/* [out] */ UINT* startPosition, /* [out] */ UINT* length);

    HRESULT STDMETHODCALLTYPE firstRectForCharacterRangeForTesting(
    /* [in] */ UINT location, 
    /* [in] */ UINT length, 
    /* [out, retval] */ RECT* resultRect);

    HRESULT STDMETHODCALLTYPE selectedRangeForTesting(/* [out] */ UINT* location, /* [out] */ UINT* length);
private:
    void setZoomMultiplier(float multiplier, bool isTextOnly);
    float zoomMultiplier(bool isTextOnly);
    bool canZoomIn(bool isTextOnly);
    HRESULT zoomIn(bool isTextOnly);
    bool canZoomOut(bool isTextOnly);
    HRESULT zoomOut(bool isTextOnly);
    bool canResetZoom(bool isTextOnly);
    HRESULT resetZoom(bool isTextOnly);
    bool active();

    void sizeChanged(const WebCore::IntSize&);

    enum WindowsToPaint { PaintWebViewOnly, PaintWebViewAndChildren };
    void paintIntoBackingStore(WebCore::FrameView*, HDC bitmapDC, const WebCore::IntRect& dirtyRect, WindowsToPaint);
    void updateBackingStore(WebCore::FrameView*, HDC = 0, bool backingStoreCompletelyDirty = false, WindowsToPaint = PaintWebViewOnly);

    void performLayeredWindowUpdate();

    WebCore::DragOperation keyStateToDragOperation(DWORD grfKeyState) const;

    // FIXME: This variable is part of a workaround. The drop effect (pdwEffect) passed to Drop is incorrect. 
    // We set this variable in DragEnter and DragOver so that it can be used in Drop to set the correct drop effect. 
    // Thus, on return from DoDragDrop we have the correct pdwEffect for the drag-and-drop operation.
    // (see https://bugs.webkit.org/show_bug.cgi?id=29264)
    DWORD m_lastDropEffect;

#if USE(ACCELERATED_COMPOSITING)
    // GraphicsLayerClient
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double time);
    virtual void notifyFlushRequired(const WebCore::GraphicsLayer*);
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect& inClip);

    // CACFLayerTreeHostClient
    virtual void flushPendingGraphicsLayerChanges();
#endif

    bool m_shouldInvertColors;
    void setShouldInvertColors(bool);

protected:
    static bool registerWebViewWindowClass();
    static LRESULT CALLBACK WebViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    HIMC getIMMContext();
    void releaseIMMContext(HIMC);
    static bool allowSiteSpecificHacks() { return s_allowSiteSpecificHacks; } 
    void preflightSpellChecker();
    bool continuousCheckingAllowed();
    void initializeToolTipWindow();
    void prepareCandidateWindow(WebCore::Frame*, HIMC);
    void updateSelectionForIME();
    LRESULT onIMERequestCharPosition(WebCore::Frame*, IMECHARPOSITION*);
    LRESULT onIMERequestReconvertString(WebCore::Frame*, RECONVERTSTRING*);
    bool developerExtrasEnabled() const;

    bool shouldInitializeTrackPointHack();

    // AllWebViewSet functions
    void addToAllWebViewsSet();
    void removeFromAllWebViewsSet();

    virtual void windowReceivedMessage(HWND, UINT message, WPARAM, LPARAM);

#if ENABLE(FULLSCREEN_API)
    virtual HWND fullScreenClientWindow() const;
    virtual HWND fullScreenClientParentWindow() const;
    virtual void fullScreenClientSetParentWindow(HWND);
    virtual void fullScreenClientWillEnterFullScreen();
    virtual void fullScreenClientDidEnterFullScreen();
    virtual void fullScreenClientWillExitFullScreen();
    virtual void fullScreenClientDidExitFullScreen();
    virtual void fullScreenClientForceRepaint();
    virtual void fullScreenClientSaveScrollPosition();
    virtual void fullScreenClientRestoreScrollPosition();
#endif

    ULONG m_refCount;
#if !ASSERT_DISABLED
    bool m_deletionHasBegun;
#endif
    HWND m_hostWindow;
    HWND m_viewWindow;
    WebFrame* m_mainFrame;
    WebCore::Page* m_page;
#if ENABLE(INSPECTOR)
    WebInspectorClient* m_inspectorClient;
#endif // ENABLE(INSPECTOR)
    
    RefPtr<RefCountedHBITMAP> m_backingStoreBitmap;
    SIZE m_backingStoreSize;
    RefPtr<RefCountedHRGN> m_backingStoreDirtyRegion;

    COMPtr<IAccessibilityDelegate> m_accessibilityDelegate;
    COMPtr<IWebEditingDelegate> m_editingDelegate;
    COMPtr<IWebFrameLoadDelegate> m_frameLoadDelegate;
    COMPtr<IWebFrameLoadDelegatePrivate> m_frameLoadDelegatePrivate;
    COMPtr<IWebUIDelegate> m_uiDelegate;
    COMPtr<IWebUIDelegatePrivate> m_uiDelegatePrivate;
    COMPtr<IWebFormDelegate> m_formDelegate;
    COMPtr<IWebPolicyDelegate> m_policyDelegate;
    COMPtr<IWebResourceLoadDelegate> m_resourceLoadDelegate;
    COMPtr<IWebDownloadDelegate> m_downloadDelegate;
    COMPtr<IWebHistoryDelegate> m_historyDelegate;
    COMPtr<WebPreferences> m_preferences;
#if ENABLE(INSPECTOR)
    COMPtr<WebInspector> m_webInspector;
#endif // ENABLE(INSPECTOR)
    COMPtr<IWebGeolocationProvider> m_geolocationProvider;

    bool m_userAgentOverridden;
    bool m_useBackForwardList;
    WTF::String m_userAgentCustom;
    WTF::String m_userAgentStandard;
    float m_zoomMultiplier;
    bool m_zoomsTextOnly;
    WTF::String m_overrideEncoding;
    WTF::String m_applicationName;
    bool m_mouseActivated;
    // WebCore dragging logic needs to be able to inspect the drag data
    // this is updated in DragEnter/Leave/Drop
    COMPtr<IDataObject> m_dragData;
    COMPtr<IDropTargetHelper> m_dropTargetHelper;
    UChar m_currentCharacterCode;
    bool m_isBeingDestroyed;
    unsigned m_paintCount;
    bool m_hasSpellCheckerDocumentTag;
    bool m_didClose;
    bool m_hasCustomDropTarget;
    unsigned m_inIMEComposition;
    HWND m_toolTipHwnd;
    WTF::String m_toolTip;
    bool m_deleteBackingStoreTimerActive;

    bool m_transparent;

    static bool s_allowSiteSpecificHacks;

    WebCore::SuspendableTimer* m_closeWindowTimer;
    OwnPtr<TRACKMOUSEEVENT> m_mouseOutTracker;

    HWND m_topLevelParent;

    OwnPtr<HashSet<WTF::String> > m_embeddedViewMIMETypes;

    //Variables needed to store gesture information
    RefPtr<WebCore::Node> m_gestureTargetNode;
    long m_lastPanX;
    long m_lastPanY;
    long m_xOverpan;
    long m_yOverpan;

#if ENABLE(VIDEO)
    OwnPtr<FullscreenVideoController> m_fullScreenVideoController;
#endif

#if USE(ACCELERATED_COMPOSITING)
    bool isAcceleratedCompositing() const { return m_isAcceleratedCompositing; }
    void setAcceleratedCompositing(bool);

    RefPtr<WebCore::CACFLayerTreeHost> m_layerTreeHost;
    OwnPtr<WebCore::GraphicsLayer> m_backingLayer;
    bool m_isAcceleratedCompositing;
#endif

    bool m_nextDisplayIsSynchronous;
    bool m_usesLayeredWindow;

    HCURSOR m_lastSetCursor;

    RefPtr<WebCore::HistoryItem> m_globalHistoryItem;

#if ENABLE(FULLSCREEN_API)
    RefPtr<WebCore::Element> m_fullScreenElement;
    OwnPtr<WebCore::FullScreenController> m_fullscreenController;
    WebCore::IntPoint m_scrollPosition;
#endif
};

#endif
