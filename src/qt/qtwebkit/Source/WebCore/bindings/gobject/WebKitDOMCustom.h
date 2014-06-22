/*
 *  Copyright (C) 2011 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef WebKitDOMCustom_h
#define WebKitDOMCustom_h

#include <glib.h>
#include <webkitdom/webkitdomdefines.h>

G_BEGIN_DECLS

/**
 * webkit_dom_html_text_area_element_is_edited:
 * @input: A #WebKitDOMHTMLTextAreaElement
 *
 */
WEBKIT_API gboolean webkit_dom_html_text_area_element_is_edited(WebKitDOMHTMLTextAreaElement* input);

/**
 * webkit_dom_html_input_element_is_edited:
 * @input: A #WebKitDOMHTMLInputElement
 *
 */
WEBKIT_API gboolean webkit_dom_html_input_element_is_edited(WebKitDOMHTMLInputElement* input);

/**
 * webkit_dom_blob_webkit_slice:
 * @self: A #WebKitDOMBlob
 * @start: A #gint64
 * @end: A #gint64
 * @content_type: A #gchar
 *
 * Returns: (transfer none):
 */
WEBKIT_API WebKitDOMBlob* webkit_dom_blob_webkit_slice(WebKitDOMBlob* self, gint64 start, gint64 end, const gchar* content_type);

/**
 * webkit_dom_html_element_get_class_name:
 * @element: A #WebKitDOMHTMLElement
 *
 * Returns:
 *
 */
WEBKIT_API gchar* webkit_dom_html_element_get_class_name(WebKitDOMHTMLElement* element);

/**
 * webkit_dom_html_element_set_class_name:
 * @element: A #WebKitDOMHTMLElement
 * @value: A #gchar
 *
 */
WEBKIT_API void webkit_dom_html_element_set_class_name(WebKitDOMHTMLElement* element, const gchar* value);

/**
 * webkit_dom_html_element_get_class_list:
 * @element: A #WebKitDOMHTMLElement
 *
 * Returns: (transfer none):
 *
 */
WEBKIT_API WebKitDOMDOMTokenList* webkit_dom_html_element_get_class_list(WebKitDOMHTMLElement* element);

/**
 * webkit_dom_html_form_element_dispatch_form_change:
 * @self: A #WebKitDOMHTMLFormElement
 *
 */
WEBKIT_API void webkit_dom_html_form_element_dispatch_form_change(WebKitDOMHTMLFormElement* self);

/**
 * webkit_dom_html_form_element_dispatch_form_input:
 * @self: A #WebKitDOMHTMLFormElement
 *
 */
WEBKIT_API void webkit_dom_html_form_element_dispatch_form_input(WebKitDOMHTMLFormElement* self);

/**
 * webkit_dom_webkit_named_flow_get_overflow:
 * @flow: A #WebKitDOMWebKitNamedFlow
 *
 * Returns:
 *
 */
WEBKIT_API gboolean webkit_dom_webkit_named_flow_get_overflow(WebKitDOMWebKitNamedFlow* flow);

/**
 * webkit_dom_element_get_webkit_region_overflow:
 * @element: A #WebKitDOMElement
 *
 * Returns:
 *
 */
WEBKIT_API gchar* webkit_dom_element_get_webkit_region_overflow(WebKitDOMElement* element);

/**
 * webkit_dom_webkit_named_flow_get_content_nodes:
 * @flow: A #WebKitDOMWebKitNamedFlow
 *
 * Returns: (transfer none):
 *
 */
WEBKIT_API WebKitDOMNodeList* webkit_dom_webkit_named_flow_get_content_nodes(WebKitDOMWebKitNamedFlow* flow);

/**
 * webkit_dom_webkit_named_flow_get_regions_by_content_node:
 * @flow: A #WebKitDOMWebKitNamedFlow
 * @content_node: A #WebKitDOMNode
 *
 * Returns: (transfer none):
 *
 */
WEBKIT_API WebKitDOMNodeList* webkit_dom_webkit_named_flow_get_regions_by_content_node(WebKitDOMWebKitNamedFlow* flow, WebKitDOMNode* content_node);

G_END_DECLS

#endif
