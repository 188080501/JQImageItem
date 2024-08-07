QT *= core gui qml quick opengl

!wasm {
    QT *= concurrent
}

TARGET = JQImageItemTester

INCLUDEPATH *= \
    $$PWD/cpp \
    $$PWD/../cpp

HEADERS *= \
    $$PWD/cpp/helper.h \
    $$PWD/../cpp/jqimageitem.h

SOURCES *= \
    $$PWD/cpp/main.cpp \
    $$PWD/cpp/helper.cpp \
    $$PWD/../cpp/jqimageitem.cpp

RESOURCES *= \
    $$PWD/qml/qml.qrc \
    $$PWD/images/images.qrc

win32-msvc {

    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
}
