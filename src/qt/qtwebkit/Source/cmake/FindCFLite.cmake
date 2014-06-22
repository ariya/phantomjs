# - Try to find the CFLite library
# Once done this will define
#
#  CFLITE_FOUND - System has CFLite
#  CFLITE_INCLUDE_DIR - The CFLite include directory
#  CFLITE_LIBRARIES - The libraries needed to use CFLite

find_path(CFLITE_INCLUDE_DIR NAMES CoreFoundation/CoreFoundation.h)

find_library(CFLITE_LIBRARIES NAMES CFLite.lib)

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set COREFOUNDATION_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CFLite DEFAULT_MSG CFLITE_LIBRARIES CFLITE_INCLUDE_DIR)

mark_as_advanced(CFLITE_INCLUDE_DIR CFLITE_LIBRARIES)
