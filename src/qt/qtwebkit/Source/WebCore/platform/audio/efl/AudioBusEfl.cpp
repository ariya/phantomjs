/*
    Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License 2.1 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#if ENABLE(WEB_AUDIO)
#include "AudioBus.h"

#include "AudioFileReader.h"
#include "FileSystem.h"
#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>

namespace WebCore {

PassRefPtr<AudioBus> AudioBus::loadPlatformResource(const char* name, float sampleRate)
{
    String absoluteFilename(makeString(DATA_DIR, "/webaudio/resources/", name, ".wav"));
    if (!fileExists(absoluteFilename))
        absoluteFilename = makeString(UNINSTALLED_AUDIO_RESOURCES_DIR, "/", name, ".wav");

    return createBusFromAudioFile(absoluteFilename.utf8().data(), false, sampleRate);
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
