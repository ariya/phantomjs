if (ENABLE_INSPECTOR)
    add_custom_target(
        web-inspector-resources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${WEBCORE_DIR}/inspector/front-end ${DATA_BUILD_DIR}/webinspector
        COMMAND ${CMAKE_COMMAND} -E copy ${WEBCORE_DIR}/English.lproj/localizedStrings.js ${DATA_BUILD_DIR}/webinspector
        COMMAND ${CMAKE_COMMAND} -E copy ${WEBKIT2_DIR}/UIProcess/InspectorServer/front-end/inspectorPageIndex.html ${DATA_BUILD_DIR}/webinspector
        COMMAND ${CMAKE_COMMAND} -E copy ${DERIVED_SOURCES_WEBCORE_DIR}/InspectorBackendCommands.js ${DATA_BUILD_DIR}/webinspector
        DEPENDS WebCore
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    install(DIRECTORY "${DATA_BUILD_DIR}/webinspector"
        DESTINATION "${DATA_INSTALL_DIR}/webinspector"
        FILES_MATCHING PATTERN "*.js"
                       PATTERN "*.html"
                       PATTERN "*.css"
                       PATTERN "*.gif"
                       PATTERN "*.png")
endif ()
