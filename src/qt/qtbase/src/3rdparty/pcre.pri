DEFINES += PCRE_HAVE_CONFIG_H

win32:DEFINES += PCRE_STATIC
ios:DEFINES += PCRE_DISABLE_JIT
qnx:DEFINES += PCRE_DISABLE_JIT
winrt:DEFINES += PCRE_DISABLE_JIT

INCLUDEPATH += $$PWD/pcre
SOURCES += \
    $$PWD/pcre/pcre16_byte_order.c \
    $$PWD/pcre/pcre16_chartables.c \
    $$PWD/pcre/pcre16_compile.c \
    $$PWD/pcre/pcre16_config.c \
    $$PWD/pcre/pcre16_dfa_exec.c \
    $$PWD/pcre/pcre16_exec.c \
    $$PWD/pcre/pcre16_fullinfo.c \
    $$PWD/pcre/pcre16_get.c \
    $$PWD/pcre/pcre16_globals.c \
    $$PWD/pcre/pcre16_jit_compile.c \
    $$PWD/pcre/pcre16_maketables.c \
    $$PWD/pcre/pcre16_newline.c \
    $$PWD/pcre/pcre16_ord2utf16.c \
    $$PWD/pcre/pcre16_refcount.c \
    $$PWD/pcre/pcre16_string_utils.c \
    $$PWD/pcre/pcre16_study.c \
    $$PWD/pcre/pcre16_tables.c \
    $$PWD/pcre/pcre16_ucd.c \
    $$PWD/pcre/pcre16_utf16_utils.c \
    $$PWD/pcre/pcre16_valid_utf16.c \
    $$PWD/pcre/pcre16_version.c \
    $$PWD/pcre/pcre16_xclass.c

TR_EXCLUDE += $$PWD/*
