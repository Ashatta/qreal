# Copyright 2007-2015 QReal Research Group
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
	migrationEditor.h \
	migrationManager.h

SOURCES += \
	metaEditorSupportPlugin.cpp \
	editorGenerator.cpp \
	xmlParser.cpp \
	compilerPage.cpp \
	versionChooserDialog.cpp \
	migrationDialog.cpp \
	migrationEditor.cpp \
	migrationManager.cpp

FORMS += \
	compilerPage.ui \
	versionchooserdialog.ui \
	migrationDialog.ui \
	migrationEditor.ui \
	migrationManager.ui

TRANSLATIONS = $$PWD/../../../qrtranslations/ru/plugins/metaEditorSupport_ru.ts

RESOURCES = metaEditorSupport.qrc
