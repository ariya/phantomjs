x11 {
        contains(QT_CONFIG, nas): LIBS_PRIVATE += -laudio -lXt
}
 
