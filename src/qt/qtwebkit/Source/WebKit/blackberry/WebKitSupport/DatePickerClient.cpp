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
#include "DatePickerClient.h"

#include "Document.h"
#include "Frame.h"
#include "HTMLInputElement.h"
#include "PopupPicker.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformString.h>
#include <LocaleHandler.h>
#include <LocalizeResource.h>
#include <unicode/dtfmtsym.h>
#include <wtf/text/StringBuilder.h>

using namespace WebCore;
using namespace icu;

namespace BlackBerry {
namespace WebKit {

DEFINE_STATIC_LOCAL(BlackBerry::Platform::LocalizeResource, s_resource, ());

DatePickerClient::DatePickerClient(BlackBerry::Platform::BlackBerryInputType type, const BlackBerry::Platform::String& value, const BlackBerry::Platform::String& min, const BlackBerry::Platform::String& max, double step, WebPagePrivate* webPagePrivate, WebCore::HTMLInputElement* element)
    : PagePopupClient(webPagePrivate)
    , m_type(type)
    , m_element(element)
{
    generateHTML(type, value, min, max, step);
}

DatePickerClient::~DatePickerClient()
{
}

void DatePickerClient::generateHTML(BlackBerry::Platform::BlackBerryInputType type, const BlackBerry::Platform::String& value, const BlackBerry::Platform::String& min, const BlackBerry::Platform::String& max, double step)
{
    StringBuilder source;
    String title = "";
    source.appendLiteral("<style>\n");
    // Include CSS file.
    source.append(popupControlBlackBerryCss, sizeof(popupControlBlackBerryCss));
    source.appendLiteral("</style>\n<style>");
    source.append(timeControlBlackBerryCss, sizeof(timeControlBlackBerryCss));
    source.appendLiteral("</style></head><body>\n");
    source.appendLiteral("<script>\n");
    source.appendLiteral("window.addEventListener('load', function showIt() {");
    source.appendLiteral("window.popupcontrol.show({");
    // Add DatePicker type
    source.appendLiteral("type:");
    switch (type) {
    case BlackBerry::Platform::InputTypeDate:
        source.appendLiteral("'Date', ");
        title = String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_DATE_TITLE));
        break;
    case BlackBerry::Platform::InputTypeTime:
        source.appendLiteral("'Time', ");
        title = String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_TIME_TITLE));
        break;
    case BlackBerry::Platform::InputTypeDateTime:
        source.appendLiteral("'DateTime', ");
        title = String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_DATE_TIME_TITLE));
        break;
    case BlackBerry::Platform::InputTypeDateTimeLocal:
        source.appendLiteral("'DateTimeLocal', ");
        title = String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_DATE_TIME_LOCAL_TITLE));
        break;
    case BlackBerry::Platform::InputTypeMonth:
        source.appendLiteral("'Month', ");
        title = String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_MONTH_TITLE));
        break;
    case BlackBerry::Platform::InputTypeWeek:
    default:
        break;
    }
    // Add datetime value
    source.appendLiteral("initialValue:");
    if (!value.empty())
        source.append("'" + String(value) + "', ");
    else
        source.appendLiteral("null, ");
    // Add lower and upper bounds
    source.appendLiteral("min:");
    if (!min.empty())
        source.append("'" + String(min) + "', ");
    else
        source.appendLiteral("null, ");
    source.appendLiteral("max:");
    if (!max.empty())
        source.append("'" + String(max) + "', ");
    else
        source.appendLiteral("null, ");
    // Add step size
    source.append("step:" + String::number(step) + ", ");
    // Add UI text
    source.appendLiteral("uiText: {");
    source.append("title:'" + title + "',");
    source.append("doneButtonLabel:'" + String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_DONE_BUTTON_LABEL)) + "',");
    source.append("cancelButtonLabel:'" + String::fromUTF8(s_resource.getString(BlackBerry::Platform::PICKER_CANCEL_BUTTON_LABEL)) + "',");
    source.append("monthLabels:" + DatePickerClient::generateDateLabels(UDAT_STANDALONE_MONTHS) + ",");
    source.append("shortMonthLabels:" + DatePickerClient::generateDateLabels(UDAT_SHORT_MONTHS) + ",");
    source.append("daysOfWeekLabels:" + DatePickerClient::generateDateLabels(UDAT_STANDALONE_WEEKDAYS) + ",");
    source.append("amPmLabels:" + DatePickerClient::generateDateLabels(UDAT_AM_PMS) + ",");
    source.appendLiteral("},");
    // Add directionality
    bool isRtl = BlackBerry::Platform::LocaleHandler::instance()->isRtlLocale();
    source.append("direction:'" + String(isRtl ? "rtl" : "ltr") + "',");
    source.appendLiteral("});\n");
    source.appendLiteral(" window.removeEventListener('load', showIt); }); \n");
    source.append(timeControlBlackBerryJs, sizeof(timeControlBlackBerryJs));
    source.appendLiteral("</script>\n</body> </html>\n");
    m_source = source.toString();
}

void DatePickerClient::setValueAndClosePopup(const String& value)
{
    // Popup closed.
    if (!m_element)
        return;

    // We hide caret when we select date input field, restore it when we close date picker.
    m_element->document()->frame()->selection()->setCaretVisible(true);

    // Return -1 if user cancel the selection.
    if (value != "-1")
        m_element->setValue(value, DispatchChangeEvent);
    closePopup();
}

void DatePickerClient::didClosePopup()
{
    PagePopupClient::didClosePopup();
    m_element = 0;
}

// UDAT_foo are for labels that are meant to be formatted as part of a date.
// UDAT_STANDALONE_foo are for labels that are displayed separately from other date components.
// For example, UDAT_SHORT_MONTHS in Catalan puts a preposition in front of the month but UDAT_STANDALONE_SHORT_MONTHS does not.
const String DatePickerClient::generateDateLabels(UDateFormatSymbolType symbolType)
{
    UErrorCode uerrStatus = U_ZERO_ERROR;
    DateFormatSymbols dateSymbols = DateFormatSymbols(uerrStatus); // constructor will never fail
    const UnicodeString* labels = 0;
    int32_t labelCount = 0;

    switch (symbolType) {
    // dateSymbols retain ownership of return values from getFoo calls
    case UDAT_STANDALONE_MONTHS:
        labelCount = 12;
        labels = dateSymbols.getMonths(labelCount, DateFormatSymbols::STANDALONE, DateFormatSymbols::WIDE);
        break;
    case UDAT_STANDALONE_SHORT_MONTHS:
        labelCount = 12;
        labels = dateSymbols.getMonths(labelCount, DateFormatSymbols::STANDALONE, DateFormatSymbols::ABBREVIATED);
        break;
    case UDAT_SHORT_MONTHS:
        labelCount = 12;
        labels = dateSymbols.getMonths(labelCount, DateFormatSymbols::FORMAT, DateFormatSymbols::ABBREVIATED);
        break;
    case UDAT_STANDALONE_WEEKDAYS:
        labelCount = 8;
        // getWeekdays returns an array where the zeroeth element is empty; the first label is placed in index 1
        labels = &(dateSymbols.getWeekdays(labelCount, DateFormatSymbols::STANDALONE, DateFormatSymbols::WIDE))[1]; // skip zeroeth element
        --labelCount;
        break;
    case UDAT_AM_PMS:
        labelCount = 2;
        labels = dateSymbols.getAmPmStrings(labelCount);
        break;
    default:
        ASSERT(0);
        break;
    }

    StringBuilder printedLabels;
    printedLabels.appendLiteral("[");
    for (int32_t i = 0; i < labelCount; ++i) {
        String escapedLabel = String(labels[i].getBuffer(), labels[i].length()).replace('\\', "\\\\").replace('\'', "\\'"); // TODO PR 243547: refactor escaping of strings for DatePickerClient and SelectPopupClient
        printedLabels.append("'" + escapedLabel + "'");
        if (i < labelCount - 1)
            printedLabels.appendLiteral(",");
    }
    printedLabels.appendLiteral("]");
    return printedLabels.toString();
}

}
}
