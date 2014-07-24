# -------------------------------------------------------------------
# Main project file for WebKitTestRunner (WTR)
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = subdirs
CONFIG += ordered

derived_sources.file = DerivedSources.pri
target.file = Target.pri

SUBDIRS = derived_sources target

addStrictSubdirOrderBetween(derived_sources, target)

injected_bundle.file = InjectedBundle/InjectedBundle.pro
injected_bundle.makefile = Makefile.InjectedBundle
SUBDIRS += injected_bundle


