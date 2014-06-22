/*
 * Copyright (C) 2012 Igalia S.L.
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
 */

#include "config.h"
#include "WebKitJavascriptResult.h"

#include "WebKitJavascriptResultPrivate.h"
#include "WebSerializedScriptValue.h"
#include <wtf/gobject/GRefPtr.h>

using namespace WebKit;

struct _WebKitJavascriptResult {
    _WebKitJavascriptResult(WebKitWebView* view, WebSerializedScriptValue* serializedScriptValue)
        : webView(view)
        , referenceCount(1)
    {
        value = serializedScriptValue->deserialize(webkit_web_view_get_javascript_global_context(view), 0);
    }

    GRefPtr<WebKitWebView> webView;
    JSValueRef value;

    int referenceCount;
};

G_DEFINE_BOXED_TYPE(WebKitJavascriptResult, webkit_javascript_result, webkit_javascript_result_ref, webkit_javascript_result_unref)

WebKitJavascriptResult* webkitJavascriptResultCreate(WebKitWebView* webView, WebSerializedScriptValue* serializedScriptValue)
{
    WebKitJavascriptResult* result = g_slice_new(WebKitJavascriptResult);
    new (result) WebKitJavascriptResult(webView, serializedScriptValue);
    return result;
}

/**
 * webkit_javascript_result_ref:
 * @js_result: a #WebKitJavascriptResult
 *
 * Atomically increments the reference count of @js_result by one. This
 * function is MT-safe and may be called from any thread.
 *
 * Returns: The passed in #WebKitJavascriptResult
 */
WebKitJavascriptResult* webkit_javascript_result_ref(WebKitJavascriptResult* javascriptResult)
{
    g_atomic_int_inc(&javascriptResult->referenceCount);
    return javascriptResult;
}

/**
 * webkit_javascript_result_unref:
 * @js_result: a #WebKitJavascriptResult
 *
 * Atomically decrements the reference count of @js_result by one. If the
 * reference count drops to 0, all memory allocated by the #WebKitJavascriptResult is
 * released. This function is MT-safe and may be called from any
 * thread.
 */
void webkit_javascript_result_unref(WebKitJavascriptResult* javascriptResult)
{
    if (g_atomic_int_dec_and_test(&javascriptResult->referenceCount)) {
        javascriptResult->~WebKitJavascriptResult();
        g_slice_free(WebKitJavascriptResult, javascriptResult);
    }
}

/**
 * webkit_javascript_result_get_global_context:
 * @js_result: a #WebKitJavascriptResult
 *
 * Get the global Javascript context that should be used with the
 * <function>JSValueRef</function> returned by webkit_javascript_result_get_value().
 *
 * Returns: the <function>JSGlobalContextRef</function> for the #WebKitJavascriptResult
 */
JSGlobalContextRef webkit_javascript_result_get_global_context(WebKitJavascriptResult* javascriptResult)
{
    return webkit_web_view_get_javascript_global_context(javascriptResult->webView.get());
}

/**
 * webkit_javascript_result_get_value:
 * @js_result: a #WebKitJavascriptResult
 *
 * Get the value of @js_result. You should use the <function>JSGlobalContextRef</function>
 * returned by webkit_javascript_result_get_global_context() to use the <function>JSValueRef</function>.
 *
 * Returns: the <function>JSValueRef</function> of the #WebKitJavascriptResult
 */
JSValueRef webkit_javascript_result_get_value(WebKitJavascriptResult* javascriptResult)
{
    return javascriptResult->value;
}
