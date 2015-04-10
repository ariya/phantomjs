/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#! [0]
make all
#! [0]


#! [1]
CONFIG += qt debug
#! [1]


#! [2]
QT += network xml
#! [2]


#! [3]
QT = network xml # This will omit the core and gui modules.
#! [3]


#! [4]
QT -= gui # Only the core module is used.
#! [4]


#! [5]
CONFIG += link_pkgconfig
PKGCONFIG += ogg dbus-1
#! [5]


#! [6]
LIBS += -L/usr/local/lib -lmath
#! [6]


#! [7]
INCLUDEPATH = c:/msdev/include d:/stl/include
#! [7]


#! [8]
qmake [mode] [options] files
#! [8]


#! [9]
qmake -makefile [options] files
#! [9]


#! [10]
qmake -makefile -o Makefile "CONFIG+=test" test.pro
#! [10]


#! [11]
qmake "CONFIG+=test" test.pro
#! [11]


#! [12]
qmake -project [options] files
#! [12]


#! [13]
qmake -spec macx-g++
#! [13]


#! [14]
QMAKE_LFLAGS += -F/path/to/framework/directory/
#! [14]


#! [15]
LIBS += -framework TheFramework
#! [15]


#! [16]
TEMPLATE = lib
CONFIG += lib_bundle
#! [16]


#! [17]
FRAMEWORK_HEADERS.version = Versions
FRAMEWORK_HEADERS.files = path/to/header_one.h path/to/header_two.h
FRAMEWORK_HEADERS.path = Headers
QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS
#! [17]


#! [19]
qmake -spec macx-xcode project.pro
#! [19]


#! [20]
qmake -tp vc
#! [20]


#! [21]
qmake -tp vc -r
#! [21]


#! [22]
CONFIG -= embed_manifest_exe
#! [22]


#! [23]
CONFIG -= embed_manifest_dll
#! [23]


#! [24]
make all
#! [24]


#! [25]
build_pass:CONFIG(debug, debug|release) {
    unix: TARGET = $$join(TARGET,,,_debug)
    else: TARGET = $$join(TARGET,,,d)
}
#! [25]


#! [26]
CONFIG += console newstuff
...
newstuff {
    SOURCES += new.cpp
    HEADERS += new.h
}
#! [26]


#! [27]
DEFINES += USE_MY_STUFF
#! [27]


#! [28]
myFiles.files = path\*.png
DEPLOYMENT += myFiles
#! [28]


#! [29]
myFiles.files = path\file1.ext1 path2\file2.ext1 path3\*
myFiles.path = \some\path\on\device
someother.files = C:\additional\files\*
someother.path = \myFiles\path2
DEPLOYMENT += myFiles someother
#! [29]


#! [30]
DESTDIR = ../../lib
#! [30]


#! [31]
DISTFILES += ../program.txt
#! [31]


#! [32]
FORMS = mydialog.ui \
    mywidget.ui \
        myconfig.ui
#! [32]


#! [33]
FORMS3 = my_uic3_dialog.ui \
     my_uic3_widget.ui \
         my_uic3_config.ui
#! [33]


#! [34]
HEADERS = myclass.h \
          login.h \
          mainwindow.h
#! [34]


#! [35]
INCLUDEPATH = c:/msdev/include d:/stl/include
#! [35]


#! [36]
target.path += $$[QT_INSTALL_PLUGINS]/imageformats
INSTALLS += target
#! [36]


#! [37]
LEXSOURCES = lexer.l
#! [37]


#! [38]
unix:LIBS += -L/usr/local/lib -lmath
win32:LIBS += c:/mylibs/math.lib
#! [38]


#! [39]
CONFIG += no_lflags_merge
#! [39]


#! [40]
unix:MOC_DIR = ../myproject/tmp
win32:MOC_DIR = c:/myproject/tmp
#! [40]


#! [41]
unix:OBJECTS_DIR = ../myproject/tmp
win32:OBJECTS_DIR = c:/myproject/tmp
#! [41]


#! [43]
FRAMEWORK_HEADERS.version = Versions
FRAMEWORK_HEADERS.files = path/to/header_one.h path/to/header_two.h
FRAMEWORK_HEADERS.path = Headers
QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS
#! [43]


#! [44]
QMAKE_BUNDLE_EXTENSION = .myframework
#! [44]


#! [45]
QMAKE_RESOURCE_FLAGS += -threshold 0 -compress 9
#! [45]


#! [47]
QT -= gui # Only the core module is used.
#! [47]


#! [48]
unix:RCC_DIR = ../myproject/resources
win32:RCC_DIR = c:/myproject/resources
#! [48]


#! [49]
SOURCES = myclass.cpp \
      login.cpp \
      mainwindow.cpp
#! [49]


#! [50]
SUBDIRS = kernel \
          tools \
          myapp
#! [50]


#! [51]
CONFIG += ordered
#! [51]


#! [52]
TEMPLATE = app
TARGET = myapp
SOURCES = main.cpp
#! [52]


#! [53]
TEMPLATE = lib
SOURCES = main.cpp
TARGET = mylib
#! [53]


#! [54]
unix:UI_DIR = ../myproject/ui
win32:UI_DIR = c:/myproject/ui
#! [54]


#! [57]
VERSION = 1.2.3
#! [57]


#! [58]
YACCSOURCES = moc.y
#! [58]


#! [59]
FILE = /etc/passwd
FILENAME = $$basename(FILE) #passwd
#! [59]


#! [60]
CONFIG = debug
CONFIG += release
CONFIG(release, debug|release):message(Release build!) #will print
CONFIG(debug, debug|release):message(Debug build!) #no print
#! [60]


#! [61]
contains( drivers, network ) {
    # drivers contains 'network'
    message( "Configuring for network build..." )
    HEADERS += network.h
    SOURCES += network.cpp
}
#! [61]


#! [62]
error(An error has occurred in the configuration process.)
#! [62]


#! [63]
exists( $(QTDIR)/lib/libqt-mt* ) {
      message( "Configuring for multi-threaded Qt..." )
      CONFIG += thread
}
#! [63]


#! [64]
MY_VAR = one two three four
MY_VAR2 = $$join(MY_VAR, " -L", -L) -Lfive
MY_VAR3 = $$member(MY_VAR, 2) $$find(MY_VAR, t.*)
#! [64]


#! [65]
LIST = 1 2 3
for(a, LIST):exists(file.$${a}):message(I see a file.$${a}!)
#! [65]


#! [66]
include( shared.pri )
OPTIONS = standard custom
!include( options.pri ) {
    message( "No custom build options specified" )
OPTIONS -= custom
}
#! [66]


#! [67]
isEmpty( CONFIG ) {
CONFIG += warn_on debug
}
#! [67]


#! [68]
message( "This is a message" )
#! [68]


#! [69]
!build_pass:message( "This is a message" )
#! [69]


#! [70]
This is a test.
#! [70]


#! [71]
system(ls /bin):HAS_BIN=FALSE
#! [71]


#! [72]
UNAME = $$system(uname -s)
contains( UNAME, [lL]inux ):message( This looks like Linux ($$UNAME) to me )
#! [72]


#! [73]
ARGS = 1 2 3 2 5 1
ARGS = $$unique(ARGS) #1 2 3 5
#! [73]


#! [74]
qmake -set PROPERTY VALUE
#! [74]


#! [75]
qmake -query PROPERTY
qmake -query #queries all current PROPERTY/VALUE pairs
#! [75]


#! [77]
qmake -query "QT_INSTALL_PREFIX"
#! [77]


#! [78]
QMAKE_VERS = $$[QMAKE_VERSION]
#! [78]


#! [79]
documentation.path = /usr/local/program/doc
documentation.files = docs/*
#! [79]


#! [80]
INSTALLS += documentation
#! [80]


#! [81]
unix:documentation.extra = create_docs; mv master.doc toc.doc
#! [81]


#! [82]
target.path = /usr/local/myprogram
INSTALLS += target
#! [82]


#! [83]
CONFIG += create_prl
#! [83]


#! [84]
CONFIG += link_prl
#! [84]


#! [85]
QMAKE_EXT_MOC = .mymoc
#! [85]


#! [86]
mytarget.target = .buildfile
mytarget.commands = touch $$mytarget.target
mytarget.depends = mytarget2

mytarget2.commands = @echo Building $$mytarget.target
#! [86]


#! [87]
QMAKE_EXTRA_TARGETS += mytarget mytarget2
#! [87]


#! [88]
new_moc.output  = moc_${QMAKE_FILE_BASE}.cpp
new_moc.commands = moc ${QMAKE_FILE_NAME} -o ${QMAKE_FILE_OUT}
new_moc.depend_command = g++ -E -M ${QMAKE_FILE_NAME} | sed "s,^.*: ,,"
new_moc.input = NEW_HEADERS
QMAKE_EXTRA_COMPILERS += new_moc
#! [88]


#! [89]
TARGET = myapp
#! [89]


#! [90]
DEFINES += USE_MY_STUFF
#! [90]


#! [91]
DEFINES -= USE_MY_STUFF
#! [91]


#! [92]
DEFINES *= USE_MY_STUFF
#! [92]


#! [93]
DEFINES ~= s/QT_[DT].+/QT
#! [93]


#! [94]
EVERYTHING = $$SOURCES $$HEADERS
message("The project contains the following files:")
message($$EVERYTHING)
#! [94]


#! [95]
win32:DEFINES += USE_MY_STUFF
#! [95]


#! [96]
win32:xml {
    message(Building for Windows)
    SOURCES += xmlhandler_win.cpp
} else:xml {
    SOURCES += xmlhandler.cpp
} else {
    message("Unknown configuration")
}
#! [96]


#! [97]
MY_VARIABLE = value
#! [97]


#! [98]
MY_DEFINES = $$DEFINES
#! [98]


#! [99]
MY_DEFINES = $${DEFINES}
#! [99]


#! [100]
TARGET = myproject_$${TEMPLATE}
#! [100]


#! [101]
target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS += target
#! [101]


#! [102]
defineReplace(functionName){
    #function code
}
#! [102]


#! [103]
CONFIG += myfeatures
#! [103]


#! [105]
PRECOMPILED_HEADER = stable.h
#! [105]


#! [106]
precompile_header:!isEmpty(PRECOMPILED_HEADER) {
DEFINES += USING_PCH
}
#! [106]


#! [107]
PRECOMPILED_HEADER = window.h
SOURCES            = window.cpp
#! [107]


#! [108]
SOURCES += hello.cpp
#! [108]


#! [109]
SOURCES += hello.cpp
SOURCES += main.cpp
#! [109]


#! [110]
SOURCES = hello.cpp \
          main.cpp
#! [110]


#! [111]
HEADERS += hello.h
SOURCES += hello.cpp
SOURCES += main.cpp
#! [111]


#! [112]
TARGET = helloworld
#! [112]


#! [113]
HEADERS += hello.h
SOURCES += hello.cpp
SOURCES += main.cpp
#! [113]


#! [114]
qmake -o Makefile hello.pro
#! [114]


#! [115]
qmake -tp vc hello.pro
#! [115]


#! [116]
CONFIG += debug
HEADERS += hello.h
SOURCES += hello.cpp
SOURCES += main.cpp
#! [116]


#! [117]
win32 {
    SOURCES += hellowin.cpp
}
#! [117]


#! [118]
CONFIG += debug
HEADERS += hello.h
SOURCES += hello.cpp
SOURCES += main.cpp
win32 {
    SOURCES += hellowin.cpp
}
unix {
    SOURCES += hellounix.cpp
}
#! [118]


#! [119]
!exists( main.cpp ) {
    error( "No main.cpp file found" )
}
#! [119]


#! [120]
CONFIG += debug
HEADERS += hello.h
SOURCES += hello.cpp
SOURCES += main.cpp
win32 {
    SOURCES += hellowin.cpp
}
unix {
    SOURCES += hellounix.cpp
}
!exists( main.cpp ) {
    error( "No main.cpp file found" )
}
#! [120]


#! [121]
win32 {
    debug {
        CONFIG += console
    }
}
#! [121]


#! [122]
CONFIG += debug
HEADERS += hello.h
SOURCES += hello.cpp
SOURCES += main.cpp
win32 {
    SOURCES += hellowin.cpp
}
unix {
    SOURCES += hellounix.cpp
}
!exists( main.cpp ) {
    error( "No main.cpp file found" )
}
win32:debug {
    CONFIG += console
}
#! [122]


#! [123]
TEMPLATE = app
DESTDIR  = c:/helloapp
HEADERS += hello.h
SOURCES += hello.cpp
SOURCES += main.cpp
DEFINES += USE_MY_STUFF
CONFIG  += release
#! [123]


#! [124]
make all
#! [124]


#! [125]
make
#! [125]


#! [126]
make install
#! [126]


#! [127]
CONFIG(debug, debug|release) {
    mac: TARGET = $$join(TARGET,,,_debug)
    win32: TARGET = $$join(TARGET,,d)
}
#! [127]

#! [142]
DEPLOYMENT_PLUGIN += qjpeg
#! [142]

#! [149]
SUBDIRS += my_executable my_library
my_executable.subdir = app
my_executable.depends = my_library
my_library.subdir = lib
#! [149]

#! [157]
packagesExist(sqlite3 QtNetwork QtDeclarative) {
    DEFINES += USE_FANCY_UI
}
#! [157]

#! [158]
#ifdef USE_FANCY_UI
    // Use the fancy UI, as we have extra packages available
#endif
#! [158]

#! [159]
message($$absolute_path("readme.txt", "/home/johndoe/myproject"))
#! [159]


#! [160]
TARGET = helloworld
equals(TARGET, "helloworld") {
    message("The target assignment was successful.")
}
#! [160]


#! [161]
CONTACT = firstname middlename surname phone
message($$first(CONTACT))
#! [161]


#! [162]
CONTACT = firstname middlename surname phone
message($$last(CONTACT))
#! [162]


#! [163]
message($$format_number(BAD, ibase=16 width=6 zeropad))
#! [163]


#! [164]
ANSWER = 42
greaterThan(ANSWER, 1) {
    message("The answer might be correct.")
}
#! [164]


#! [165]
ANSWER = 42
lessThan(ANSWER, 1) {
    message("The answer might be wrong.")
}
#! [165]


#! [166]
if(linux-g++*|macx-g++*):CONFIG(debug, debug|release) {
    message("We are on Linux or Mac OS, and we are in debug mode.")
}
#! [166]


#! [167]
CONTACT = firstname:middlename:surname:phone
message($$section(CONTACT, :, 2, 2))
#! [167]


#! [168]
CONTACT = firstname:middlename:surname:phone
message($$split(CONTACT, :))
#! [168]

#! [169]
NARF = zort
unset(NARF)
!defined(NARF, var) {
    message("NARF is not defined.")
}
#! [169]


#! [170]
for(var, $$list(foo bar baz)) {
    ...
}
#! [170]


#! [171]
values = foo bar baz
for(var, values) {
    ...
}
#! [171]


#! [172]
VALUE = 123
TMP_VALUE = x$$VALUE
greaterThan(TMP_VALUE, x456): message("Condition may be true.")
#! [172]


#! [173]
message("First line$$escape_expand(\\n)Second line")
#! [173]


#! [174]
TEMPLATE = subdirs
SUBDIRS = one two three
prepareRecursiveTarget(check)
#! [174]


#! [175]
two.CONFIG += no_check_target
#! [175]


#! [176]
QMAKE_EXTRA_TARGETS += check
#! [176]


#! [177]
# <project root>/features/mycheck.prf
equals(TEMPLATE, subdirs) {
    prepareRecursiveTarget(check)
} else {
    check.commands = echo hello user
}
QMAKE_EXTRA_TARGETS += check
#! [177]


#! [178]
# <project root>/.qmake.conf
CONFIG += mycheck
#! [178]


#! [179]
# <project root>/project.pro
load(configure)
#! [179]


#! [180]
# <project root>/config.tests/test/test.pro
SOURCES = main.cpp
LIBS += -ltheFeature
# Note that the test project is built without Qt by default.
#! [180]


#! [181]
// <project root>/config.tests/test/main.cpp
#include <TheFeature/MainHeader.h>
int main() { return featureFunction(); }
#! [181]


#! [182]
# <project root>/project.pro
qtCompileTest(test)
#! [182]
