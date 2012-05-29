/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FileChooser.h"

#include "Icon.h"

namespace WebCore {
    
FileChooserClient::~FileChooserClient()
{
}

inline FileChooser::FileChooser(FileChooserClient* client, const Vector<String>& initialFilenames)
    : m_client(client)
    , m_isInitializing(true)
{
    m_filenames = initialFilenames;
}

void FileChooser::initialize()
{
    loadIcon();
    m_isInitializing = false;
}

PassRefPtr<FileChooser> FileChooser::create(FileChooserClient* client, const Vector<String>& initialFilenames)
{
    RefPtr<FileChooser> chooser(adoptRef(new FileChooser(client, initialFilenames)));
    chooser->initialize();
    return chooser;
}

FileChooser::~FileChooser()
{
}

void FileChooser::clear()
{
    m_filenames.clear();
    m_icon = 0;
}

void FileChooser::chooseFile(const String& filename)
{
    Vector<String> filenames;
    filenames.append(filename);
    chooseFiles(filenames);
}

void FileChooser::chooseFiles(const Vector<String>& filenames)
{
    if (m_filenames == filenames)
        return;
    m_filenames = filenames;
    loadIcon();
    if (m_client)
        m_client->valueChanged();
}

void FileChooser::loadIcon()
{
    if (m_filenames.size() && m_client)
        m_client->chooseIconForFiles(this, m_filenames);
}

void FileChooser::iconLoaded(PassRefPtr<Icon> icon)
{
    m_icon = icon;
    if (!m_isInitializing && m_icon && m_client)
        m_client->repaint();
}

}
