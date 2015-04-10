SCRIPTS_PATH = ../Scripts

ifneq (,$(findstring iphoneos,$(SDKROOT)))
	OTHER_OPTIONS += -target All-iOS
else ifneq (,$(findstring iphonesimulator,$(SDKROOT)))
	OTHER_OPTIONS += -target All-iOS
endif

include ../../Makefile.shared
