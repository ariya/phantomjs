SOURCES = main.cpp
#! [0]
CONFIG += opengl
#! [0]

#! [1]
opengl {
    TARGET = application-gl
} else {
#! [1] #! [2]
    TARGET = application
#! [2] #! [3]
}
#! [3]

#! [4]
CONFIG(opengl) {
    message(Building with OpenGL support.)
} else {
#! [4] #! [5]
    message(OpenGL support is not available.)
}
#! [5]
