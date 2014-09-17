/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef npinterface_h
#define npinterface_h

#include "npfunctions.h"
#include <QtPlugin>

class NPInterface {
public:
    virtual NPError NP_Initialize(NPNetscapeFuncs* aNPNFuncs, NPPluginFuncs* aNPPFuncs) = 0;
    virtual void NP_Shutdown() = 0;
    virtual char* NP_GetMIMEDescription() = 0;
};


QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(NPInterface, "com.nokia.qts60.webplugin/1.0");
QT_END_NAMESPACE

#endif // npinterface_h
