TEMPLATE = subdirs
CONFIG += ordered

derived_sources.file = DerivedSources.pri
target.file = Target.pri

SUBDIRS = derived_sources target

addStrictSubdirOrderBetween(derived_sources, target)
