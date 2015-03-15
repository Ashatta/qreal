TEMPLATE = lib

TARGET = qrgui-palette

include(../../global.pri)

QT += widgets

links(qrkernel)
includes(qrgui)

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

