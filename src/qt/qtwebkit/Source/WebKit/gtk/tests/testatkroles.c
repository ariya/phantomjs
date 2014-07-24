/*
 * Copyright © 2010 Joanmarie Diggs
 * Copyright © 2010 Igalia S.L.
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
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

/* Non form roles */
#define HTML_DOCUMENT_FRAME "<html><body>This is a test.</body></html>"
#define HTML_HEADING "<html><body><h1>1</h1><h2>2</h2><h3>3</h3><h4>4</h4><h5>5</h5><h6>6</h6></body></html>"
#define HTML_IMAGE "<html><body><img src='foobar.png' alt='This is a test.'/></body></html>"
#define HTML_LINK_TEXT "<html><body><a href='foobar.html'>This is a test.</a></body></html>"
#define HTML_LIST "<html><body><ul><li>1</li><li>2</li></ul><ol><li>1</li><li>2</li></ol></body></html>"
#define HTML_PARAGRAPH "<html><body><p>This is a test.</p></body></html>"
#define HTML_SECTION "<html><body><div>This is a test.</div></body></html>"
#define HTML_TABLE "<html><body><table border='1'><tr><td>This is</td><td>a test.</td></tr></table></body></html>"
#define HTML_SEPARATOR "<html><body><hr/></body></html>"
#define HTML_COMBOBOX "<html><body><select size='1'><option>one</option><option>two</option><option>three</option></select></body></html>"
/* Form roles */
#define HTML_FORM "<html><body><form>This is a test.</form></body></html>"
#define HTML_CHECK_BOX "<html><body><input type='checkbox' />This is a test.</body></html>"
#define HTML_LABELED_ENTRY "<html><body><label for='foo'>Name:</label><input type='text' id='foo' /></body></html>"
#define HTML_LISTBOX "<html><body><select size='3'><option>one</option><option>two</option><option>three</option></select></body></html>"
#define HTML_PASSWORD_TEXT "<html><body><input type='password' /></body></html>"
#define HTML_PUSH_BUTTON "<html><body><input type='submit' value='ok' />This is a test.</body></html>"
#define HTML_RADIO_BUTTON "<html><body><input type='radio' />This is a test.</body></html>"

typedef struct {
    AtkObject* documentFrame;
    AtkObject* obj;
    AtkRole role;
    GtkWidget* webView;
    GtkAllocation alloc;
    GMainLoop* loop;
} AtkRolesFixture;

static gboolean finish_loading(AtkRolesFixture* fixture)
{
    if (g_main_loop_is_running(fixture->loop))
        g_main_loop_quit(fixture->loop);

    // With the change to support WK2 accessibility, the root object
    // has changed and it's no longer the document frame, but a scroll
    // pane containing the document frame as its only child. See the
    // bug 72390 for more details on this change.
    // https://bugs.webkit.org/show_bug.cgi?id=72390
    AtkObject* rootObject = gtk_widget_get_accessible(fixture->webView);
    fixture->documentFrame = atk_object_ref_accessible_child(rootObject, 0);
    g_assert(fixture->documentFrame);

    // Remove the reference added by ref_accessible_child() and
    // return, since we don't need to keep that extra ref at all.
    g_object_unref(fixture->documentFrame);
    return FALSE;
}

static void atk_roles_fixture_setup(AtkRolesFixture* fixture, gconstpointer data)
{
    fixture->loop = g_main_loop_new(NULL, TRUE);
    fixture->alloc = (GtkAllocation) { 0, 0, 800, 600 };
    fixture->webView = webkit_web_view_new();
    g_object_ref_sink(fixture->webView);

    gtk_widget_size_allocate(fixture->webView, &fixture->alloc);

    if (data != NULL)
        webkit_web_view_load_string(WEBKIT_WEB_VIEW (fixture->webView), (const char*) data, NULL, NULL, NULL);

    g_idle_add((GSourceFunc) finish_loading, fixture);
    g_main_loop_run(fixture->loop);
}

static void atk_roles_fixture_teardown(AtkRolesFixture* fixture, gconstpointer data)
{
    g_object_unref(fixture->webView);
    g_main_loop_unref(fixture->loop);
}

static void get_child_and_test_role(AtkObject* obj, gint pos, AtkRole role)
{
    AtkObject* child;
    AtkRole child_role;

    child = atk_object_ref_accessible_child(obj, pos);
    g_assert(child);
    child_role = atk_object_get_role(child);
    g_assert(child_role == role);

    g_object_unref(child);
}

static void test_webkit_atk_get_role_document_frame(AtkRolesFixture* fixture, gconstpointer data)
{
    fixture->role = atk_object_get_role(fixture->documentFrame);
    g_assert(fixture->role == ATK_ROLE_DOCUMENT_FRAME);
}

static void test_webkit_atk_get_role_heading(AtkRolesFixture* fixture, gconstpointer data)
{
    get_child_and_test_role(fixture->documentFrame, 0, ATK_ROLE_HEADING);
    get_child_and_test_role(fixture->documentFrame, 1, ATK_ROLE_HEADING);
    get_child_and_test_role(fixture->documentFrame, 2, ATK_ROLE_HEADING);
    get_child_and_test_role(fixture->documentFrame, 3, ATK_ROLE_HEADING);
    get_child_and_test_role(fixture->documentFrame, 4, ATK_ROLE_HEADING);
    get_child_and_test_role(fixture->documentFrame, 5, ATK_ROLE_HEADING);
}

static void test_webkit_atk_get_role_image(AtkRolesFixture* fixture, gconstpointer data)
{
    // This is an extraneous object of ATK_ROLE_PANEL which we should get rid of.
    fixture->obj = atk_object_ref_accessible_child(fixture->documentFrame, 0);
    g_assert(fixture->obj);

    get_child_and_test_role(fixture->obj, 0, ATK_ROLE_IMAGE);

    g_object_unref(fixture->obj);
}

static void test_webkit_atk_get_role_link(AtkRolesFixture* fixture, gconstpointer data)
{
    // This is an extraneous object of ATK_ROLE_PANEL which we should get rid of.
    fixture->obj = atk_object_ref_accessible_child(fixture->documentFrame, 0);
    g_assert(fixture->obj);

    get_child_and_test_role(fixture->obj, 0, ATK_ROLE_LINK);

    g_object_unref(fixture->obj);
}

static void test_webkit_atk_get_role_list_and_item(AtkRolesFixture* fixture, gconstpointer data)
{
    AtkObject* listObj;

    listObj = atk_object_ref_accessible_child(fixture->documentFrame, 0);
    g_assert(listObj);
    fixture->role = atk_object_get_role(listObj);
    g_assert(fixture->role == ATK_ROLE_LIST);

    get_child_and_test_role(listObj, 0, ATK_ROLE_LIST_ITEM);
    get_child_and_test_role(listObj, 1, ATK_ROLE_LIST_ITEM);
    g_object_unref(listObj);

    listObj = atk_object_ref_accessible_child(fixture->documentFrame, 1);
    g_assert(listObj);
    fixture->role = atk_object_get_role(listObj);
    g_assert(fixture->role == ATK_ROLE_LIST);

    get_child_and_test_role(listObj, 0, ATK_ROLE_LIST_ITEM);
    get_child_and_test_role(listObj, 1, ATK_ROLE_LIST_ITEM);
    g_object_unref(listObj);
}

static void test_webkit_atk_get_role_paragraph(AtkRolesFixture* fixture, gconstpointer data)
{
    get_child_and_test_role(fixture->documentFrame, 0, ATK_ROLE_PARAGRAPH);
}

static void test_webkit_atk_get_role_section(AtkRolesFixture* fixture, gconstpointer data)
{
    get_child_and_test_role(fixture->documentFrame, 0, ATK_ROLE_SECTION);
}

// Does not yet test table cells because of bug 30895.
static void test_webkit_atk_get_role_table(AtkRolesFixture* fixture, gconstpointer data)
{
    get_child_and_test_role(fixture->documentFrame, 0, ATK_ROLE_TABLE);
}

static void test_webkit_atk_get_role_separator(AtkRolesFixture *fixture, gconstpointer data)
{
    get_child_and_test_role(fixture->documentFrame, 0, ATK_ROLE_SEPARATOR);
}

static void test_webkit_atk_get_role_combobox(AtkRolesFixture *fixture, gconstpointer data)
{
    AtkObject* comboboxMenu;

    // This is an extraneous object of ATK_ROLE_PANEL which we should get rid of.
    fixture->obj = atk_object_ref_accessible_child(fixture->documentFrame, 0);
    g_assert(fixture->obj);

    fixture->obj = atk_object_ref_accessible_child(fixture->obj, 0);
    g_assert(fixture->obj);
    fixture->role = atk_object_get_role(fixture->obj);
    g_assert(fixture->role == ATK_ROLE_COMBO_BOX);

    comboboxMenu = atk_object_ref_accessible_child(fixture->obj, 0);
    g_assert(comboboxMenu);
    fixture->role = atk_object_get_role(comboboxMenu);
    g_assert(fixture->role == ATK_ROLE_MENU);

    get_child_and_test_role(comboboxMenu, 0, ATK_ROLE_MENU_ITEM);
    get_child_and_test_role(comboboxMenu, 1, ATK_ROLE_MENU_ITEM);
    get_child_and_test_role(comboboxMenu, 2, ATK_ROLE_MENU_ITEM);

    g_object_unref(fixture->obj);
    g_object_unref(comboboxMenu);
}

/* Form roles */
static void test_webkit_atk_get_role_form(AtkRolesFixture *fixture, gconstpointer data)
{
    get_child_and_test_role(fixture->documentFrame, 0, ATK_ROLE_FORM);
}

static void test_webkit_atk_get_role_check_box(AtkRolesFixture* fixture, gconstpointer data)
{
    // This is an extraneous object of ATK_ROLE_PANEL which we should get rid of.
    fixture->obj = atk_object_ref_accessible_child(fixture->documentFrame, 0);
    g_assert(fixture->obj);

    get_child_and_test_role(fixture->obj, 0, ATK_ROLE_CHECK_BOX);

    g_object_unref(fixture->obj);
}

static void test_webkit_atk_get_role_entry(AtkRolesFixture* fixture, gconstpointer data)
{
    // This is an extraneous object of ATK_ROLE_PANEL which we should get rid of.
    fixture->obj = atk_object_ref_accessible_child(fixture->documentFrame, 0);
    g_assert(fixture->obj);

    get_child_and_test_role(fixture->obj, 1, ATK_ROLE_ENTRY);

    g_object_unref(fixture->obj);
}

static void test_webkit_atk_get_role_label(AtkRolesFixture* fixture, gconstpointer data)
{
    // This is an extraneous object of ATK_ROLE_PANEL which we should get rid of.
    fixture->obj = atk_object_ref_accessible_child(fixture->documentFrame, 0);
    g_assert(fixture->obj);

    get_child_and_test_role(fixture->obj, 0, ATK_ROLE_LABEL);

    g_object_unref(fixture->obj);
}

static void test_webkit_atk_get_role_listbox(AtkRolesFixture* fixture, gconstpointer data)
{
    AtkObject* listboxObj;
    // This is an extraneous object of ATK_ROLE_PANEL which we should get rid of.
    fixture->obj = atk_object_ref_accessible_child(fixture->documentFrame, 0);
    g_assert(fixture->obj);

    listboxObj = atk_object_ref_accessible_child(fixture->obj, 0);
    g_assert(listboxObj);
    fixture->role = atk_object_get_role(listboxObj);
    g_assert(fixture->role == ATK_ROLE_LIST);

    get_child_and_test_role(listboxObj, 0, ATK_ROLE_LIST_ITEM);
    get_child_and_test_role(listboxObj, 1, ATK_ROLE_LIST_ITEM);
    get_child_and_test_role(listboxObj, 2, ATK_ROLE_LIST_ITEM);

    g_object_unref(fixture->obj);
    g_object_unref(listboxObj);
}

static void test_webkit_atk_get_role_password_text(AtkRolesFixture* fixture, gconstpointer data)
{
    // This is an extraneous object of ATK_ROLE_PANEL which we should get rid of.
    fixture->obj = atk_object_ref_accessible_child(fixture->documentFrame, 0);
    g_assert(fixture->obj);

    get_child_and_test_role(fixture->obj, 0, ATK_ROLE_PASSWORD_TEXT);

    g_object_unref(fixture->obj);
}

static void test_webkit_atk_get_role_push_button(AtkRolesFixture* fixture, gconstpointer data)
{
    // This is an extraneous object of ATK_ROLE_PANEL which we should get rid of.
    fixture->obj = atk_object_ref_accessible_child(fixture->documentFrame, 0);
    g_assert(fixture->obj);

    get_child_and_test_role(fixture->obj, 0, ATK_ROLE_PUSH_BUTTON);

    g_object_unref(fixture->obj);
}

static void test_webkit_atk_get_role_radio_button(AtkRolesFixture* fixture, gconstpointer data)
{
    // This is an extraneous object of ATK_ROLE_PANEL which we should get rid of.
    fixture->obj = atk_object_ref_accessible_child(fixture->documentFrame, 0);
    g_assert(fixture->obj);

    get_child_and_test_role(fixture->obj, 0, ATK_ROLE_RADIO_BUTTON);

    g_object_unref(fixture->obj);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");

    g_test_add("/webkit/atk/test_webkit_atk_get_role_document_frame",
               AtkRolesFixture, HTML_DOCUMENT_FRAME,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_document_frame,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_heading",
               AtkRolesFixture, HTML_HEADING,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_heading,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_image",
               AtkRolesFixture, HTML_IMAGE,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_image,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_link",
               AtkRolesFixture, HTML_LINK_TEXT,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_link,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_list_and_item",
               AtkRolesFixture, HTML_LIST,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_list_and_item,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_paragraph",
               AtkRolesFixture, HTML_PARAGRAPH,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_paragraph,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_section",
               AtkRolesFixture, HTML_SECTION,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_section,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_table",
               AtkRolesFixture, HTML_TABLE,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_table,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_separator",
               AtkRolesFixture, HTML_SEPARATOR,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_separator,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_combobox",
               AtkRolesFixture, HTML_COMBOBOX,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_combobox,
               atk_roles_fixture_teardown);

    /* Form roles */
    g_test_add("/webkit/atk/test_webkit_atk_get_role_form",
               AtkRolesFixture, HTML_FORM,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_form,
               atk_roles_fixture_teardown);
    g_test_add("/webkit/atk/test_webkit_atk_get_role_check_box",
               AtkRolesFixture, HTML_CHECK_BOX,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_check_box,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_entry",
               AtkRolesFixture, HTML_LABELED_ENTRY,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_entry,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_label",
               AtkRolesFixture, HTML_LABELED_ENTRY,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_label,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_listbox",
               AtkRolesFixture, HTML_LISTBOX,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_listbox,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_password_text",
               AtkRolesFixture, HTML_PASSWORD_TEXT,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_password_text,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_push_button",
               AtkRolesFixture, HTML_PUSH_BUTTON,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_push_button,
               atk_roles_fixture_teardown);

    g_test_add("/webkit/atk/test_webkit_atk_get_role_radio_button",
               AtkRolesFixture, HTML_RADIO_BUTTON,
               atk_roles_fixture_setup,
               test_webkit_atk_get_role_radio_button,
               atk_roles_fixture_teardown);

    return g_test_run();
}
