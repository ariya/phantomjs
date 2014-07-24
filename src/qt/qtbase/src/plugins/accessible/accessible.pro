TEMPLATE = subdirs

contains(QT_CONFIG, accessibility) {
     SUBDIRS += widgets 
}
