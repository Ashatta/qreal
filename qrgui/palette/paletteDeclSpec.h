#pragma once

#include <QtCore/qglobal.h>

#ifndef QRGUI_PALETTE_EXPORT
#  if defined(QRGUI_PALETTE_LIBRARY)
#    define QRGUI_PALETTE_EXPORT Q_DECL_EXPORT
#  else
#    define QRGUI_PALETTE_EXPORT Q_DECL_IMPORT
#  endif
#endif
