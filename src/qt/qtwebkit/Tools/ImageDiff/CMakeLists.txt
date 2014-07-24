set(IMAGE_DIFF_DIR "${TOOLS_DIR}/ImageDiff")

set(IMAGE_DIFF_INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

set(IMAGE_DIFF_LIBRARIES
    WTF
)

INCLUDE_IF_EXISTS(${IMAGE_DIFF_DIR}/Platform${PORT}.cmake)

include_directories(${IMAGE_DIFF_INCLUDE_DIRECTORIES})
add_executable(ImageDiff ${IMAGE_DIFF_SOURCES})
target_link_libraries(ImageDiff ${IMAGE_DIFF_LIBRARIES})
set_target_properties(ImageDiff PROPERTIES FOLDER "Tools")
