CONFIG += java
DESTDIR = $$[QT_INSTALL_PREFIX/get]/jar
API_VERSION = android-16

PATHPREFIX = $$PWD/src/org/qtproject/qt5/android/accessibility

JAVACLASSPATH += $$PWD/src/
JAVASOURCES += \
    $$PATHPREFIX/QtAccessibilityDelegate.java \
    $$PATHPREFIX/QtNativeAccessibility.java

# install
target.path = $$[QT_INSTALL_PREFIX]/jar
INSTALLS += target