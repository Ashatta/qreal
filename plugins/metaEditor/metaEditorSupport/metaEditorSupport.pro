include(../../../global.pri)

QT += xml widgets

TEMPLATE = lib
CONFIG += plugin

DESTDIR = $$DESTDIR/plugins/tools/

links(qrkernel qrutils qrmc qrgui-preferences-dialog)
includes(qrgui)

HEADERS += \
	metaEditorSupportPlugin.h \
	editorGenerator.h \
	xmlParser.h \
	compilerPage.h \
	versionChooserDialog.h \
	migrationDialog.h \
	migrationEditor.h

SOURCES += \
	metaEditorSupportPlugin.cpp \
	editorGenerator.cpp \
	xmlParser.cpp \
	compilerPage.cpp \
	versionChooserDialog.cpp \
	migrationDialog.cpp \
	migrationEditor.cpp

FORMS += \
	compilerPage.ui \
	versionchooserdialog.ui \
	migrationDialog.ui \
	migrationEditor.ui

TRANSLATIONS = $$PWD/../../../qrtranslations/ru/plugins/metaEditorSupport_ru.ts

RESOURCES = metaEditorSupport.qrc
