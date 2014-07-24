/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
#include "SelectPopupClient.h"

#include "HTMLOptionElement.h"
#include "HTMLSelectElement.h"
#include "PopupPicker.h"
#include "RenderObject.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformString.h>
#include <LocaleHandler.h>
#include <LocalizeResource.h>
#include <wtf/text/StringBuilder.h>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

DEFINE_STATIC_LOCAL(BlackBerry::Platform::LocalizeResource, s_resource, ());

SelectPopupClient::SelectPopupClient(bool multiple, int size, const ScopeArray<BlackBerry::Platform::String>& labels, bool* enableds, const int* itemType, bool* selecteds, WebPagePrivate* webPagePrivate, HTMLSelectElement* element)
    : PagePopupClient(webPagePrivate)
    , m_multiple(multiple)
    , m_size(size)
    , m_element(element)
    , m_notifyChangeTimer(this, &SelectPopupClient::notifySelectionChange)
{
    generateHTML(multiple, size, labels, enableds, itemType, selecteds);
}

SelectPopupClient::~SelectPopupClient()
{
}

void SelectPopupClient::generateHTML(bool, int size, const ScopeArray<BlackBerry::Platform::String>& labels, bool* enableds,
    const int* itemType, bool* selecteds)
{
    StringBuilder source;
    source.appendLiteral("<style>\n");
    // Include CSS file.
    source.append(popupControlBlackBerryCss, sizeof(popupControlBlackBerryCss));
    source.appendLiteral("</style>\n<style>");
    source.append(selectControlBlackBerryCss, sizeof(selectControlBlackBerryCss));
    source.appendLiteral("</style></head><body>\n");
    source.appendLiteral("<script>\n");
    source.appendLiteral("window.addEventListener('load', function showIt() {");
    source.appendLiteral("window.select.show({");
    // Indicate if it is a multiselect.
    source.appendLiteral("isMultiSelect:");
    if (m_multiple)
        source.appendLiteral("true,");
    else
        source.appendLiteral("false,");
    // Add labels.
    source.appendLiteral("labels: [");
    for (int i = 0; i < size; i++) {
        source.append("'" + String(labels[i]).replaceWithLiteral('\\', "\\\\").replaceWithLiteral('\'', "\\'") + "'");
        // Don't append ',' to last element.
        if (i != size - 1)
            source.appendLiteral(", ");
    }
    source.appendLiteral("], ");
    // Add enables.
    source.append("enableds: [");
    for (int i = 0; i < size; i++) {
        if (enableds[i])
            source.appendLiteral("true");
        else
            source.appendLiteral("false");
        // Don't append ',' to last element.
        if (i != size - 1)
            source.appendLiteral(", ");
    }
    source.appendLiteral("], ");
    // Add itemType.
    source.appendLiteral("itemTypes: [");
    for (int i = 0; i < size; i++) {
        source.appendNumber(itemType[i]);
        // Don't append ',' to last element.
        if (i != size - 1)
            source.appendLiteral(", ");
    }
    source.appendLiteral("], ");
    // Add selecteds
    source.appendLiteral("selecteds: [");
    for (int i = 0; i < size; i++) {
        if (selecteds[i])
            source.appendLiteral("true");
        else
            source.appendLiteral("false");
        // Don't append ',' to last element.
        if (i != size - 1)
            source.appendLiteral(", ");
    }
    source.appendLiteral("], ");
    // Add UI text
    source.appendLiteral("uiText: {");
    source.append("title:'" + String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_SELECT_TITLE)) + "',");
    source.append("doneButtonLabel:'" + String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_DONE_BUTTON_LABEL)) + "',");
    source.append("cancelButtonLabel:'" + String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_CANCEL_BUTTON_LABEL)) + "',");
    source.appendLiteral("},");
    // Add directionality
    bool isRtl = BlackBerry::Platform::LocaleHandler::instance()->isRtlLocale();
    source.append("direction:'" + String(isRtl ? "rtl" : "ltr") + "',");
    source.appendLiteral("});\n");
    source.appendLiteral(" window.removeEventListener('load', showIt); }); \n");
    source.append(selectControlBlackBerryJs, sizeof(selectControlBlackBerryJs));
    source.appendLiteral("</script>\n</body> </html>\n");
    m_source = source.toString();
}

void SelectPopupClient::setValueAndClosePopup(const String& stringValue)
{
    // Popup closed.
    if (!m_element)
        return;

    static const char* cancelValue = "-1";
    if (stringValue == cancelValue) {
        closePopup();
        return;
    }

    if (m_size > 0) {
        bool selecteds[m_size];
        for (unsigned i = 0; i < m_size; i++)
            selecteds[i] = stringValue[i] - '0';

        const Vector<HTMLElement*>& items = m_element->listItems();

        // If element changed after select UI showed, do nothing but closePopup().
        if (items.size() != static_cast<unsigned>(m_size)) {
            closePopup();
            return;
        }

        HTMLOptionElement* option;
        for (unsigned i = 0; i < m_size; i++) {
            if (isHTMLOptionElement(items[i])) {
                option = toHTMLOptionElement(items[i]);
                option->setSelectedState(selecteds[i]);
            }
        }
    }
    // Force repaint because we do not send mouse events to the select element
    // and the element doesn't automatically repaint itself.
    if (m_element->renderer())
        m_element->renderer()->repaint();

    m_notifyChangeTimer.startOneShot(0);
}

void SelectPopupClient::didClosePopup()
{
    PagePopupClient::didClosePopup();
    m_element = 0;
}

void SelectPopupClient::notifySelectionChange(WebCore::Timer<SelectPopupClient>*)
{
    if (m_element)
        m_element->dispatchFormControlChangeEvent();
    closePopup();
}

}
}
