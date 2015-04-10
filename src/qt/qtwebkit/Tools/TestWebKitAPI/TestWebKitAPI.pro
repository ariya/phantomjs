TEMPLATE = subdirs
CONFIG += ordered

derived_sources.file = DerivedSources.pri
injected_bundle.file = InjectedBundle.pri
tests.file = Tests.pri

SUBDIRS += derived_sources injected_bundle tests

addStrictSubdirOrderBetween(derived_sources, injected_bundle)
addStrictSubdirOrderBetween(derived_sources, tests)
