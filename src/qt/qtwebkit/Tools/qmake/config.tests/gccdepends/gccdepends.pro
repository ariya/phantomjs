TEMPLATE = aux
OBJECTS_DIR = obj
QMAKE_CXXFLAGS += -MD

base_filename = empty
object_file = $$OBJECTS_DIR/$${base_filename}.o
deps_filename = $${base_filename}.d

SOURCES += $${base_filename}.cpp

test.commands = \
    # Earlier teambuilder and icecream versions would not
    # respect the -o argument for the .d file, so the file
    # would end up in the root build dir.
    test ! -f $${deps_filename} && \
    \
    # But it should end up in the OBJECTS_DIR
    test -f $$OBJECTS_DIR/$${deps_filename} && \
    \
    # Icecream 0.9.7 and earlier does not ensure that the
    # target rule matches the path of the .o file, since
    # the file is compiled into the current dir and then
    # moved. Verify that we don't hit that case.
    grep -q \"$${object_file}:\" $$OBJECTS_DIR/$${deps_filename} && \
    \
    # If everything is all right we mark the test as succeeded
    echo success > $$basename(PWD)

test.depends = $${object_file}
QMAKE_EXTRA_TARGETS += test

default.target = all
default.depends += test
QMAKE_EXTRA_TARGETS += default
