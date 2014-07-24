# -------------------------------------------------------------------
# Target file for the leveldb static library
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = lib
TARGET = leveldb

include(leveldb.pri)

CONFIG += staticlib

HEADERS += \
    db/builder.h \
    db/dbformat.h \
    db/db_impl.h \
    db/db_iter.h \
    db/filename.h \
    db/log_format.h \
    db/log_reader.h \
    db/log_writer.h \
    db/memtable.h \
    db/skiplist.h \
    db/snapshot.h \
    db/table_cache.h \
    db/version_edit.h \
    db/version_set.h \
    db/write_batch_internal.h \
    port/port.h \
    port/port_qt.h \
    port/thread_annotations.h \
    port/win/stdint.h \
    helpers/memenv/memenv.h \
    table/block_builder.h \
    table/block.h \
    table/filter_block.h \
    table/format.h \
    table/iterator_wrapper.h \
    table/merger.h \
    table/two_level_iterator.h \
    include/leveldb/cache.h \
    include/leveldb/c.h \
    include/leveldb/comparator.h \
    include/leveldb/db.h \
    include/leveldb/env.h \
    include/leveldb/filter_policy.h \
    include/leveldb/iterator.h \
    include/leveldb/options.h \
    include/leveldb/slice.h \
    include/leveldb/status.h \
    include/leveldb/table_builder.h \
    include/leveldb/table.h \
    include/leveldb/write_batch.h \
    util/arena.h \
    util/coding.h \
    util/crc32c.h \
    util/hash.h \
    util/histogram.h \
    util/logging.h \
    util/mutexlock.h \
    util/qt_logger.h \
    util/random.h

SOURCES += \
    db/builder.cc\
    db/c.cc \
    db/dbformat.cc \
    db/db_impl.cc \
    db/db_iter.cc \
    db/filename.cc \
    db/log_reader.cc \
    db/log_writer.cc \
    db/memtable.cc \
    db/repair.cc \
    db/table_cache.cc \
    db/version_edit.cc \
    db/version_set.cc \
    db/write_batch.cc \
    helpers/memenv/memenv.cc \
    table/block_builder.cc \
    table/block.cc \
    table/filter_block.cc \
    table/format.cc \
    table/iterator.cc \
    table/merger.cc \
    table/table_builder.cc \
    table/table.cc \
    table/two_level_iterator.cc \
    util/arena.cc \
    util/bloom.cc \
    util/cache.cc \
    util/coding.cc \
    util/comparator.cc \
    util/crc32c.cc \
    util/env.cc \
    util/env_qt.cc \
    util/filter_policy.cc \
    util/hash.cc \
    util/histogram.cc \
    util/logging.cc \
    util/options.cc \
    util/status.cc

DEFINES += LEVELDB_PLATFORM_QT
win: DEFINES += OS_WIN
mac: DEFINES += OS_MACOSX
linux: DEFINES += OS_LINUX
freebsd*: DEFINES += OS_FREEBSD

gcc {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-error=unused-but-set-variable
    QMAKE_CXXFLAGS += -Wno-error=unused-but-set-variable
}

QT += core
