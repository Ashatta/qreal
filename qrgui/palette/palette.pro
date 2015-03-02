TEMPLATE = lib

DESTDIR = $$PWD/../../bin

TARGET = qrgui-palette

LIBS += -L$$PWD/../../bin -lqrkernel

INCLUDEPATH = \
	$$PWD/ \
	$$PWD/../ \
	$$PWD/../../ \

QT += widgets

CONFIG += c++11

OBJECTS_DIR = .obj
UI_DIR = .ui
MOC_DIR = .moc
RCC_DIR = .moc

DEFINES += QRGUI_PALETTE_LIBRARY

HEADERS += \
	$$PWD/paletteDeclSpec.h \
	$$PWD/draggableElement.h \
	$$PWD/paletteElement.h \
	$$PWD/paletteTree.h \
	$$PWD/paletteTreeWidget.h \
	$$PWD/paletteTreeWidgets.h \

SOURCES += \
	$$PWD/draggableElement.cpp \
	$$PWD/paletteElement.cpp \
	$$PWD/paletteTree.cpp \
	$$PWD/paletteTreeWidget.cpp \
	$$PWD/paletteTreeWidgets.cpp \

