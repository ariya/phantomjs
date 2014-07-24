/*
 * Copyright (C) 2010 Igalia S.L.
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

#include "autotoolsconfig.h"
#include "test_utils.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

#define HTML_DOCUMENT_TITLE "<html><head><title>This is the title</title></head><body></body></html>"
#define HTML_DOCUMENT_ELEMENTS "<html><body><ul><li>1</li><li>2</li><li>3</li></ul></body></html>"
#define HTML_DOCUMENT_ELEMENTS_CLASS "<html><body><div class=\"test\"></div><div class=\"strange\"></div><div class=\"test\"></div></body></html>"
#define HTML_DOCUMENT_ELEMENTS_ID "<html><body><div id=\"testok\"></div><div id=\"testbad\">first</div><div id=\"testbad\">second</div></body></html>"
#define HTML_DOCUMENT_LINKS "<html><head><title>Title</title></head><body><a href=\"about:blank\">blank</a><a href=\"http://www.google.com\">google</a><a href=\"http://www.webkit.org\">webkit</a></body></html>"
#define HTML_DOCUMENT_IFRAME "<html><head><title>IFrame</title></head><body><iframe id='iframe'></iframe><div id='test'></div></body></html>"
#define HTML_DOCUMENT_TABLE "<html><body><table id=\"table\"></table></body></html>"
#define HTML_DOCUMENT_EVALUATE "<html><head><title></title></head><body><div>First div</div><div>Second div</div></body></html>"

typedef struct {
    GtkWidget* webView;
    GMainLoop* loop;
} DomDocumentFixture;

static gboolean finish_loading(DomDocumentFixture* fixture)
{
    if (g_main_loop_is_running(fixture->loop))
        g_main_loop_quit(fixture->loop);

    return FALSE;
}

static void dom_document_fixture_setup(DomDocumentFixture* fixture, gconstpointer data)
{
    fixture->loop = g_main_loop_new(NULL, TRUE);
    fixture->webView = webkit_web_view_new();
    g_object_ref_sink(fixture->webView);

    if (data != NULL)
        webkit_web_view_load_string(WEBKIT_WEB_VIEW (fixture->webView), (const char*) data, NULL, NULL, NULL);

    g_idle_add((GSourceFunc)finish_loading, fixture);
    g_main_loop_run(fixture->loop);
}

static void dom_document_fixture_teardown(DomDocumentFixture* fixture, gconstpointer data)
{
    if (fixture->webView)
        g_object_unref(fixture->webView);
    g_main_loop_unref(fixture->loop);
}

static void test_dom_document_title(DomDocumentFixture* fixture, gconstpointer data)
{
    g_assert(fixture);
    WebKitWebView* view = (WebKitWebView*)fixture->webView;
    g_assert(view);
    WebKitDOMDocument* document = webkit_web_view_get_dom_document(view);
    g_assert(document);
    gchar* title = webkit_dom_document_get_title(document);
    g_assert(title);
    g_assert_cmpstr(title, ==, "This is the title");
    g_free(title);
    webkit_dom_document_set_title(document, "This is the second title");
    title = webkit_dom_document_get_title(document);
    g_assert(title);
    g_assert_cmpstr(title, ==, "This is the second title");
    g_free(title);
}

static void test_dom_document_get_elements_by_tag_name(DomDocumentFixture* fixture, gconstpointer data)
{
    g_assert(fixture);
    WebKitWebView* view = (WebKitWebView*)fixture->webView;
    g_assert(view);
    WebKitDOMDocument* document = webkit_web_view_get_dom_document(view);
    g_assert(document);
    WebKitDOMNodeList* list = webkit_dom_document_get_elements_by_tag_name(document, "li");
    g_assert(list);
    gulong length = webkit_dom_node_list_get_length(list);
    g_assert_cmpint(length, ==, 3);

    guint i;

    for (i = 0; i < length; i++) {
        WebKitDOMNode* item = webkit_dom_node_list_item(list, i);
        g_assert(item);
        WebKitDOMElement* element = (WebKitDOMElement*)item;
        g_assert(element);
        g_assert_cmpstr(webkit_dom_element_get_tag_name(element), ==, "LI");
        WebKitDOMHTMLElement* htmlElement = (WebKitDOMHTMLElement*)element;
        char* n = g_strdup_printf("%d", i+1);
        g_assert_cmpstr(webkit_dom_html_element_get_inner_text(htmlElement), ==, n);
        g_free(n);
    }

    g_object_unref(list);
}

static void test_dom_document_get_elements_by_class_name(DomDocumentFixture* fixture, gconstpointer data)
{
    g_assert(fixture);
    WebKitWebView* view = (WebKitWebView*)fixture->webView;
    g_assert(view);
    WebKitDOMDocument* document = webkit_web_view_get_dom_document(view);
    g_assert(document);
    WebKitDOMNodeList* list = webkit_dom_document_get_elements_by_class_name(document, "test");
    g_assert(list);
    gulong length = webkit_dom_node_list_get_length(list);
    g_assert_cmpint(length, ==, 2);

    guint i;

    for (i = 0; i < length; i++) {
        WebKitDOMNode* item = webkit_dom_node_list_item(list, i);
        g_assert(item);
        WebKitDOMElement* element = (WebKitDOMElement*)item;
        g_assert(element);
        g_assert_cmpstr(webkit_dom_element_get_tag_name(element), ==, "DIV");
    }

    g_object_unref(list);
}

static void test_dom_document_get_element_by_id(DomDocumentFixture* fixture, gconstpointer data)
{
    g_assert(fixture);
    WebKitWebView* view = (WebKitWebView*)fixture->webView;
    g_assert(view);
    WebKitDOMDocument* document = webkit_web_view_get_dom_document(view);
    g_assert(document);
    WebKitDOMElement* element = webkit_dom_document_get_element_by_id(document, "testok");
    g_assert(element);
    element = webkit_dom_document_get_element_by_id(document, "this-id-does-not-exist");
    g_assert(element == 0);
    /* The DOM spec says the return value is undefined when there's
     * more than one element with the same id; in our case the first
     * one will be returned */
    element = webkit_dom_document_get_element_by_id(document, "testbad");
    g_assert(element);
    WebKitDOMHTMLElement* htmlElement = (WebKitDOMHTMLElement*)element;
    g_assert_cmpstr(webkit_dom_html_element_get_inner_text(htmlElement), ==, "first");
}

static void test_dom_document_get_links(DomDocumentFixture* fixture, gconstpointer data)
{
    g_assert(fixture);
    WebKitWebView* view = (WebKitWebView*)fixture->webView;
    g_assert(view);
    WebKitDOMDocument* document = webkit_web_view_get_dom_document(view);
    g_assert(document);
    WebKitDOMHTMLCollection *collection = webkit_dom_document_get_links(document);
    g_assert(collection);
    gulong length = webkit_dom_html_collection_get_length(collection);
    g_assert_cmpint(length, ==, 3);

    guint i;

    for (i = 0; i < length; i++) {
        static const char* names[] = { "blank", "google", "webkit" };
        static const char* uris[] = { "about:blank", "http://www.google.com/", "http://www.webkit.org/" };
        WebKitDOMNode *node = webkit_dom_html_collection_item(collection, i);
        g_assert(node);
        WebKitDOMElement* element = (WebKitDOMElement*)node;
        g_assert_cmpstr(webkit_dom_element_get_tag_name(element), ==, "A");
        WebKitDOMHTMLElement *htmlElement = (WebKitDOMHTMLElement*)element;
        g_assert_cmpstr(webkit_dom_html_element_get_inner_text(htmlElement), ==, names[i]);
        WebKitDOMHTMLAnchorElement *anchor = (WebKitDOMHTMLAnchorElement*)element;
        g_assert_cmpstr(webkit_dom_html_anchor_element_get_href(anchor), ==, uris[i]);
    }
    g_object_unref(collection);
}

static void test_dom_document_insert_row(DomDocumentFixture* fixture, gconstpointer data)
{
    g_assert(fixture);
    WebKitWebView* view = (WebKitWebView*)fixture->webView;
    g_assert(view);
    WebKitDOMDocument* document = webkit_web_view_get_dom_document(view);
    g_assert(WEBKIT_DOM_IS_DOCUMENT(document));
    WebKitDOMElement* table = webkit_dom_document_get_element_by_id(document, "table");
    g_assert(WEBKIT_DOM_IS_HTML_ELEMENT(table));
    WebKitDOMHTMLCollection* rows = webkit_dom_html_table_element_get_rows(WEBKIT_DOM_HTML_TABLE_ELEMENT(table));
    g_assert(WEBKIT_DOM_IS_HTML_COLLECTION(rows));

    // Table is initially empty.
    g_assert_cmpint(webkit_dom_html_collection_get_length(rows), ==, 0);
    WebKitDOMHTMLElement* row = webkit_dom_html_table_element_insert_row(WEBKIT_DOM_HTML_TABLE_ELEMENT(table), -1, NULL);
    g_assert(WEBKIT_DOM_IS_HTML_TABLE_ROW_ELEMENT(row));
    rows = webkit_dom_html_table_element_get_rows(WEBKIT_DOM_HTML_TABLE_ELEMENT(table));
    g_assert(WEBKIT_DOM_IS_HTML_COLLECTION(rows));
    g_assert_cmpint(webkit_dom_html_collection_get_length(rows), ==, 1);
}

static void test_dom_document_evaluate(DomDocumentFixture* fixture, gconstpointer data)
{
    g_assert(fixture);
    WebKitWebView* view = (WebKitWebView*)fixture->webView;
    g_assert(view);
    WebKitDOMDocument* document = webkit_web_view_get_dom_document(view);
    g_assert(WEBKIT_DOM_IS_DOCUMENT(document));
    WebKitDOMNodeList* list = webkit_dom_document_get_elements_by_tag_name(document, "html");
    g_assert(list);
    gulong length = webkit_dom_node_list_get_length(list);
    g_assert_cmpint(length, ==, 1);
    WebKitDOMNode* html = webkit_dom_node_list_item(list, 0);
    g_assert(WEBKIT_DOM_IS_NODE(html));

    WebKitDOMXPathResult* result = webkit_dom_document_evaluate(document, "//div", html, NULL, 0, NULL, NULL);
    g_assert(WEBKIT_DOM_IS_XPATH_RESULT(result));

    int i = 0;
    WebKitDOMNode* node;
    while ( (node = webkit_dom_xpath_result_iterate_next(result, NULL)) != NULL) {
        g_assert(node);
        WebKitDOMElement* element = (WebKitDOMElement*)node;
        g_assert_cmpstr(webkit_dom_element_get_tag_name(element), ==, "DIV");
        i++;
    }
    g_assert_cmpint(i, ==, 2);

    g_object_unref(list);
}

static void weak_notify(gpointer data, GObject* zombie)
{
    guint* count = (guint*)data;
    (*count)++;
}

static void test_dom_document_garbage_collection(DomDocumentFixture* fixture, gconstpointer data)
{
    guint count = 0;
    g_assert(fixture);
    WebKitWebView* view = (WebKitWebView*)fixture->webView;
    g_assert(view);
    WebKitDOMDocument* document = webkit_web_view_get_dom_document(view);
    g_assert(document);
    g_object_weak_ref(G_OBJECT(document), (GWeakNotify)weak_notify, &count);
    WebKitDOMHTMLHeadElement* head = webkit_dom_document_get_head(document);
    g_assert(head);
    g_object_weak_ref(G_OBJECT(head), (GWeakNotify)weak_notify, &count);
    WebKitDOMHTMLElement* body = webkit_dom_document_get_body(document);
    g_assert(body);
    g_object_weak_ref(G_OBJECT(body), (GWeakNotify)weak_notify, &count);
    WebKitDOMHTMLCollection *collection = webkit_dom_document_get_links(document);
    g_assert(collection);
    g_object_weak_ref(G_OBJECT(collection), (GWeakNotify)weak_notify, &count);

    webkit_web_view_load_string(WEBKIT_WEB_VIEW(view), HTML_DOCUMENT_LINKS, NULL, NULL, NULL);

    while (g_main_context_pending(NULL))
        g_main_context_iteration(NULL, FALSE);

    g_assert_cmpuint(count, ==, 3);

    g_object_unref(collection);
    g_assert_cmpuint(count, ==, 4);

    count = 0;

    document = webkit_web_view_get_dom_document(view);
    g_assert(document);
    g_object_weak_ref(G_OBJECT(document), (GWeakNotify)weak_notify, &count);
    head = webkit_dom_document_get_head(document);
    g_assert(head);
    g_object_weak_ref(G_OBJECT(head), (GWeakNotify)weak_notify, &count);
    body = webkit_dom_document_get_body(document);
    g_assert(body);
    g_object_weak_ref(G_OBJECT(body), (GWeakNotify)weak_notify, &count);
    collection = webkit_dom_document_get_links(document);
    g_assert(collection);
    g_object_weak_ref(G_OBJECT(collection), (GWeakNotify)weak_notify, &count);
    /* Ask twice for the same object */
    WebKitDOMHTMLCollection* collection2 = webkit_dom_document_get_links(document);
    g_assert(collection2);
    g_object_weak_ref(G_OBJECT(collection2), (GWeakNotify)weak_notify, &count);

    g_object_unref(document);
    g_object_unref(head);
    g_object_unref(body);
    g_object_unref(collection);
    g_object_unref(collection2);

    g_assert_cmpuint(count, ==, 5);

    webkit_web_view_load_string(WEBKIT_WEB_VIEW(view), HTML_DOCUMENT_IFRAME, NULL, NULL, NULL);

    while (g_main_context_pending(NULL))
        g_main_context_iteration(NULL, FALSE);

    count = 0;

    document = webkit_web_view_get_dom_document(view);
    WebKitDOMElement* div = webkit_dom_document_get_element_by_id(document, "test");
    g_assert(div);
    g_object_weak_ref(G_OBJECT(div), (GWeakNotify)weak_notify, &count);
    WebKitDOMElement* iframe = webkit_dom_document_get_element_by_id(document, "iframe");
    g_assert(iframe);

    webkit_dom_element_set_attribute(iframe, "src", "data:<html><head></head></html>", NULL);

    while (g_main_context_pending(NULL))
        g_main_context_iteration(NULL, FALSE);

    WebKitDOMDocument* iframeDocument = webkit_dom_html_iframe_element_get_content_document(WEBKIT_DOM_HTML_IFRAME_ELEMENT(iframe));
    g_assert(iframeDocument);
    head = webkit_dom_document_get_head(iframeDocument);
    g_assert(head);
    g_object_weak_ref(G_OBJECT(head), (GWeakNotify)weak_notify, &count);

    webkit_dom_element_set_attribute(iframe, "src", "about:blank", NULL);

    while (g_main_context_pending(NULL))
        g_main_context_iteration(NULL, FALSE);

    g_assert_cmpuint(count, ==, 1);

    webkit_web_view_load_string(WEBKIT_WEB_VIEW(view), HTML_DOCUMENT_LINKS, NULL, NULL, NULL);

    while (g_main_context_pending(NULL))
        g_main_context_iteration(NULL, FALSE);

    g_assert_cmpuint(count, ==, 2);

    count = 0;

    document = webkit_web_view_get_dom_document(view);
    g_assert(document);
    g_object_weak_ref(G_OBJECT(document), (GWeakNotify)weak_notify, &count);
    /* Ask twice for the Document */
    WebKitDOMDocument* document2 = webkit_web_view_get_dom_document(view);
    g_assert(document2);
    g_object_weak_ref(G_OBJECT(document2), (GWeakNotify)weak_notify, &count);
    head = webkit_dom_document_get_head(document);
    g_assert(head);
    g_object_weak_ref(G_OBJECT(head), (GWeakNotify)weak_notify, &count);
    body = webkit_dom_document_get_body(document);
    g_assert(body);
    g_object_weak_ref(G_OBJECT(body), (GWeakNotify)weak_notify, &count);
    collection = webkit_dom_document_get_links(document);
    g_assert(collection);
    g_object_weak_ref(G_OBJECT(collection), (GWeakNotify)weak_notify, &count);

    gtk_widget_destroy(fixture->webView);
    fixture->webView = NULL;

    g_assert_cmpuint(count, ==, 4);

    g_object_unref(collection);

    g_assert_cmpuint(count, ==, 5);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");

    g_test_add("/webkit/domdocument/test_title",
               DomDocumentFixture, HTML_DOCUMENT_TITLE,
               dom_document_fixture_setup,
               test_dom_document_title,
               dom_document_fixture_teardown);

    g_test_add("/webkit/domdocument/test_get_elements_by_tag_name",
               DomDocumentFixture, HTML_DOCUMENT_ELEMENTS,
               dom_document_fixture_setup,
               test_dom_document_get_elements_by_tag_name,
               dom_document_fixture_teardown);

    g_test_add("/webkit/domdocument/test_get_elements_by_class_name",
               DomDocumentFixture, HTML_DOCUMENT_ELEMENTS_CLASS,
               dom_document_fixture_setup,
               test_dom_document_get_elements_by_class_name,
               dom_document_fixture_teardown);

    g_test_add("/webkit/domdocument/test_get_element_by_id",
               DomDocumentFixture, HTML_DOCUMENT_ELEMENTS_ID,
               dom_document_fixture_setup,
               test_dom_document_get_element_by_id,
               dom_document_fixture_teardown);

    g_test_add("/webkit/domdocument/test_get_links",
               DomDocumentFixture, HTML_DOCUMENT_LINKS,
               dom_document_fixture_setup,
               test_dom_document_get_links,
               dom_document_fixture_teardown);

    g_test_add("/webkit/domdocument/test_table_insert_row",
               DomDocumentFixture, HTML_DOCUMENT_TABLE,
               dom_document_fixture_setup,
               test_dom_document_insert_row,
               dom_document_fixture_teardown);

    g_test_add("/webkit/domdocument/test_document_evaluate",
               DomDocumentFixture, HTML_DOCUMENT_EVALUATE,
               dom_document_fixture_setup,
               test_dom_document_evaluate,
               dom_document_fixture_teardown);

    g_test_add("/webkit/domdocument/test_garbage_collection",
               DomDocumentFixture, HTML_DOCUMENT_LINKS,
               dom_document_fixture_setup,
               test_dom_document_garbage_collection,
               dom_document_fixture_teardown);

    return g_test_run();
}

