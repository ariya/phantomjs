SOURCES = mysql.cpp
CONFIG -= qt dylib
mac:CONFIG -= app_bundle
LIBS += -lmysqlclient
