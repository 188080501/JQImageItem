QT *= core gui qml quick opengl

TARGET = JQImageItem

INCLUDEPATH *= \
    $$PWD/cpp

HEADERS *= \
    $$PWD/cpp/helper.h \
    $$PWD/cpp/jqimageitem.h

SOURCES *= \
    $$PWD/cpp/main.cpp \
    $$PWD/cpp/helper.cpp \
    $$PWD/cpp/jqimageitem.cpp

RESOURCES *= \
    $$PWD/qml/qml.qrc \
    $$PWD/images/images.qrc
