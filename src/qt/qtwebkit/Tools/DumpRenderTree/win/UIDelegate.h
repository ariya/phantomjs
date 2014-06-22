/*
 * Copyright (C) 2005, 2006, 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UIDelegate_h
#define UIDelegate_h

#include <WebCore/COMPtr.h>
#include <WebKit/WebKit.h>
#include <wtf/OwnPtr.h>
#include <windef.h>

class DRTUndoManager;
class DRTDesktopNotificationPresenter;

class UIDelegate : public IWebUIDelegate2, IWebUIDelegatePrivate3 {
public:
    UIDelegate();

    void resetUndoManager();

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebUIDelegate
    virtual HRESULT STDMETHODCALLTYPE createWebViewWithRequest( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebURLRequest *request,
        /* [retval][out] */ IWebView **newWebView);

    virtual HRESULT STDMETHODCALLTYPE webViewShow( 
        /* [in] */ IWebView *sender) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewClose( 
        /* [in] */ IWebView *sender);

    virtual HRESULT STDMETHODCALLTYPE webViewFocus( 
        /* [in] */ IWebView *sender);

    virtual HRESULT STDMETHODCALLTYPE webViewUnfocus( 
        /* [in] */ IWebView *sender);

    virtual HRESULT STDMETHODCALLTYPE webViewFirstResponder( 
        /* [in] */ IWebView *sender,
        /* [retval][out] */ OLE_HANDLE *responder) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE makeFirstResponder( 
        /* [in] */ IWebView *sender,
        /* [in] */ OLE_HANDLE responder) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE setStatusText( 
        /* [in] */ IWebView *sender,
        /* [in] */ BSTR text);

    virtual HRESULT STDMETHODCALLTYPE webViewStatusText( 
        /* [in] */ IWebView *sender,
        /* [retval][out] */ BSTR *text) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewAreToolbarsVisible( 
        /* [in] */ IWebView *sender,
        /* [retval][out] */ BOOL *visible) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE setToolbarsVisible( 
        /* [in] */ IWebView *sender,
        /* [in] */ BOOL visible) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewIsStatusBarVisible( 
        /* [in] */ IWebView *sender,
        /* [retval][out] */ BOOL *visible) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE setStatusBarVisible( 
        /* [in] */ IWebView *sender,
        /* [in] */ BOOL visible) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewIsResizable( 
        /* [in] */ IWebView *sender,
        /* [retval][out] */ BOOL *resizable) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE setResizable( 
        /* [in] */ IWebView *sender,
        /* [in] */ BOOL resizable) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE setFrame( 
        /* [in] */ IWebView *sender,
        /* [in] */ RECT *frame);

    virtual HRESULT STDMETHODCALLTYPE webViewFrame( 
        /* [in] */ IWebView *sender,
        /* [retval][out] */ RECT *frame);

    virtual HRESULT STDMETHODCALLTYPE setContentRect( 
        /* [in] */ IWebView *sender,
        /* [in] */ RECT *contentRect) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE webViewContentRect( 
        /* [in] */ IWebView *sender,
        /* [retval][out] */ RECT *contentRect) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE runJavaScriptAlertPanelWithMessage( 
        /* [in] */ IWebView *sender,
        /* [in] */ BSTR message);

    virtual HRESULT STDMETHODCALLTYPE runJavaScriptConfirmPanelWithMessage( 
        /* [in] */ IWebView *sender,
        /* [in] */ BSTR message,
        /* [retval][out] */ BOOL *result);

    virtual HRESULT STDMETHODCALLTYPE runJavaScriptTextInputPanelWithPrompt( 
        /* [in] */ IWebView *sender,
        /* [in] */ BSTR message,
        /* [in] */ BSTR defaultText,
        /* [retval][out] */ BSTR *result);

    virtual HRESULT STDMETHODCALLTYPE runBeforeUnloadConfirmPanelWithMessage( 
        /* [in] */ IWebView *sender,
        /* [in] */ BSTR message,
        /* [in] */ IWebFrame *initiatedByFrame,
        /* [retval][out] */ BOOL *result);

    virtual HRESULT STDMETHODCALLTYPE runOpenPanelForFileButtonWithResultListener( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebOpenPanelResultListener *resultListener) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE mouseDidMoveOverElement( 
        /* [in] */ IWebView *sender,
        /* [in] */ IPropertyBag *elementInformation,
        /* [in] */ UINT modifierFlags) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE contextMenuItemsForElement( 
        /* [in] */ IWebView *sender,
        /* [in] */ IPropertyBag *element,
        /* [in] */ OLE_HANDLE defaultItems,
        /* [retval][out] */ OLE_HANDLE *resultMenu) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE validateUserInterfaceItem( 
        /* [in] */ IWebView *webView,
        /* [in] */ UINT itemCommandID,
        /* [in] */ BOOL defaultValidation,
        /* [retval][out] */ BOOL *isValid) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE shouldPerformAction( 
        /* [in] */ IWebView *webView,
        /* [in] */ UINT itemCommandID,
        /* [in] */ UINT sender) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE dragDestinationActionMaskForDraggingInfo( 
        /* [in] */ IWebView *webView,
        /* [in] */ IDataObject *draggingInfo,
        /* [retval][out] */ WebDragDestinationAction *action) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE willPerformDragDestinationAction( 
        /* [in] */ IWebView *webView,
        /* [in] */ WebDragDestinationAction action,
        /* [in] */ IDataObject *draggingInfo) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE dragSourceActionMaskForPoint( 
        /* [in] */ IWebView *webView,
        /* [in] */ LPPOINT point,
        /* [retval][out] */ WebDragSourceAction *action) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE willPerformDragSourceAction( 
        /* [in] */ IWebView *webView,
        /* [in] */ WebDragSourceAction action,
        /* [in] */ LPPOINT point,
        /* [in] */ IDataObject *pasteboard,
        /* [retval][out] */ IDataObject **newPasteboard) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE contextMenuItemSelected( 
        /* [in] */ IWebView *sender,
        /* [in] */ void *item,
        /* [in] */ IPropertyBag *element) { return E_NOTIMPL; } 
        
    virtual HRESULT STDMETHODCALLTYPE hasCustomMenuImplementation( 
        /* [retval][out] */ BOOL *hasCustomMenus);
    
    virtual HRESULT STDMETHODCALLTYPE trackCustomPopupMenu( 
        /* [in] */ IWebView *sender,
        /* [in] */ OLE_HANDLE menu,
        /* [in] */ LPPOINT point);
       
    virtual HRESULT STDMETHODCALLTYPE measureCustomMenuItem( 
        /* [in] */ IWebView *sender,
        /* [in] */ void *measureItem) { return E_NOTIMPL; }
        
    virtual HRESULT STDMETHODCALLTYPE drawCustomMenuItem( 
        /* [in] */ IWebView *sender,
        /* [in] */ void *drawItem) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE addCustomMenuDrawingData( 
        /* [in] */ IWebView *sender,
        /* [in] */ OLE_HANDLE menu) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE cleanUpCustomMenuDrawingData( 
        /* [in] */ IWebView *sender,
        /* [in] */ OLE_HANDLE menu) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE canTakeFocus( 
        /* [in] */ IWebView *sender,
        /* [in] */ BOOL forward,
        /* [out] */ BOOL *result) { return E_NOTIMPL; }
        
    virtual HRESULT STDMETHODCALLTYPE takeFocus( 
        /* [in] */ IWebView *sender,
        /* [in] */ BOOL forward) { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE registerUndoWithTarget( 
        /* [in] */ IWebUndoTarget *target,
        /* [in] */ BSTR actionName,
        /* [in] */ IUnknown *actionArg);
    
    virtual HRESULT STDMETHODCALLTYPE removeAllActionsWithTarget( 
        /* [in] */ IWebUndoTarget *target);
    
    virtual HRESULT STDMETHODCALLTYPE setActionTitle( 
        /* [in] */ BSTR actionTitle);
    
    virtual HRESULT STDMETHODCALLTYPE undo();
    
    virtual HRESULT STDMETHODCALLTYPE redo();
    
    virtual HRESULT STDMETHODCALLTYPE canUndo( 
        /* [retval][out] */ BOOL *result);
    
    virtual HRESULT STDMETHODCALLTYPE canRedo( 
        /* [retval][out] */ BOOL *result);

    virtual HRESULT STDMETHODCALLTYPE printFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame);
    
    virtual HRESULT STDMETHODCALLTYPE ftpDirectoryTemplatePath( 
        /* [in] */ IWebView *webView,
        /* [retval][out] */ BSTR *path);
    
    virtual HRESULT STDMETHODCALLTYPE webViewHeaderHeight( 
        /* [in] */ IWebView *webView,
        /* [retval][out] */ float *result);
    
    virtual HRESULT STDMETHODCALLTYPE webViewFooterHeight( 
        /* [in] */ IWebView *webView,
        /* [retval][out] */ float *result);
    
    virtual HRESULT STDMETHODCALLTYPE drawHeaderInRect( 
        /* [in] */ IWebView *webView,
        /* [in] */ RECT *rect,
        /* [in] */ OLE_HANDLE drawingContext);
    
    virtual HRESULT STDMETHODCALLTYPE drawFooterInRect( 
        /* [in] */ IWebView *webView,
        /* [in] */ RECT *rect,
        /* [in] */ OLE_HANDLE drawingContext,
        /* [in] */ UINT pageIndex,
        /* [in] */ UINT pageCount);
    
    virtual HRESULT STDMETHODCALLTYPE webViewPrintingMarginRect( 
        /* [in] */ IWebView *webView,
        /* [retval][out] */ RECT *rect);
    
    virtual HRESULT STDMETHODCALLTYPE canRunModal( 
        /* [in] */ IWebView *webView,
        /* [retval][out] */ BOOL *canRunBoolean);
    
    virtual HRESULT STDMETHODCALLTYPE createModalDialog( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebURLRequest *request,
        /* [retval][out] */ IWebView **newWebView);
    
    virtual HRESULT STDMETHODCALLTYPE runModal( 
        /* [in] */ IWebView *webView);
    
    virtual HRESULT STDMETHODCALLTYPE isMenuBarVisible( 
        /* [in] */ IWebView *webView,
        /* [retval][out] */ BOOL *visible);
    
    virtual HRESULT STDMETHODCALLTYPE setMenuBarVisible( 
        /* [in] */ IWebView *webView,
        /* [in] */ BOOL visible);
    
    virtual HRESULT STDMETHODCALLTYPE runDatabaseSizeLimitPrompt( 
        /* [in] */ IWebView *webView,
        /* [in] */ BSTR displayName,
        /* [in] */ IWebFrame *initiatedByFrame,
        /* [retval][out] */ BOOL *allowed);
    
    virtual HRESULT STDMETHODCALLTYPE paintCustomScrollbar( 
        /* [in] */ IWebView *webView,
        /* [in] */ HDC hDC,
        /* [in] */ RECT rect,
        /* [in] */ WebScrollBarControlSize size,
        /* [in] */ WebScrollbarControlState state,
        /* [in] */ WebScrollbarControlPart pressedPart,
        /* [in] */ BOOL vertical,
        /* [in] */ float value,
        /* [in] */ float proportion,
        /* [in] */ WebScrollbarControlPartMask parts);
    
    virtual HRESULT STDMETHODCALLTYPE paintCustomScrollCorner( 
        /* [in] */ IWebView *webView,
        /* [in] */ HDC hDC,
        /* [in] */ RECT rect);

    virtual HRESULT STDMETHODCALLTYPE createWebViewWithRequest(IWebView* sender, IWebURLRequest* request, IPropertyBag* windowFeatures, IWebView** newWebView);

    virtual HRESULT STDMETHODCALLTYPE drawBackground(IWebView* sender, OLE_HANDLE hdc, const RECT* dirtyRect);

    virtual HRESULT STDMETHODCALLTYPE decidePolicyForGeolocationRequest(IWebView* sender, IWebFrame* frame, IWebSecurityOrigin* origin, IWebGeolocationPolicyListener* listener);

    virtual HRESULT STDMETHODCALLTYPE didPressMissingPluginButton(IDOMElement*);

protected:
    // IWebUIDelegatePrivate

    virtual HRESULT STDMETHODCALLTYPE unused1() { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE unused2() { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE unused3() { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE webViewScrolled( 
        /* [in] */ IWebView *sender) { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE webViewAddMessageToConsole( 
        /* [in] */ IWebView *sender,
        /* [in] */ BSTR message,
        /* [in] */ int lineNumber,
        /* [in] */ BSTR url,
        /* [in] */ BOOL isError);
    
    virtual HRESULT STDMETHODCALLTYPE webViewShouldInterruptJavaScript( 
        /* [in] */ IWebView *sender,
        /* [retval][out] */ BOOL *result) { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE webViewReceivedFocus( 
        /* [in] */ IWebView *sender) { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE webViewLostFocus( 
        /* [in] */ IWebView *sender,
        /* [in] */ OLE_HANDLE loseFocusTo) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE doDragDrop( 
        /* [in] */ IWebView *sender,
        /* [in] */ IDataObject *dataObject,
        /* [in] */ IDropSource *dropSource,
        /* [in] */ DWORD okEffect,
        /* [retval][out] */ DWORD *performedEffect);

    virtual HRESULT STDMETHODCALLTYPE webViewGetDlgCode( 
        /* [in] */ IWebView *sender,
        /* [in] */ UINT keyCode,
        /* [retval][out] */ LONG_PTR *code);

    virtual HRESULT STDMETHODCALLTYPE webViewPainted( 
        /* [in] */ IWebView *sender);

    virtual HRESULT STDMETHODCALLTYPE exceededDatabaseQuota( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame,
        /* [in] */ IWebSecurityOrigin *origin,
        /* [in] */ BSTR databaseIdentifier);

    virtual HRESULT STDMETHODCALLTYPE embeddedViewWithArguments( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame,
        /* [in] */ IPropertyBag *arguments,
        /* [retval][out] */ IWebEmbeddedView **view);
        
    virtual HRESULT STDMETHODCALLTYPE webViewClosing( 
        /* [in] */ IWebView *sender);
    
    virtual HRESULT STDMETHODCALLTYPE webViewSetCursor( 
        /* [in] */ IWebView *sender,
        /* [in] */ OLE_HANDLE cursor);
    
    virtual HRESULT STDMETHODCALLTYPE webViewDidInvalidate( 
        /* [in] */ IWebView *sender);

    virtual HRESULT STDMETHODCALLTYPE desktopNotificationsDelegate(
        /* [out] */ IWebDesktopNotificationsDelegate** result);

    ULONG                   m_refCount;

private:
    RECT m_frame;
    OwnPtr<DRTUndoManager> m_undoManager;

    COMPtr<IWebDesktopNotificationsDelegate> m_desktopNotifications;
};

#endif
