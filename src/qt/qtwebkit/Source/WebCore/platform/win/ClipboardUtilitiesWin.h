/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ClipboardUtilitiesWin_h
#define ClipboardUtilitiesWin_h

#include "DragData.h"
#include <windows.h>
#include <wtf/Forward.h>

namespace WebCore {

class Document;
class KURL;

HGLOBAL createGlobalData(const String&);
HGLOBAL createGlobalData(const Vector<char>&);
HGLOBAL createGlobalData(const KURL& url, const String& title);

FORMATETC* urlWFormat();
FORMATETC* urlFormat();
FORMATETC* plainTextWFormat();
FORMATETC* plainTextFormat();
FORMATETC* filenameWFormat();
FORMATETC* filenameFormat();
FORMATETC* htmlFormat();
FORMATETC* cfHDropFormat();
FORMATETC* smartPasteFormat();
FORMATETC* fileDescriptorFormat();
FORMATETC* fileContentFormatZero();

void markupToCFHTML(const String& markup, const String& srcURL, Vector<char>& result);

void replaceNewlinesWithWindowsStyleNewlines(String&);
void replaceNBSPWithSpace(String&);

bool containsFilenames(const IDataObject*);
bool containsFilenames(const DragDataMap*);
bool containsHTML(IDataObject*);
bool containsHTML(const DragDataMap*);

PassRefPtr<DocumentFragment> fragmentFromFilenames(Document*, const IDataObject*);
PassRefPtr<DocumentFragment> fragmentFromFilenames(Document*, const DragDataMap*);
PassRefPtr<DocumentFragment> fragmentFromHTML(Document*, IDataObject*);
PassRefPtr<DocumentFragment> fragmentFromHTML(Document*, const DragDataMap*);
PassRefPtr<DocumentFragment> fragmentFromCFHTML(Document*, const String& cfhtml);

String getURL(IDataObject*, DragData::FilenameConversionPolicy, String* title = 0);
String getURL(const DragDataMap*, DragData::FilenameConversionPolicy, String* title = 0);
String getPlainText(IDataObject*);
String getPlainText(const DragDataMap*);
String getTextHTML(IDataObject*);
String getTextHTML(const DragDataMap*);
String getCFHTML(IDataObject*);
String getCFHTML(const DragDataMap*);

void getClipboardData(IDataObject*, FORMATETC* fetc, Vector<String>& dataStrings);
void setClipboardData(IDataObject*, UINT format, const Vector<String>& dataStrings);
void getFileDescriptorData(IDataObject*, int& size, String& pathname);
void getFileContentData(IDataObject*, int size, void* dataBlob);
void setFileDescriptorData(IDataObject*, int size, const String& pathname);
void setFileContentData(IDataObject*, int size, void* dataBlob);

} // namespace WebCore

#endif // ClipboardUtilitiesWin_h
