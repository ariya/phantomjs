/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifdef MOC_MWERKS_PLUGIN

#include "mwerks_mac.h"
#include "qt_mac.h"

/* compiler headers */
#include "DropInCompilerLinker.h"
#include "CompilerMapping.h"
#include "CWPluginErrors.h"

/* standard headers */
#include <stdio.h>
#include <string.h>

QT_BEGIN_NAMESPACE

//qglobal.cpp
const unsigned char * p_str(const char * c);
QCString pstring2qstring(const unsigned char *c);

#if CW_USE_PRAGMA_EXPORT
#pragma export on
#endif

CWPLUGIN_ENTRY(CWPlugin_GetDropInFlags)(const DropInFlags** flags, long* flagsSize)
{
        static const DropInFlags sFlags = {
                kCurrentDropInFlagsVersion,
                CWDROPINCOMPILERTYPE,
                DROPINCOMPILERLINKERAPIVERSION_7,
                kCompAlwaysReload|kCompRequiresProjectBuildStartedMsg,
                Lang_C_CPP,
                DROPINCOMPILERLINKERAPIVERSION
        };
        *flags = &sFlags;
        *flagsSize = sizeof(sFlags);
        return cwNoErr;
}



CWPLUGIN_ENTRY(CWPlugin_GetDropInName)(const char** dropinName)
{
        static const char sDropInName[] = "McMoc";
        *dropinName = sDropInName;
        return cwNoErr;
}

CWPLUGIN_ENTRY(CWPlugin_GetDisplayName)(const char** displayName)
{
        static const char sDisplayName[] = "McMoc";
        *displayName = sDisplayName;
        return cwNoErr;
}

CWPLUGIN_ENTRY(CWPlugin_GetTargetList)(const CWTargetList** targetList)
{
        static CWDataType sCPU = targetCPUAny;
        static CWDataType sOS = targetOSMacintosh;
        static CWTargetList sTargetList = {kCurrentCWTargetListVersion, 1, &sCPU, 1, &sOS};
        *targetList = &sTargetList;
        return cwNoErr;
}

CWPLUGIN_ENTRY(CWPlugin_GetDefaultMappingList)(const CWExtMapList** defaultMappingList)
{
        static CWExtensionMapping sExtension[] = { {'TEXT', ".mocs", kPrecompile } };
        static CWExtMapList sExtensionMapList = {kCurrentCWExtMapListVersion, 3, sExtension};
        *defaultMappingList = &sExtensionMapList;
        return cwNoErr;
}

#if CW_USE_PRAGMA_EXPORT
#pragma export off
#endif
typedef short CWFileRef;

static int line_count = 0;
moc_status do_moc(CWPluginContext, const QCString &, const QCString &, CWFileSpec *, bool);

static CWResult        mocify(CWPluginContext context, const QCString &source)
{
    CWDisplayLines(context, line_count++);

    source.stripWhiteSpace();

    CWResult err;
        bool            dotmoc=false;
        QCString stem = source, ext;
        int dotpos = stem.findRev('.');
    if(dotpos != -1) {
        ext = stem.right(stem.length() - (dotpos+1));
        stem = stem.left(dotpos);
        if(ext == "cpp")
            dotmoc = true;
    } else {
        //whoa!
    }
    QCString dest;
    if(dotmoc)
        dest = stem + ".moc";
    else
        dest = "moc_" + stem + ".cpp";

    //moc it
    CWFileSpec destSpec;
        moc_status mocd = do_moc(context, source, dest, &destSpec, dotmoc);

#if 0
    QCString derr = "Weird";
    switch(mocd) {
    case moc_success: derr = "Success"; break;
    case moc_parse_error: derr = "Parser Error"; break;
    case moc_no_qobject:derr = "No QOBJECT"; break;
    case moc_not_time: derr = "Not Time"; break;
    case moc_no_source: derr = "No Source"; break;
    case moc_general_error: derr = "General Error"; break;
    }
        char        dmsg[200];
        sprintf(dmsg, "\"%s\" %s", source.data(), derr.data());
        CWReportMessage(context, NULL, dmsg, NULL, messagetypeError, 0);
#endif

    //handle project
    if(mocd == moc_no_qobject) {
        char        msg[400];
                sprintf(msg, "\"%s\" No relevant classes found. No output generated.", source.data());
                CWReportMessage(context, NULL, msg, NULL, messagetypeWarning, 0);
        } else if ((mocd == moc_success || mocd == moc_not_time) && !dotmoc)
        {
                long                        whichFile;
                CWNewProjectEntryInfo ei;
                memset(&ei, '\0', sizeof(ei));
                ei.groupPath = "QtGenerated";
                    err = CWAddProjectEntry(context, &destSpec, true, &ei, &whichFile);
                    if (!CWSUCCESS(err))
                    {
                            char        msg[200];
                            sprintf(msg, "\"%s\" not added", dest.data());
                            CWReportMessage(context, NULL, msg, NULL, messagetypeWarning, 0);
                    }
                    if(mocd == moc_success)
                        CWSetModDate(context, &destSpec, NULL, true);
        }
        return cwNoErr;
}

pascal short main(CWPluginContext context)
{
        short                result;
        long                request;

        if (CWGetPluginRequest(context, &request) != cwNoErr)
                return cwErrRequestFailed;
        result = cwErrInvalidParameter;

        /* dispatch on compiler request */
        switch (request)
        {
        case reqInitCompiler:
        case reqTermCompiler:
            result = cwNoErr;
        break;

        case reqCompile:
        {
            line_count = 0;
            const char *files = NULL;
            long filelen;
            CWGetMainFileText(context, &files, &filelen);
            const char *beg = files;
            for(int x = 0; x < filelen; x++) {
                if(*(files++) == '\r') {
                    char file[1024];
                    memcpy(file, beg, files - beg);
                    file[(files-beg)-1] = '\0';
                    mocify(context, file);
                beg = files;
            }
        }
        if(beg != files) {
                char file[1024];
                memcpy(file, beg, files - beg);
                file[(files-beg)] = '\0';
                mocify(context, file);
        }

        result = cwNoErr;
                break;
        }
        }

        /* return result code */
        return result;
}

#endif

QT_END_NAMESPACE
