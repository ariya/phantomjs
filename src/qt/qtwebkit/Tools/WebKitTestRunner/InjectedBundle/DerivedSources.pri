# -------------------------------------------------------------------
# Derived sources for WebKitTestRunner's InjectedBundle
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

# This file is both a top level target, and included from Target.pri,
# so that the resulting generated sources can be added to SOURCES.
# We only set the template if we're a top level target, so that we
# don't override what Target.pri has already set.
sanitizedFile = $$toSanitizedPath($$_FILE_)
equals(sanitizedFile, $$toSanitizedPath($$_PRO_FILE_)):TEMPLATE = derived

IDL_BINDINGS += \
    Bindings/AccessibilityController.idl \
    Bindings/AccessibilityTextMarker.idl \
    Bindings/AccessibilityTextMarkerRange.idl \
    Bindings/AccessibilityUIElement.idl \
    Bindings/EventSendingController.idl \
    Bindings/GCController.idl \
    Bindings/TestRunner.idl \
    Bindings/TextInputController.idl \

qtPrepareTool(QMAKE_MOC, moc)

# GENERATOR 1: IDL compiler
idl.output = JS${QMAKE_FILE_BASE}.cpp
idl.input = IDL_BINDINGS
idl.script = $${ROOT_WEBKIT_DIR}/Source/WebCore/bindings/scripts/generate-bindings.pl
idl.commands = perl -I$${ROOT_WEBKIT_DIR}/Source/WebCore/bindings/scripts -I$$PWD/Bindings $$idl.script --defines \"$$javascriptFeatureDefines()\" --generator TestRunner --include $$PWD/Bindings --outputDir ${QMAKE_FUNC_FILE_OUT_PATH} --preprocessor \"$${QMAKE_MOC} -E\" ${QMAKE_FILE_NAME}
idl.depends = $${ROOT_WEBKIT_DIR}/Source/WebCore/bindings/scripts/CodeGenerator.pm \
              $$PWD/Bindings/CodeGeneratorTestRunner.pm \
              $${ROOT_WEBKIT_DIR}/Source/WebCore/bindings/scripts/IDLParser.pm \
              $${ROOT_WEBKIT_DIR}/Source/WebCore/bindings/scripts/InFilesParser.pm \
              $${ROOT_WEBKIT_DIR}/Source/WebCore/bindings/scripts/generate-bindings.pl
GENERATORS += idl

INCLUDEPATH += $${ROOT_BUILD_DIR}/Tools/WebKitTestRunner/InjectedBundle/$${GENERATED_SOURCES_DESTDIR}

