#! [0]
EXTRAS = handlers tests docs
for(dir, EXTRAS) {
    exists($$dir) {
        SUBDIRS += $$dir
    }
}
#! [0]

SOURCES = paintwidget_mac.cpp paintwidget_unix.cpp paintwidget_win.cpp
macx {
    SOURCES = $$find(SOURCES, "_mac")
}

#! [1]
HEADERS = model.h
HEADERS += $$OTHER_HEADERS
HEADERS = $$unique(HEADERS)
#! [1]

CONFIG += debug
#! [2]
options = $$find(CONFIG, "debug") $$find(CONFIG, "release")
#! [3]
count(options, 2) {
    message(Both release and debug specified.)
}
#! [2] #! [3]

#! [4]
eval(TARGET = myapp) {
    message($$TARGET)
}
#! [4]
