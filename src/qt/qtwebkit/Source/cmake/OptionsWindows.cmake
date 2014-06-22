add_definitions(-D_HAS_EXCEPTIONS=0 -DNOMINMAX -DUNICODE)

include_directories(${JAVASCRIPTCORE_DIR}/os-win32)

if (MSVC)
    add_definitions(/WX
        /wd4018 /wd4065 /wd4068 /wd4099 /wd4100 /wd4127 /wd4138 /wd4180 /wd4189 /wd4201 /wd4244 /wd4251 /wd4275 /wd4288 /wd4291
        /wd4305 /wd4344 /wd4355 /wd4389 /wd4396 /wd4503 /wd4505 /wd4510 /wd4512 /wd4610 /wd4706 /wd4800 /wd4951 /wd4952 /wd4996)

    string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # Disable C++ exceptions
    string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # Disable RTTI

    if (NOT MSVC_VERSION LESS 1500)
        set(CMAKE_C_FLAGS "/MP ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "/MP ${CMAKE_CXX_FLAGS}")
    endif ()
endif ()
