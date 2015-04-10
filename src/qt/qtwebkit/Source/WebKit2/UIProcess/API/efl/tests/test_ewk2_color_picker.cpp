/*
    Copyright (C) 2012 Samsung Electronics
    Copyright (C) 2012 Intel Corporation. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "config.h"

#include "UnitTestUtils/EWK2UnitTestBase.h"

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;

#if ENABLE(INPUT_TYPE_COLOR)
static const int initialRed = 0x12;
static const int initialGreen = 0x34;
static const int initialBlue = 0x56;
static const int initialAlpha = 0xff;
static const int changedRed = 0x98;
static const int changedGreen = 0x76;
static const int changedBlue = 0x54;
static const int changedAlpha = 0xff;

static bool s_isColorPickerShown = false;
static Ewk_Color_Picker* s_colorPicker = 0;

class EWK2ColorPickerTest : public EWK2UnitTestBase {
public:
    static void onColorPickerDone(void* userData, Evas_Object*, void*)
    {
        bool* handled = static_cast<bool*>(userData);

        *handled = true;
    }

    static unsigned char setColorPickerColor(void* data)
    {
        Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);

        // 4. Change color to changed color.
        EXPECT_TRUE(ewk_color_picker_color_set(s_colorPicker, changedRed, changedGreen, changedBlue, changedAlpha));

        evas_object_smart_callback_call(smartData->self, "input,type,color,request", 0);

        return 0;
    }

    static Eina_Bool showColorPicker(Ewk_View_Smart_Data* smartData, Ewk_Color_Picker* colorPicker)
    {
        static bool isFirstRun = true;

        s_colorPicker = colorPicker;
        s_isColorPickerShown = true;

        int r, g, b, a;
        EXPECT_TRUE(ewk_color_picker_color_get(colorPicker, &r, &g, &b, &a));

        if (isFirstRun) {
            // 2. Check initial value from html file.
            EXPECT_EQ(initialRed, r);
            EXPECT_EQ(initialGreen, g);
            EXPECT_EQ(initialBlue, b);
            EXPECT_EQ(initialAlpha, a);

            isFirstRun = false;
        } else {
            // 7. Input values should be same as changed color.
            EXPECT_EQ(changedRed, r);
            EXPECT_EQ(changedGreen, g);
            EXPECT_EQ(changedBlue, b);
            EXPECT_EQ(changedAlpha, a);

            evas_object_smart_callback_call(smartData->self, "input,type,color,request", 0);
            return true;
        }

        // 3. Return after making a color picker.
        ecore_timer_add(0.0, setColorPickerColor, smartData);
        return true;
    }

    static Eina_Bool hideColorPicker(Ewk_View_Smart_Data*)
    {
        // 5. Test color picker is shown.
        EXPECT_TRUE(s_isColorPickerShown);
        s_isColorPickerShown = false;
    }

    static Eina_Bool hideColorPickerByRemovingElement(Ewk_View_Smart_Data* smartData)
    {
        // 9. input_picker_color_dismiss() is called if the element is removed.
        EXPECT_TRUE(s_isColorPickerShown);
        s_isColorPickerShown = false;
        evas_object_smart_callback_call(smartData->self, "input,type,color,request", 0);
    }

protected:
    enum Button { ShowColorPickerButton, HideColorPickerButton };

    void clickButton(Button button)
    {
        switch (button) {
        case ShowColorPickerButton:
            mouseClick(30, 20);
            break;
        case HideColorPickerButton:
            mouseClick(80, 20);
            break;
        }
    }
};

TEST_F(EWK2ColorPickerTest, ewk_color_picker_color_set)
{
    Ewk_View_Smart_Class* api = ewkViewClass();
    api->input_picker_color_request = showColorPicker;
    api->input_picker_color_dismiss = hideColorPicker;

    const char colorPickerHTML[] =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<script>function removeInputElement(){"
        "var parentElement = document.getElementById('parent');"
        "var inputElement = document.getElementById('color');"
        "parentElement.removeChild(inputElement);"
        "}</script>"
        "</head>"
        "<body>"
        "<div id='parent'>"
        "<input type='color' value='#123456' id='color'>"
        "<button onclick='removeInputElement();'>Remove Element</button>"
        "</div>"
        "</body>"
        "</html>";

    ewk_view_html_string_load(webView(), colorPickerHTML, 0, 0);
    waitUntilLoadFinished();

    clickButton(ShowColorPickerButton);

    bool handled = false;
    evas_object_smart_callback_add(webView(), "input,type,color,request", onColorPickerDone, &handled);
    while (!handled)
        ecore_main_loop_iterate();

    clickButton(ShowColorPickerButton);

    handled = false;
    while (!handled)
        ecore_main_loop_iterate();

    api->input_picker_color_dismiss = hideColorPickerByRemovingElement;
    clickButton(HideColorPickerButton);

    handled = false;
    while (!handled)
        ecore_main_loop_iterate();
    evas_object_smart_callback_del(webView(), "input,type,color,request", onColorPickerDone);
}
#endif // ENABLE(INPUT_TYPE_COLOR)
