#pragma once

#include <QtCore/qglobal.h>

#ifndef QRGUI_TOOL_PLUGIN_INTERFACE_EXPORT
#  if defined(QRGUI_TOOL_PLUGIN_INTERFACE_LIBRARY)
#    define QRGUI_TOOL_PLUGIN_INTERFACE_EXPORT Q_DECL_EXPORT
#  else
#    define QRGUI_TOOL_PLUGIN_INTERFACE_EXPORT Q_DECL_IMPORT
#  endif
#endif
