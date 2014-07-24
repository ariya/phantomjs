/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "config.h"
#include "PagePopup.h"

#include "DocumentLoader.h"
#include "DocumentWriter.h"
#include "EmptyClients.h"
#include "FrameView.h"
#include "JSDOMBinding.h"
#include "JSDOMWindowBase.h"
#include "JSObject.h"
#include "JSRetainPtr.h"
#include "KURL.h"
#include "PagePopupClient.h"
#include "Settings.h"
#include "WebPage.h"
#include "WebPage_p.h"
#include <JavaScriptCore/API/JSCallbackObject.h>
#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/JSValueRef.h>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

PagePopup::PagePopup(WebPagePrivate* webPage, PagePopupClient* client)
    : m_webPagePrivate(webPage)
    , m_client(adoptPtr(client))
{
}

PagePopup::~PagePopup()
{
}

void PagePopup::initialize(WebPage* webPage)
{
    // Don't use backing store for the pop-up web page.
    webPage->settings()->setBackingStoreEnabled(false);

    webPage->d->setLoadState(WebPagePrivate::Committed);

    writeDocument(webPage->d->mainFrame()->loader()->activeDocumentLoader()->writer());
    installDOMFunction(webPage->d->mainFrame());

    webPage->d->setLoadState(WebPagePrivate::Finished);
    webPage->client()->notifyLoadFinished(0);
}

void PagePopup::writeDocument(DocumentWriter* writer)
{
    ASSERT(writer);
    writer->setMIMEType("text/html");
    writer->begin(KURL());

    // All the popups have the same html head and the page content should be non-zoomable.
    StringBuilder source;
    // FIXME: the hardcoding padding will be removed soon.
    source.appendLiteral("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>\n");
    source.appendLiteral("<meta name=\"viewport\" content=\"user-scalable=no, width=device-width\" />\n");
    CString utf8Data = source.toString().utf8();
    writer->addData(utf8Data.data(), utf8Data.length());

    m_client->writeDocument(*writer);
    writer->end();
}

JSValueRef PagePopup::setValueAndClosePopupCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef*)
{
    JSValueRef jsRetVal = JSValueMakeUndefined(context);
    if (argumentCount <= 0)
        return jsRetVal;

    JSRetainPtr<JSStringRef> string(Adopt, JSValueToStringCopy(context, arguments[0], 0));
    size_t sizeUTF8 = JSStringGetMaximumUTF8CStringSize(string.get());
    Vector<char> utf8String(sizeUTF8 + 1);
    utf8String[sizeUTF8] = 0;
    JSStringGetUTF8CString(string.get(), utf8String.data(), sizeUTF8);
    JSObjectRef popUpObject = JSValueToObject(context, arguments[argumentCount - 1], 0);
    PagePopup* pagePopup = static_cast<PagePopup*>(JSObjectGetPrivate(popUpObject));
    pagePopup->m_client->setValueAndClosePopup(String::fromUTF8(utf8String.data()));

    return jsRetVal;
}

static void popUpExtensionInitialize(JSContextRef context, JSObjectRef object)
{
    UNUSED_PARAM(context);
    PagePopup* pagePopup = static_cast<PagePopup*>(JSObjectGetPrivate(object));
    pagePopup->ref();
}

static void popUpExtensionFinalize(JSObjectRef object)
{
    PagePopup* pagePopup = static_cast<PagePopup*>(JSObjectGetPrivate(object));
    pagePopup->deref();
}

static JSStaticFunction popUpExtensionStaticFunctions[] =
{
{ 0, 0, 0 },
{ 0, 0, 0 }
};

static JSStaticValue popUpExtensionStaticValues[] =
{
{ 0, 0, 0, 0 }
};

void PagePopup::installDOMFunction(Frame* frame)
{
    JSDOMWindow* window = toJSDOMWindow(frame, mainThreadNormalWorld());
    ASSERT(window);

    JSC::ExecState* exec = window->globalExec();
    ASSERT(exec);
    JSC::JSLockHolder lock(exec);

    JSContextRef context = ::toRef(exec);
    JSObjectRef globalObject = JSContextGetGlobalObject(context);
    JSStringRef functionName = JSStringCreateWithUTF8CString("setValueAndClosePopup");
    JSObjectRef function = JSObjectMakeFunctionWithCallback(context, functionName, setValueAndClosePopupCallback);
    JSObjectSetProperty(context, globalObject, functionName, function, kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly, 0);

    // Register client into DOM
    JSClassDefinition definition = kJSClassDefinitionEmpty;
    definition.staticValues = popUpExtensionStaticValues;
    definition.staticFunctions = popUpExtensionStaticFunctions;
    definition.initialize = popUpExtensionInitialize;
    definition.finalize = popUpExtensionFinalize;
    JSClassRef clientClass = JSClassCreate(&definition);

    JSObjectRef clientClassObject = JSObjectMake(context, clientClass, this);

    String name("popUp");

    JSC::PutPropertySlot slot;
    window->put(window, exec, JSC::Identifier(exec, name), toJS(clientClassObject), slot);

    JSClassRelease(clientClass);
}

void PagePopup::close()
{
    m_client->didClosePopup();
}

}
}
