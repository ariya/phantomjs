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

// This was originally Mozilla code, titled ParseFTPList.cpp
// Original version of this file can currently be found at: http://mxr.mozilla.org/mozilla1.8/source/netwerk/streamconv/converters/ParseFTPList.cpp

#include "config.h"
#if ENABLE(FTPDIR)
#include "FTPDirectoryParser.h"

#if PLATFORM(QT)
#include <QDateTime>
// On Windows, use the threadsafe *_r functions provided by pthread.
#elif OS(WINDOWS) && (USE(PTHREADS) || HAVE(PTHREAD_H))
#include <pthread.h>
#endif

#include <wtf/ASCIICType.h>
#include <stdio.h>

using namespace WTF;

namespace WebCore {
#if PLATFORM(QT) && defined(Q_WS_WIN32)

// Replacement for gmtime_r() which is not available on MinGW.
// We use this on Win32 Qt platform for portability.
struct tm gmtimeQt(const QDateTime& input)
{
    tm result;

    QDate date(input.date());
    result.tm_year = date.year() - 1900;
    result.tm_mon = date.month();
    result.tm_mday = date.day();
    result.tm_wday = date.dayOfWeek();
    result.tm_yday = date.dayOfYear();

    QTime time(input.time());
    result.tm_sec = time.second();
    result.tm_min = time.minute();
    result.tm_hour = time.hour();

    return result;
}

static struct tm *gmtimeQt(const time_t *const timep, struct tm *result)
{
    const QDateTime dt(QDateTime::fromTime_t(*timep));
    *result = WebCore::gmtimeQt(dt);
    return result;
}

#define gmtime_r(x, y) gmtimeQt(x, y)
#elif OS(WINDOWS) && !defined(gmtime_r)
#if defined(_MSC_VER) && (_MSC_VER >= 1400) 
#define gmtime_r(x, y) gmtime_s((y), (x))
#else /* !_MSC_VER */ 
#define gmtime_r(x,y) (gmtime(x)?(*(y)=*gmtime(x),(y)):0)
#endif
#endif

static inline FTPEntryType ParsingFailed(ListState& state)
{
  if (state.parsedOne || state.listStyle) /* junk if we fail to parse */
    return FTPJunkEntry;      /* this time but had previously parsed sucessfully */
  return FTPMiscEntry;        /* its part of a comment or error message */
}

FTPEntryType parseOneFTPLine(const char* line, ListState& state, ListResult& result)
{
  result.clear();
    
  if (!line)
    return FTPJunkEntry;

  state.numLines++;

  /* carry buffer is only valid from one line to the next */
  unsigned int carry_buf_len = state.carryBufferLength;
  state.carryBufferLength = 0;

  unsigned linelen = 0;

  /* strip leading whitespace */
  while (*line == ' ' || *line == '\t')
    line++;
  
  /* line is terminated at first '\0' or '\n' */
  const char* p = line;
  while (*p && *p != '\n')
    p++;
  linelen = p - line;

  if (linelen > 0 && *p == '\n' && *(p-1) == '\r')
    linelen--;

  /* DON'T strip trailing whitespace. */

  if (linelen > 0)
  {
    static const char *month_names = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *tokens[16]; /* 16 is more than enough */
    unsigned int toklen[WTF_ARRAY_LENGTH(tokens)];
    unsigned int linelen_sans_wsp;  // line length sans whitespace
    unsigned int numtoks = 0;
    unsigned int tokmarker = 0; /* extra info for lstyle handler */
    unsigned int month_num = 0;
    char tbuf[4];
    int lstyle = 0;

    if (carry_buf_len) /* VMS long filename carryover buffer */
    {
      tokens[0] = state.carryBuffer;
      toklen[0] = carry_buf_len;
      numtoks++;
    }

    unsigned int pos = 0;
    while (pos < linelen && numtoks < WTF_ARRAY_LENGTH(tokens))
    {
      while (pos < linelen && 
            (line[pos] == ' ' || line[pos] == '\t' || line[pos] == '\r'))
        pos++;
      if (pos < linelen)
      {
        tokens[numtoks] = &line[pos];
        while (pos < linelen && 
           (line[pos] != ' ' && line[pos] != '\t' && line[pos] != '\r'))
          pos++;
        if (tokens[numtoks] != &line[pos])
        {
          toklen[numtoks] = (&line[pos] - tokens[numtoks]);
          numtoks++;  
        }
      }
    }    

    if (!numtoks)
      return ParsingFailed(state);

    linelen_sans_wsp = &(tokens[numtoks-1][toklen[numtoks-1]]) - tokens[0];
    if (numtoks == WTF_ARRAY_LENGTH(tokens))
    {
      pos = linelen;
      while (pos > 0 && (line[pos-1] == ' ' || line[pos-1] == '\t'))
        pos--;
      linelen_sans_wsp = pos;
    }

    /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#if defined(SUPPORT_EPLF)
    /* EPLF handling must come somewhere before /bin/dls handling. */
    if (!lstyle && (!state.listStyle || state.listStyle == 'E'))
    {
      if (*line == '+' && linelen > 4 && numtoks >= 2)
      {
        pos = 1;
        while (pos < (linelen-1))
        {
          p = &line[pos++];
          if (*p == '/') 
            result.type = FTPDirectoryEntry; /* its a dir */
          else if (*p == 'r')
            result.type = FTPFileEntry; /* its a file */
          else if (*p == 'm')
          {
            if (isASCIIDigit(line[pos]))
            {
              while (pos < linelen && isASCIIDigit(line[pos]))
                pos++;
              if (pos < linelen && line[pos] == ',')
              {
                unsigned long long seconds = 0;
#if OS(WINDOWS)
                sscanf(p + 1, "%I64u", &seconds);
#else
                sscanf(p + 1, "%llu", &seconds);
#endif
                time_t t = static_cast<time_t>(seconds);

                // FIXME: This code has the year 2038 bug
                gmtime_r(&t, &result.modifiedTime);
                result.modifiedTime.tm_year += 1900;
              }
            }
          }
          else if (*p == 's')
          {
            if (isASCIIDigit(line[pos]))
            {
              while (pos < linelen && isASCIIDigit(line[pos]))
                pos++;
              if (pos < linelen && line[pos] == ',')
                result.fileSize = String(p + 1, &line[pos] - p + 1);
            }
          }
          else if (isASCIIAlpha(*p)) /* 'i'/'up' or unknown "fact" (property) */
          {
            while (pos < linelen && *++p != ',')
              pos++;
          }
          else if (*p != '\t' || (p+1) != tokens[1])
          {
            break; /* its not EPLF after all */
          }
          else
          {
            state.parsedOne = true;
            state.listStyle = lstyle = 'E';

            p = &(line[linelen_sans_wsp]);
            result.filename = tokens[1];
            result.filenameLength = p - tokens[1];

            if (!result.type) /* access denied */
            {
              result.type = FTPFileEntry; /* is assuming 'f'ile correct? */
              return FTPJunkEntry;            /* NO! junk it. */
            }
            return result.type;
          }
          if (pos >= (linelen-1) || line[pos] != ',')
            break;
          pos++;
        } /* while (pos < linelen) */
        result.clear();
      } /* if (*line == '+' && linelen > 4 && numtoks >= 2) */
    } /* if (!lstyle && (!state.listStyle || state.listStyle == 'E')) */
#endif /* SUPPORT_EPLF */

    /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#if defined(SUPPORT_VMS)
    if (!lstyle && (!state.listStyle || state.listStyle == 'V'))
    {                          /* try VMS Multinet/UCX/CMS server */
      /*
       * Legal characters in a VMS file/dir spec are [A-Z0-9$.-_~].
       * '$' cannot begin a filename and `-' cannot be used as the first 
       * or last character. '.' is only valid as a directory separator 
       * and <file>.<type> separator. A canonical filename spec might look 
       * like this: DISK$VOL:[DIR1.DIR2.DIR3]FILE.TYPE;123
       * All VMS FTP servers LIST in uppercase.
       *
       * We need to be picky about this in order to support
       * multi-line listings correctly.
      */
      if (!state.parsedOne &&
          (numtoks == 1 || (numtoks == 2 && toklen[0] == 9 &&
                            memcmp(tokens[0], "Directory", 9)==0 )))
      {
        /* If no dirstyle has been detected yet, and this line is a 
         * VMS list's dirname, then turn on VMS dirstyle.
         * eg "ACA:[ANONYMOUS]", "DISK$FTP:[ANONYMOUS]", "SYS$ANONFTP:" 
        */
        p = tokens[0];
        pos = toklen[0];
        if (numtoks == 2)
        {
          p = tokens[1];
          pos = toklen[1];
        }
        pos--;
        if (pos >= 3)
        {
          while (pos > 0 && p[pos] != '[')
          {
            pos--;
            if (p[pos] == '-' || p[pos] == '$')
            {
              if (pos == 0 || p[pos-1] == '[' || p[pos-1] == '.' ||
                  (p[pos] == '-' && (p[pos+1] == ']' || p[pos+1] == '.')))
                break;
            }
            else if (p[pos] != '.' && p[pos] != '~' && 
                     !isASCIIDigit(p[pos]) && !isASCIIAlpha(p[pos]))
              break;
            else if (isASCIIAlpha(p[pos]) && p[pos] != toASCIIUpper(p[pos]))
              break;
          }
          if (pos > 0)
          {
            pos--;
            if (p[pos] != ':' || p[pos+1] != '[')
              pos = 0;
          }
        }
        if (pos > 0 && p[pos] == ':')
        {
          while (pos > 0)
          {
            pos--;
            if (p[pos] != '$' && p[pos] != '_' && p[pos] != '-' &&
                p[pos] != '~' && !isASCIIDigit(p[pos]) && !isASCIIAlpha(p[pos]))
              break;
            else if (isASCIIAlpha(p[pos]) && p[pos] != toASCIIUpper(p[pos]))
              break;
          }
          if (pos == 0)
          {  
            state.listStyle = 'V';
            return FTPJunkEntry; /* its junk */
          }
        }
        /* fallthrough */ 
      }
      else if ((tokens[0][toklen[0]-1]) != ';')
      {
        if (numtoks == 1 && (state.listStyle == 'V' && !carry_buf_len))
          lstyle = 'V';
        else if (numtoks < 4)
          ;
        else if (toklen[1] >= 10 && memcmp(tokens[1], "%RMS-E-PRV", 10) == 0)
          lstyle = 'V';
        else if ((&line[linelen] - tokens[1]) >= 22 &&
                  memcmp(tokens[1], "insufficient privilege", 22) == 0)
          lstyle = 'V';
        else if (numtoks != 4 && numtoks != 6)
          ;
        else if (numtoks == 6 && (
                 toklen[5] < 4 || *tokens[5] != '(' ||        /* perms */
                           (tokens[5][toklen[5]-1]) != ')'  ))
          ;
        else if (  (toklen[2] == 10 || toklen[2] == 11) &&      
                        (tokens[2][toklen[2]-5]) == '-' &&
                        (tokens[2][toklen[2]-9]) == '-' &&
        (((toklen[3]==4 || toklen[3]==5 || toklen[3]==7 || toklen[3]==8) &&
                        (tokens[3][toklen[3]-3]) == ':' ) ||
         ((toklen[3]==10 || toklen[3]==11 ) &&
                        (tokens[3][toklen[3]-3]) == '.' )
        ) &&  /* time in [H]H:MM[:SS[.CC]] format */
                                    isASCIIDigit(*tokens[1]) && /* size */
                                    isASCIIDigit(*tokens[2]) && /* date */
                                    isASCIIDigit(*tokens[3])    /* time */
                )
        {
          lstyle = 'V';
        }
        if (lstyle == 'V')
        {
          /* 
          * MultiNet FTP:
          *   LOGIN.COM;2                 1   4-NOV-1994 04:09 [ANONYMOUS] (RWE,RWE,,)
          *   PUB.DIR;1                   1  27-JAN-1994 14:46 [ANONYMOUS] (RWE,RWE,RE,RWE)
          *   README.FTP;1        %RMS-E-PRV, insufficient privilege or file protection violation
          *   ROUSSOS.DIR;1               1  27-JAN-1994 14:48 [CS,ROUSSOS] (RWE,RWE,RE,R)
          *   S67-50903.JPG;1           328  22-SEP-1998 16:19 [ANONYMOUS] (RWED,RWED,,)
          * UCX FTP: 
          *   CII-MANUAL.TEX;1  213/216  29-JAN-1996 03:33:12  [ANONYMOU,ANONYMOUS] (RWED,RWED,,)
          * CMU/VMS-IP FTP
          *   [VMSSERV.FILES]ALARM.DIR;1 1/3 5-MAR-1993 18:09
          * TCPware FTP
          *   FOO.BAR;1 4 5-MAR-1993 18:09:01.12
          * Long filename example:
          *   THIS-IS-A-LONG-VMS-FILENAME.AND-THIS-IS-A-LONG-VMS-FILETYPE\r\n
          *                    213[/nnn]  29-JAN-1996 03:33[:nn]  [ANONYMOU,ANONYMOUS] (RWED,RWED,,)
          */
          tokmarker = 0;
          p = tokens[0];
          pos = 0;
          if (*p == '[' && toklen[0] >= 4) /* CMU style */
          {
            if (p[1] != ']') 
            {
              p++;
              pos++;
            }
            while (lstyle && pos < toklen[0] && *p != ']')
            {
              if (*p != '$' && *p != '.' && *p != '_' && *p != '-' &&
                  *p != '~' && !isASCIIDigit(*p) && !isASCIIAlpha(*p))              
                lstyle = 0;
              pos++;
              p++;
            }
            if (lstyle && pos < (toklen[0]-1))
            {
              /* ']' was found and there is at least one character after it */
              ASSERT(*p == ']');
              pos++;
              p++;
              tokmarker = pos; /* length of leading "[DIR1.DIR2.etc]" */
            } else {
              /* not a CMU style listing */
              lstyle = 0;
            }
          }
          while (lstyle && pos < toklen[0] && *p != ';')
          {
            if (*p != '$' && *p != '.' && *p != '_' && *p != '-' &&
                *p != '~' && !isASCIIDigit(*p) && !isASCIIAlpha(*p))
              lstyle = 0;
            else if (isASCIIAlpha(*p) && *p != toASCIIUpper(*p))
              lstyle = 0;
            p++;
            pos++;
          }
          if (lstyle && *p == ';')
          {
            if (pos == 0 || pos == (toklen[0]-1))
              lstyle = 0;
            for (pos++;lstyle && pos < toklen[0];pos++)
            {
              if (!isASCIIDigit(tokens[0][pos]))
                lstyle = 0;
            }
          }
          pos = (p - tokens[0]); /* => fnlength sans ";####" */
          pos -= tokmarker;      /* => fnlength sans "[DIR1.DIR2.etc]" */
          p = &(tokens[0][tokmarker]); /* offset of basename */

          if (!lstyle || pos == 0 || pos > 80) /* VMS filenames can't be longer than that */
          {
            lstyle = 0;
          }
          else if (numtoks == 1)
          { 
            /* if VMS has been detected and there is only one token and that 
             * token was a VMS filename then this is a multiline VMS LIST entry.
            */
            if (pos >= (sizeof(state.carryBuffer)-1))
              pos = (sizeof(state.carryBuffer)-1); /* shouldn't happen */
            memcpy( state.carryBuffer, p, pos );
            state.carryBufferLength = pos;
            return FTPJunkEntry; /* tell caller to treat as junk */
          }
          else if (isASCIIDigit(*tokens[1])) /* not no-privs message */
          {
            for (pos = 0; lstyle && pos < (toklen[1]); pos++)
            {
              if (!isASCIIDigit((tokens[1][pos])) && (tokens[1][pos]) != '/')
                lstyle = 0;
            }
            if (lstyle && numtoks > 4) /* Multinet or UCX but not CMU */
            {
              for (pos = 1; lstyle && pos < (toklen[5]-1); pos++)
              {
                p = &(tokens[5][pos]);
                if (*p!='R' && *p!='W' && *p!='E' && *p!='D' && *p!=',')
                  lstyle = 0;
              }
            }
          }
        } /* passed initial tests */
      } /* else if ((tokens[0][toklen[0]-1]) != ';') */    

      if (lstyle == 'V')
      {
        state.parsedOne = true;
        state.listStyle = lstyle;

        if (isASCIIDigit(*tokens[1]))  /* not permission denied etc */
        {
          /* strip leading directory name */
          if (*tokens[0] == '[') /* CMU server */
          {
            pos = toklen[0]-1;
            p = tokens[0]+1;
            while (*p != ']')
            {
              p++;
              pos--;
            }
            toklen[0] = --pos;
            tokens[0] = ++p;
          }
          pos = 0;
          while (pos < toklen[0] && (tokens[0][pos]) != ';')
            pos++;
       
          result.caseSensitive = true;
          result.type = FTPFileEntry;
          result.filename = tokens[0];
          result.filenameLength = pos;

          if (pos > 4)
          {
            p = &(tokens[0][pos-4]);
            if (p[0] == '.' && p[1] == 'D' && p[2] == 'I' && p[3] == 'R')
            {
              result.filenameLength -= 4;
              result.type = FTPDirectoryEntry;
            }
          }

          if (result.type != FTPDirectoryEntry)
          {
            /* #### or used/allocated form. If used/allocated form, then
             * 'used' is the size in bytes if and only if 'used'<=allocated.
             * If 'used' is size in bytes then it can be > 2^32
             * If 'used' is not size in bytes then it is size in blocks.
            */
            pos = 0;
            while (pos < toklen[1] && (tokens[1][pos]) != '/')
              pos++;
            
/*
 * I've never seen size come back in bytes, its always in blocks, and 
 * the following test fails. So, always perform the "size in blocks".
 * I'm leaving the "size in bytes" code if'd out in case we ever need
 * to re-instate it.
*/
#if 0
            if (pos < toklen[1] && ( (pos<<1) > (toklen[1]-1) ||
                 (strtoul(tokens[1], (char **)0, 10) > 
                  strtoul(tokens[1]+pos+1, (char **)0, 10))        ))
            {                                   /* size is in bytes */
              if (pos > (sizeof(result.fe_size)-1))
                pos = sizeof(result.fe_size)-1;
              memcpy( result.fe_size, tokens[1], pos );
              result.fe_size[pos] = '\0';
            }
            else /* size is in blocks */
#endif
            {
              /* size requires multiplication by blocksize. 
               *
               * We could assume blocksize is 512 (like Lynx does) and
               * shift by 9, but that might not be right. Even if it 
               * were, doing that wouldn't reflect what the file's 
               * real size was. The sanest thing to do is not use the
               * LISTing's filesize, so we won't (like ftpmirror).
               *
               * ulltoa(((unsigned long long)fsz)<<9, result.fe_size, 10);
               *
               * A block is always 512 bytes on OpenVMS, compute size.
               * So its rounded up to the next block, so what, its better
               * than not showing the size at all.
               * A block is always 512 bytes on OpenVMS, compute size.
               * So its rounded up to the next block, so what, its better
               * than not showing the size at all.
              */
              uint64_t size = strtoul(tokens[1], NULL, 10) * 512;
              result.fileSize = String::number(size);
            } 

          } /* if (result.type != FTPDirectoryEntry) */

          p = tokens[2] + 2;
          if (*p == '-')
            p++;
          tbuf[0] = p[0];
          tbuf[1] = toASCIILower(p[1]);
          tbuf[2] = toASCIILower(p[2]);
          month_num = 0;
          for (pos = 0; pos < (12*3); pos+=3)
          {
            if (tbuf[0] == month_names[pos+0] && 
                tbuf[1] == month_names[pos+1] && 
                tbuf[2] == month_names[pos+2])
              break;
            month_num++;
          }
          if (month_num >= 12)
            month_num = 0;
          result.modifiedTime.tm_mon = month_num;
          result.modifiedTime.tm_mday = atoi(tokens[2]);
          result.modifiedTime.tm_year = atoi(p+4); // NSPR wants year as XXXX

          p = tokens[3] + 2;
          if (*p == ':')
            p++;
          if (p[2] == ':')
            result.modifiedTime.tm_sec = atoi(p+3);
          result.modifiedTime.tm_hour = atoi(tokens[3]);
          result.modifiedTime.tm_min  = atoi(p);
      
          return result.type;

        } /* if (isASCIIDigit(*tokens[1])) */

        return FTPJunkEntry; /* junk */

      } /* if (lstyle == 'V') */
    } /* if (!lstyle && (!state.listStyle || state.listStyle == 'V')) */
#endif

    /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#if defined(SUPPORT_CMS)
    /* Virtual Machine/Conversational Monitor System (IBM Mainframe) */
    if (!lstyle && (!state.listStyle || state.listStyle == 'C'))  /* VM/CMS */
    {
      /* LISTing according to mirror.pl
       * Filename FileType  Fm Format Lrecl  Records Blocks Date      Time
       * LASTING  GLOBALV   A1 V      41     21     1       9/16/91   15:10:32
       * J43401   NETLOG    A0 V      77     1      1       9/12/91   12:36:04
       * PROFILE  EXEC      A1 V      17     3      1       9/12/91   12:39:07
       * DIRUNIX  SCRIPT    A1 V      77     1216   17      1/04/93   20:30:47
       * MAIL     PROFILE   A2 F      80     1      1       10/14/92  16:12:27
       * BADY2K   TEXT      A0 V      1      1      1       1/03/102  10:11:12
       * AUTHORS            A1 DIR    -      -      -       9/20/99   10:31:11
       *
       * LISTing from vm.marist.edu and vm.sc.edu
       * 220-FTPSERVE IBM VM Level 420 at VM.MARIST.EDU, 04:58:12 EDT WEDNESDAY 2002-07-10
       * AUTHORS           DIR        -          -          - 1999-09-20 10:31:11 -
       * HARRINGTON        DIR        -          -          - 1997-02-12 15:33:28 -
       * PICS              DIR        -          -          - 2000-10-12 15:43:23 -
       * SYSFILE           DIR        -          -          - 2000-07-20 17:48:01 -
       * WELCNVT  EXEC     V         72          9          1 1999-09-20 17:16:18 -
       * WELCOME  EREADME  F         80         21          1 1999-12-27 16:19:00 -
       * WELCOME  README   V         82         21          1 1999-12-27 16:19:04 -
       * README   ANONYMOU V         71         26          1 1997-04-02 12:33:20 TCP291
       * README   ANONYOLD V         71         15          1 1995-08-25 16:04:27 TCP291
      */
      if (numtoks >= 7 && (toklen[0]+toklen[1]) <= 16)
      {
        for (pos = 1; !lstyle && (pos+5) < numtoks; pos++)
        {
          p = tokens[pos];
          if ((toklen[pos] == 1 && (*p == 'F' || *p == 'V')) ||
              (toklen[pos] == 3 && *p == 'D' && p[1] == 'I' && p[2] == 'R'))
          {
            if (toklen[pos+5] == 8 && (tokens[pos+5][2]) == ':' &&
                                      (tokens[pos+5][5]) == ':'   )
            {
              p = tokens[pos+4];
              if ((toklen[pos+4] == 10 && p[4] == '-' && p[7] == '-') ||
                  (toklen[pos+4] >= 7 && toklen[pos+4] <= 9 && 
                            p[((p[1]!='/')?(2):(1))] == '/' && 
                            p[((p[1]!='/')?(5):(4))] == '/'))
               /* Y2K bugs possible ("7/06/102" or "13/02/101") */
              {
                if ( (*tokens[pos+1] == '-' &&
                      *tokens[pos+2] == '-' &&
                      *tokens[pos+3] == '-')  ||
                      (isASCIIDigit(*tokens[pos+1]) &&
                       isASCIIDigit(*tokens[pos+2]) &&
                       isASCIIDigit(*tokens[pos+3])) )
                {
                  lstyle = 'C';
                  tokmarker = pos;
                }
              }
            }
          }
        } /* for (pos = 1; !lstyle && (pos+5) < numtoks; pos++) */
      } /* if (numtoks >= 7) */

      /* extra checking if first pass */
      if (lstyle && !state.listStyle) 
      {
        for (pos = 0, p = tokens[0]; lstyle && pos < toklen[0]; pos++, p++)
        {  
          if (isASCIIAlpha(*p) && toASCIIUpper(*p) != *p)
            lstyle = 0;
        } 
        for (pos = tokmarker+1; pos <= tokmarker+3; pos++)
        {
          if (!(toklen[pos] == 1 && *tokens[pos] == '-'))
          {
            for (p = tokens[pos]; lstyle && p<(tokens[pos]+toklen[pos]); p++)
            {
              if (!isASCIIDigit(*p))
                lstyle = 0;
            }
          }
        }
        for (pos = 0, p = tokens[tokmarker+4]; 
             lstyle && pos < toklen[tokmarker+4]; pos++, p++)
        {
          if (*p == '/')
          { 
            /* There may be Y2K bugs in the date. Don't simplify to
             * pos != (len-3) && pos != (len-6) like time is done.
            */             
            if ((tokens[tokmarker+4][1]) == '/')
            {
              if (pos != 1 && pos != 4)
                lstyle = 0;
            }
            else if (pos != 2 && pos != 5)
              lstyle = 0;
          }
          else if (*p != '-' && !isASCIIDigit(*p))
            lstyle = 0;
          else if (*p == '-' && pos != 4 && pos != 7)
            lstyle = 0;
        }
        for (pos = 0, p = tokens[tokmarker+5]; 
             lstyle && pos < toklen[tokmarker+5]; pos++, p++)
        {
          if (*p != ':' && !isASCIIDigit(*p))
            lstyle = 0;
          else if (*p == ':' && pos != (toklen[tokmarker+5]-3)
                             && pos != (toklen[tokmarker+5]-6))
            lstyle = 0;
        }
      } /* initial if() */

      if (lstyle == 'C')
      {
        state.parsedOne = true;
        state.listStyle = lstyle;

        p = tokens[tokmarker+4];
        if (toklen[tokmarker+4] == 10) /* newstyle: YYYY-MM-DD format */
        {
          result.modifiedTime.tm_year = atoi(p+0) - 1900;
          result.modifiedTime.tm_mon  = atoi(p+5) - 1;
          result.modifiedTime.tm_mday = atoi(p+8);
        }
        else /* oldstyle: [M]M/DD/YY format */
        {
          pos = toklen[tokmarker+4];
          result.modifiedTime.tm_mon  = atoi(p) - 1;
          result.modifiedTime.tm_mday = atoi((p+pos)-5);
          result.modifiedTime.tm_year = atoi((p+pos)-2);
          if (result.modifiedTime.tm_year < 70)
            result.modifiedTime.tm_year += 100;
        }

        p = tokens[tokmarker+5];
        pos = toklen[tokmarker+5];
        result.modifiedTime.tm_hour  = atoi(p);
        result.modifiedTime.tm_min = atoi((p+pos)-5);
        result.modifiedTime.tm_sec = atoi((p+pos)-2);

        result.caseSensitive = true;
        result.filename = tokens[0];
        result.filenameLength = toklen[0];
        result.type  = FTPFileEntry;

        p = tokens[tokmarker];
        if (toklen[tokmarker] == 3 && *p=='D' && p[1]=='I' && p[2]=='R')
          result.type  = FTPDirectoryEntry;

        if ((/*newstyle*/ toklen[tokmarker+4] == 10 && tokmarker > 1) ||
            (/*oldstyle*/ toklen[tokmarker+4] != 10 && tokmarker > 2))
        {                            /* have a filetype column */
          char *dot;
          p = &(tokens[0][toklen[0]]);
          memcpy( &dot, &p, sizeof(dot) ); /* NASTY! */
          *dot++ = '.';
          p = tokens[1];
          for (pos = 0; pos < toklen[1]; pos++)
            *dot++ = *p++;
          result.filenameLength += 1 + toklen[1];
        }

        /* oldstyle LISTING: 
         * files/dirs not on the 'A' minidisk are not RETRievable/CHDIRable 
        if (toklen[tokmarker+4] != 10 && *tokens[tokmarker-1] != 'A')
          return FTPJunkEntry;
        */
        
        /* VM/CMS LISTings have no usable filesize field. 
         * Have to use the 'SIZE' command for that.
        */
        return result.type;

      } /* if (lstyle == 'C' && (!state.listStyle || state.listStyle == lstyle)) */
    } /* VM/CMS */
#endif

    /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#if defined(SUPPORT_DOS) /* WinNT DOS dirstyle */
    if (!lstyle && (!state.listStyle || state.listStyle == 'W'))
    {
      /*
       * "10-23-00  01:27PM       <DIR>          veronist"
       * "06-15-00  07:37AM       <DIR>          zoe"
       * "07-14-00  01:35PM              2094926 canprankdesk.tif"
       * "07-21-00  01:19PM                95077 Jon Kauffman Enjoys the Good Life.jpg"
       * "07-21-00  01:19PM                52275 Name Plate.jpg"
       * "07-14-00  01:38PM              2250540 Valentineoffprank-HiRes.jpg"
      */
      if ((numtoks >= 4) && toklen[0] == 8 && toklen[1] == 7 && 
          (*tokens[2] == '<' || isASCIIDigit(*tokens[2])) )
      {
        p = tokens[0];
        if ( isASCIIDigit(p[0]) && isASCIIDigit(p[1]) && p[2]=='-' && 
             isASCIIDigit(p[3]) && isASCIIDigit(p[4]) && p[5]=='-' &&
             isASCIIDigit(p[6]) && isASCIIDigit(p[7]) )
        {
          p = tokens[1];
          if ( isASCIIDigit(p[0]) && isASCIIDigit(p[1]) && p[2]==':' && 
               isASCIIDigit(p[3]) && isASCIIDigit(p[4]) && 
               (p[5]=='A' || p[5]=='P') && p[6]=='M')
          {
            lstyle = 'W';
            if (!state.listStyle)
            {            
              p = tokens[2];
              /* <DIR> or <JUNCTION> */
              if (*p != '<' || p[toklen[2]-1] != '>')
              {
                for (pos = 1; (lstyle && pos < toklen[2]); pos++)
                {
                  if (!isASCIIDigit(*++p))
                    lstyle = 0;
                }
              }
            }
          }
        }
      }

      if (lstyle == 'W')
      {
        state.parsedOne = true;
        state.listStyle = lstyle;

        p = &(line[linelen]); /* line end */
        result.caseSensitive = true;
        result.filename = tokens[3];
        result.filenameLength = p - tokens[3];
        result.type = FTPDirectoryEntry;

        if (*tokens[2] != '<') /* not <DIR> or <JUNCTION> */
        {
          // try to handle correctly spaces at the beginning of the filename
          // filesize (token[2]) must end at offset 38
          if (tokens[2] + toklen[2] - line == 38) {
            result.filename = &(line[39]);
            result.filenameLength = p - result.filename;
          }
          result.type = FTPFileEntry;
          pos = toklen[2];
          result.fileSize = String(tokens[2], pos);
        }
        else {
          // try to handle correctly spaces at the beginning of the filename
          // token[2] must begin at offset 24, the length is 5 or 10
          // token[3] must begin at offset 39 or higher
          if (tokens[2] - line == 24 && (toklen[2] == 5 || toklen[2] == 10) &&
              tokens[3] - line >= 39) {
            result.filename = &(line[39]);
            result.filenameLength = p - result.filename;
          }

          if ((tokens[2][1]) != 'D') /* not <DIR> */
          {
            result.type = FTPJunkEntry; /* unknown until junc for sure */
            if (result.filenameLength > 4)
            {
              p = result.filename;
              for (pos = result.filenameLength - 4; pos > 0; pos--)
              {
                if (p[0] == ' ' && p[3] == ' ' && p[2] == '>' &&
                    (p[1] == '=' || p[1] == '-'))
                {
                  result.type = FTPLinkEntry;
                  result.filenameLength = p - result.filename;
                  result.linkname = p + 4;
                  result.linknameLength = &(line[linelen]) 
                                     - result.linkname;
                  break;
                }
                p++;
              }
            }    
          }
        }
      
        result.modifiedTime.tm_mon = atoi(tokens[0]+0);
        if (result.modifiedTime.tm_mon != 0)
        {
          result.modifiedTime.tm_mon--;
          result.modifiedTime.tm_mday = atoi(tokens[0]+3);
          result.modifiedTime.tm_year = atoi(tokens[0]+6);
          /* if year has only two digits then assume that
               00-79 is 2000-2079
               80-99 is 1980-1999 */
          if (result.modifiedTime.tm_year < 80)
            result.modifiedTime.tm_year += 2000;
          else if (result.modifiedTime.tm_year < 100)
            result.modifiedTime.tm_year += 1900;
        }

        result.modifiedTime.tm_hour = atoi(tokens[1]+0);
        result.modifiedTime.tm_min = atoi(tokens[1]+3);
        if ((tokens[1][5]) == 'P' && result.modifiedTime.tm_hour < 12)
          result.modifiedTime.tm_hour += 12;

        /* the caller should do this (if dropping "." and ".." is desired)
        if (result.type == FTPDirectoryEntry && result.filename[0] == '.' &&
            (result.filenameLength == 1 || (result.filenameLength == 2 &&
                                      result.filename[1] == '.')))
          return FTPJunkEntry;
        */

        return result.type;  
      } /* if (lstyle == 'W' && (!state.listStyle || state.listStyle == lstyle)) */
    } /* if (!lstyle && (!state.listStyle || state.listStyle == 'W')) */
#endif

    /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#if defined(SUPPORT_OS2)
    if (!lstyle && (!state.listStyle || state.listStyle == 'O')) /* OS/2 test */
    {
      /* 220 server IBM TCP/IP for OS/2 - FTP Server ver 23:04:36 on Jan 15 1997 ready.
      * fixed position, space padded columns. I have only a vague idea 
      * of what the contents between col 18 and 34 might be: All I can infer
      * is that there may be attribute flags in there and there may be 
      * a " DIR" in there.
      *
      *          1         2         3         4         5         6
      *0123456789012345678901234567890123456789012345678901234567890123456789
      *----- size -------|??????????????? MM-DD-YY|  HH:MM| nnnnnnnnn....
      *                 0  DIR            04-11-95   16:26  .
      *                 0  DIR            04-11-95   16:26  ..
      *                 0  DIR            04-11-95   16:26  ADDRESS
      *               612  RHSA           07-28-95   16:45  air_tra1.bag
      *               195  A              08-09-95   10:23  Alfa1.bag
      *                 0  RHS   DIR      04-11-95   16:26  ATTACH
      *               372  A              08-09-95   10:26  Aussie_1.bag
      *            310992                 06-28-94   09:56  INSTALL.EXE
      *                            1         2         3         4
      *                  01234567890123456789012345678901234567890123456789
      * dirlist from the mirror.pl project, col positions from Mozilla.
      */
      p = &(line[toklen[0]]);
      /* \s(\d\d-\d\d-\d\d)\s+(\d\d:\d\d)\s */
      if (numtoks >= 4 && toklen[0] <= 18 && isASCIIDigit(*tokens[0]) &&
         (linelen - toklen[0]) >= (53-18)                        &&
         p[18-18] == ' ' && p[34-18] == ' '                      &&
         p[37-18] == '-' && p[40-18] == '-' && p[43-18] == ' '   &&
         p[45-18] == ' ' && p[48-18] == ':' && p[51-18] == ' '   &&
         isASCIIDigit(p[35-18]) && isASCIIDigit(p[36-18])        &&
         isASCIIDigit(p[38-18]) && isASCIIDigit(p[39-18])        &&
         isASCIIDigit(p[41-18]) && isASCIIDigit(p[42-18])        &&
         isASCIIDigit(p[46-18]) && isASCIIDigit(p[47-18])        &&
         isASCIIDigit(p[49-18]) && isASCIIDigit(p[50-18])
      )
      {
        lstyle = 'O'; /* OS/2 */
        if (!state.listStyle)
        {            
          for (pos = 1; lstyle && pos < toklen[0]; pos++)
          {
            if (!isASCIIDigit(tokens[0][pos]))
              lstyle = 0;
          }
        }
      }

      if (lstyle == 'O')
      {
        state.parsedOne = true;
        state.listStyle = lstyle;

        p = &(line[toklen[0]]);

        result.caseSensitive = true;
        result.filename = &p[53-18];
        result.filenameLength = (&(line[linelen_sans_wsp]))
                           - (result.filename);
        result.type = FTPFileEntry;

        /* I don't have a real listing to determine exact pos, so scan. */
        for (pos = (18-18); pos < ((35-18)-4); pos++)
        {
          if (p[pos+0] == ' ' && p[pos+1] == 'D' && 
              p[pos+2] == 'I' && p[pos+3] == 'R')
          {
            result.type = FTPDirectoryEntry;
            break;
          }
        }
    
        if (result.type != FTPDirectoryEntry)
        {
          pos = toklen[0];
          result.fileSize = String(tokens[0], pos);
        }  
    
        result.modifiedTime.tm_mon = atoi(&p[35-18]) - 1;
        result.modifiedTime.tm_mday = atoi(&p[38-18]);
        result.modifiedTime.tm_year = atoi(&p[41-18]);
        if (result.modifiedTime.tm_year < 80)
          result.modifiedTime.tm_year += 100;
        result.modifiedTime.tm_hour = atoi(&p[46-18]);
        result.modifiedTime.tm_min = atoi(&p[49-18]);
   
        /* the caller should do this (if dropping "." and ".." is desired)
        if (result.type == FTPDirectoryEntry && result.filename[0] == '.' &&
            (result.filenameLength == 1 || (result.filenameLength == 2 &&
                                      result.filename[1] == '.')))
          return FTPJunkEntry;
        */

        return result.type;
      } /* if (lstyle == 'O') */

    } /* if (!lstyle && (!state.listStyle || state.listStyle == 'O')) */
#endif

    /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
    
#if defined(SUPPORT_LSL)
    if (!lstyle && (!state.listStyle || state.listStyle == 'U')) /* /bin/ls & co. */
    {
      /* UNIX-style listing, without inum and without blocks
       * "-rw-r--r--   1 root     other        531 Jan 29 03:26 README"
       * "dr-xr-xr-x   2 root     other        512 Apr  8  1994 etc"
       * "dr-xr-xr-x   2 root     512 Apr  8  1994 etc"
       * "lrwxrwxrwx   1 root     other          7 Jan 25 00:17 bin -> usr/bin"
       * Also produced by Microsoft's FTP servers for Windows:
       * "----------   1 owner    group         1803128 Jul 10 10:18 ls-lR.Z"
       * "d---------   1 owner    group               0 May  9 19:45 Softlib"
       * Also WFTPD for MSDOS:
       * "-rwxrwxrwx   1 noone    nogroup      322 Aug 19  1996 message.ftp"
       * Hellsoft for NetWare:
       * "d[RWCEMFA] supervisor            512       Jan 16 18:53    login"
       * "-[RWCEMFA] rhesus             214059       Oct 20 15:27    cx.exe"
       * Newer Hellsoft for NetWare: (netlab2.usu.edu)
       * - [RWCEAFMS] NFAUUser               192 Apr 27 15:21 HEADER.html
       * d [RWCEAFMS] jrd                    512 Jul 11 03:01 allupdates
       * Also NetPresenz for the Mac:
       * "-------r--         326  1391972  1392298 Nov 22  1995 MegaPhone.sit"
       * "drwxrwxr-x               folder        2 May 10  1996 network"
       * Protected directory:
       * "drwx-wx-wt  2 root  wheel  512 Jul  1 02:15 incoming"
       * uid/gid instead of username/groupname:
       * "drwxr-xr-x  2 0  0  512 May 28 22:17 etc"
      */
    
      bool isOldHellsoft = false;
    
      if (numtoks >= 6)
      {
        /* there are two perm formats (Hellsoft/NetWare and *IX strmode(3)).
         * Scan for size column only if the perm format is one or the other.
         */
        if (toklen[0] == 1 || (tokens[0][1]) == '[')
        {
          if (*tokens[0] == 'd' || *tokens[0] == '-')
          {
            pos = toklen[0]-1;
            p = tokens[0] + 1;
            if (pos == 0)
            {
              p = tokens[1];
              pos = toklen[1];
            }
            if ((pos == 9 || pos == 10)        && 
                (*p == '[' && p[pos-1] == ']') &&
                (p[1] == 'R' || p[1] == '-')   &&
                (p[2] == 'W' || p[2] == '-')   &&
                (p[3] == 'C' || p[3] == '-')   &&
                (p[4] == 'E' || p[4] == '-'))
            {
              /* rest is FMA[S] or AFM[S] */
              lstyle = 'U'; /* very likely one of the NetWare servers */
              if (toklen[0] == 10)
                isOldHellsoft = true;
            }
          }
        }
        else if ((toklen[0] == 10 || toklen[0] == 11) 
                   && strchr("-bcdlpsw?DFam", *tokens[0]))
        {
          p = &(tokens[0][1]);
          if ((p[0] == 'r' || p[0] == '-') &&
              (p[1] == 'w' || p[1] == '-') &&
              (p[3] == 'r' || p[3] == '-') &&
              (p[4] == 'w' || p[4] == '-') &&
              (p[6] == 'r' || p[6] == '-') &&
              (p[7] == 'w' || p[7] == '-'))
            /* 'x'/p[9] can be S|s|x|-|T|t or implementation specific */
          {
            lstyle = 'U'; /* very likely /bin/ls */
          }
        }
      }
      if (lstyle == 'U') /* first token checks out */
      {
        lstyle = 0;
        for (pos = (numtoks-5); !lstyle && pos > 1; pos--)
        {
          /* scan for: (\d+)\s+([A-Z][a-z][a-z])\s+
           *  (\d\d\d\d|\d\:\d\d|\d\d\:\d\d|\d\:\d\d\:\d\d|\d\d\:\d\d\:\d\d)
           *  \s+(.+)$
          */
          if (isASCIIDigit(*tokens[pos]) /* size */
              /* (\w\w\w) */
           && toklen[pos+1] == 3 && isASCIIAlpha(*tokens[pos+1]) &&
              isASCIIAlpha(tokens[pos+1][1]) && isASCIIAlpha(tokens[pos+1][2])
              /* (\d|\d\d) */
           && isASCIIDigit(*tokens[pos+2]) &&
                (toklen[pos+2] == 1 || 
                  (toklen[pos+2] == 2 && isASCIIDigit(tokens[pos+2][1])))
           && toklen[pos+3] >= 4 && isASCIIDigit(*tokens[pos+3]) 
              /* (\d\:\d\d\:\d\d|\d\d\:\d\d\:\d\d) */
           && (toklen[pos+3] <= 5 || (
               (toklen[pos+3] == 7 || toklen[pos+3] == 8) &&
               (tokens[pos+3][toklen[pos+3]-3]) == ':'))
           && isASCIIDigit(tokens[pos+3][toklen[pos+3]-2])
           && isASCIIDigit(tokens[pos+3][toklen[pos+3]-1])
           && (
              /* (\d\d\d\d) */
                 ((toklen[pos+3] == 4 || toklen[pos+3] == 5) &&
                  isASCIIDigit(tokens[pos+3][1]) &&
                  isASCIIDigit(tokens[pos+3][2])  )
              /* (\d\:\d\d|\d\:\d\d\:\d\d) */
              || ((toklen[pos+3] == 4 || toklen[pos+3] == 7) && 
                  (tokens[pos+3][1]) == ':' &&
                  isASCIIDigit(tokens[pos+3][2]) && isASCIIDigit(tokens[pos+3][3]))
              /* (\d\d\:\d\d|\d\d\:\d\d\:\d\d) */
              || ((toklen[pos+3] == 5 || toklen[pos+3] == 8) && 
                  isASCIIDigit(tokens[pos+3][1]) && (tokens[pos+3][2]) == ':' &&
                  isASCIIDigit(tokens[pos+3][3]) && isASCIIDigit(tokens[pos+3][4])) 
              )
           )
          {
            lstyle = 'U'; /* assume /bin/ls or variant format */
            tokmarker = pos;

            /* check that size is numeric */
            p = tokens[tokmarker];
            for (unsigned int i = 0; lstyle && i < toklen[tokmarker]; ++i)
            {
              if (!isASCIIDigit(*p++))
                lstyle = 0;
            }
            if (lstyle)
            {
              month_num = 0;
              p = tokens[tokmarker+1];
              for (unsigned int i = 0; i < (12*3); i+=3)
              {
                if (p[0] == month_names[i+0] && 
                    p[1] == month_names[i+1] && 
                    p[2] == month_names[i+2])
                  break;
                month_num++;
              }
              if (month_num >= 12)
                lstyle = 0;
            }
          } /* relative position test */
        } /* for (pos = (numtoks-5); !lstyle && pos > 1; pos--) */
      } /* if (lstyle == 'U') */

      if (lstyle == 'U')
      {
        state.parsedOne = true;
        state.listStyle = lstyle;
    
        result.caseSensitive = false;
        result.type = FTPJunkEntry;
        if (*tokens[0] == 'd' || *tokens[0] == 'D')
          result.type = FTPDirectoryEntry;
        else if (*tokens[0] == 'l')
          result.type = FTPLinkEntry;
        else if (*tokens[0] == '-' || *tokens[0] == 'F')
          result.type = FTPFileEntry; /* (hopefully a regular file) */

        if (result.type != FTPDirectoryEntry)
        {
          pos = toklen[tokmarker];
          result.fileSize = String(tokens[tokmarker], pos);
        }

        result.modifiedTime.tm_mon  = month_num;
        result.modifiedTime.tm_mday = atoi(tokens[tokmarker+2]);
        if (result.modifiedTime.tm_mday == 0)
          result.modifiedTime.tm_mday++;

        p = tokens[tokmarker+3];
        pos = (unsigned int)atoi(p);
        if (p[1] == ':') /* one digit hour */
          p--;
        if (p[2] != ':') /* year */
        {
          result.modifiedTime.tm_year = pos;
        }
        else
        {
          result.modifiedTime.tm_hour = pos;
          result.modifiedTime.tm_min  = atoi(p+3);
          if (p[5] == ':')
            result.modifiedTime.tm_sec = atoi(p+6);
       
          if (!state.now)
          {
            time_t now = time(NULL);
            state.now = now * 1000000.0;

            // FIXME: This code has the year 2038 bug
            gmtime_r(&now, &state.nowFTPTime);
            state.nowFTPTime.tm_year += 1900;
          }

          result.modifiedTime.tm_year = state.nowFTPTime.tm_year;
          if ( (( state.nowFTPTime.tm_mon << 5) + state.nowFTPTime.tm_mday) <
               ((result.modifiedTime.tm_mon << 5) + result.modifiedTime.tm_mday) )
            result.modifiedTime.tm_year--;
       
        } /* time/year */
        
        // there is exactly 1 space between filename and previous token in all
        // outputs except old Hellsoft
        if (!isOldHellsoft)
          result.filename = tokens[tokmarker+3] + toklen[tokmarker+3] + 1;
        else
          result.filename = tokens[tokmarker+4];

        result.filenameLength = (&(line[linelen]))
                           - (result.filename);

        if (result.type == FTPLinkEntry && result.filenameLength > 4)
        {
          /* First try to use result.fe_size to find " -> " sequence.
             This can give proper result for cases like "aaa -> bbb -> ccc". */
          unsigned int fileSize = result.fileSize.toUInt();

          if (result.filenameLength > (fileSize + 4) &&
              strncmp(result.filename + result.filenameLength - fileSize - 4, " -> ", 4) == 0)
          {
            result.linkname = result.filename + (result.filenameLength - fileSize);
            result.linknameLength = (&(line[linelen])) - (result.linkname);
            result.filenameLength -= fileSize + 4;
          }
          else
          {
            /* Search for sequence " -> " from the end for case when there are
               more occurrences. F.e. if ftpd returns "a -> b -> c" assume
               "a -> b" as a name. Powerusers can remove unnecessary parts
               manually but there is no way to follow the link when some
               essential part is missing. */
            p = result.filename + (result.filenameLength - 5);
            for (pos = (result.filenameLength - 5); pos > 0; pos--)
            {
              if (strncmp(p, " -> ", 4) == 0)
              {
                result.linkname = p + 4;
                result.linknameLength = (&(line[linelen]))
                                 - (result.linkname);
                result.filenameLength = pos;
                break;
              }
              p--;
            }
          }
        }

#if defined(SUPPORT_LSLF) /* some (very rare) servers return ls -lF */
        if (result.filenameLength > 1)
        {
          p = result.filename[result.filenameLength-1];
          pos = result.type;
          if (pos == 'd') { 
             if (*p == '/') result.filenameLength--; /* directory */
          } else if (pos == 'l') { 
             if (*p == '@') result.filenameLength--; /* symlink */
          } else if (pos == 'f') { 
             if (*p == '*') result.filenameLength--; /* executable */
          } else if (*p == '=' || *p == '%' || *p == '|') {
            result.filenameLength--; /* socket, whiteout, fifo */
          }
        }
#endif
     
        /* the caller should do this (if dropping "." and ".." is desired)
        if (result.type == FTPDirectoryEntry && result.filename[0] == '.' &&
            (result.filenameLength == 1 || (result.filenameLength == 2 &&
                                      result.filename[1] == '.')))
          return FTPJunkEntry;
        */

        return result.type;  

      } /* if (lstyle == 'U') */

    } /* if (!lstyle && (!state.listStyle || state.listStyle == 'U')) */
#endif

    /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#if defined(SUPPORT_W16) /* 16bit Windows */
    if (!lstyle && (!state.listStyle || state.listStyle == 'w'))
    {       /* old SuperTCP suite FTP server for Win3.1 */
            /* old NetManage Chameleon TCP/IP suite FTP server for Win3.1 */
      /*
      * SuperTCP dirlist from the mirror.pl project
      * mon/day/year separator may be '/' or '-'.
      * .               <DIR>           11-16-94        17:16
      * ..              <DIR>           11-16-94        17:16
      * INSTALL         <DIR>           11-16-94        17:17
      * CMT             <DIR>           11-21-94        10:17
      * DESIGN1.DOC          11264      05-11-95        14:20
      * README.TXT            1045      05-10-95        11:01
      * WPKIT1.EXE          960338      06-21-95        17:01
      * CMT.CSV                  0      07-06-95        14:56
      *
      * Chameleon dirlist guessed from lynx
      * .               <DIR>      Nov 16 1994 17:16   
      * ..              <DIR>      Nov 16 1994 17:16   
      * INSTALL         <DIR>      Nov 16 1994 17:17
      * CMT             <DIR>      Nov 21 1994 10:17
      * DESIGN1.DOC     11264      May 11 1995 14:20   A
      * README.TXT       1045      May 10 1995 11:01
      * WPKIT1.EXE     960338      Jun 21 1995 17:01   R
      * CMT.CSV             0      Jul 06 1995 14:56   RHA
      */
      if (numtoks >= 4 && toklen[0] < 13 && 
          ((toklen[1] == 5 && *tokens[1] == '<') || isASCIIDigit(*tokens[1])) )
      {
        if (numtoks == 4
         && (toklen[2] == 8 || toklen[2] == 9)
         && (((tokens[2][2]) == '/' && (tokens[2][5]) == '/') ||
             ((tokens[2][2]) == '-' && (tokens[2][5]) == '-'))
         && (toklen[3] == 4 || toklen[3] == 5)
         && (tokens[3][toklen[3]-3]) == ':'
         && isASCIIDigit(tokens[2][0]) && isASCIIDigit(tokens[2][1])
         && isASCIIDigit(tokens[2][3]) && isASCIIDigit(tokens[2][4])
         && isASCIIDigit(tokens[2][6]) && isASCIIDigit(tokens[2][7])
         && (toklen[2] < 9 || isASCIIDigit(tokens[2][8]))
         && isASCIIDigit(tokens[3][toklen[3]-1]) && isASCIIDigit(tokens[3][toklen[3]-2])
         && isASCIIDigit(tokens[3][toklen[3]-4]) && isASCIIDigit(*tokens[3]) 
         )
        {
          lstyle = 'w';
        }
        else if ((numtoks == 6 || numtoks == 7)
         && toklen[2] == 3 && toklen[3] == 2
         && toklen[4] == 4 && toklen[5] == 5
         && (tokens[5][2]) == ':'
         && isASCIIAlpha(tokens[2][0]) && isASCIIAlpha(tokens[2][1])
         &&                          isASCIIAlpha(tokens[2][2])
         && isASCIIDigit(tokens[3][0]) && isASCIIDigit(tokens[3][1])
         && isASCIIDigit(tokens[4][0]) && isASCIIDigit(tokens[4][1])
         && isASCIIDigit(tokens[4][2]) && isASCIIDigit(tokens[4][3])
         && isASCIIDigit(tokens[5][0]) && isASCIIDigit(tokens[5][1])
         && isASCIIDigit(tokens[5][3]) && isASCIIDigit(tokens[5][4])
         /* could also check that (&(tokens[5][5]) - tokens[2]) == 17 */
        )
        {
          lstyle = 'w';
        }
        if (lstyle && state.listStyle != lstyle) /* first time */
        {
          p = tokens[1];   
          if (toklen[1] != 5 || p[0] != '<' || p[1] != 'D' || 
                 p[2] != 'I' || p[3] != 'R' || p[4] != '>')
          {
            for (pos = 0; lstyle && pos < toklen[1]; pos++)
            {
              if (!isASCIIDigit(*p++))
                lstyle = 0;
            }
          } /* not <DIR> */
        } /* if (first time) */
      } /* if (numtoks == ...) */

      if (lstyle == 'w')
      {
        state.parsedOne = true;
        state.listStyle = lstyle;

        result.caseSensitive = true;
        result.filename = tokens[0];
        result.filenameLength = toklen[0];
        result.type = FTPDirectoryEntry;

        p = tokens[1];
        if (isASCIIDigit(*p))
        {
          result.type = FTPFileEntry;
          pos = toklen[1];
          result.fileSize = String(p, pos);
        }

        p = tokens[2];
        if (toklen[2] == 3) /* Chameleon */
        {
          tbuf[0] = toASCIIUpper(p[0]);
          tbuf[1] = toASCIILower(p[1]);
          tbuf[2] = toASCIILower(p[2]);
          for (pos = 0; pos < (12*3); pos+=3)
          {
            if (tbuf[0] == month_names[pos+0] &&
                tbuf[1] == month_names[pos+1] && 
                tbuf[2] == month_names[pos+2])
            {
              result.modifiedTime.tm_mon = pos/3;
              result.modifiedTime.tm_mday = atoi(tokens[3]);
              result.modifiedTime.tm_year = atoi(tokens[4]) - 1900;
              break;
            }
          }          
          pos = 5; /* Chameleon toknum of date field */
        }
        else
        {
          result.modifiedTime.tm_mon = atoi(p+0)-1;
          result.modifiedTime.tm_mday = atoi(p+3);
          result.modifiedTime.tm_year = atoi(p+6);
          if (result.modifiedTime.tm_year < 80) /* SuperTCP */
            result.modifiedTime.tm_year += 100;

          pos = 3; /* SuperTCP toknum of date field */
        }

        result.modifiedTime.tm_hour = atoi(tokens[pos]);
        result.modifiedTime.tm_min = atoi(&(tokens[pos][toklen[pos]-2]));

        /* the caller should do this (if dropping "." and ".." is desired)
        if (result.type == FTPDirectoryEntry && result.filename[0] == '.' &&
            (result.filenameLength == 1 || (result.filenameLength == 2 &&
                                      result.filename[1] == '.')))
          return FTPJunkEntry;
        */

        return result.type;
      } /* (lstyle == 'w') */

    } /* if (!lstyle && (!state.listStyle || state.listStyle == 'w'))  */
#endif

    /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#if defined(SUPPORT_DLS) /* dls -dtR */
    if (!lstyle && 
       (state.listStyle == 'D' || (!state.listStyle && state.numLines == 1)))
       /* /bin/dls lines have to be immediately recognizable (first line) */
    {
      /* I haven't seen an FTP server that delivers a /bin/dls listing,
       * but can infer the format from the lynx and mirror.pl projects.
       * Both formats are supported.
       *
       * Lynx says:
       * README              763  Information about this server\0
       * bin/                  -  \0
       * etc/                  =  \0
       * ls-lR                 0  \0
       * ls-lR.Z               3  \0
       * pub/                  =  Public area\0
       * usr/                  -  \0
       * morgan               14  -> ../real/morgan\0
       * TIMIT.mostlikely.Z\0
       *                   79215  \0
       *
       * mirror.pl says:
       * filename:  ^(\S*)\s+
       * size:      (\-|\=|\d+)\s+
       * month/day: ((\w\w\w\s+\d+|\d+\s+\w\w\w)\s+
       * time/year: (\d+:\d+|\d\d\d\d))\s+
       * rest:      (.+) 
       *
       * README              763  Jul 11 21:05  Information about this server
       * bin/                  -  Apr 28  1994
       * etc/                  =  11 Jul 21:04
       * ls-lR                 0   6 Aug 17:14
       * ls-lR.Z               3  05 Sep 1994
       * pub/                  =  Jul 11 21:04  Public area
       * usr/                  -  Sep  7 09:39
       * morgan               14  Apr 18 09:39  -> ../real/morgan
       * TIMIT.mostlikely.Z
       *                   79215  Jul 11 21:04
      */
      if (!state.listStyle && line[linelen-1] == ':' && 
          linelen >= 2 && toklen[numtoks-1] != 1)
      { 
        /* code in mirror.pl suggests that a listing may be preceded
         * by a PWD line in the form "/some/dir/names/here:"
         * but does not necessarily begin with '/'. *sigh*
        */
        pos = 0;
        p = line;
        while (pos < (linelen-1))
        {
          /* illegal (or extremely unusual) chars in a dirspec */
          if (*p == '<' || *p == '|' || *p == '>' ||
              *p == '?' || *p == '*' || *p == '\\')
            break;
          if (*p == '/' && pos < (linelen-2) && p[1] == '/')
            break;
          pos++;
          p++;
        }
        if (pos == (linelen-1))
        {
          state.listStyle = 'D';
          return FTPJunkEntry;
        }
      }

      if (!lstyle && numtoks >= 2)
      {
        pos = 22; /* pos of (\d+|-|=) if this is not part of a multiline */
        if (state.listStyle && carry_buf_len) /* first is from previous line */
          pos = toklen[1]-1; /* and is 'as-is' (may contain whitespace) */

        if (linelen > pos)
        {
          p = &line[pos];
          if ((*p == '-' || *p == '=' || isASCIIDigit(*p)) &&
              ((linelen == (pos+1)) || 
               (linelen >= (pos+3) && p[1] == ' ' && p[2] == ' ')) )
          {
            tokmarker = 1;
            if (!carry_buf_len)
            {
              pos = 1;
              while (pos < numtoks && (tokens[pos]+toklen[pos]) < (&line[23]))
                pos++;
              tokmarker = 0;
              if ((tokens[pos]+toklen[pos]) == (&line[23]))
                tokmarker = pos;
            }
            if (tokmarker)  
            {
              lstyle = 'D';
              if (*tokens[tokmarker] == '-' || *tokens[tokmarker] == '=')
              {
                if (toklen[tokmarker] != 1 ||
                   (tokens[tokmarker-1][toklen[tokmarker-1]-1]) != '/')
                  lstyle = 0;
              }              
              else
              {
                for (pos = 0; lstyle && pos < toklen[tokmarker]; pos++) 
                {
                  if (!isASCIIDigit(tokens[tokmarker][pos]))
                    lstyle = 0; 
                }
              }
              if (lstyle && !state.listStyle) /* first time */
              {
                /* scan for illegal (or incredibly unusual) chars in fname */
                for (p = tokens[0]; lstyle &&
                     p < &(tokens[tokmarker-1][toklen[tokmarker-1]]); p++)
                {
                  if (*p == '<' || *p == '|' || *p == '>' || 
                      *p == '?' || *p == '*' || *p == '/' || *p == '\\')
                    lstyle = 0;
                }
              }

            } /* size token found */
          } /* expected chars behind expected size token */
        } /* if (linelen > pos) */
      } /* if (!lstyle && numtoks >= 2) */

      if (!lstyle && state.listStyle == 'D' && !carry_buf_len)
      {
        /* the filename of a multi-line entry can be identified
         * correctly only if dls format had been previously established.
         * This should always be true because there should be entries
         * for '.' and/or '..' and/or CWD that precede the rest of the
         * listing.
        */
        pos = linelen;
        if (pos > (sizeof(state.carryBuffer)-1))
          pos = sizeof(state.carryBuffer)-1;
        memcpy( state.carryBuffer, line, pos );
        state.carryBufferLength = pos;
        return FTPJunkEntry;
      }

      if (lstyle == 'D')
      {
        state.parsedOne = true;
        state.listStyle = lstyle;

        p = &(tokens[tokmarker-1][toklen[tokmarker-1]]);
        result.filename = tokens[0];
        result.filenameLength = p - tokens[0];
        result.type  = FTPFileEntry;

        if (result.filename[result.filenameLength-1] == '/')
        {
          if (result.linknameLength == 1)
            result.type = FTPJunkEntry;
          else
          {
            result.filenameLength--;
            result.type  = FTPDirectoryEntry;
          }
        }
        else if (isASCIIDigit(*tokens[tokmarker]))
        {
          pos = toklen[tokmarker];
          result.fileSize = String(tokens[tokmarker], pos);
        }

        if ((tokmarker+3) < numtoks && 
              (&(tokens[numtoks-1][toklen[numtoks-1]]) - 
               tokens[tokmarker+1]) >= (1+1+3+1+4) )
        {
          pos = (tokmarker+3);
          p = tokens[pos];
          pos = toklen[pos];

          if ((pos == 4 || pos == 5)
          &&  isASCIIDigit(*p) && isASCIIDigit(p[pos-1]) && isASCIIDigit(p[pos-2])
          &&  ((pos == 5 && p[2] == ':') ||  
               (pos == 4 && (isASCIIDigit(p[1]) || p[1] == ':')))
             )
          {
            month_num = tokmarker+1; /* assumed position of month field */
            pos = tokmarker+2;       /* assumed position of mday field */
            if (isASCIIDigit(*tokens[month_num])) /* positions are reversed */
            {
              month_num++;
              pos--;
            }
            p = tokens[month_num];
            if (isASCIIDigit(*tokens[pos]) 
            && (toklen[pos] == 1 || 
                  (toklen[pos] == 2 && isASCIIDigit(tokens[pos][1])))
            && toklen[month_num] == 3
            && isASCIIAlpha(*p) && isASCIIAlpha(p[1]) && isASCIIAlpha(p[2])  )
            {
              pos = atoi(tokens[pos]);
              if (pos > 0 && pos <= 31)
              {
                result.modifiedTime.tm_mday = pos;
                month_num = 1;
                for (pos = 0; pos < (12*3); pos+=3)
                {
                  if (p[0] == month_names[pos+0] &&
                      p[1] == month_names[pos+1] &&
                      p[2] == month_names[pos+2])
                    break;
                  month_num++;
                }
                if (month_num > 12)
                  result.modifiedTime.tm_mday = 0;
                else
                  result.modifiedTime.tm_mon = month_num - 1;
              }
            }
            if (result.modifiedTime.tm_mday)
            {
              tokmarker += 3; /* skip mday/mon/yrtime (to find " -> ") */
              p = tokens[tokmarker];

              pos = atoi(p);
              if (pos > 24)
                result.modifiedTime.tm_year = pos-1900;
              else
              {
                if (p[1] == ':')
                  p--;
                result.modifiedTime.tm_hour = pos;
                result.modifiedTime.tm_min = atoi(p+3);
                if (!state.now)
                {
                  time_t now = time(NULL);
                  state.now = now * 1000000.0;
                  
                  // FIXME: This code has the year 2038 bug
                  gmtime_r(&now, &state.nowFTPTime);
                  state.nowFTPTime.tm_year += 1900;
                }
                result.modifiedTime.tm_year = state.nowFTPTime.tm_year;
                if ( (( state.nowFTPTime.tm_mon  << 4) + state.nowFTPTime.tm_mday) <
                     ((result.modifiedTime.tm_mon << 4) + result.modifiedTime.tm_mday) )
                  result.modifiedTime.tm_year--;
              } /* got year or time */
            } /* got month/mday */
          } /* may have year or time */
        } /* enough remaining to possibly have date/time */

        if (numtoks > (tokmarker+2))
        {
          pos = tokmarker+1;
          p = tokens[pos];
          if (toklen[pos] == 2 && *p == '-' && p[1] == '>')
          {
            p = &(tokens[numtoks-1][toklen[numtoks-1]]);
            result.type  = FTPLinkEntry;
            result.linkname = tokens[pos+1];
            result.linknameLength = p - result.linkname;
            if (result.linknameLength > 1 &&
                result.linkname[result.linknameLength-1] == '/')
              result.linknameLength--;
          }
        } /* if (numtoks > (tokmarker+2)) */

        /* the caller should do this (if dropping "." and ".." is desired)
        if (result.type == FTPDirectoryEntry && result.filename[0] == '.' &&
            (result.filenameLength == 1 || (result.filenameLength == 2 &&
                                      result.filename[1] == '.')))
          return FTPJunkEntry;
        */

        return result.type;

      } /* if (lstyle == 'D') */
    } /* if (!lstyle && (!state.listStyle || state.listStyle == 'D')) */
#endif

    /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

  } /* if (linelen > 0) */

  return ParsingFailed(state);
}

} // namespace WebCore

#endif // ENABLE(FTPDIR)
