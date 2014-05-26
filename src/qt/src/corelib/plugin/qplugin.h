/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPLUGIN_H
#define QPLUGIN_H

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef Q_EXTERN_C
#  ifdef __cplusplus
#    define Q_EXTERN_C extern "C"
#  else
#    define Q_EXTERN_C extern
#  endif
#endif

typedef QObject *(*QtPluginInstanceFunction)();

void Q_CORE_EXPORT qRegisterStaticPluginInstanceFunction(QtPluginInstanceFunction function);

#define Q_IMPORT_PLUGIN(PLUGIN) \
        extern QT_PREPEND_NAMESPACE(QObject) *qt_plugin_instance_##PLUGIN(); \
        class Static##PLUGIN##PluginInstance{ \
        public: \
                Static##PLUGIN##PluginInstance() { \
                qRegisterStaticPluginInstanceFunction(qt_plugin_instance_##PLUGIN); \
                } \
        }; \
       static Static##PLUGIN##PluginInstance static##PLUGIN##Instance;

#define Q_PLUGIN_INSTANCE(IMPLEMENTATION) \
        { \
            static QT_PREPEND_NAMESPACE(QPointer)<QT_PREPEND_NAMESPACE(QObject)> _instance; \
            if (!_instance)      \
                _instance = new IMPLEMENTATION; \
            return _instance; \
        }

#  define Q_EXPORT_PLUGIN(PLUGIN) \
            Q_EXPORT_PLUGIN2(PLUGIN, PLUGIN)

#  define Q_EXPORT_STATIC_PLUGIN(PLUGIN) \
            Q_EXPORT_STATIC_PLUGIN2(PLUGIN, PLUGIN)

#if defined(QT_STATICPLUGIN)

#  define Q_EXPORT_PLUGIN2(PLUGIN, PLUGINCLASS) \
            QT_PREPEND_NAMESPACE(QObject) \
                *qt_plugin_instance_##PLUGIN() \
            Q_PLUGIN_INSTANCE(PLUGINCLASS)

#  define Q_EXPORT_STATIC_PLUGIN2(PLUGIN, PLUGINCLASS) \
            Q_EXPORT_PLUGIN2(PLUGIN, PLUGINCLASS)

#else
// NOTE: if you change pattern, you MUST change the pattern in
// qlibrary.cpp as well.  changing the pattern will break all
// backwards compatibility as well (no old plugins will be loaded).
// QT5: should probably remove the entire pattern thing and do the section
//      trick for all platforms. for now, keep it and fallback to scan for it.
#  ifdef QPLUGIN_DEBUG_STR
#    undef QPLUGIN_DEBUG_STR
#  endif
#  ifdef QT_NO_DEBUG
#    define QPLUGIN_DEBUG_STR "false"
#    define QPLUGIN_SECTION_DEBUG_STR ""
#  else
#    define QPLUGIN_DEBUG_STR "true"
#    define QPLUGIN_SECTION_DEBUG_STR ".debug"
#  endif
#  define Q_PLUGIN_VERIFICATION_DATA \
    static const char qt_plugin_verification_data[] = \
      "pattern=QT_PLUGIN_VERIFICATION_DATA\n" \
      "version=" QT_VERSION_STR "\n" \
      "debug=" QPLUGIN_DEBUG_STR "\n" \
      "buildkey=" QT_BUILD_KEY;

#  if defined (Q_OF_ELF) && defined (Q_CC_GNU)
#  define Q_PLUGIN_VERIFICATION_SECTION \
    __attribute__ ((section (".qtplugin"))) __attribute__((used))
#  else
#  define Q_PLUGIN_VERIFICATION_SECTION
#  endif

#  if defined (Q_OS_WIN32) && defined(Q_CC_BOR)
#     define Q_STANDARD_CALL __stdcall
#  else
#     define Q_STANDARD_CALL
#  endif

#  define Q_EXPORT_PLUGIN2(PLUGIN, PLUGINCLASS)      \
            Q_PLUGIN_VERIFICATION_SECTION Q_PLUGIN_VERIFICATION_DATA \
            Q_EXTERN_C Q_DECL_EXPORT \
            const char * Q_STANDARD_CALL qt_plugin_query_verification_data() \
            { return qt_plugin_verification_data; } \
            Q_EXTERN_C Q_DECL_EXPORT QT_PREPEND_NAMESPACE(QObject) * Q_STANDARD_CALL qt_plugin_instance() \
            Q_PLUGIN_INSTANCE(PLUGINCLASS)

#  define Q_EXPORT_STATIC_PLUGIN2(PLUGIN, PLUGINCLASS)

#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // Q_PLUGIN_H
