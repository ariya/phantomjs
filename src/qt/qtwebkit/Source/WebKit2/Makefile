# Build WebKit2 only on SnowLeopard and later.

OSX_VERSION ?= $(shell sw_vers -productVersion | cut -d. -f 2)
BUILD_WEBKIT2 = $(shell (( $(OSX_VERSION) >= 6 )) && echo "YES" )

ifeq "$(BUILD_WEBKIT2)" "YES"

include ../Makefile.shared

else

all: ;

debug d development dev develop: ;

release r deployment dep deploy: ;

clean: ;

endif
