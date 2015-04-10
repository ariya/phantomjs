contains(QT_CONFIG, harfbuzz) {
    INCLUDEPATH += $$PWD/harfbuzz-ng/include
    LIBS_PRIVATE += -L$$QT_BUILD_TREE/lib -lqtharfbuzzng$$qtPlatformTargetSuffix()
} else:contains(QT_CONFIG, system-harfbuzz) {
    LIBS_PRIVATE += -lharfbuzz
}
