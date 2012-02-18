/*
 *  Copyright (C) 2009 Patrick Gansterer (paroga@paroga.com)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "Vector.h"
#include <winbase.h>
#include <winnls.h>
#include <wtf/UnusedParam.h>

int main(int argc, char** argv);

static inline char* convertToUtf8(LPCWSTR widecharString, int length)
{
    int requiredSize = WideCharToMultiByte(CP_UTF8, 0, widecharString, length, 0, 0, 0, 0);
    char* multibyteString = new char[requiredSize + 1];

    WideCharToMultiByte(CP_UTF8, 0, widecharString, length, multibyteString, requiredSize, 0, 0);
    multibyteString[requiredSize] = '\0';

    return multibyteString;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNUSED_PARAM(hInstance);
    UNUSED_PARAM(hPrevInstance);
    UNUSED_PARAM(nCmdShow);

    Vector<char*> arguments;
    TCHAR buffer[MAX_PATH];

    int length = GetModuleFileNameW(0, buffer, MAX_PATH);
    arguments.append(convertToUtf8(buffer, length));

    WCHAR* commandLine = lpCmdLine;
    while (commandLine[0] != '\0') {
        int commandLineLength = 1;
        WCHAR endChar = ' ';

        while (commandLine[0] == ' ')
            ++commandLine;

        if (commandLine[0] == '\"') {
            ++commandLine;
            endChar = '\"';
        }

        while (commandLine[commandLineLength] != endChar && commandLine[commandLineLength] != '\0')
            ++commandLineLength;

        arguments.append(convertToUtf8(commandLine, commandLineLength));

        commandLine += commandLineLength;
        if (endChar != ' ' && commandLine[0] != '\0')
            ++commandLine;
    }

    int res = main(arguments.size(), arguments.data());

    for (size_t i = 0; i < arguments.size(); i++)
        delete arguments[i];

    return res;
}
