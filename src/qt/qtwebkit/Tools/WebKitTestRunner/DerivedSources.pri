# -------------------------------------------------------------------
# Derived sources for WebKitTestRunner
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = derived

# Make sure forwarded headers needed by this project are present
fwheader_generator.commands = perl $${ROOT_WEBKIT_DIR}/Source/WebKit2/Scripts/generate-forwarding-headers.pl $${ROOT_WEBKIT_DIR}/Tools/WebKitTestRunner $${ROOT_BUILD_DIR}/Source/include qt
fwheader_generator.depends  = $${ROOT_WEBKIT_DIR}/Source/WebKit2/Scripts/generate-forwarding-headers.pl
GENERATORS += fwheader_generator
