TEMPLATE = subdirs

TOOLS_SUBDIRS = src_tools_bootstrap src_tools_moc src_tools_rcc
!contains(QT_CONFIG, no-gui): TOOLS_SUBDIRS += src_tools_uic
!cross_compile {
    contains(QT_CONFIG, qt3support): SRC_SUBDIRS += src_tools_uic3
    win32:!wince*:!win32-g++*: SRC_SUBDIRS += src_tools_idc
}

# Set subdir and respective target name
src_tools_bootstrap.subdir = $$QT_SOURCE_TREE/src/tools/bootstrap
src_tools_bootstrap.target = sub-tools-bootstrap
src_tools_moc.subdir = $$QT_SOURCE_TREE/src/tools/moc
src_tools_moc.target = sub-moc
src_tools_rcc.subdir = $$QT_SOURCE_TREE/src/tools/rcc
src_tools_rcc.target = sub-rcc
src_tools_uic.subdir = $$QT_SOURCE_TREE/src/tools/uic
src_tools_uic.target = sub-uic
src_tools_uic3.subdir = $$QT_SOURCE_TREE/src/tools/uic3
src_tools_uic3.target = sub-uic3
src_tools_idc.subdir = $$QT_SOURCE_TREE/src/tools/idc
src_tools_idc.target = sub-idc

!wince*:!ordered {
    # Set dependencies for each subdir
    src_tools_moc.depends = src_tools_bootstrap
    src_tools_rcc.depends = src_tools_bootstrap
    src_tools_uic.depends = src_tools_bootstrap
}

# Special handling, depending on type of project, if it used debug/release or only has one configuration
EXTRA_DEBUG_TARGETS =
EXTRA_RELEASE_TARGETS =
!symbian {
    for(subname, TOOLS_SUBDIRS) {
        subdir = $$subname
        !isEmpty($${subname}.subdir):subdir = $$eval($${subname}.subdir)
        subpro = $$subdir/$${basename(subdir)}.pro
        !exists($$subpro):next()
        subtarget = $$replace(subdir, [^A-Za-z0-9], _)
        reg_src = $$replace(QT_SOURCE_TREE, \\\\, \\\\)
        subdir = $$replace(subdir, $$reg_src, $$QT_BUILD_TREE)
        subdir = $$replace(subdir, /, $$QMAKE_DIR_SEP)
        subdir = $$replace(subdir, \\\\, $$QMAKE_DIR_SEP)
        SUB_TEMPLATE = $$list($$fromfile($$subpro, TEMPLATE))
        !isEqual(subname, src_tools_bootstrap):if(isEqual($$SUB_TEMPLATE, lib) | isEqual($$SUB_TEMPLATE, subdirs) | isEqual(subname, src_tools_idc) | isEqual(subname, src_tools_uic3)):!separate_debug_info {
            #debug
            debug-$${subtarget}.depends = $${subdir}$${QMAKE_DIR_SEP}$(MAKEFILE) $$EXTRA_DEBUG_TARGETS
            debug-$${subtarget}.commands = (cd $$subdir && $(MAKE) -f $(MAKEFILE) debug)
            EXTRA_DEBUG_TARGETS += debug-$${subtarget}
            QMAKE_EXTRA_TARGETS += debug-$${subtarget}
            #release
            release-$${subtarget}.depends = $${subdir}$${QMAKE_DIR_SEP}$(MAKEFILE) $$EXTRA_RELEASE_TARGETS
            release-$${subtarget}.commands = (cd $$subdir && $(MAKE) -f $(MAKEFILE) release)
            EXTRA_RELEASE_TARGETS += release-$${subtarget}
            QMAKE_EXTRA_TARGETS += release-$${subtarget}
        } else { #do not have a real debug target/release
            #debug
            debug-$${subtarget}.depends = $${subdir}$${QMAKE_DIR_SEP}$(MAKEFILE) $$EXTRA_DEBUG_TARGETS
            debug-$${subtarget}.commands = (cd $$subdir && $(MAKE) -f $(MAKEFILE) first)
            EXTRA_DEBUG_TARGETS += debug-$${subtarget}
            QMAKE_EXTRA_TARGETS += debug-$${subtarget}
            #release
            release-$${subtarget}.depends = $${subdir}$${QMAKE_DIR_SEP}$(MAKEFILE) $$EXTRA_RELEASE_TARGETS
            release-$${subtarget}.commands = (cd $$subdir && $(MAKE) -f $(MAKEFILE) first)
            EXTRA_RELEASE_TARGETS += release-$${subtarget}
            QMAKE_EXTRA_TARGETS += release-$${subtarget}
        }
    }
}

SUBDIRS = $$TOOLS_SUBDIRS $$SUBDIRS
isEqual(TARGET,tools): SUBDIRS += $$SRC_SUBDIRS
