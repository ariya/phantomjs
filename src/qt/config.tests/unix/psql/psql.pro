SOURCES = psql.cpp
CONFIG -= qt dylib
mac:CONFIG -= app_bundle
LIBS *= -lpq
