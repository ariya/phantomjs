set(Resources_DIR "${WEBCORE_DIR}/Resources")
macro(GENERATE_THEME _target_name _name _option)
    set(DefaultTheme_DIR "${WEBCORE_DIR}/platform/efl/DefaultTheme")

    add_custom_command(
        OUTPUT ${THEME_BINARY_DIR}/${_name}
        COMMAND ${EDJE_CC_EXECUTABLE} -v ${_option} default.edc ${THEME_BINARY_DIR}/${_name}
        DEPENDS
            default.edc
            widget/button/button.edc
            widget/button/img_button_focus.png
            widget/button/img_button_hover.png
            widget/button/img_button_normal.png
            widget/button/img_button_press.png
            widget/check/check.edc
            widget/check/img_check_off_focus.png
            widget/check/img_check_off_hover.png
            widget/check/img_check_off.png
            widget/check/img_check_on_focus.png
            widget/check/img_check_on_hover.png
            widget/check/img_check_on.png
            widget/combo/combo.edc
            widget/combo/combo_focus_button.png
            widget/combo/combo_focus.png
            widget/combo/combo_hover_button.png
            widget/combo/combo_hover.png
            widget/combo/combo_normal_button.png
            widget/combo/combo_normal.png
            widget/combo/combo_press_button.png
            widget/combo/combo_press.png
            widget/combo/icon.png
            widget/cursor/cursor.edc
            widget/entry/entry.edc
            widget/entry/img_focused.png
            widget/entry/img_hovered.png
            widget/entry/img_normal.png
            widget/file/file.edc
            widget/file/file_focus.png
            widget/file/file_hover.png
            widget/file/file_normal.png
            widget/file/file_press.png
            widget/mediacontrol/fullscreenbutton/enterfullscreenbutton.png
            widget/mediacontrol/fullscreenbutton/exitfullscreenbutton.png
            widget/mediacontrol/fullscreenbutton/fullscreen_button.edc
            widget/mediacontrol/mutebutton/mute_button.edc
            widget/mediacontrol/mutebutton/mutebutton.png
            widget/mediacontrol/mutebutton/unmutebutton.png
            widget/mediacontrol/playpausebutton/pausebutton.png
            widget/mediacontrol/playpausebutton/playbutton.png
            widget/mediacontrol/playpausebutton/playpause_button.edc
            widget/mediacontrol/seekbackwardbutton/seekbackward_button.edc
            widget/mediacontrol/seekbackwardbutton/seekbackwardbutton.png
            widget/mediacontrol/seekforwardbutton/seekforward_button.edc
            widget/mediacontrol/seekforwardbutton/seekforwardbutton.png
            widget/mediacontrol/togglecaptionsbutton/closedcaption_disabled.png
            widget/mediacontrol/togglecaptionsbutton/closedcaption.png
            widget/mediacontrol/togglecaptionsbutton/toggle_captions_button.edc
            widget/progressbar/bt_base.png
            widget/progressbar/progressbar.edc
            widget/progressbar/shelf_inset.png
            widget/radio/img_radio_off_focus.png
            widget/radio/img_radio_off_hover.png
            widget/radio/img_radio_off.png
            widget/radio/img_radio_on_focus.png
            widget/radio/img_radio_on_hover.png
            widget/radio/img_radio_on.png
            widget/radio/radio.edc
            widget/scrollbar/scrollbar.edc
            widget/scrollbar/scrollbar_h.png
            widget/scrollbar/scrollbar_knob_h.png
            widget/scrollbar/scrollbar_knob_v.png
            widget/scrollbar/scrollbar_v.png
            widget/search/cancel/cancel_normal_button2.png
            widget/search/cancel/cancel_normal_button.png
            widget/search/cancel/search_cancel.edc
            widget/search/decoration/decoration_normal_button.png
            widget/search/decoration/search_decoration.edc
            widget/search/field/field_focused.png
            widget/search/field/field_hovered.png
            widget/search/field/field_normal.png
            widget/search/field/search_field.edc
            widget/slider/slider.edc
            widget/slider/slider_fill_h.png
            widget/slider/slider_fill_v.png
            widget/slider/slider_h.png
            widget/slider/slider_thumb_h.png
            widget/slider/slider_thumb_press_h.png
            widget/slider/slider_thumb_press_v.png
            widget/slider/slider_thumb_v.png
            widget/slider/slider_v.png
            widget/spinner/sp_bg.png
            widget/spinner/sp_down_default.png
            widget/spinner/sp_down_hover.png
            widget/spinner/sp_down_pressed.png
            widget/spinner/spinner.edc
            widget/spinner/sp_up_default.png
            widget/spinner/sp_up_hover.png
            widget/spinner/sp_up_pressed.png
            ${Resources_DIR}/aliasCursor.png
            ${Resources_DIR}/cellCursor.png
            ${Resources_DIR}/contextMenuCursor.png
            ${Resources_DIR}/copyCursor.png
            ${Resources_DIR}/eastResizeCursor.png
            ${Resources_DIR}/eastWestResizeCursor.png
            ${Resources_DIR}/helpCursor.png
            ${Resources_DIR}/linkCursor.png
            ${Resources_DIR}/moveCursor.png
            ${Resources_DIR}/noDropCursor.png
            ${Resources_DIR}/noneCursor.png
            ${Resources_DIR}/northEastResizeCursor.png
            ${Resources_DIR}/northEastSouthWestResizeCursor.png
            ${Resources_DIR}/northResizeCursor.png
            ${Resources_DIR}/northSouthResizeCursor.png
            ${Resources_DIR}/northWestResizeCursor.png
            ${Resources_DIR}/northWestSouthEastResizeCursor.png
            ${Resources_DIR}/panIcon.png
            ${Resources_DIR}/progressCursor.png
            ${Resources_DIR}/southEastResizeCursor.png
            ${Resources_DIR}/southResizeCursor.png
            ${Resources_DIR}/southWestResizeCursor.png
            ${Resources_DIR}/verticalTextCursor.png
            ${Resources_DIR}/waitCursor.png
            ${Resources_DIR}/westResizeCursor.png
        WORKING_DIRECTORY ${DefaultTheme_DIR}
        VERBATIM
    )

    add_custom_target(${_target_name} ALL
        DEPENDS ${THEME_BINARY_DIR}/${_name}
    )
endmacro()

set(DefaultTheme_DEFINITION "")
if (ENABLE_PROGRESS_ELEMENT)
    list(APPEND DefaultTheme_DEFINITION "-DENABLE_PROGRESS_ELEMENT")
endif ()

GENERATE_THEME(DefaultTheme "default.edj" "-DCURSOR_IMAGE_DIR=${Resources_DIR} ${DefaultTheme_DEFINITION}")
install(FILES "${THEME_BINARY_DIR}/default.edj" DESTINATION ${DATA_INSTALL_DIR}/themes)

if (ENABLE_API_TESTS)
    GENERATE_THEME(BigButtonTheme "big_button_theme.edj" "-DBIG_BUTTON_THEME_FOR_TESTING -DCURSOR_IMAGE_DIR=${Resources_DIR}")
endif ()
