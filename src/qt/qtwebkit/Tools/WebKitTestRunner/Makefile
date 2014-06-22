# Build WebKitTestRunner only on Snow Leopard and later.

OSX_VERSION ?= $(shell sw_vers -productVersion | cut -d. -f 2)
BUILD_WEBKITTESTRUNNER = $(shell (( $(OSX_VERSION) >= 6 )) && echo "YES" )

ifeq "$(BUILD_WEBKITTESTRUNNER)" "YES"

SCRIPTS_PATH = ../Scripts
include ../../Makefile.shared

else

all: ;

debug d development dev develop: ;

release r deployment dep deploy: ;

clean: ;

endif
