# common project include file for JavaScriptCore and WebCore

contains(JAVASCRIPTCORE_JIT,yes): DEFINES+=ENABLE_JIT=1
contains(JAVASCRIPTCORE_JIT,no): DEFINES+=ENABLE_JIT=0

linux-g++ {
isEmpty($$(SBOX_DPKG_INST_ARCH)):exists(/usr/bin/ld.gold) {
    message(Using gold linker)
    QMAKE_LFLAGS+=-fuse-ld=gold
}
}

# We use this flag on production branches
# See https://bugs.webkit.org/show_bug.cgi?id=60824
CONFIG += production
