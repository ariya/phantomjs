#! [0]
CONFIG += debug_and_release

CONFIG(debug, debug|release) {
    TARGET = debug_binary
} else {
#! [0] #! [1]
    TARGET = release_binary
}
#! [1]

#! [2]
CONFIG += build_all
#! [2]
