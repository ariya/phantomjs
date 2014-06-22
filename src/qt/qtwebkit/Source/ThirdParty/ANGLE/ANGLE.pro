# -------------------------------------------------------------------
# Main project file for ANGLE
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = subdirs

derived_sources.file = DerivedSources.pri
target.file = Target.pri

SUBDIRS += derived_sources target

addStrictSubdirOrderBetween(derived_sources, target)
