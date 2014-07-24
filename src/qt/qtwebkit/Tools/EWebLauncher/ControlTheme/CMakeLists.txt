set(ControlTheme_DIR "${TOOLS_DIR}/EWebLauncher/ControlTheme")

add_custom_command(
    OUTPUT ${THEME_BINARY_DIR}/entry.edj
    COMMAND ${EDJE_CC_EXECUTABLE} -v ${ControlTheme_DIR}/entry.edc ${THEME_BINARY_DIR}/entry.edj
    DEPENDS
        ${ControlTheme_DIR}/entry.edc
    VERBATIM
)

add_custom_target(ControlTheme DEPENDS ${THEME_BINARY_DIR}/entry.edj)
