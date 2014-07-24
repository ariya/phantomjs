#! [0] #! [1]
DESTDIR = $$(PWD)
message(The project will be installed in $$DESTDIR)
#! [0]

DESTDIR = $(PWD)
message(The project will be installed in the value of PWD)
message(when the Makefile is processed.)
#! [1]
