## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libharfbuzz-1.la

MAINSOURCES =  \
	harfbuzz-buffer.c \
	harfbuzz-stream.c \
	harfbuzz-dump.c \
	harfbuzz-gdef.c \
	harfbuzz-gpos.c \
	harfbuzz-gsub.c \
	harfbuzz-impl.c \
	harfbuzz-open.c \
	harfbuzz-shaper.cpp \
        harfbuzz-greek.c \
        harfbuzz-tibetan.c \
	harfbuzz-khmer.c \
	harfbuzz-indic.cpp \
	harfbuzz-hebrew.c \
	harfbuzz-arabic.c \
	harfbuzz-hangul.c \
	harfbuzz-myanmar.c \
	harfbuzz-thai.c

EXTRA_SOURCES = harfbuzz.c

PUBLICHEADERS = \
	harfbuzz.h \
	harfbuzz-buffer.h \
	harfbuzz-dump.h \
	harfbuzz-gdef.h \
	harfbuzz-gpos.h \
	harfbuzz-gsub.h \
	harfbuzz-open.h \
	harfbuzz-global.h \
	harfbuzz-external.h \
	harfbuzz-shaper.h \
	harfbuzz-stream.h

PRIVATEHEADERS = \
	harfbuzz-impl.h \
	harfbuzz-buffer-private.h \
	harfbuzz-stream-private.h \
	harfbuzz-gdef-private.h \
	harfbuzz-gpos-private.h \
	harfbuzz-gsub-private.h \
	harfbuzz-open-private.h \
	harfbuzz-shaper-private.h

libharfbuzz_1_la_SOURCES = \
	$(MAINSOURCES) \
	$(PUBLICHEADERS) \
	$(PRIVATEHEADERS)

#noinst_PROGRAMS = harfbuzz-dump
#
#harfbuzz_dump_SOURCES = 	\
#	harfbuzz-dump-main.c
#
#harfbuzz_dump_LDADD = 		\
#	libharfbuzz-1.la

EXTRA_DIST = 		\
	README		\
	COPYING.FTL	\
	COPYING.GPL	\
	COPYING		\
	$(EXTRA_SOURCES)

