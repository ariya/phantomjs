/*
 * Copyright (C) 2009 Igalia S.L.
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
#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <unistd.h>
#include <webkit/webkit.h>

static const char* centeredContents = "<html><body><p style='text-align: center;'>Short line</p><p style='text-align: center;'>Long-size line with some foo bar baz content</p><p style='text-align: center;'>Short line</p><p style='text-align: center;'>This is a multi-line paragraph<br />where the first line<br />is the biggest one</p></body></html>";

static const char* contents = "<html><body><p>This is a test. This is the second sentence. And this the third.</p></body></html>";

static const char* contentsWithNewlines = "<html><body><p>This is a test. \n\nThis\n is the second sentence. And this the third.</p></body></html>";

static const char* contentsWithPreformattedText = "<html><body><pre>\n\t\n\tfirst line\n\tsecond line\n</pre></body></html>";

static const char* contentsWithSpecialChars = "<html><body><p>&laquo;&nbsp;This is a paragraph with &ldquo;special&rdquo; characters inside.&nbsp;&raquo;</p><ul><li style='max-width:100px;'>List item with some text that wraps across different lines.</li><li style='max-width:100px;'><p>List item with some text that wraps across different lines.</p></li></ul></body></html>";

static const char* contentsInTextarea = "<html><body><textarea cols='80'>This is a test. This is the second sentence. And this the third.</textarea></body></html>";

static const char* contentsInTextInput = "<html><body><input type='text' size='80' value='This is a test. This is the second sentence. And this the third.'/></body></html>";

static const char* contentsInParagraphAndBodySimple = "<html><body><p>This is a test.</p>Hello world.</body></html>";

static const char* contentsInParagraphAndBodyModerate = "<html><body><p>This is a test.</p>Hello world.<br /><font color='#00cc00'>This sentence is green.</font><br />This one is not.</body></html>";

static const char* contentsInTable = "<html><body><table><tr><td>foo</td><td>bar</td></tr></table></body></html>";

static const char* contentsInTableWithHeaders = "<html><body><table><tr><th>foo</th><th>bar</th><th colspan='2'>baz</th></tr><tr><th>qux</th><td>1</td><td>2</td><td>3</td></tr><tr><th rowspan='2'>quux</th><td>4</td><td>5</td><td>6</td></tr><tr><td>6</td><td>7</td><td>8</td></tr><tr><th>corge</th><td>9</td><td>10</td><td>11</td></tr></table><table><tr><td>1</td><td>2</td></tr><tr><td>3</td><td>4</td></tr></table></body></html>";

static const char* contentsWithExtraneousWhiteSpaces = "<html><head><body><p>This\n                          paragraph\n                                                      is\n                                                                                                                                                                                                                                                                                                                                                                            borked!</p></body></html>";

static const char* contentsWithWrappedLines = "<html><body><p style='max-width:150px;'>This is one line wrapped because of the maximum width of its container.</p><p>This is another line wrapped<br>because of one forced<br>line break in the middle.</body></html>";

static const char* comboBoxSelector = "<html><body><select><option selected value='foo'>foo</option><option value='bar'>bar</option></select></body></html>";

static const char* embeddedObjects = "<html><body><p>Choose: <input value='foo' type='checkbox'/>foo <input value='bar' type='checkbox'/>bar (pick one)</p><p>Choose: <select name='foo'><option>bar</option><option>baz</option></select> (pick one)</p><p><input name='foobarbutton' value='foobar' type='button'/></p></body></html>";

static const char* formWithTextInputs = "<html><body><form><input type='text' name='entry' /><input type='password' name='passwordEntry' /></form></body></html>";

static const char* hypertextAndHyperlinks = "<html><body><p>A paragraph with no links at all</p><p><a href='http://foo.bar.baz/'>A line</a> with <a href='http://bar.baz.foo/'>a link in the middle</a> as well as at the beginning and <a href='http://baz.foo.bar/'>at the end</a></p><ol><li>List item with a <span><a href='http://foo.bar.baz/'>link inside a span node</a></span></li></ol></body></html>";

static const char* layoutAndDataTables = "<html><body><table><tr><th>Odd</th><th>Even</th></tr><tr><td>1</td><td>2</td></tr></table><table><tr><td>foo</td><td>bar</td></tr></table></body></html>";

static const char* linksWithInlineImages = "<html><head><style>a.http:before {content: url(no-image.png);}</style><body><p><a class='http' href='foo'>foo</a> bar baz</p><p>foo <a class='http' href='bar'>bar</a> baz</p><p>foo bar <a class='http' href='baz'>baz</a></p></body></html>";

static const char* listsOfItems = "<html><body><ul><li>text only</li><li><a href='foo'>link only</a></li><li>text and a <a href='bar'>link</a></li></ul><ol><li>text only</li><li><a href='foo'>link only</a></li><li>text and a <a href='bar'>link</a></li></ol></body></html>";

static const char* textForCaretBrowsing = "<html><body><h1>A text header</h1><p>A paragraph <a href='http://foo.bar.baz/'>with a link</a> in the middle</p><ol><li>A list item</li><li><span style='display:block;'>Block span in a list item</span><span>Inline span in a list item</span></li><li><a href='foo'><span style='display:block;'>Block span in a link in a list item</span><span>Inline span in a link in a list item</span></a></li></ol><select><option selected value='foo'>An option in a combo box</option></select><input type='text' name='foo' value='foo bar baz' /><table><tr><td>a table cell</td><td></td><td><a href='foo'><span style='display:block;'>Block span in a link in a table cell</span><span>Inline span in a link in a table cell</span></a></td><td><span style='display:block;'>Block span in a table cell</span><span>Inline span in a table cell</span></td></tr></table><h4><a href='foo'><span style='display:block;'>Block span in a link in a heading</span><span>Inline span in a link in a heading</span></h4><h4><span style='display:block;'>Block span in a heading</span><span>Inline span in a heading</span></h4></body></html>";

static const char* textForSelections = "<html><body><p>A paragraph with plain text</p><p>A paragraph with <a href='http://webkit.org'>a link</a> in the middle</p><ol><li>A list item</li></ol><select></body></html>";

static const char* textWithAttributes = "<html><head><style>.st1 {font-family: monospace; color:rgb(120,121,122);} .st2 {text-decoration:underline; background-color:rgb(80,81,82);}</style></head><body><p style=\"font-size:14; text-align:right;\">This is the <i>first</i><b> sentence of this text.</b></p><p class=\"st1\">This sentence should have an style applied <span class=\"st2\">and this part should have another one</span>.</p><p>x<sub>1</sub><sup>2</sup>=x<sub>2</sub><sup>3</sup></p><p style=\"text-align:center;\">This sentence is the <strike>last</strike> one.</p></body></html>";

static AtkObject* getWebAreaObject(WebKitWebView* webView)
{
    /* Manually spin the main context to make sure the accessible
       objects are properly created before continuing. */
    while (g_main_context_pending(0))
        g_main_context_iteration(0, TRUE);

    AtkObject* rootObject = gtk_widget_get_accessible(GTK_WIDGET(webView));
    if (!rootObject)
        return NULL;

    AtkObject* webAreaObject = atk_object_ref_accessible_child(rootObject, 0);
    if (!webAreaObject)
        return NULL;

    /* We don't need the extra ref here. */
    g_object_unref(webAreaObject);

    return webAreaObject;
}

typedef gchar* (*AtkGetTextFunction) (AtkText*, gint, AtkTextBoundary, gint*, gint*);

static void testGetTextFunction(AtkText* textObject, AtkGetTextFunction fn, AtkTextBoundary boundary, gint offset, const char* textResult, gint startOffsetResult, gint endOffsetResult)
{
    gint startOffset;
    gint endOffset;
    char* text = fn(textObject, offset, boundary, &startOffset, &endOffset);
    g_assert_cmpstr(text, ==, textResult);
    g_assert_cmpint(startOffset, ==, startOffsetResult);
    g_assert_cmpint(endOffset, ==, endOffsetResult);
    g_free(text);
}

static void runGetTextTests(AtkText* textObject)
{
    char* text = atk_text_get_text(textObject, 0, -1);
    g_assert_cmpstr(text, ==, "This is a test. This is the second sentence. And this the third.");
    g_free(text);

    /* ATK_TEXT_BOUNDARY_CHAR */
    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_CHAR,
                        0, "T", 0, 1);

    testGetTextFunction(textObject, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_CHAR,
                        0, "h", 1, 2);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_CHAR,
                        0, "", 0, 0);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_CHAR,
                        1, "T", 0, 1);

    /* ATK_TEXT_BOUNDARY_WORD_START */
    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_WORD_START,
                        0, "This ", 0, 5);

    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_WORD_START,
                        4, "This ", 0, 5);

    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_WORD_START,
                        10, "test. ", 10, 16);

    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_WORD_START,
                        58, "third.", 58, 64);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_WORD_START,
                        5, "This ", 0, 5);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_WORD_START,
                        7, "This ", 0, 5);

    testGetTextFunction(textObject, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_WORD_START,
                        0, "is ", 5, 8);

    testGetTextFunction(textObject, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_WORD_START,
                        4, "is ", 5, 8);

    testGetTextFunction(textObject, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_WORD_START,
                        3, "is ", 5, 8);

    /* ATK_TEXT_BOUNDARY_WORD_END */
    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_WORD_END,
                        0, "This", 0, 4);

    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_WORD_END,
                        4, " is", 4, 7);

    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_WORD_END,
                        5, " is", 4, 7);

    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_WORD_END,
                        9, " test", 9, 14);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_WORD_END,
                        5, "This", 0, 4);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_WORD_END,
                        4, "This", 0, 4);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_WORD_END,
                        7, " is", 4, 7);

    testGetTextFunction(textObject, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_WORD_END,
                        5, " a", 7, 9);

    testGetTextFunction(textObject, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_WORD_END,
                        4, " a", 7, 9);

    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_WORD_END,
                        58, " third", 57, 63);

    /* ATK_TEXT_BOUNDARY_SENTENCE_START */
    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_SENTENCE_START,
                        0, "This is a test. ", 0, 16);

    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_SENTENCE_START,
                        15, "This is a test. ", 0, 16);

    testGetTextFunction(textObject, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_SENTENCE_START,
                        0, "This is the second sentence. ", 16, 45);

    testGetTextFunction(textObject, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_SENTENCE_START,
                        15, "This is the second sentence. ", 16, 45);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_SENTENCE_START,
                        16, "This is a test. ", 0, 16);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_SENTENCE_START,
                        44, "This is a test. ", 0, 16);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_SENTENCE_START,
                        15, "", 0, 0);

    /* ATK_TEXT_BOUNDARY_SENTENCE_END */
    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_SENTENCE_END,
                        0, "This is a test.", 0, 15);

    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_SENTENCE_END,
                        15, " This is the second sentence.", 15, 44);

    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_SENTENCE_END,
                        16, " This is the second sentence.", 15, 44);

    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_SENTENCE_END,
                        17, " This is the second sentence.", 15, 44);

    testGetTextFunction(textObject, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_SENTENCE_END,
                        0, " This is the second sentence.", 15, 44);

    testGetTextFunction(textObject, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_SENTENCE_END,
                        15, " And this the third.", 44, 64);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_SENTENCE_END,
                        16, "This is a test.", 0, 15);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_SENTENCE_END,
                        15, "This is a test.", 0, 15);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_SENTENCE_END,
                        14, "", 0, 0);

    testGetTextFunction(textObject, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_SENTENCE_END,
                        44, " This is the second sentence.", 15, 44);

    /* It's tricky to test these properly right now, since our a11y
       implementation splits different lines in different a11y items. */
    /* ATK_TEXT_BOUNDARY_LINE_START */
    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_START,
                        0, "This is a test. This is the second sentence. And this the third.", 0, 64);

    /* ATK_TEXT_BOUNDARY_LINE_END */
    testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_END,
                        0, "This is a test. This is the second sentence. And this the third.", 0, 64);

    /* For objects implementing AtkEditableText, try to change the
       exposed text and retrieve it again as a full line.
       (see https://bugs.webkit.org/show_bug.cgi?id=72830) */
    if (ATK_IS_EDITABLE_TEXT(textObject)) {
        atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(textObject), "foo bar baz");
        testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_START, 0, "foo bar baz", 0, 11);
        testGetTextFunction(textObject, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_END, 0, "foo bar baz", 0, 11);
    }
}

static gchar* textCaretMovedResult = 0;

static void textCaretMovedCallback(AtkText* text, gint pos, gpointer data)
{
    g_assert(ATK_IS_TEXT(text));

    g_free(textCaretMovedResult);
    AtkRole role = atk_object_get_role(ATK_OBJECT(text));
    textCaretMovedResult = g_strdup_printf("|%s|%d|", atk_role_get_name(role), pos);
}

static void testWebkitAtkCaretOffsets()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, textForCaretBrowsing, 0, 0, 0);

    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    AtkObject* header = atk_object_ref_accessible_child(object, 0);
    g_assert(ATK_IS_TEXT(header));
    g_signal_connect(header, "text-caret-moved", G_CALLBACK(textCaretMovedCallback), 0);

    gchar* text = atk_text_get_text(ATK_TEXT(header), 0, -1);
    g_assert_cmpstr(text, ==, "A text header");
    g_free (text);

    /* It should be possible to place the caret inside a header. */
    gboolean result = atk_text_set_caret_offset(ATK_TEXT(header), 5);
    g_assert_cmpint(result, ==, TRUE);
    gint offset = atk_text_get_caret_offset(ATK_TEXT(header));
    g_assert_cmpint(offset, ==, 5);
    g_assert_cmpstr(textCaretMovedResult, ==, "|heading|5|");

    AtkObject* paragraph = atk_object_ref_accessible_child(object, 1);
    g_assert(ATK_IS_TEXT(paragraph));
    g_signal_connect(paragraph, "text-caret-moved", G_CALLBACK(textCaretMovedCallback), 0);

    text = atk_text_get_text(ATK_TEXT(paragraph), 0, -1);
    g_assert_cmpstr(text, ==, "A paragraph with a link in the middle");
    g_free (text);

    /* It should be possible to place the caret inside a paragraph and a link. */
    result = atk_text_set_caret_offset(ATK_TEXT(paragraph), 5);
    g_assert_cmpint(result, ==, TRUE);
    offset = atk_text_get_caret_offset(ATK_TEXT(paragraph));
    g_assert_cmpint(offset, ==, 5);
    g_assert_cmpstr(textCaretMovedResult, ==, "|paragraph|5|");

    result = atk_text_set_caret_offset(ATK_TEXT(paragraph), 20);
    g_assert_cmpint(result, ==, TRUE);
    offset = atk_text_get_caret_offset(ATK_TEXT(paragraph));
    g_assert_cmpint(offset, ==, 20);
    g_assert_cmpstr(textCaretMovedResult, ==, "|paragraph|20|");

    result = atk_text_set_caret_offset(ATK_TEXT(paragraph), 30);
    g_assert_cmpint(result, ==, TRUE);
    offset = atk_text_get_caret_offset(ATK_TEXT(paragraph));
    g_assert_cmpint(offset, ==, 30);
    g_assert_cmpstr(textCaretMovedResult, ==, "|paragraph|30|");

    AtkObject* link = atk_object_ref_accessible_child(paragraph, 0);
    g_assert(ATK_IS_TEXT(link));
    text = atk_text_get_text(ATK_TEXT(link), 0, -1);
    g_assert_cmpstr(text, ==, "with a link");
    g_free (text);

    result = atk_text_set_caret_offset(ATK_TEXT(link), 5);
    g_assert_cmpint(result, ==, TRUE);
    offset = atk_text_get_caret_offset(ATK_TEXT(link));
    g_assert_cmpint(offset, ==, 5);
    /* Positions inside links are reported relative to the paragraph. */
    g_assert_cmpstr(textCaretMovedResult, ==, "|paragraph|17|");

    AtkObject* list = atk_object_ref_accessible_child(object, 2);
    g_assert(ATK_OBJECT(list));
    g_assert(atk_object_get_role(list) == ATK_ROLE_LIST);
    g_assert_cmpint(atk_object_get_n_accessible_children(list), ==, 3);

    AtkObject* listItem = atk_object_ref_accessible_child(list, 0);
    g_assert(ATK_IS_TEXT(listItem));
    text = atk_text_get_text(ATK_TEXT(listItem), 0, -1);
    g_assert_cmpstr(text, ==, "1. A list item");
    g_free (text);

    listItem = atk_object_ref_accessible_child(list, 1);
    g_assert(ATK_IS_TEXT(listItem));
    text = atk_text_get_text(ATK_TEXT(listItem), 0, -1);
    g_assert_cmpstr(text, ==, "2. Block span in a list item\nInline span in a list item");
    g_free (text);

    listItem = atk_object_ref_accessible_child(list, 2);
    g_assert(ATK_IS_TEXT(listItem));
    text = atk_text_get_text(ATK_TEXT(listItem), 0, -1);
    g_assert_cmpstr(text, ==, "3. Block span in a link in a list item\nInline span in a link in a list item");
    g_free (text);

    /* It's not possible to place the caret inside an item's marker. */
    result = atk_text_set_caret_offset(ATK_TEXT(listItem), 1);
    g_assert_cmpint(result, ==, FALSE);

    /* It should be possible to place the caret inside an item's text. */
    result = atk_text_set_caret_offset(ATK_TEXT(listItem), 5);
    g_assert_cmpint(result, ==, TRUE);
    offset = atk_text_get_caret_offset(ATK_TEXT(listItem));
    g_assert_cmpint(offset, ==, 5);

    AtkObject* panel = atk_object_ref_accessible_child(object, 3);
    g_assert(ATK_IS_OBJECT(panel));
    g_assert(atk_object_get_role(panel) == ATK_ROLE_PANEL);

    AtkObject* comboBox = atk_object_ref_accessible_child(panel, 0);
    g_assert(ATK_IS_OBJECT(comboBox));
    g_assert(!ATK_IS_TEXT(comboBox));
    g_assert(atk_object_get_role(comboBox) == ATK_ROLE_COMBO_BOX);

    AtkObject* menuPopup = atk_object_ref_accessible_child(comboBox, 0);
    g_assert(ATK_IS_OBJECT(menuPopup));
    g_assert(!ATK_IS_TEXT(menuPopup));
    g_assert(atk_object_get_role(menuPopup) == ATK_ROLE_MENU);

    AtkObject* comboBoxOption = atk_object_ref_accessible_child(menuPopup, 0);
    g_assert(ATK_IS_OBJECT(comboBoxOption));
    g_assert(atk_object_get_role(comboBoxOption) == ATK_ROLE_MENU_ITEM);
    g_assert(ATK_IS_TEXT(comboBoxOption));
    text = atk_text_get_text(ATK_TEXT(comboBoxOption), 0, -1);
    g_assert_cmpstr(text, ==, "An option in a combo box");
    g_free(text);

    /* It's not possible to place the caret inside an option for a combobox. */
    result = atk_text_set_caret_offset(ATK_TEXT(comboBoxOption), 1);
    g_assert_cmpint(result, ==, FALSE);

    AtkObject* textEntry = atk_object_ref_accessible_child(panel, 1);
    g_assert(ATK_IS_OBJECT(textEntry));
    g_assert(atk_object_get_role(textEntry) == ATK_ROLE_ENTRY);
    g_assert(ATK_IS_TEXT(textEntry));
    text = atk_text_get_text(ATK_TEXT(textEntry), 0, -1);
    g_assert_cmpstr(text, ==, "foo bar baz");
    g_free(text);

    result = atk_text_set_caret_offset(ATK_TEXT(textEntry), 5);
    g_assert_cmpint(result, ==, TRUE);
    offset = atk_text_get_caret_offset(ATK_TEXT(textEntry));
    g_assert_cmpint(offset, ==, 5);

    AtkObject* table = atk_object_ref_accessible_child(object, 4);
    g_assert(ATK_IS_OBJECT(table));
    g_assert(!ATK_IS_TEXT(table));
    g_assert(atk_object_get_role(table) == ATK_ROLE_TABLE);
    g_assert_cmpint(atk_object_get_n_accessible_children(table), ==, 4);

    AtkObject* tableCell = atk_object_ref_accessible_child(table, 0);
    g_assert(ATK_IS_TEXT(tableCell));
    g_assert(atk_object_get_role(tableCell) == ATK_ROLE_TABLE_CELL);
    text = atk_text_get_text(ATK_TEXT(tableCell), 0, -1);
    g_assert_cmpstr(text, ==, "a table cell");
    g_free(text);

    result = atk_text_set_caret_offset(ATK_TEXT(tableCell), 2);
    g_assert_cmpint(result, ==, TRUE);
    offset = atk_text_get_caret_offset(ATK_TEXT(tableCell));
    g_assert_cmpint(offset, ==, 2);

    /* Even empty table cells should implement AtkText, but report an empty string */
    tableCell = atk_object_ref_accessible_child(table, 1);
    g_assert(ATK_IS_TEXT(tableCell));
    g_assert(atk_object_get_role(tableCell) == ATK_ROLE_TABLE_CELL);
    text = atk_text_get_text(ATK_TEXT(tableCell), 0, -1);
    g_assert_cmpstr(text, ==, "");
    g_free(text);

    tableCell = atk_object_ref_accessible_child(table, 2);
    g_assert(ATK_IS_TEXT(tableCell));
    g_assert(atk_object_get_role(tableCell) == ATK_ROLE_TABLE_CELL);
    text = atk_text_get_text(ATK_TEXT(tableCell), 0, -1);
    g_assert_cmpstr(text, ==, "Block span in a link in a table cell\nInline span in a link in a table cell");
    g_free(text);

    tableCell = atk_object_ref_accessible_child(table, 3);
    g_assert(ATK_IS_TEXT(tableCell));
    g_assert(atk_object_get_role(tableCell) == ATK_ROLE_TABLE_CELL);
    text = atk_text_get_text(ATK_TEXT(tableCell), 0, -1);
    g_assert_cmpstr(text, ==, "Block span in a table cell\nInline span in a table cell");
    g_free(text);

    header = atk_object_ref_accessible_child(object, 5);
    g_assert(ATK_IS_TEXT(header));
    g_assert(atk_object_get_role(header) == ATK_ROLE_HEADING);
    text = atk_text_get_text(ATK_TEXT(header), 0, -1);
    g_assert_cmpstr(text, ==, "Block span in a link in a heading\nInline span in a link in a heading");
    g_free(text);

    header = atk_object_ref_accessible_child(object, 6);
    g_assert(ATK_IS_TEXT(header));
    g_assert(atk_object_get_role(header) == ATK_ROLE_HEADING);
    text = atk_text_get_text(ATK_TEXT(header), 0, -1);
    g_assert_cmpstr(text, ==, "Block span in a heading\nInline span in a heading");
    g_free(text);

    g_free(textCaretMovedResult);

    g_object_unref(header);
    g_object_unref(paragraph);
    g_object_unref(link);
    g_object_unref(list);
    g_object_unref(listItem);
    g_object_unref(panel);
    g_object_unref(comboBox);
    g_object_unref(menuPopup);
    g_object_unref(comboBoxOption);
    g_object_unref(textEntry);
    g_object_unref(table);
    g_object_unref(tableCell);
    g_object_unref(webView);
}

static void testWebkitAtkCaretOffsetsAndExtranousWhiteSpaces()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contentsWithExtraneousWhiteSpaces, 0, 0, 0);

    /* Enable caret browsing. */
    WebKitWebSettings* settings = webkit_web_view_get_settings(webView);
    g_object_set(G_OBJECT(settings), "enable-caret-browsing", TRUE, NULL);
    webkit_web_view_set_settings(webView, settings);

    /* Get to the inner AtkText object. */
    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);
    object = atk_object_ref_accessible_child(object, 0);
    g_assert(object);

    AtkText* textObject = ATK_TEXT(object);
    g_assert(ATK_IS_TEXT(textObject));

    gchar* text = atk_text_get_text(textObject, 0, -1);
    g_assert_cmpstr(text, ==, "This paragraph is borked!");
    g_free(text);

    gint characterCount = atk_text_get_character_count(textObject);
    g_assert_cmpint(characterCount, ==, 25);

    gboolean result = atk_text_set_caret_offset(textObject, characterCount - 1);
    g_assert_cmpint(result, ==, TRUE);

    gint caretOffset = atk_text_get_caret_offset(textObject);
    g_assert_cmpint(caretOffset, ==, characterCount - 1);

    result = atk_text_set_caret_offset(textObject, characterCount);
    g_assert_cmpint(result, ==, TRUE);

    caretOffset = atk_text_get_caret_offset(textObject);
    g_assert_cmpint(caretOffset, ==, characterCount);

    g_object_unref(webView);
}

static void testWebkitAtkComboBox()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, comboBoxSelector, 0, 0, 0);

    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    AtkObject* formObject = atk_object_ref_accessible_child(object, 0);
    g_assert(formObject);

    AtkObject* comboBox = atk_object_ref_accessible_child(formObject, 0);
    g_assert(ATK_IS_OBJECT(comboBox));

    AtkObject* menuPopup = atk_object_ref_accessible_child(comboBox, 0);
    g_assert(ATK_IS_OBJECT(menuPopup));

    AtkObject* item1 = atk_object_ref_accessible_child(menuPopup, 0);
    g_assert(ATK_IS_OBJECT(item1));

    AtkObject* item2 = atk_object_ref_accessible_child(menuPopup, 1);
    g_assert(ATK_IS_OBJECT(item2));

    /* Check roles. */
    g_assert(atk_object_get_role(comboBox) == ATK_ROLE_COMBO_BOX);
    g_assert(atk_object_get_role(menuPopup) == ATK_ROLE_MENU);
    g_assert(atk_object_get_role(item1) == ATK_ROLE_MENU_ITEM);
    g_assert(atk_object_get_role(item2) == ATK_ROLE_MENU_ITEM);

    /* Check the implementation of the AtkSelection interface. */
    g_assert(ATK_IS_SELECTION(comboBox));
    AtkSelection* atkSelection = ATK_SELECTION(comboBox);
    g_assert_cmpint(atk_selection_get_selection_count(atkSelection), ==, 1);
    g_assert(atk_selection_is_child_selected(atkSelection, 0));
    g_assert(!atk_selection_is_child_selected(atkSelection, 1));
    AtkObject* selectedItem = atk_selection_ref_selection(atkSelection, 0);
    g_assert(selectedItem == item1);
    g_object_unref(selectedItem);

    /* Check that the menu popup has 0 links and doesn't crash from checking. */
    gint nLinks = atk_hypertext_get_n_links(ATK_HYPERTEXT(menuPopup));
    g_assert_cmpint(nLinks, ==, 0);

    /* Check the implementations of the AtkAction interface. */
    g_assert(ATK_IS_ACTION(comboBox));
    AtkAction* atkAction = ATK_ACTION(comboBox);
    g_assert_cmpint(atk_action_get_n_actions(atkAction), ==, 1);
    g_assert(atk_action_do_action(atkAction, 0));

    g_assert(ATK_IS_ACTION(menuPopup));
    atkAction = ATK_ACTION(menuPopup);
    g_assert_cmpint(atk_action_get_n_actions(atkAction), ==, 1);
    g_assert(atk_action_do_action(atkAction, 0));

    g_assert(ATK_IS_ACTION(item1));
    atkAction = ATK_ACTION(item1);
    g_assert_cmpint(atk_action_get_n_actions(atkAction), ==, 1);
    g_assert(atk_action_do_action(atkAction, 0));

    g_assert(ATK_IS_ACTION(item2));
    atkAction = ATK_ACTION(item2);
    g_assert_cmpint(atk_action_get_n_actions(atkAction), ==, 1);
    g_assert(atk_action_do_action(atkAction, 0));

    /* After selecting the second item, selection should have changed. */
    g_assert_cmpint(atk_selection_get_selection_count(atkSelection), ==, 1);
    g_assert(!atk_selection_is_child_selected(atkSelection, 0));
    g_assert(atk_selection_is_child_selected(atkSelection, 1));
    selectedItem = atk_selection_ref_selection(atkSelection, 0);
    g_assert(selectedItem == item2);
    g_object_unref(selectedItem);

    /* Check the implementation of the AtkText interface. */
    g_assert(ATK_IS_TEXT(item1));
    AtkText* atkText = ATK_TEXT(item1);
    char *text = atk_text_get_text(atkText, 0, -1);
    g_assert_cmpstr(text, ==, "foo");
    g_free(text);
    text = atk_text_get_text(atkText, 0, 2);
    g_assert_cmpstr(text, ==, "fo");
    g_free(text);

    g_assert(ATK_IS_TEXT(item2));
    atkText = ATK_TEXT(item2);
    text = atk_text_get_text(atkText, 0, -1);
    g_assert_cmpstr(text, ==, "bar");
    g_free(text);
    text = atk_text_get_text(atkText, 1, 3);
    g_assert_cmpstr(text, ==, "ar");
    g_free(text);

    g_object_unref(formObject);
    g_object_unref(comboBox);
    g_object_unref(menuPopup);
    g_object_unref(item1);
    g_object_unref(item2);
    g_object_unref(webView);
}

static gchar* loadingEventsResult = 0;

static void updateLoadingEventsResult(const gchar* signalName)
{
    g_assert(signalName);

    gchar* previousResult = loadingEventsResult;
    loadingEventsResult = g_strdup_printf("%s|%s", previousResult, signalName);
    g_free(previousResult);
}

static gboolean documentLoadingEventCallback(GSignalInvocationHint *signalHint, guint numParamValues, const GValue *paramValues, gpointer data)
{
    // At least we should receive the instance emitting the signal.
    if (numParamValues < 1)
        return TRUE;

    GSignalQuery signal_query;
    g_signal_query(signalHint->signal_id, &signal_query);

    updateLoadingEventsResult(signal_query.signal_name);
    return TRUE;
}

static void testWebkitAtkDocumentLoadingEvents()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);

    /* Connect globally to see those events during a future load. */
    guint loadCompleteListenerId = atk_add_global_event_listener(documentLoadingEventCallback, "ATK:AtkDocument:load-complete");

    /* Do the load, so we can see those events happening. */
    loadingEventsResult = g_strdup("");
    webkit_web_view_load_string(webView, contents, 0, 0, 0);

    /* Trigger the creation of the full accessibility hierarchy by
       asking for the webArea object, so we can listen to events. */
    getWebAreaObject(webView);

    atk_remove_global_event_listener(loadCompleteListenerId);

    g_assert_cmpstr(loadingEventsResult, ==, "|load-complete");

    g_free(loadingEventsResult);
    g_object_unref(webView);
}

static void testWebkitAtkEmbeddedObjects()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, embeddedObjects, 0, 0, 0);

    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    AtkText* paragraph1 = ATK_TEXT(atk_object_ref_accessible_child(object, 0));
    g_assert(ATK_IS_TEXT(paragraph1));
    g_assert(ATK_IS_HYPERTEXT(paragraph1));

    const gchar* expectedText = "Choose: \357\277\274foo \357\277\274bar (pick one)";
    char* text = atk_text_get_text(paragraph1, 0, -1);
    g_assert_cmpstr(text, ==, expectedText);
    g_free(text);

    gint nLinks = atk_hypertext_get_n_links(ATK_HYPERTEXT(paragraph1));
    g_assert_cmpint(nLinks, ==, 2);

    AtkHyperlink* hLink = atk_hypertext_get_link(ATK_HYPERTEXT(paragraph1), 0);
    g_assert(ATK_HYPERLINK(hLink));
    AtkObject* hLinkObject = atk_hyperlink_get_object(hLink, 0);
    g_assert(ATK_OBJECT(hLinkObject));
    g_assert(atk_object_get_role(hLinkObject) == ATK_ROLE_CHECK_BOX);
    g_assert_cmpint(atk_hyperlink_get_start_index(hLink), ==, 8);
    g_assert_cmpint(atk_hyperlink_get_end_index(hLink), ==, 9);
    g_assert_cmpint(atk_hyperlink_get_n_anchors(hLink), ==, 1);
    g_assert_cmpstr(atk_hyperlink_get_uri(hLink, 0), ==, 0);

    AtkText* paragraph2 = ATK_TEXT(atk_object_ref_accessible_child(object, 1));
    g_assert(ATK_IS_TEXT(paragraph2));
    g_assert(ATK_IS_HYPERTEXT(paragraph2));

    expectedText = "Choose: \357\277\274 (pick one)";
    text = atk_text_get_text(paragraph2, 0, -1);
    g_assert_cmpstr(text, ==, expectedText);
    g_free(text);

    nLinks = atk_hypertext_get_n_links(ATK_HYPERTEXT(paragraph2));
    g_assert_cmpint(nLinks, ==, 1);

    hLink = atk_hypertext_get_link(ATK_HYPERTEXT(paragraph2), 0);
    g_assert(ATK_HYPERLINK(hLink));
    hLinkObject = atk_hyperlink_get_object(hLink, 0);
    g_assert(ATK_OBJECT(hLinkObject));
    g_assert(atk_object_get_role(hLinkObject) == ATK_ROLE_COMBO_BOX);
    g_assert_cmpint(atk_hyperlink_get_start_index(hLink), ==, 8);
    g_assert_cmpint(atk_hyperlink_get_end_index(hLink), ==, 9);
    g_assert_cmpint(atk_hyperlink_get_n_anchors(hLink), ==, 1);
    g_assert_cmpstr(atk_hyperlink_get_uri(hLink, 0), ==, 0);

    AtkText* paragraph3 = ATK_TEXT(atk_object_ref_accessible_child(object, 2));
    g_assert(ATK_IS_TEXT(paragraph3));
    g_assert(ATK_IS_HYPERTEXT(paragraph3));

    expectedText = "\357\277\274";
    text = atk_text_get_text(paragraph3, 0, -1);
    g_assert_cmpstr(text, ==, expectedText);
    g_free(text);

    nLinks = atk_hypertext_get_n_links(ATK_HYPERTEXT(paragraph3));
    g_assert_cmpint(nLinks, ==, 1);

    hLink = atk_hypertext_get_link(ATK_HYPERTEXT(paragraph3), 0);
    g_assert(ATK_HYPERLINK(hLink));
    hLinkObject = atk_hyperlink_get_object(hLink, 0);
    g_assert(ATK_OBJECT(hLinkObject));
    g_assert(atk_object_get_role(hLinkObject) == ATK_ROLE_PUSH_BUTTON);
    g_assert_cmpint(atk_hyperlink_get_start_index(hLink), ==, 0);
    g_assert_cmpint(atk_hyperlink_get_end_index(hLink), ==, 1);
    g_assert_cmpint(atk_hyperlink_get_n_anchors(hLink), ==, 1);
    g_assert_cmpstr(atk_hyperlink_get_uri(hLink, 0), ==, 0);

    g_object_unref(paragraph1);
    g_object_unref(paragraph2);
    g_object_unref(paragraph3);
    g_object_unref(webView);
}

static void testWebkitAtkGetTextAtOffset()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contents, 0, 0, 0);

    /* Get to the inner AtkText object. */
    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);
    object = atk_object_ref_accessible_child(object, 0);
    g_assert(object);

    AtkText* textObject = ATK_TEXT(object);
    g_assert(ATK_IS_TEXT(textObject));

    runGetTextTests(textObject);

    g_object_unref(webView);
}

static void testWebkitAtkGetTextAtOffsetNewlines()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contentsWithNewlines, 0, 0, 0);

    /* Get to the inner AtkText object. */
    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);
    object = atk_object_ref_accessible_child(object, 0);
    g_assert(object);

    AtkText* textObject = ATK_TEXT(object);
    g_assert(ATK_IS_TEXT(textObject));

    runGetTextTests(textObject);

    g_object_unref(webView);
}

static void testWebkitAtkGetTextAtOffsetTextarea()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contentsInTextarea, 0, 0, 0);

    /* Get to the inner AtkText object. */
    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);
    object = atk_object_ref_accessible_child(object, 0);
    g_assert(object);
    object = atk_object_ref_accessible_child(object, 0);
    g_assert(object);

    AtkText* textObject = ATK_TEXT(object);
    g_assert(ATK_IS_TEXT(textObject));

    runGetTextTests(textObject);

    g_object_unref(webView);
}

static void testWebkitAtkGetTextAtOffsetTextInput()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contentsInTextInput, 0, 0, 0);

    /* Get to the inner AtkText object. */
    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);
    object = atk_object_ref_accessible_child(object, 0);
    g_assert(object);
    object = atk_object_ref_accessible_child(object, 0);
    g_assert(object);

    AtkText* textObject = ATK_TEXT(object);
    g_assert(ATK_IS_TEXT(textObject));

    runGetTextTests(textObject);

    g_object_unref(webView);
}

static void testWebkitAtkGetTextAtOffsetWithPreformattedText()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contentsWithPreformattedText, 0, 0, 0);

    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    AtkObject* preformattedText = atk_object_ref_accessible_child(object, 0);
    g_assert(ATK_IS_OBJECT(preformattedText));
    g_assert(atk_object_get_role(preformattedText) == ATK_ROLE_PANEL);
    g_assert(ATK_IS_TEXT(preformattedText));
    char* text = atk_text_get_text(ATK_TEXT(preformattedText), 0, -1);
    g_assert_cmpstr(text, ==, "\t\n\tfirst line\n\tsecond line\n");
    g_free(text);

    /* Try retrieving all the lines indicating the position of the offsets at the beginning of each of them. */
    testGetTextFunction(ATK_TEXT(preformattedText), atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_START, 0, "\t\n", 0, 2);
    testGetTextFunction(ATK_TEXT(preformattedText), atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_START, 2, "\tfirst line\n", 2, 14);
    testGetTextFunction(ATK_TEXT(preformattedText), atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_START, 14, "\tsecond line\n", 14, 27);

    g_object_unref(preformattedText);
    g_object_unref(webView);
}

static void testWebkitAtkGetTextAtOffsetWithSpecialCharacters()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contentsWithSpecialChars, 0, 0, 0);

    /* Get to the inner AtkText object. */
    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    AtkObject* paragraph = atk_object_ref_accessible_child(object, 0);
    g_assert(ATK_IS_TEXT(paragraph));

    gchar* expectedText = g_strdup("\302\253\302\240This is a paragraph with \342\200\234special\342\200\235 characters inside.\302\240\302\273");
    char* text = atk_text_get_text(ATK_TEXT(paragraph), 0, -1);
    g_assert_cmpstr(text, ==, expectedText);
    g_free(text);

    /* Check that getting the text with ATK_TEXT_BOUNDARY_LINE_START
       and ATK_TEXT_BOUNDARY_LINE_END does not crash because of not
       properly handling characters inside the UTF-8 string. */
    testGetTextFunction(ATK_TEXT(paragraph), atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_START, 0, expectedText, 0, 57);
    testGetTextFunction(ATK_TEXT(paragraph), atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_END, 0, expectedText, 0, 57);
    g_free(expectedText);

    AtkObject* list = atk_object_ref_accessible_child(object, 1);
    g_assert(ATK_OBJECT(list));

    AtkText* listItem = ATK_TEXT(atk_object_ref_accessible_child(list, 0));
    g_assert(ATK_IS_TEXT(listItem));

    text = atk_text_get_text(ATK_TEXT(listItem), 0, -1);
    g_assert_cmpstr(text, ==, "\342\200\242 List item with some text that wraps across different lines.");
    g_free(text);

    /* Check that getting the text with ATK_TEXT_BOUNDARY_LINE_START
       and ATK_TEXT_BOUNDARY_LINE_END for line items with bullets
       (special character) and wrapped text always return the right
       piece of text for each line. */
    testGetTextFunction(ATK_TEXT(listItem), atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_START, 3, "\342\200\242 List item ", 0, 12);
    testGetTextFunction(ATK_TEXT(listItem), atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_START, 13, "with some ", 12, 22);
    testGetTextFunction(ATK_TEXT(listItem), atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_END, 0, "\342\200\242 List item", 0, 11);
    testGetTextFunction(ATK_TEXT(listItem), atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_END, 12, " with some", 11, 21);

    g_object_unref(listItem);

    listItem = ATK_TEXT(atk_object_ref_accessible_child(list, 1));
    g_assert(ATK_IS_TEXT(listItem));

    /* Check that placing the same text in a paragraph doesn't break things. */
    text = atk_text_get_text(ATK_TEXT(listItem), 0, -1);
    g_assert_cmpstr(text, ==, "\342\200\242 List item with some text that wraps across different lines.");
    g_free(text);

    testGetTextFunction(ATK_TEXT(listItem), atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_START, 3, "\342\200\242 List item ", 0, 12);
    testGetTextFunction(ATK_TEXT(listItem), atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_START, 13, "with some ", 12, 22);
    testGetTextFunction(ATK_TEXT(listItem), atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_END, 0, "\342\200\242 List item", 0, 11);
    testGetTextFunction(ATK_TEXT(listItem), atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_END, 12, " with some", 11, 21);

    g_object_unref(list);
    g_object_unref(listItem);
    g_object_unref(paragraph);
    g_object_unref(webView);
}

static void testWebkitAtkGetTextAtOffsetWithWrappedLines()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contentsWithWrappedLines, 0, 0, 0);

    /* Enable caret browsing. */
    WebKitWebSettings* settings = webkit_web_view_get_settings(webView);
    g_object_set(settings, "enable-caret-browsing", TRUE, NULL);
    webkit_web_view_set_settings(webView, settings);

    /* Get to the inner AtkText object. */
    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    /* Check the paragraph with the text wrapped because of max-width. */
    AtkText* paragraph1 = ATK_TEXT(atk_object_ref_accessible_child(object, 0));
    g_assert(ATK_IS_TEXT(paragraph1));

    gchar* text = atk_text_get_text(paragraph1, 0, -1);
    g_assert_cmpstr(text, ==, "This is one line wrapped because of the maximum width of its container.");
    g_free(text);

    testGetTextFunction(paragraph1, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_CHAR, 16, "e", 15, 16);
    testGetTextFunction(paragraph1, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_CHAR, 16, " ", 16, 17);
    testGetTextFunction(paragraph1, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_CHAR, 16, "w", 17, 18);

    testGetTextFunction(paragraph1, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_WORD_START, 16, "one ", 8, 12);
    testGetTextFunction(paragraph1, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_WORD_START, 16, "line ", 12, 17);
    testGetTextFunction(paragraph1, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_WORD_START, 16, "wrapped ", 17, 25);

    testGetTextFunction(paragraph1, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_WORD_END, 16, " line", 11, 16);
    testGetTextFunction(paragraph1, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_WORD_END, 16, " wrapped", 16, 24);
    testGetTextFunction(paragraph1, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_WORD_END, 16, " because", 24, 32);

    testGetTextFunction(paragraph1, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_LINE_START, 17, "This is one line ", 0, 17);
    testGetTextFunction(paragraph1, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_START, 17, "wrapped because ", 17, 33);
    testGetTextFunction(paragraph1, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_LINE_START, 17, "of the maximum ", 33, 48);

    /* The following line won't work at the moment because of a bug in GailTextUtil.
       see https://bugzilla.gnome.org/show_bug.cgi?id=703554
       testGetTextFunction(paragraph1, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_LINE_END, 17, "This is one line", 0, 16); */
    testGetTextFunction(paragraph1, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_END, 17, " wrapped because", 16, 32);
    testGetTextFunction(paragraph1, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_LINE_END, 17, " of the maximum", 32, 47);

    g_object_unref(paragraph1);

    /* Check the paragraph with the text wrapped because of <br> elements. */
    AtkText* paragraph2 = ATK_TEXT(atk_object_ref_accessible_child(object, 1));
    g_assert(ATK_IS_TEXT(paragraph2));

    text = atk_text_get_text(paragraph2, 0, -1);
    g_assert_cmpstr(text, ==, "This is another line wrapped\nbecause of one forced\nline break in the middle.");
    g_free(text);

    testGetTextFunction(paragraph2, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_CHAR, 28, "d", 27, 28);
    testGetTextFunction(paragraph2, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_CHAR, 28, "\n", 28, 29);
    testGetTextFunction(paragraph2, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_CHAR, 28, "b", 29, 30);

    testGetTextFunction(paragraph2, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_WORD_START, 28, "line ", 16, 21);
    testGetTextFunction(paragraph2, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_WORD_START, 28, "wrapped\n", 21, 29);
    testGetTextFunction(paragraph2, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_WORD_START, 28, "because ", 29, 37);

    testGetTextFunction(paragraph2, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_WORD_END, 28, " wrapped", 20, 28);
    testGetTextFunction(paragraph2, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_WORD_END, 28, "\nbecause", 28, 36);
    testGetTextFunction(paragraph2, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_WORD_END, 28, " of", 36, 39);

    testGetTextFunction(paragraph2, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_LINE_START, 30, "This is another line wrapped\n", 0, 29);
    testGetTextFunction(paragraph2, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_START, 30, "because of one forced\n", 29, 51);
    testGetTextFunction(paragraph2, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_LINE_START, 30, "line break in the middle.", 51, 76);

    /* The following line won't work at the moment because of a bug in GailTextUtil.
       see https://bugzilla.gnome.org/show_bug.cgi?id=703554
       testGetTextFunction(paragraph2, atk_text_get_text_before_offset, ATK_TEXT_BOUNDARY_LINE_END, 30, "This is another line wrapped", 0, 28); */
    testGetTextFunction(paragraph2, atk_text_get_text_at_offset, ATK_TEXT_BOUNDARY_LINE_END, 30, "\nbecause of one forced", 28, 50);
    testGetTextFunction(paragraph2, atk_text_get_text_after_offset, ATK_TEXT_BOUNDARY_LINE_END, 30, "\nline break in the middle.", 50, 76);

    g_object_unref(paragraph2);

    g_object_unref(webView);
}

static void testWebkitAtkGetTextInParagraphAndBodySimple()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contentsInParagraphAndBodySimple, 0, 0, 0);

    /* Get to the inner AtkText object. */
    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);
    AtkObject* object1 = atk_object_ref_accessible_child(object, 0);
    g_assert(object1);
    AtkObject* object2 = atk_object_ref_accessible_child(object, 1);
    g_assert(object2);

    AtkText* textObject1 = ATK_TEXT(object1);
    g_assert(ATK_IS_TEXT(textObject1));
    AtkText* textObject2 = ATK_TEXT(object2);
    g_assert(ATK_IS_TEXT(textObject2));

    char *text = atk_text_get_text(textObject1, 0, -1);
    g_assert_cmpstr(text, ==, "This is a test.");

    text = atk_text_get_text(textObject2, 0, 12);
    g_assert_cmpstr(text, ==, "Hello world.");

    g_object_unref(object1);
    g_object_unref(object2);
    g_object_unref(webView);
}

static void testWebkitAtkGetTextInParagraphAndBodyModerate()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contentsInParagraphAndBodyModerate, 0, 0, 0);

    /* Get to the inner AtkText object. */
    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);
    AtkObject* object1 = atk_object_ref_accessible_child(object, 0);
    g_assert(object1);
    AtkObject* object2 = atk_object_ref_accessible_child(object, 1);
    g_assert(object2);

    AtkText* textObject1 = ATK_TEXT(object1);
    g_assert(ATK_IS_TEXT(textObject1));
    AtkText* textObject2 = ATK_TEXT(object2);
    g_assert(ATK_IS_TEXT(textObject2));

    char *text = atk_text_get_text(textObject1, 0, -1);
    g_assert_cmpstr(text, ==, "This is a test.");

    text = atk_text_get_text(textObject2, 0, 53);
    g_assert_cmpstr(text, ==, "Hello world.\nThis sentence is green.\nThis one is not.");

    g_object_unref(object1);
    g_object_unref(object2);
    g_object_unref(webView);
}

static void testWebkitAtkGetTextInTable()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contentsInTable, 0, 0, 0);

    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);
    object = atk_object_ref_accessible_child(object, 0);
    g_assert(object);

    /* Tables should not implement AtkText. */
    g_assert(!G_TYPE_INSTANCE_GET_INTERFACE(object, ATK_TYPE_TEXT, AtkTextIface));

    g_object_unref(object);
    g_object_unref(webView);
}

static void testWebkitAtkGetHeadersInTable()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contentsInTableWithHeaders, 0, 0, 0);

    AtkObject* axWebView = getWebAreaObject(webView);
    g_assert(axWebView);

    /* Check table with both column and row headers. */
    AtkObject* table = atk_object_ref_accessible_child(axWebView, 0);
    g_assert(table);
    g_assert(atk_object_get_role(table) == ATK_ROLE_TABLE);

    AtkObject* colHeader = atk_table_get_column_header(ATK_TABLE(table), 0);
    g_assert(colHeader);
    g_assert(atk_object_get_role(colHeader) == ATK_ROLE_TABLE_CELL);
    g_assert(atk_object_get_index_in_parent(colHeader) == 0);

    colHeader = atk_table_get_column_header(ATK_TABLE(table), 1);
    g_assert(colHeader);
    g_assert(atk_object_get_role(colHeader) == ATK_ROLE_TABLE_CELL);
    g_assert(atk_object_get_index_in_parent(colHeader) == 1);

    colHeader = atk_table_get_column_header(ATK_TABLE(table), 2);
    g_assert(colHeader);
    g_assert(atk_object_get_role(colHeader) == ATK_ROLE_TABLE_CELL);
    g_assert(atk_object_get_index_in_parent(colHeader) == 2);

    colHeader = atk_table_get_column_header(ATK_TABLE(table), 3);
    g_assert(colHeader);
    g_assert(atk_object_get_role(colHeader) == ATK_ROLE_TABLE_CELL);
    g_assert(atk_object_get_index_in_parent(colHeader) == 2);

    AtkObject* rowHeader = atk_table_get_row_header(ATK_TABLE(table), 0);
    g_assert(rowHeader);
    g_assert(atk_object_get_role(rowHeader) == ATK_ROLE_TABLE_CELL);
    g_assert(atk_object_get_index_in_parent(rowHeader) == 0);

    rowHeader = atk_table_get_row_header(ATK_TABLE(table), 1);
    g_assert(rowHeader);
    g_assert(atk_object_get_role(rowHeader) == ATK_ROLE_TABLE_CELL);
    g_assert(atk_object_get_index_in_parent(rowHeader) == 3);

    rowHeader = atk_table_get_row_header(ATK_TABLE(table), 2);
    g_assert(rowHeader);
    g_assert(atk_object_get_role(rowHeader) == ATK_ROLE_TABLE_CELL);
    g_assert(atk_object_get_index_in_parent(rowHeader) == 7);

    rowHeader = atk_table_get_row_header(ATK_TABLE(table), 3);
    g_assert(rowHeader);
    g_assert(atk_object_get_role(rowHeader) == ATK_ROLE_TABLE_CELL);
    g_assert(atk_object_get_index_in_parent(rowHeader) == 7);

    g_object_unref(table);

    /* Check table with no headers at all. */
    table = atk_object_ref_accessible_child(axWebView, 1);
    g_assert(table);
    g_assert(atk_object_get_role(table) == ATK_ROLE_TABLE);

    colHeader = atk_table_get_column_header(ATK_TABLE(table), 0);
    g_assert(colHeader == 0);

    colHeader = atk_table_get_column_header(ATK_TABLE(table), 1);
    g_assert(colHeader == 0);

    rowHeader = atk_table_get_row_header(ATK_TABLE(table), 0);
    g_assert(rowHeader == 0);

    rowHeader = atk_table_get_row_header(ATK_TABLE(table), 1);
    g_assert(rowHeader == 0);

    g_object_unref(table);
    g_object_unref(webView);
}

static gint compAtkAttribute(AtkAttribute* a1, AtkAttribute* a2)
{
    gint strcmpVal = g_strcmp0(a1->name, a2->name);
    if (strcmpVal)
        return strcmpVal;
    return g_strcmp0(a1->value, a2->value);
}

static gint compAtkAttributeName(AtkAttribute* a1, AtkAttribute* a2)
{
    return g_strcmp0(a1->name, a2->name);
}

static gboolean atkAttributeSetAttributeNameHasValue(AtkAttributeSet* set, const gchar* attributeName, const gchar* value)
{
    AtkAttribute at;
    at.name = (gchar*)attributeName;
    GSList* element = g_slist_find_custom(set, &at, (GCompareFunc)compAtkAttributeName);
    return element && !g_strcmp0(((AtkAttribute*)(element->data))->value, value);
}

static gboolean atkAttributeSetContainsAttributeName(AtkAttributeSet* set, const gchar* attributeName)
{
    AtkAttribute at;
    at.name = (gchar*)attributeName;
    return g_slist_find_custom(set, &at, (GCompareFunc)compAtkAttributeName) ? true : false;
}

static gboolean atkAttributeSetAttributeHasValue(AtkAttributeSet* set, AtkTextAttribute attribute, const gchar* value)
{
    return atkAttributeSetAttributeNameHasValue(set, atk_text_attribute_get_name(attribute), value);
}

static gboolean atkAttributeSetAreEqual(AtkAttributeSet* set1, AtkAttributeSet* set2)
{
    if (!set1)
        return !set2;

    set1 = g_slist_sort(set1, (GCompareFunc)compAtkAttribute);
    set2 = g_slist_sort(set2, (GCompareFunc)compAtkAttribute);

    while (set1) {
        if (!set2 || compAtkAttribute(set1->data, set2->data))
            return FALSE;

        set1 = set1->next;
        set2 = set2->next;
    }

    return (!set2);
}

static void testWebkitAtkTextAttributes()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, textWithAttributes, 0, 0, 0);

    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    AtkObject* child = atk_object_ref_accessible_child(object, 0);
    g_assert(child && ATK_IS_TEXT(child));
    AtkText* childText = ATK_TEXT(child);

    gint startOffset;
    gint endOffset;
    AtkAttributeSet* set1 = atk_text_get_run_attributes(childText, 0, &startOffset, &endOffset);
    g_assert_cmpint(startOffset, ==, 0);
    g_assert_cmpint(endOffset, ==, 12);
    g_assert(atkAttributeSetAreEqual(set1, 0));

    AtkAttributeSet* set2 = atk_text_get_run_attributes(childText, 15, &startOffset, &endOffset);
    g_assert_cmpint(startOffset, ==, 12);
    g_assert_cmpint(endOffset, ==, 17);
    g_assert(atkAttributeSetAttributeHasValue(set2, ATK_TEXT_ATTR_STYLE, "italic"));

    AtkAttributeSet* set3 = atk_text_get_run_attributes(childText, 17, &startOffset, &endOffset);
    g_assert_cmpint(startOffset, ==, 17);
    g_assert_cmpint(endOffset, ==, 40);
    g_assert(atkAttributeSetAttributeHasValue(set3, ATK_TEXT_ATTR_WEIGHT, "700"));

    AtkAttributeSet* set4 = atk_text_get_default_attributes(childText);
    g_assert(atkAttributeSetAttributeHasValue(set4, ATK_TEXT_ATTR_STYLE, "normal"));
    g_assert(atkAttributeSetAttributeHasValue(set4, ATK_TEXT_ATTR_JUSTIFICATION, "right"));
    g_assert(atkAttributeSetAttributeHasValue(set4, ATK_TEXT_ATTR_SIZE, "14"));
    atk_attribute_set_free(set1);
    atk_attribute_set_free(set2);
    atk_attribute_set_free(set3);
    atk_attribute_set_free(set4);

    child = atk_object_ref_accessible_child(object, 1);
    g_assert(child && ATK_IS_TEXT(child));
    childText = ATK_TEXT(child);

    set1 = atk_text_get_default_attributes(childText);
    g_assert(atkAttributeSetAttributeHasValue(set1, ATK_TEXT_ATTR_FAMILY_NAME, "monospace"));
    g_assert(atkAttributeSetAttributeHasValue(set1, ATK_TEXT_ATTR_STYLE, "normal"));
    g_assert(atkAttributeSetAttributeHasValue(set1, ATK_TEXT_ATTR_STRIKETHROUGH, "false"));
    g_assert(atkAttributeSetAttributeHasValue(set1, ATK_TEXT_ATTR_WEIGHT, "400"));
    g_assert(atkAttributeSetAttributeHasValue(set1, ATK_TEXT_ATTR_FG_COLOR, "120,121,122"));

    set2 = atk_text_get_run_attributes(childText, 43, &startOffset, &endOffset);
    g_assert_cmpint(startOffset, ==, 43);
    g_assert_cmpint(endOffset, ==, 80);
    /* Checks that default attributes of text are not returned when called to atk_text_get_run_attributes. */
    g_assert(!atkAttributeSetAttributeHasValue(set2, ATK_TEXT_ATTR_FG_COLOR, "120,121,122"));
    g_assert(atkAttributeSetAttributeHasValue(set2, ATK_TEXT_ATTR_UNDERLINE, "single"));
    g_assert(atkAttributeSetAttributeHasValue(set2, ATK_TEXT_ATTR_BG_COLOR, "80,81,82"));
    atk_attribute_set_free(set1);
    atk_attribute_set_free(set2);

    child = atk_object_ref_accessible_child(object, 2);
    g_assert(child && ATK_IS_TEXT(child));
    childText = ATK_TEXT(child);

    set1 = atk_text_get_run_attributes(childText, 0, &startOffset, &endOffset);
    set2 = atk_text_get_run_attributes(childText, 3, &startOffset, &endOffset);
    g_assert(atkAttributeSetAreEqual(set1, set2));
    atk_attribute_set_free(set2);

    set2 = atk_text_get_run_attributes(childText, 1, &startOffset, &endOffset);
    set3 = atk_text_get_run_attributes(childText, 5, &startOffset, &endOffset);
    g_assert(atkAttributeSetAreEqual(set2, set3));
    g_assert(!atkAttributeSetAreEqual(set1, set2));
    atk_attribute_set_free(set3);

    set3 = atk_text_get_run_attributes(childText, 2, &startOffset, &endOffset);
    set4 = atk_text_get_run_attributes(childText, 6, &startOffset, &endOffset);
    g_assert(atkAttributeSetAreEqual(set3, set4));
    g_assert(!atkAttributeSetAreEqual(set1, set3));
    g_assert(!atkAttributeSetAreEqual(set2, set3));
    atk_attribute_set_free(set1);
    atk_attribute_set_free(set2);
    atk_attribute_set_free(set3);
    atk_attribute_set_free(set4);

    child = atk_object_ref_accessible_child(object, 3);
    g_assert(child && ATK_IS_TEXT(child));
    childText = ATK_TEXT(child);
    set1 = atk_text_get_run_attributes(childText, 24, &startOffset, &endOffset);
    g_assert_cmpint(startOffset, ==, 21);
    g_assert_cmpint(endOffset, ==, 25);
    g_assert(atkAttributeSetAttributeHasValue(set1, ATK_TEXT_ATTR_STRIKETHROUGH, "true"));

    set2 = atk_text_get_run_attributes(childText, 25, &startOffset, &endOffset);
    g_assert_cmpint(startOffset, ==, 25);
    g_assert_cmpint(endOffset, ==, 30);
    g_assert(atkAttributeSetAreEqual(set2, 0));

    set3 = atk_text_get_default_attributes(childText);
    g_assert(atkAttributeSetAttributeHasValue(set3, ATK_TEXT_ATTR_JUSTIFICATION, "center"));
    atk_attribute_set_free(set1);
    atk_attribute_set_free(set2);
    atk_attribute_set_free(set3);
}

static gchar* textSelectionChangedResult = 0;

static void textSelectionChangedCallback(AtkText* text, gpointer data)
{
    g_assert(ATK_IS_TEXT(text));

    g_free(textSelectionChangedResult);
    AtkRole role = atk_object_get_role(ATK_OBJECT(text));
    int startOffset = 0;
    int endOffset = 0;
    atk_text_get_selection(ATK_TEXT(text), 0, &startOffset, &endOffset);
    textSelectionChangedResult = g_strdup_printf("|%s|%d|%d|", atk_role_get_name(role), startOffset, endOffset);
}

static void testWebkitAtkTextSelections()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, textForSelections, 0, 0, 0);

    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    AtkText* paragraph1 = ATK_TEXT(atk_object_ref_accessible_child(object, 0));
    g_assert(ATK_IS_TEXT(paragraph1));
    g_signal_connect(paragraph1, "text-selection-changed", G_CALLBACK(textSelectionChangedCallback), 0);

    AtkText* paragraph2 = ATK_TEXT(atk_object_ref_accessible_child(object, 1));
    g_assert(ATK_IS_TEXT(paragraph2));
    g_signal_connect(paragraph2, "text-selection-changed", G_CALLBACK(textSelectionChangedCallback), 0);

    AtkText* link = ATK_TEXT(atk_object_ref_accessible_child(ATK_OBJECT(paragraph2), 0));
    g_assert(ATK_IS_TEXT(link));

    AtkObject* list = atk_object_ref_accessible_child(object, 2);
    g_assert(ATK_OBJECT(list));

    AtkText* listItem = ATK_TEXT(atk_object_ref_accessible_child(list, 0));
    g_assert(ATK_IS_TEXT(listItem));

    /* First paragraph (simple text). */

    /* Basic initial checks. */
    g_assert_cmpint(atk_text_get_n_selections(paragraph1), ==, 0);

    gint startOffset;
    gint endOffset;
    gchar* selectedText = atk_text_get_selection(paragraph1, 0, &startOffset, &endOffset);
    g_assert_cmpint(startOffset, ==, 0);
    g_assert_cmpint(endOffset, ==, 0);
    g_assert_cmpstr(selectedText, ==, 0);
    g_free (selectedText);

    /* Try removing a non existing (yet) selection. */
    gboolean result = atk_text_remove_selection(paragraph1, 0);
    g_assert(!result);

    /* Try setting a 0-char selection. */
    result = atk_text_set_selection(paragraph1, 0, 5, 5);
    g_assert(result);

    /* Make a selection and test it. */
    result = atk_text_set_selection(paragraph1, 0, 5, 25);
    g_assert(result);
    g_assert_cmpint(atk_text_get_n_selections(paragraph1), ==, 1);
    g_assert_cmpstr(textSelectionChangedResult, ==, "|paragraph|5|25|");
    selectedText = atk_text_get_selection(paragraph1, 0, &startOffset, &endOffset);
    g_assert_cmpint(startOffset, ==, 5);
    g_assert_cmpint(endOffset, ==, 25);
    g_assert_cmpstr(selectedText, ==, "agraph with plain te");
    g_free (selectedText);

    /* Try removing the selection from other AtkText object (should fail). */
    result = atk_text_remove_selection(paragraph2, 0);
    g_assert(!result);

    /* Remove the selection and test everything again. */
    result = atk_text_remove_selection(paragraph1, 0);
    g_assert(result);
    g_assert_cmpint(atk_text_get_n_selections(paragraph1), ==, 0);
    selectedText = atk_text_get_selection(paragraph1, 0, &startOffset, &endOffset);
    /* Now offsets should be the same, set to the last position of the caret. */
    g_assert_cmpint(startOffset, ==, endOffset);
    g_assert_cmpint(startOffset, ==, 25);
    g_assert_cmpint(endOffset, ==, 25);
    g_assert_cmpstr(selectedText, ==, 0);
    g_free (selectedText);

    /* Second paragraph (text + link + text). */

    /* Set a selection partially covering the link and test it. */
    result = atk_text_set_selection(paragraph2, 0, 7, 21);
    g_assert(result);

    /* Test the paragraph first. */
    g_assert_cmpint(atk_text_get_n_selections(paragraph2), ==, 1);
    selectedText = atk_text_get_selection(paragraph2, 0, &startOffset, &endOffset);
    g_assert_cmpint(startOffset, ==, 7);
    g_assert_cmpint(endOffset, ==, 21);
    g_assert_cmpstr(selectedText, ==, "raph with a li");
    g_free (selectedText);

    /* Now test just the link. */
    g_assert_cmpint(atk_text_get_n_selections(link), ==, 1);
    selectedText = atk_text_get_selection(link, 0, &startOffset, &endOffset);
    g_assert_cmpint(startOffset, ==, 0);
    g_assert_cmpint(endOffset, ==, 4);
    g_assert_cmpstr(selectedText, ==, "a li");
    g_free (selectedText);

    /* Make a selection after the link and check selection for the whole paragraph. */
    result = atk_text_set_selection(paragraph2, 0, 27, 37);
    g_assert(result);
    g_assert_cmpint(atk_text_get_n_selections(paragraph2), ==, 1);
    g_assert_cmpstr(textSelectionChangedResult, ==, "|paragraph|27|37|");
    selectedText = atk_text_get_selection(paragraph2, 0, &startOffset, &endOffset);
    g_assert_cmpint(startOffset, ==, 27);
    g_assert_cmpint(endOffset, ==, 37);
    g_assert_cmpstr(selectedText, ==, "the middle");
    g_free (selectedText);

    /* Remove selections and text everything again. */
    result = atk_text_remove_selection(paragraph2, 0);
    g_assert(result);
    g_assert_cmpint(atk_text_get_n_selections(paragraph2), ==, 0);
    selectedText = atk_text_get_selection(paragraph2, 0, &startOffset, &endOffset);
    /* Now offsets should be the same (no selection). */
    g_assert_cmpint(startOffset, ==, endOffset);
    g_assert_cmpstr(selectedText, ==, 0);
    g_free (selectedText);

    g_assert_cmpint(atk_text_get_n_selections(link), ==, 0);
    selectedText = atk_text_get_selection(link, 0, &startOffset, &endOffset);
    /* Now offsets should be the same (no selection). */
    g_assert_cmpint(startOffset, ==, endOffset);
    g_assert_cmpstr(selectedText, ==, 0);
    g_free (selectedText);

    /* List item */

    g_assert(atk_object_get_role(list) == ATK_ROLE_LIST);
    g_assert_cmpint(atk_object_get_n_accessible_children(list), ==, 1);

    gchar* text = atk_text_get_text(listItem, 0, -1);
    g_assert_cmpstr(text, ==, "1. A list item");
    g_free (text);

    /* It's not possible to select text inside an item's marker. */
    result = atk_text_set_selection (listItem, 0, 0, 9);
    g_assert(!result);
    result = atk_text_set_selection (listItem, 0, 9, 1);
    g_assert(!result);

    /* It should be possible to select text inside an item's text. */
    result = atk_text_set_selection (listItem, 0, 3, 9);
    g_assert(result);

    g_assert_cmpint(atk_text_get_n_selections(listItem), ==, 1);
    selectedText = atk_text_get_selection(listItem, 0, &startOffset, &endOffset);
    g_assert_cmpint(startOffset, ==, 3);
    g_assert_cmpint(endOffset, ==, 9);
    g_assert_cmpstr(selectedText, ==, "A list");
    g_free (selectedText);

    g_free(textSelectionChangedResult);

    g_object_unref(paragraph1);
    g_object_unref(paragraph2);
    g_object_unref(list);
    g_object_unref(listItem);
    g_object_unref(webView);
}

static void testWebkitAtkGetExtents()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, centeredContents, 0, 0, 0);

    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    AtkText* shortText1 = ATK_TEXT(atk_object_ref_accessible_child(object, 0));
    g_assert(ATK_IS_TEXT(shortText1));
    AtkText* longText = ATK_TEXT(atk_object_ref_accessible_child(object, 1));
    g_assert(ATK_IS_TEXT(longText));
    AtkText* shortText2 = ATK_TEXT(atk_object_ref_accessible_child(object, 2));
    g_assert(ATK_IS_TEXT(shortText2));
    AtkText* multilineText = ATK_TEXT(atk_object_ref_accessible_child(object, 3));
    g_assert(ATK_IS_TEXT(multilineText));

    /* Start with window extents. */
    AtkTextRectangle sline_window1, sline_window2, lline_window, mline_window;
    atk_text_get_range_extents(shortText1, 0, 10, ATK_XY_WINDOW, &sline_window1);
    atk_text_get_range_extents(longText, 0, 44, ATK_XY_WINDOW, &lline_window);
    atk_text_get_range_extents(shortText2, 0, 10, ATK_XY_WINDOW, &sline_window2);
    atk_text_get_range_extents(multilineText, 0, 60, ATK_XY_WINDOW, &mline_window);

    /* Check vertical line position. */
    g_assert_cmpint(sline_window1.y + sline_window1.height, <=, lline_window.y);
    g_assert_cmpint(lline_window.y + lline_window.height + sline_window2.height, <=, mline_window.y);

    /* Paragraphs 1 and 3 have identical text and alignment. */
    g_assert_cmpint(sline_window1.x, ==, sline_window2.x);
    g_assert_cmpint(sline_window1.width, ==, sline_window2.width);
    g_assert_cmpint(sline_window1.height, ==, sline_window2.height);

    /* All lines should be the same height; line 2 is the widest line. */
    g_assert_cmpint(sline_window1.height, ==, lline_window.height);
    g_assert_cmpint(sline_window1.width, <, lline_window.width);

    /* Make sure the character extents jive with the range extents. */
    gint x;
    gint y;
    gint width;
    gint height;

    /* First paragraph (short text). */
    atk_text_get_character_extents(shortText1, 0, &x, &y, &width, &height, ATK_XY_WINDOW);
    g_assert_cmpint(x, ==, sline_window1.x);
    g_assert_cmpint(y, ==, sline_window1.y);
    g_assert_cmpint(height, ==, sline_window1.height);

    atk_text_get_character_extents(shortText1, 9, &x, &y, &width, &height, ATK_XY_WINDOW);
    g_assert_cmpint(x, ==, sline_window1.x + sline_window1.width - width);
    g_assert_cmpint(y, ==, sline_window1.y);
    g_assert_cmpint(height, ==, sline_window1.height);

    /* Second paragraph (long text). */
    atk_text_get_character_extents(longText, 0, &x, &y, &width, &height, ATK_XY_WINDOW);
    g_assert_cmpint(x, ==, lline_window.x);
    g_assert_cmpint(y, ==, lline_window.y);
    g_assert_cmpint(height, ==, lline_window.height);

    atk_text_get_character_extents(longText, 43, &x, &y, &width, &height, ATK_XY_WINDOW);
    g_assert_cmpint(x, ==, lline_window.x + lline_window.width - width);
    g_assert_cmpint(y, ==, lline_window.y);
    g_assert_cmpint(height, ==, lline_window.height);

    /* Third paragraph (short text). */
    atk_text_get_character_extents(shortText2, 0, &x, &y, &width, &height, ATK_XY_WINDOW);
    g_assert_cmpint(x, ==, sline_window2.x);
    g_assert_cmpint(y, ==, sline_window2.y);
    g_assert_cmpint(height, ==, sline_window2.height);

    atk_text_get_character_extents(shortText2, 9, &x, &y, &width, &height, ATK_XY_WINDOW);
    g_assert_cmpint(x, ==, sline_window2.x + sline_window2.width - width);
    g_assert_cmpint(y, ==, sline_window2.y);
    g_assert_cmpint(height, ==, sline_window2.height);

    /* Four paragraph (3 lines multi-line text). */
    atk_text_get_character_extents(multilineText, 0, &x, &y, &width, &height, ATK_XY_WINDOW);
    g_assert_cmpint(x, ==, mline_window.x);
    g_assert_cmpint(y, ==, mline_window.y);
    g_assert_cmpint(3 * height, ==, mline_window.height);

    atk_text_get_character_extents(multilineText, 59, &x, &y, &width, &height, ATK_XY_WINDOW);
    /* Last line won't fill the whole width of the rectangle. */
    g_assert_cmpint(x, <=, mline_window.x + mline_window.width - width);
    g_assert_cmpint(y, ==, mline_window.y + mline_window.height - height);
    g_assert_cmpint(height, <=, mline_window.height);

    /* Check that extent for a full line are the same height than for
       a partial section of the same line */
    gint startOffset;
    gint endOffset;
    gchar* text = atk_text_get_text_at_offset(multilineText, 0, ATK_TEXT_BOUNDARY_LINE_START, &startOffset, &endOffset);
    g_free(text);

    AtkTextRectangle fline_window;
    AtkTextRectangle afline_window;
    atk_text_get_range_extents(multilineText, startOffset, endOffset, ATK_XY_WINDOW, &fline_window);
    atk_text_get_range_extents(multilineText, startOffset, endOffset - 1, ATK_XY_WINDOW, &afline_window);
    g_assert_cmpint(fline_window.x, ==, afline_window.x);
    g_assert_cmpint(fline_window.y, ==, afline_window.y);
    g_assert_cmpint(fline_window.height, ==, afline_window.height);

    g_object_unref(shortText1);
    g_object_unref(shortText2);
    g_object_unref(longText);
    g_object_unref(multilineText);
    g_object_unref(webView);
}

static void testWebkitAtkLayoutAndDataTables()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, layoutAndDataTables, 0, 0, 0);

    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    /* Check the non-layout table (data table). */

    AtkObject* table1 = atk_object_ref_accessible_child(object, 0);
    g_assert(ATK_IS_TABLE(table1));
    AtkAttributeSet* set1 = atk_object_get_attributes(table1);
    g_assert(set1);
    g_assert(!atkAttributeSetContainsAttributeName(set1, "layout-guess"));
    atk_attribute_set_free(set1);

    /* Check the layout table. */

    AtkObject* table2 = atk_object_ref_accessible_child(object, 1);
    g_assert(ATK_IS_TABLE(table2));
    AtkAttributeSet* set2 = atk_object_get_attributes(table2);
    g_assert(set2);
    g_assert(atkAttributeSetContainsAttributeName(set2, "layout-guess"));
    g_assert(atkAttributeSetAttributeNameHasValue(set2, "layout-guess", "true"));
    atk_attribute_set_free(set2);

    g_object_unref(table1);
    g_object_unref(table2);
    g_object_unref(webView);
}

static void testWebkitAtkLinksWithInlineImages()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, linksWithInlineImages, 0, 0, 0);

    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    /* First paragraph (link at the beginning). */
    AtkObject* paragraph = atk_object_ref_accessible_child(object, 0);
    g_assert(ATK_IS_TEXT(paragraph));
    gint startOffset;
    gint endOffset;
    gchar* text = atk_text_get_text_at_offset(ATK_TEXT(paragraph), 0, ATK_TEXT_BOUNDARY_LINE_START, &startOffset, &endOffset);
    g_assert(text);
    g_assert_cmpstr(text, ==, "foo bar baz");
    g_assert_cmpint(startOffset, ==, 0);
    g_assert_cmpint(endOffset, ==, 11);
    g_free(text);
    g_object_unref(paragraph);

    /* Second paragraph (link in the middle). */
    paragraph = atk_object_ref_accessible_child(object, 1);
    g_assert(ATK_IS_TEXT(paragraph));
    text = atk_text_get_text_at_offset(ATK_TEXT(paragraph), 0, ATK_TEXT_BOUNDARY_LINE_START, &startOffset, &endOffset);
    g_assert(text);
    g_assert_cmpstr(text, ==, "foo bar baz");
    g_assert_cmpint(startOffset, ==, 0);
    g_assert_cmpint(endOffset, ==, 11);
    g_free(text);
    g_object_unref(paragraph);

    /* Third paragraph (link at the end). */
    paragraph = atk_object_ref_accessible_child(object, 2);
    g_assert(ATK_IS_TEXT(paragraph));
    text = atk_text_get_text_at_offset(ATK_TEXT(paragraph), 0, ATK_TEXT_BOUNDARY_LINE_START, &startOffset, &endOffset);
    g_assert(text);
    g_assert_cmpstr(text, ==, "foo bar baz");
    g_assert_cmpint(startOffset, ==, 0);
    g_assert_cmpint(endOffset, ==, 11);
    g_free(text);
    g_object_unref(paragraph);

    g_object_unref(webView);
}

static void testWebkitAtkHypertextAndHyperlinks()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, hypertextAndHyperlinks, 0, 0, 0);

    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    AtkObject* paragraph1 = atk_object_ref_accessible_child(object, 0);
    g_assert(ATK_OBJECT(paragraph1));
    g_assert(atk_object_get_role(paragraph1) == ATK_ROLE_PARAGRAPH);
    g_assert(ATK_IS_HYPERTEXT(paragraph1));

    /* No links in the first paragraph. */
    gint nLinks = atk_hypertext_get_n_links(ATK_HYPERTEXT(paragraph1));
    g_assert_cmpint(nLinks, ==, 0);

    AtkObject* paragraph2 = atk_object_ref_accessible_child(object, 1);
    g_assert(ATK_OBJECT(paragraph2));
    g_assert(atk_object_get_role(paragraph2) == ATK_ROLE_PARAGRAPH);
    g_assert(ATK_IS_HYPERTEXT(paragraph2));

    /* Check links in the second paragraph.
       nLinks = atk_hypertext_get_n_links(ATK_HYPERTEXT(paragraph2));
       g_assert_cmpint(nLinks, ==, 3); */

    AtkHyperlink* hLink1 = atk_hypertext_get_link(ATK_HYPERTEXT(paragraph2), 0);
    g_assert(ATK_HYPERLINK(hLink1));
    AtkObject* hLinkObject1 = atk_hyperlink_get_object(hLink1, 0);
    g_assert(ATK_OBJECT(hLinkObject1));
    g_assert(atk_object_get_role(hLinkObject1) == ATK_ROLE_LINK);
    g_assert_cmpint(atk_hyperlink_get_start_index(hLink1), ==, 0);
    g_assert_cmpint(atk_hyperlink_get_end_index(hLink1), ==, 6);
    g_assert_cmpint(atk_hyperlink_get_n_anchors(hLink1), ==, 1);
    g_assert_cmpstr(atk_hyperlink_get_uri(hLink1, 0), ==, "http://foo.bar.baz/");

    AtkHyperlink* hLink2 = atk_hypertext_get_link(ATK_HYPERTEXT(paragraph2), 1);
    g_assert(ATK_HYPERLINK(hLink2));
    AtkObject* hLinkObject2 = atk_hyperlink_get_object(hLink2, 0);
    g_assert(ATK_OBJECT(hLinkObject2));
    g_assert(atk_object_get_role(hLinkObject2) == ATK_ROLE_LINK);
    g_assert_cmpint(atk_hyperlink_get_start_index(hLink2), ==, 12);
    g_assert_cmpint(atk_hyperlink_get_end_index(hLink2), ==, 32);
    g_assert_cmpint(atk_hyperlink_get_n_anchors(hLink2), ==, 1);
    g_assert_cmpstr(atk_hyperlink_get_uri(hLink2, 0), ==, "http://bar.baz.foo/");

    AtkHyperlink* hLink3 = atk_hypertext_get_link(ATK_HYPERTEXT(paragraph2), 2);
    g_assert(ATK_HYPERLINK(hLink3));
    AtkObject* hLinkObject3 = atk_hyperlink_get_object(hLink3, 0);
    g_assert(ATK_OBJECT(hLinkObject3));
    g_assert(atk_object_get_role(hLinkObject3) == ATK_ROLE_LINK);
    g_assert_cmpint(atk_hyperlink_get_start_index(hLink3), ==, 65);
    g_assert_cmpint(atk_hyperlink_get_end_index(hLink3), ==, 75);
    g_assert_cmpint(atk_hyperlink_get_n_anchors(hLink3), ==, 1);
    g_assert_cmpstr(atk_hyperlink_get_uri(hLink3, 0), ==, "http://baz.foo.bar/");

    AtkObject* list = atk_object_ref_accessible_child(object, 2);
    g_assert(ATK_OBJECT(list));
    g_assert(atk_object_get_role(list) == ATK_ROLE_LIST);
    g_assert_cmpint(atk_object_get_n_accessible_children(list), ==, 1);

    AtkObject* listItem = atk_object_ref_accessible_child(list, 0);
    g_assert(ATK_IS_TEXT(listItem));
    g_assert(ATK_IS_HYPERTEXT(listItem));

    AtkHyperlink* hLinkInListItem = atk_hypertext_get_link(ATK_HYPERTEXT(listItem), 0);
    g_assert(ATK_HYPERLINK(hLinkInListItem));
    AtkObject* hLinkObject = atk_hyperlink_get_object(hLinkInListItem, 0);
    g_assert(ATK_OBJECT(hLinkObject));
    g_assert(atk_object_get_role(hLinkObject) == ATK_ROLE_LINK);
    g_assert_cmpint(atk_hyperlink_get_start_index(hLinkInListItem), ==, 20);
    g_assert_cmpint(atk_hyperlink_get_end_index(hLinkInListItem), ==, 43);
    g_assert_cmpint(atk_hyperlink_get_n_anchors(hLinkInListItem), ==, 1);
    g_assert_cmpstr(atk_hyperlink_get_uri(hLinkInListItem, 0), ==, "http://foo.bar.baz/");

    /* Finally check the AtkAction interface for a given AtkHyperlink. */
    g_assert(ATK_IS_ACTION(hLink1));
    g_assert_cmpint(atk_action_get_n_actions(ATK_ACTION(hLink1)), ==, 1);
    g_assert_cmpstr(atk_action_get_keybinding(ATK_ACTION(hLink1), 0), ==, "");
    g_assert_cmpstr(atk_action_get_name(ATK_ACTION(hLink1), 0), ==, "jump");
    g_assert(atk_action_do_action(ATK_ACTION(hLink1), 0));

    g_object_unref(paragraph1);
    g_object_unref(paragraph2);
    g_object_unref(list);
    g_object_unref(listItem);
    g_object_unref(webView);
}

static void testWebkitAtkListsOfItems()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, listsOfItems, 0, 0, 0);

    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    /* Unordered list. */

    AtkObject* uList = atk_object_ref_accessible_child(object, 0);
    g_assert(ATK_OBJECT(uList));
    g_assert(atk_object_get_role(uList) == ATK_ROLE_LIST);
    g_assert_cmpint(atk_object_get_n_accessible_children(uList), ==, 3);

    AtkObject* item1 = atk_object_ref_accessible_child(uList, 0);
    g_assert(ATK_IS_TEXT(item1));
    AtkObject* item2 = atk_object_ref_accessible_child(uList, 1);
    g_assert(ATK_IS_TEXT(item2));
    AtkObject* item3 = atk_object_ref_accessible_child(uList, 2);
    g_assert(ATK_IS_TEXT(item3));

    g_assert_cmpint(atk_object_get_n_accessible_children(item1), ==, 0);
    g_assert_cmpint(atk_object_get_n_accessible_children(item2), ==, 1);
    g_assert_cmpint(atk_object_get_n_accessible_children(item3), ==, 1);

    g_assert_cmpstr(atk_text_get_text(ATK_TEXT(item1), 0, -1), ==, "\342\200\242 text only");
    g_assert_cmpstr(atk_text_get_text(ATK_TEXT(item2), 0, -1), ==, "\342\200\242 link only");
    g_assert_cmpstr(atk_text_get_text(ATK_TEXT(item3), 0, -1), ==, "\342\200\242 text and a link");

    g_object_unref(item1);
    g_object_unref(item2);
    g_object_unref(item3);

    /* Ordered list. */

    AtkObject* oList = atk_object_ref_accessible_child(object, 1);
    g_assert(ATK_OBJECT(oList));
    g_assert(atk_object_get_role(oList) == ATK_ROLE_LIST);
    g_assert_cmpint(atk_object_get_n_accessible_children(oList), ==, 3);

    item1 = atk_object_ref_accessible_child(oList, 0);
    g_assert(ATK_IS_TEXT(item1));
    item2 = atk_object_ref_accessible_child(oList, 1);
    g_assert(ATK_IS_TEXT(item2));
    item3 = atk_object_ref_accessible_child(oList, 2);
    g_assert(ATK_IS_TEXT(item3));

    g_assert_cmpstr(atk_text_get_text(ATK_TEXT(item1), 0, -1), ==, "1. text only");
    g_assert_cmpstr(atk_text_get_text(ATK_TEXT(item2), 0, -1), ==, "2. link only");
    g_assert_cmpstr(atk_text_get_text(ATK_TEXT(item3), 0, -1), ==, "3. text and a link");

    g_assert_cmpint(atk_object_get_n_accessible_children(item1), ==, 0);
    g_assert_cmpint(atk_object_get_n_accessible_children(item2), ==, 1);
    g_assert_cmpint(atk_object_get_n_accessible_children(item3), ==, 1);

    g_object_unref(item1);
    g_object_unref(item2);
    g_object_unref(item3);

    g_object_unref(uList);
    g_object_unref(oList);
    g_object_unref(webView);
}

typedef enum {
  TEXT_CHANGE_INSERT = 1,
  TEXT_CHANGE_REMOVE = 2
} TextChangeType;

static gchar* textChangedResult = 0;

static void textChangedCb(AtkText* text, gint pos, gint len, gchar* modifiedText, gpointer data)
{
    g_assert(text && ATK_IS_OBJECT(text));

    TextChangeType type = GPOINTER_TO_INT(data);
    g_free(textChangedResult);
    textChangedResult = g_strdup_printf("|%d|%d|%d|'%s'|", type, pos, len, modifiedText);
}

static void testWebkitAtkTextChangedNotifications()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, formWithTextInputs, 0, 0, 0);

    AtkObject* object = getWebAreaObject(webView);
    g_assert(object);

    AtkObject* form = atk_object_ref_accessible_child(object, 0);
    g_assert(ATK_IS_OBJECT(form));

    /* First check normal text entries. */
    AtkObject* textEntry = atk_object_ref_accessible_child(form, 0);
    g_assert(ATK_IS_EDITABLE_TEXT(textEntry));
    g_assert(atk_object_get_role(ATK_OBJECT(textEntry)) == ATK_ROLE_ENTRY);

    g_signal_connect(textEntry, "text-insert",
                     G_CALLBACK(textChangedCb),
                     GINT_TO_POINTER(TEXT_CHANGE_INSERT));
    g_signal_connect(textEntry, "text-remove",
                     G_CALLBACK(textChangedCb),
                     GINT_TO_POINTER(TEXT_CHANGE_REMOVE));

    gint pos = 0;
    atk_editable_text_insert_text(ATK_EDITABLE_TEXT(textEntry), "foo bar baz", 11, &pos);
    char* text = atk_text_get_text(ATK_TEXT(textEntry), 0, -1);
    g_assert_cmpstr(text, ==, "foo bar baz");
    g_assert_cmpstr(textChangedResult, ==, "|1|0|11|'foo bar baz'|");
    g_free(text);

    atk_editable_text_delete_text(ATK_EDITABLE_TEXT(textEntry), 4, 7);
    text = atk_text_get_text(ATK_TEXT(textEntry), 0, -1);
    g_assert_cmpstr(text, ==, "foo  baz");
    g_assert_cmpstr(textChangedResult, ==, "|2|4|3|'bar'|");
    g_free(text);

    pos = 4;
    atk_editable_text_insert_text(ATK_EDITABLE_TEXT(textEntry), "qux quux tobeignored", 8, &pos);
    text = atk_text_get_text(ATK_TEXT(textEntry), 0, -1);
    g_assert_cmpstr(text, ==, "foo qux quux baz");
    g_assert_cmpstr(textChangedResult, ==, "|1|4|8|'qux quux'|");
    g_free(text);

    /* Now check for password entries. */
    AtkObject* passwordEntry = atk_object_ref_accessible_child(form, 1);
    g_assert(ATK_IS_EDITABLE_TEXT(passwordEntry));
    g_assert(atk_object_get_role(ATK_OBJECT(passwordEntry)) == ATK_ROLE_PASSWORD_TEXT);

    g_signal_connect(passwordEntry, "text-insert",
                     G_CALLBACK(textChangedCb),
                     GINT_TO_POINTER(TEXT_CHANGE_INSERT));
    g_signal_connect(passwordEntry, "text-remove",
                     G_CALLBACK(textChangedCb),
                     GINT_TO_POINTER(TEXT_CHANGE_REMOVE));

    pos = 0;
    /* A single bullet character is '\342\200\242' */
    atk_editable_text_insert_text(ATK_EDITABLE_TEXT(passwordEntry), "foobar", 6, &pos);
    g_assert_cmpstr(textChangedResult, ==, "|1|0|6|'\342\200\242\342\200\242\342\200\242\342\200\242\342\200\242\342\200\242'|");
    text = atk_text_get_text(ATK_TEXT(passwordEntry), 0, -1);
    g_assert_cmpstr(text, ==, "\342\200\242\342\200\242\342\200\242\342\200\242\342\200\242\342\200\242");
    g_free(text);

    atk_editable_text_delete_text(ATK_EDITABLE_TEXT(passwordEntry), 2, 4);
    g_assert_cmpstr(textChangedResult, ==, "|2|2|2|'\342\200\242\342\200\242'|");

    text = atk_text_get_text(ATK_TEXT(passwordEntry), 0, -1);
    g_assert_cmpstr(text, ==, "\342\200\242\342\200\242\342\200\242\342\200\242");
    g_free(text);

    pos = 3;
    atk_editable_text_insert_text(ATK_EDITABLE_TEXT(passwordEntry), "qux tobeignored", 3, &pos);
    g_assert_cmpstr(textChangedResult, ==, "|1|3|3|'\342\200\242\342\200\242\342\200\242'|");

    text = atk_text_get_text(ATK_TEXT(passwordEntry), 0, -1);
    g_assert_cmpstr(text, ==, "\342\200\242\342\200\242\342\200\242\342\200\242\342\200\242\342\200\242\342\200\242");
    g_free(text);

    g_free(textChangedResult);

    g_object_unref(form);
    g_object_unref(textEntry);
    g_object_unref(passwordEntry);
    g_object_unref(webView);
}

static void testWebkitAtkParentForRootObject()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contents, 0, 0, 0);

    /* We need a parent container widget for the webview. */
    GtkWidget* parentContainer = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_object_ref_sink(parentContainer);
    gtk_container_add(GTK_CONTAINER(parentContainer), GTK_WIDGET(webView));

    AtkObject* axParent = gtk_widget_get_accessible(parentContainer);
    g_assert(ATK_IS_OBJECT(axParent));

    AtkObject* axRoot = gtk_widget_get_accessible(GTK_WIDGET(webView));
    g_assert(ATK_IS_OBJECT(axRoot));

    /* The child for the parent container's accessibility object
       should be the accessibility object for the WebView's root. */
    AtkObject* axParentChild = atk_object_ref_accessible_child(axParent, 0);
    g_assert(axParentChild == axRoot);

    /* Bottom-up navigation should match top-down one. */
    g_assert(atk_object_get_parent(axParentChild) == axParent);

    g_object_unref(axParentChild);
    g_object_unref(parentContainer);
}

static void testWebkitAtkSetParentForObject()
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    GtkAllocation allocation = { 0, 0, 800, 600 };
    gtk_widget_size_allocate(GTK_WIDGET(webView), &allocation);
    webkit_web_view_load_string(webView, contents, 0, 0, 0);

    /* Put the webview in a parent container widget to check that the
       normal behaviour keeps working as expected by default. */
    GtkWidget* parentContainer = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_object_ref_sink(parentContainer);
    gtk_container_add(GTK_CONTAINER(parentContainer), GTK_WIDGET(webView));

    AtkObject* axRoot = gtk_widget_get_accessible(GTK_WIDGET(webView));
    g_assert(ATK_IS_OBJECT(axRoot));

    AtkObject* axParent = gtk_widget_get_accessible(parentContainer);
    g_assert(ATK_IS_OBJECT(axParent));

    /* The parent of the root object is the parent container's a11y object. */
    g_assert(atk_object_get_parent(axRoot) == axParent);

    /* We now need to use something as a an alternative parent for
       the a11y object associated with the root of the DOM tree. */
    GtkWidget* alternativeParent = gtk_button_new();
    g_object_ref_sink(alternativeParent);

    AtkObject* axAlternativeParent = gtk_widget_get_accessible(alternativeParent);
    g_assert(ATK_IS_OBJECT(axAlternativeParent));

    /* Manually set the alternative parent's accessibility object as
       the parent for the WebKit accessibility root object and check. */
    atk_object_set_parent(axRoot, axAlternativeParent);
    g_assert(atk_object_get_parent(axRoot) == axAlternativeParent);

    g_object_unref(alternativeParent);
    g_object_unref(parentContainer);
}

/* FIXME: Please remove this function and replace its usage by
   gtk_test_init() when upgrading to GTK 3.2 or greater. */
static void initializeTestingFramework(int argc, char** argv)
{
    /* Ensure GAIL is the only module loaded. */
    g_setenv("GTK_MODULES", "gail", TRUE);

    /* Following lines were taken from gtk_test_init(). */
    g_test_init(&argc, &argv, 0);
    gtk_disable_setlocale();
    setlocale(LC_ALL, "C");

#ifndef GTK_API_VERSION_2
    /* gdk_disable_multidevice() available since GTK+ 3.0 only. */
    gdk_disable_multidevice();
#endif

    gtk_init(&argc, &argv);
}

int main(int argc, char** argv)
{
  /* We can't just call to gtk_test_init() in this case because its
     implementation makes sure that no GTK+ module will be loaded, and
     we will need to load GAIL for tests that need to use AtkObjects
     from non-WebKit GtkWidgets (e.g parentForRootObject). However, it
     shouldn't be needed to do this in the future, as GAIL won't longer
     be a separate module (but part of GTK+) since GTK+ 3.2 on. */
    initializeTestingFramework(argc, argv);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add_func("/webkit/atk/caretOffsets", testWebkitAtkCaretOffsets);
    g_test_add_func("/webkit/atk/caretOffsetsAndExtranousWhiteSpaces", testWebkitAtkCaretOffsetsAndExtranousWhiteSpaces);
    g_test_add_func("/webkit/atk/comboBox", testWebkitAtkComboBox);
    g_test_add_func("/webkit/atk/documentLoadingEvents", testWebkitAtkDocumentLoadingEvents);
    g_test_add_func("/webkit/atk/embeddedObjects", testWebkitAtkEmbeddedObjects);
    g_test_add_func("/webkit/atk/getTextAtOffset", testWebkitAtkGetTextAtOffset);
    g_test_add_func("/webkit/atk/getTextAtOffsetNewlines", testWebkitAtkGetTextAtOffsetNewlines);
    g_test_add_func("/webkit/atk/getTextAtOffsetTextarea", testWebkitAtkGetTextAtOffsetTextarea);
    g_test_add_func("/webkit/atk/getTextAtOffsetTextInput", testWebkitAtkGetTextAtOffsetTextInput);
    g_test_add_func("/webkit/atk/getTextAtOffsetWithPreformattedText", testWebkitAtkGetTextAtOffsetWithPreformattedText);
    g_test_add_func("/webkit/atk/getTextAtOffsetWithSpecialCharacters", testWebkitAtkGetTextAtOffsetWithSpecialCharacters);
    g_test_add_func("/webkit/atk/getTextAtOffsetWithWrappedLines", testWebkitAtkGetTextAtOffsetWithWrappedLines);
    g_test_add_func("/webkit/atk/getTextInParagraphAndBodySimple", testWebkitAtkGetTextInParagraphAndBodySimple);
    g_test_add_func("/webkit/atk/getTextInParagraphAndBodyModerate", testWebkitAtkGetTextInParagraphAndBodyModerate);
    g_test_add_func("/webkit/atk/getTextInTable", testWebkitAtkGetTextInTable);
    g_test_add_func("/webkit/atk/getHeadersInTable", testWebkitAtkGetHeadersInTable);
    g_test_add_func("/webkit/atk/textAttributes", testWebkitAtkTextAttributes);
    g_test_add_func("/webkit/atk/textSelections", testWebkitAtkTextSelections);
    g_test_add_func("/webkit/atk/getExtents", testWebkitAtkGetExtents);
    g_test_add_func("/webkit/atk/hypertextAndHyperlinks", testWebkitAtkHypertextAndHyperlinks);
    g_test_add_func("/webkit/atk/layoutAndDataTables", testWebkitAtkLayoutAndDataTables);
    g_test_add_func("/webkit/atk/linksWithInlineImages", testWebkitAtkLinksWithInlineImages);
    g_test_add_func("/webkit/atk/listsOfItems", testWebkitAtkListsOfItems);
    g_test_add_func("/webkit/atk/textChangedNotifications", testWebkitAtkTextChangedNotifications);
    g_test_add_func("/webkit/atk/parentForRootObject", testWebkitAtkParentForRootObject);
    g_test_add_func("/webkit/atk/setParentForObject", testWebkitAtkSetParentForObject);
    return g_test_run ();
}

