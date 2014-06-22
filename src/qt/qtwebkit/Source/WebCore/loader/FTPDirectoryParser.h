/*
 * Copyright (C) 2002 Cyrus Patel <cyp@fb14.uni-mainz.de>
 *           (C) 2007 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/* ParseFTPList() parses lines from an FTP LIST command.
**
** Written July 2002 by Cyrus Patel <cyp@fb14.uni-mainz.de>
** with acknowledgements to squid, lynx, wget and ftpmirror.
**
** Arguments:
**   'line':       line of FTP data connection output. The line is assumed
**                 to end at the first '\0' or '\n' or '\r\n'. 
**   'state':      a structure used internally to track state between 
**                 lines. Needs to be bzero()'d at LIST begin.
**   'result':     where ParseFTPList will store the results of the parse
**                 if 'line' is not a comment and is not junk.
**
** Returns one of the following:
**    'd' - LIST line is a directory entry ('result' is valid)
**    'f' - LIST line is a file's entry ('result' is valid)
**    'l' - LIST line is a symlink's entry ('result' is valid)
**    '?' - LIST line is junk. (cwd, non-file/dir/link, etc)
**    '"' - its not a LIST line (its a "comment")
**
** It may be advisable to let the end-user see "comments" (particularly when 
** the listing results in ONLY such lines) because such a listing may be:
** - an unknown LIST format (NLST or "custom" format for example)
** - an error msg (EPERM,ENOENT,ENFILE,EMFILE,ENOTDIR,ENOTBLK,EEXDEV etc).
** - an empty directory and the 'comment' is a "total 0" line or similar.
**   (warning: a "total 0" can also mean the total size is unknown).
**
** ParseFTPList() supports all known FTP LISTing formats:
** - '/bin/ls -l' and all variants (including Hellsoft FTP for NetWare); 
** - EPLF (Easily Parsable List Format); 
** - Windows NT's default "DOS-dirstyle";
** - OS/2 basic server format LIST format;  
** - VMS (MultiNet, UCX, and CMU) LIST format (including multi-line format);
** - IBM VM/CMS, VM/ESA LIST format (two known variants);  
** - SuperTCP FTP Server for Win16 LIST format;  
** - NetManage Chameleon (NEWT) for Win16 LIST format;  
** - '/bin/dls' (two known variants, plus multi-line) LIST format;
** If there are others, then I'd like to hear about them (send me a sample).
**
** NLSTings are not supported explicitely because they cannot be machine 
** parsed consistantly: NLSTings do not have unique characteristics - even 
** the assumption that there won't be whitespace on the line does not hold
** because some nlistings have more than one filename per line and/or
** may have filenames that have spaces in them. Moreover, distinguishing
** between an error message and an NLST line would require ParseList() to
** recognize all the possible strerror() messages in the world.
*/

// This was originally Mozilla code, titled ParseFTPList.h
// Original version of this file can currently be found at: http://mxr.mozilla.org/mozilla1.8/source/netwerk/streamconv/converters/ParseFTPList.h

#ifndef FTPDirectoryParser_h
#define FTPDirectoryParser_h

#include <wtf/text/WTFString.h>

#include <time.h>

#define SUPPORT_LSL  /* Support for /bin/ls -l and dozens of variations therof */
#define SUPPORT_DLS  /* Support for /bin/dls format (very, Very, VERY rare) */
#define SUPPORT_EPLF /* Support for Extraordinarily Pathetic List Format */
#define SUPPORT_DOS  /* Support for WinNT server in 'site dirstyle' dos */
#define SUPPORT_VMS  /* Support for VMS (all: MultiNet, UCX, CMU-IP) */
#define SUPPORT_CMS  /* Support for IBM VM/CMS,VM/ESA (z/VM and LISTING forms) */
#define SUPPORT_OS2  /* Support for IBM TCP/IP for OS/2 - FTP Server */
#define SUPPORT_W16  /* Support for win16 hosts: SuperTCP or NetManage Chameleon */

namespace WebCore {

typedef struct tm FTPTime;

struct ListState {    
    ListState()
        : now(0)
        , listStyle(0)
        , parsedOne(false)
        , carryBufferLength(0)
        , numLines(0)
    { 
        memset(&nowFTPTime, 0, sizeof(FTPTime));
    }
    
    double      now;               /* needed for year determination */
    FTPTime     nowFTPTime;
    char        listStyle;         /* LISTing style */
    bool        parsedOne;         /* returned anything yet? */
    char        carryBuffer[84];   /* for VMS multiline */
    int         carryBufferLength; /* length of name in carry_buf */
    int64_t     numLines;          /* number of lines seen */
};

enum FTPEntryType {
    FTPDirectoryEntry,
    FTPFileEntry,
    FTPLinkEntry,
    FTPMiscEntry,
    FTPJunkEntry
};

struct ListResult
{
    ListResult()
    { 
        clear();
    }
    
    void clear()
    {
        valid = false;
        type = FTPJunkEntry;
        filename = 0;
        filenameLength = 0;
        linkname = 0;
        linknameLength = 0;
        fileSize.truncate(0);
        caseSensitive = false;
        memset(&modifiedTime, 0, sizeof(FTPTime));
    }
    
    bool valid;
    FTPEntryType type;        
    
    const char* filename;
    uint32_t filenameLength;
    
    const char* linkname;
    uint32_t linknameLength;
    
    String fileSize;      
    FTPTime modifiedTime; 
    bool caseSensitive; // file system is definitely case insensitive
};

FTPEntryType parseOneFTPLine(const char* inputLine, ListState&, ListResult&);
                 
} // namespace WebCore

#endif // FTPDirectoryParser_h
