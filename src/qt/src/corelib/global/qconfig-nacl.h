/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#define QT_FONTS_ARE_RESOURCES

/* Data structures */
#ifndef QT_NO_QUUID_STRING
#  define QT_NO_QUUID_STRING
#endif
#ifndef QT_NO_STL
#  define QT_NO_STL
#endif
#ifndef QT_NO_TEXTDATE
#  define QT_NO_TEXTDATE
#endif
#ifndef QT_NO_DATESTRING
#  define QT_NO_DATESTRING
#endif

/* Dialogs */
#ifndef QT_NO_FILEDIALOG
#  define QT_NO_FILEDIALOG
#endif
#ifndef QT_NO_PRINTDIALOG
#  define QT_NO_PRINTDIALOG
#endif
#ifndef QT_NO_PRINTPREVIEWDIALOG
#  define QT_NO_PRINTPREVIEWDIALOG
#endif


/* File I/O */
#ifndef QT_NO_DOM
#  define QT_NO_DOM
#endif
#ifndef QT_NO_FILESYSTEMWATCHER
#  define QT_NO_FILESYSTEMWATCHER
#endif
#ifndef QT_NO_FSFILEENGINE
#  define QT_NO_FSFILEENGINE
#endif
#ifndef QT_NO_FILESYSTEMMODEL
#  define QT_NO_FILESYSTEMMODEL
#endif
#ifndef QT_NO_FILESYSTEMMODEL
#  define QT_NO_FILESYSTEMMODEL
#endif
#ifndef QT_NO_PROCESS
#  define QT_NO_PROCESS
#endif
#ifndef QT_NO_TEMPORARYFILE
#  define QT_NO_TEMPORARYFILE
#endif
#ifndef QT_NO_SETTINGS
#  define QT_NO_SETTINGS
#endif
#ifndef QT_NO_LIBRARY
#  define QT_NO_LIBRARY
#endif

/* Fonts */
#ifndef QT_NO_QWS_QPF2
#  define QT_NO_QWS_QPF2
#endif

/* Images */
#ifndef QT_NO_IMAGEFORMATPLUGIN
#  define QT_NO_IMAGEFORMATPLUGIN
#endif
#ifndef QT_NO_IMAGE_HEURISTIC_MASK
#  define QT_NO_IMAGE_HEURISTIC_MASK
#endif
#ifndef QT_NO_IMAGE_TEXT
#  define QT_NO_IMAGE_TEXT
#endif
#ifndef QT_NO_MOVIE
#  define QT_NO_MOVIE
#endif

/* Internationalization */
#ifndef QT_NO_BIG_CODECS
#  define QT_NO_BIG_CODECS
#endif
#ifndef QT_NO_QWS_INPUTMETHODS
#  define QT_NO_QWS_INPUTMETHODS
#endif
#ifndef QT_NO_TEXTCODEC
#  define QT_NO_TEXTCODEC
#endif
#ifndef QT_NO_CODECS
#  define QT_NO_CODECS
#endif
#ifndef QT_NO_TEXTCODECPLUGIN
#  define QT_NO_TEXTCODECPLUGIN
#endif
#ifndef QT_NO_TRANSLATION
#  define QT_NO_TRANSLATION
#endif
#ifndef QT_NO_TRANSLATION_UTF8
#  define QT_NO_TRANSLATION_UTF8
#endif

/* ItemViews */

#ifndef QT_NO_DIRMODEL
#  define QT_NO_DIRMODEL
#endif

/* Kernel */
#ifndef QT_NO_CLIPBOARD
#  define QT_NO_CLIPBOARD
#endif
#ifndef QT_NO_CSSPARSER
#  define QT_NO_CSSPARSER
#endif
#ifndef QT_NO_CURSOR
#  define QT_NO_CURSOR
#endif
#ifndef QT_NO_DRAGANDDROP
#  define QT_NO_DRAGANDDROP
#endif
#ifndef QT_NO_EFFECTS
#  define QT_NO_EFFECTS
#endif
#ifndef QT_NO_SESSIONMANAGER
#  define QT_NO_SESSIONMANAGER
#endif
#ifndef QT_NO_SHAREDMEMORY
#  define QT_NO_SHAREDMEMORY
#endif
#ifndef QT_NO_SOUND
#  define QT_NO_SOUND
#endif
#ifndef QT_NO_SYSTEMLOCALE
#  define QT_NO_SYSTEMSEMAPHORE
#endif
#ifndef QT_NO_SYSTEMSEMAPHORE
#  define QT_NO_SYSTEMSEMAPHORE
#endif
#ifndef QT_NO_TABLETEVENT
#  define QT_NO_TABLETEVENT
#endif
#ifndef QT_NO_CRASHHANDLER
#  define QT_NO_CRASHHANDLER
#endif
#ifndef QT_NO_CONCURRENT
#  define QT_NO_CONCURRENT
#endif
#ifndef QT_NO_XMLSTREAM
#  define QT_NO_XMLSTREAM
#endif
#ifndef QT_NO_XMLSTREAMREADER
#  define QT_NO_XMLSTREAMREADER
#endif
#ifndef QT_NO_XMLSTREAMWRITER
#  define QT_NO_XMLSTREAMWRITER
#endif

/* Networking */
#ifndef QT_NO_COP
#  define QT_NO_COP
#endif
#ifndef QT_NO_HOSTINFO
#  define QT_NO_HOSTINFO
#endif
#ifndef QT_NO_HTTP
#  define QT_NO_HTTP
#endif
#ifndef QT_NO_NETWORKPROXY
#  define QT_NO_NETWORKPROXY
#endif
#ifndef QT_NO_SOCKS5
#  define QT_NO_SOCKS5
#endif
#ifndef QT_NO_UDPSOCKET
#  define QT_NO_UDPSOCKET
#endif
#ifndef QT_NO_URLINFO
#  define QT_NO_URLINFO
#endif
#ifndef QT_NO_FTP
#  define QT_NO_FTP
#endif

/* Painting */
#ifndef QT_NO_COLORNAMES
#  define QT_NO_COLORNAMES
#endif
#ifndef QT_NO_DIRECTPAINTER
#  define QT_NO_DIRECTPAINTER
#endif
#ifndef QT_NO_PAINTONSCREEN
#  define QT_NO_PAINTONSCREEN
#endif
#ifndef QT_NO_PAINT_DEBUG
#  define QT_NO_PAINT_DEBUG
#endif
#ifndef QT_NO_PICTURE
#  define QT_NO_PICTURE
#endif
#ifndef QT_NO_PRINTER
#  define QT_NO_PRINTER
#endif
#ifndef QT_NO_CUPS
#  define QT_NO_CUPS
#endif

/* Qt for Embedded Linux */
#ifndef QT_NO_QWSEMBEDWIDGET
#  define QT_NO_QWSEMBEDWIDGET
#endif
#ifndef QT_NO_QWS_CURSOR
#  define QT_NO_QWS_CURSOR
#endif
#ifndef QT_NO_QWS_DECORATION_DEFAULT
#  define QT_NO_QWS_DECORATION_DEFAULT
#endif
#ifndef QT_NO_QWS_DECORATION_STYLED
#  define QT_NO_QWS_DECORATION_STYLED
#endif
#ifndef QT_NO_QWS_DECORATION_WINDOWS
#  define QT_NO_QWS_DECORATION_WINDOWS
#endif
#ifndef QT_NO_QWS_MANAGER
#  define QT_NO_QWS_MANAGER
#endif
#ifndef QT_NO_QWS_KEYBOARD
#  define QT_NO_QWS_KEYBOARD
#endif
#ifndef QT_NO_QWS_MOUSE
#  define QT_NO_QWS_MOUSE
#endif
#ifndef QT_NO_QWS_MOUSE_AUTO
#  define QT_NO_QWS_MOUSE_AUTO
#endif
#ifndef QT_NO_QWS_MOUSE_MANUAL
#  define QT_NO_QWS_MOUSE_MANUAL
#endif
#ifndef QT_NO_QWS_MULTIPROCESS
#  define QT_NO_QWS_MULTIPROCESS
#endif
#ifndef QT_NO_QWS_SOUNDSERVER
#  define QT_NO_QWS_SOUNDSERVER
#endif
#ifndef QT_NO_SXE
#  define QT_NO_SXE
#endif
#ifndef QT_NO_QWS_PROPERTIES
#  define QT_NO_QWS_PROPERTIES
#endif
#ifndef QT_NO_QWS_PROXYSCREEN
#  define QT_NO_QWS_PROXYSCREEN
#endif
#ifndef QT_NO_QWS_DYNAMICSCREENTRANSFORMATION
#  define QT_NO_QWS_DYNAMICSCREENTRANSFORMATION
#endif
#ifndef QT_NO_QWS_LINUXFB
#  define QT_NO_QWS_LINUXFB
#endif
#ifndef QT_NO_QWS_MOUSE_PC
#  define QT_NO_QWS_MOUSE_PC
#endif
#ifndef QT_NO_QWS_MOUSE_LINUXTP
#  define QT_NO_QWS_MOUSE_LINUXTP
#endif
#ifndef QT_NO_QWS_QPF
#  define QT_NO_QWS_QPF
#endif

/* SVG */
#ifndef QT_NO_SVG
#  define QT_NO_SVG
#endif
#ifndef QT_NO_GRAPHICSSVGITEM
#  define QT_NO_GRAPHICSSVGITEM
#endif
#ifndef QT_NO_SVGGENERATOR
#  define QT_NO_SVGGENERATOR
#endif
#ifndef QT_NO_SVGRENDERER
#  define QT_NO_SVGRENDERER
#endif
#ifndef QT_NO_SVGWIDGET
#  define QT_NO_SVGWIDGET
#endif

/* Styles */
#ifndef QT_NO_STYLE_MOTIF
#  define QT_NO_STYLE_MOTIF
#endif
#ifndef QT_NO_STYLE_CDE
#  define QT_NO_STYLE_CDE
#endif
#ifndef QT_NO_STYLE_STYLESHEET
#  define QT_NO_STYLE_STYLESHEET
#endif
#ifndef QT_NO_STYLE_WINDOWSCE
#  define QT_NO_STYLE_WINDOWSCE
#endif
#ifndef QT_NO_STYLE_WINDOWSMOBILE
#  define QT_NO_STYLE_WINDOWSMOBILE
#endif
#ifndef QT_NO_STYLE_WINDOWSVISTA
#  define QT_NO_STYLE_WINDOWSVISTA
#endif
#ifndef QT_NO_STYLE_WINDOWSXP
#  define QT_NO_STYLE_WINDOWSXP
#endif

/* Utilities */
#ifndef QT_NO_ACCESSIBILITY
#  define QT_NO_ACCESSIBILITY
#endif
#ifndef QT_NO_COMPLETER
#  define QT_NO_COMPLETER
#endif
#ifndef QT_NO_DESKTOPSERVICES
#  define QT_NO_DESKTOPSERVICES
#endif
#ifndef QT_NO_SCRIPT
#  define QT_NO_SCRIPT
#endif
#ifndef QT_NO_SYSTEMTRAYICON
#  define QT_NO_SYSTEMTRAYICON
#endif

/* Windows */
#ifndef QT_NO_WIN_ACTIVEQT
#  define QT_NO_WIN_ACTIVEQT
#endif
