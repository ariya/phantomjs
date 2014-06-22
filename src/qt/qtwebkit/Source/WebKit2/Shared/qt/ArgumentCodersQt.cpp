/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

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

#include "config.h"
#include "ArgumentCodersQt.h"

#include "ArgumentCoders.h"
#include "WebCoreArgumentCoders.h"
#include <QMimeData>
#include <QStringList>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace CoreIPC {

typedef HashMap<String , Vector<uint8_t> > MIMEDataHashMap;

void ArgumentCoder<WebCore::DragData>::encode(ArgumentEncoder& encoder, const DragData& dragData)
{
    encoder << dragData.clientPosition();
    encoder << dragData.globalPosition();
    encoder << (uint64_t)dragData.draggingSourceOperationMask();
    encoder << (uint64_t)dragData.flags();

    bool hasPlatformData = dragData.platformData();
    encoder << hasPlatformData;
    if (!hasPlatformData)
        return;

    QStringList formats = dragData.platformData()->formats();
    MIMEDataHashMap map;
    int size = formats.size();
    for (int i = 0; i < size; i++) {
        QByteArray bytes = dragData.platformData()->data(formats[i]);
        Vector<uint8_t> vdata;
        vdata.append((uint8_t*)(bytes.data()), bytes.size());
        map.add(String(formats[i]), vdata);
    }
    encoder << map;
}

bool ArgumentCoder<WebCore::DragData>::decode(ArgumentDecoder& decoder, DragData& dragData)
{
    IntPoint clientPosition;
    IntPoint globalPosition;
    uint64_t sourceOperationMask;
    uint64_t flags;
    if (!decoder.decode(clientPosition))
        return false;
    if (!decoder.decode(globalPosition))
        return false;
    if (!decoder.decode(sourceOperationMask))
        return false;
    if (!decoder.decode(flags))
        return false;

    bool hasPlatformData;
    if (!decoder.decode(hasPlatformData))
        return false;

    QMimeData* mimeData = 0;
    if (hasPlatformData) {
        MIMEDataHashMap map;
        if (!decoder.decode(map))
            return false;

        mimeData = new QMimeData;
        MIMEDataHashMap::iterator it = map.begin();
        MIMEDataHashMap::iterator end = map.end();
        for (; it != end; ++it) {
            QByteArray bytes((char*)it->value.data(), it->value.size());
            mimeData->setData(it->key, bytes);
        }
    }

    dragData = DragData(mimeData, clientPosition, globalPosition, (DragOperation)sourceOperationMask, (DragApplicationFlags)flags);
    return true;
}

}
