SOURCES = ../mysql/mysql.cpp
CONFIG -= qt dylib
mac:CONFIG -= app_bundle
LIBS += -lmysqlclient_r
