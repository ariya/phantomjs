# Empty file since Qt 4.6
# I'm too lazy to find all places where this file is included

symbian{
    TRY_INCLUDEPATHS = $${EPOCROOT}epoc32 $${EPOCROOT}epoc32/include $${EPOCROOT}epoc32/include/stdapis $${EPOCROOT}epoc32/include/stdapis/sys $$OS_LAYER_LIBC_SYSTEMINCLUDE $$QMAKE_INCDIR $$INCLUDEPATH 
    for(p, TRY_INCLUDEPATHS) {
        pp = $$join(p, "", "", "/openssl")
        exists($$pp):INCLUDEPATH *= $$pp
    } 
}
