
check_PROGRAMS = shaping

shaping_SOURCES = main.cpp harfbuzz-qt.cpp
shaping_LDADD = $(QT_GUI_LIBS) $(QT_QTEST_LIBS) ../../src/libharfbuzz-1.la

main.o: main.moc

main.moc: $(srcdir)/main.cpp
	$(QT_MOC) -o main.moc $(srcdir)/main.cpp

INCLUDES = -I$(top_srcdir)/src $(FREETYPE_CFLAGS) $(QT_GUI_CFLAGS) $(QT_QTEST_CFLAGS)
AM_CPPFLAGS = -DQT_GUI_LIB -DSRCDIR=\"$(srcdir)\"

