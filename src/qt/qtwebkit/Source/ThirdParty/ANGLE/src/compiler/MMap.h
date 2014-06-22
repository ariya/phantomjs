//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _MMAP_INCLUDED_
#define _MMAP_INCLUDED_

//
// Encapsulate memory mapped files
//

class TMMap {
public:
    TMMap(const char* fileName) : 
        fSize(-1), // -1 is the error value returned by GetFileSize()
        fp(NULL),
        fBuff(0)   // 0 is the error value returned by MapViewOfFile()
    {
        if ((fp = fopen(fileName, "r")) == NULL)
            return;
        char c = getc(fp);
        fSize = 0;
        while (c != EOF) {
            fSize++;
            c = getc(fp);
        }
        if (c == EOF)
            fSize++;
        rewind(fp);
        fBuff = (char*)malloc(sizeof(char) * fSize);
        int count = 0;
        c = getc(fp);
        while (c != EOF) {
            fBuff[count++] = c;
            c = getc(fp);
        }
        fBuff[count++] = c;
    }

    char* getData() { return fBuff; }
    int   getSize() { return fSize; }

    ~TMMap() {
        if (fp != NULL)
            fclose(fp);
    }
    
private:
    int             fSize;      // size of file to map in
    FILE *fp;
    char*           fBuff;      // the actual data;
};

#endif // _MMAP_INCLUDED_
