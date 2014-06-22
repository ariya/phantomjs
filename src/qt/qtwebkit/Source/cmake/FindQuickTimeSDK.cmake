# - Find QuickTime SDK installation
# Find the QuickTime includes and library
# This module defines
#  QuickTime_INCLUDE_DIRS, where to find QuickTime.h, etc.
#  QuickTime_LIBRARIES, libraries to link against to use QuickTime.
#  QuickTime_FOUND, If false, do not try to use QuickTime.

find_path(QuickTimeSDK_INCLUDE_DIRS QuickTime.h PATHS
    "$ENV{PROGRAMFILES}/QuickTime SDK/CIncludes"
)

set(QuickTimeSDK_LIBRARY_PATH "${QuickTimeSDK_INCLUDE_DIRS}/../Libraries")
find_library(QuickTimeSDK_CVClient_LIBRARY CVClient ${QuickTimeSDK_LIBRARY_PATH} NO_DEFAULT_PATH)
find_library(QuickTimeSDK_QTMLClient_LIBRARY QTMLClient ${QuickTimeSDK_LIBRARY_PATH} NO_DEFAULT_PATH)
set(QuickTimeSDK_LIBRARIES ${QuickTimeSDK_CVClient_LIBRARY} ${QuickTimeSDK_QTMLClient_LIBRARY})

# handle the QUIETLY and REQUIRED arguments and set QuickTimeSDK_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(QuickTimeSDK DEFAULT_MSG QuickTimeSDK_LIBRARIES QuickTimeSDK_INCLUDE_DIRS)
mark_as_advanced(QuickTimeSDK_INCLUDE_DIRS QuickTimeSDK_CVClient_LIBRARY QuickTimeSDK_QTMLClient_LIBRARY)
