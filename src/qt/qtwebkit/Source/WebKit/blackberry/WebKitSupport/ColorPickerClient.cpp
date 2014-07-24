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
#include "ColorPickerClient.h"

#include "HTMLInputElement.h"
#include "PopupPicker.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformString.h>
#include <LocaleHandler.h>
#include <LocalizeResource.h>
#include <wtf/text/StringBuilder.h>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

DEFINE_STATIC_LOCAL(BlackBerry::Platform::LocalizeResource, s_resource, ());

ColorPickerClient::ColorPickerClient(const BlackBerry::Platform::String& value, WebPagePrivate* webPagePrivate, WebCore::HTMLInputElement* element)
    : PagePopupClient(webPagePrivate)
    , m_element(element)
{
    generateHTML(value);
}

void ColorPickerClient::generateHTML(const BlackBerry::Platform::String& value)
{
    StringBuilder source;
    source.appendLiteral("<style>\n");
    // Include CSS file.
    source.append(popupControlBlackBerryCss, sizeof(popupControlBlackBerryCss));
    source.appendLiteral("</style>\n<style>");
    source.append(colorControlBlackBerryCss, sizeof(colorControlBlackBerryCss));
    source.appendLiteral("</style></head><body>\n");
    source.appendLiteral("<script>\n");
    source.appendLiteral("window.addEventListener('load', function showIt() {");
    source.appendLiteral("window.popupcontrol.show({");
    // Add color value
    source.appendLiteral("initialValue:");
    if (!value.empty())
        source.append("'" + String(value) + "',");
    else
        source.appendLiteral("null,");
    // Add UI text
    source.appendLiteral("uiText: {");
    source.append("title:'" + String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_COLOR_TITLE)) + "',");
    source.append("slidersLabel:'" + String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_COLOR_SLIDERS_LABEL)) + "',");
    source.append("swatchesLabel:'" + String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_COLOR_SWATCHES_LABEL)) + "',");
    source.append("doneButtonLabel:'" + String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_DONE_BUTTON_LABEL)) + "',");
    source.append("cancelButtonLabel:'" + String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_CANCEL_BUTTON_LABEL)) + "',");
    source.appendLiteral("},");
    // Add directionality
    bool isRtl = BlackBerry::Platform::LocaleHandler::instance()->isRtlLocale();
    source.append("direction:'" + String(isRtl ? "rtl" : "ltr") + "',");
    source.appendLiteral("});\n");
    source.append(" window.removeEventListener('load', showIt); }); \n");
    source.append(colorControlBlackBerryJs, sizeof(colorControlBlackBerryJs));
    source.appendLiteral("</script>\n");
    source.appendLiteral("</body> </html>\n");
    m_source = source.toString();
}

void ColorPickerClient::setValueAndClosePopup(const String& value)
{
    // Popup closed.
    if (!m_element)
        return;

    static const char* cancelValue = "-1";
    if (value != cancelValue)
        m_element->setValue(value, DispatchChangeEvent);
    closePopup();
}

void ColorPickerClient::didClosePopup()
{
    PagePopupClient::didClosePopup();
    m_element = 0;
}

}
}
