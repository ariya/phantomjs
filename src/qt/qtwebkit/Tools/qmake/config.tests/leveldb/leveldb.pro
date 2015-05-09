SOURCES = leveldb.cpp
OBJECTS_DIR = obj
LIBS += -lleveldb -lmemenv

load(qt_build_config)
