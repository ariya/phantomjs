/*
    Copyright (c) 2011-2013, BogDan Vatra <bogdan@kde.org>
    Contact: http://www.qt-project.org/legal

    Commercial License Usage
    Licensees holding valid commercial Qt licenses may use this file in
    accordance with the commercial license agreement provided with the
    Software or, alternatively, in accordance with the terms contained in
    a written agreement between you and Digia.  For licensing terms and
    conditions see http://qt.digia.com/licensing.  For further information
    use the contact form at http://qt.digia.com/contact-us.

    BSD License Usage
    Alternatively, this file may be used under the BSD license as follows:
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package org.kde.necessitas.ministro;

oneway interface IMinistroCallback {
/**
* This method is called by the Ministro service back into the application which
* implements this interface.
*
* param in - loaderParams
*            loaderParams fields:
*                 * Key Name                   Key type         Explanations
*                 * "error.code"               Integer          See below
*                 * "error.message"            String           Missing if no error, otherwise will contain the error message translated into phone language where available.
*                 * "dex.path"                 String           The list of jar/apk files containing classes and resources, needed to be passed to application DexClassLoader
*                 * "lib.path"                 String           The list of directories containing native libraries; may be missing, needed to be passed to application DexClassLoader
*                 * "loader.class.name"        String           Loader class name.
*
* "error.code" field possible errors:
*  - 0 no error.
*  - 1 incompatible Ministro version. Ministro needs to be upgraded.
*  - 2 not all modules could be satisfy.
*  - 3 invalid parameters
*  - 4 invalid qt version
*  - 5 download canceled
*
* The parameter contains additional fields which are used by the loader to start your application, so it must be passed to the loader.
*/

    void loaderReady(in Bundle loaderParams);
}
