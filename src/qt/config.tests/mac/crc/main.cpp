/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the config.tests of the Qt Toolkit.
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

#include <iostream>
#include <cstdlib>
#include <cstring>


class CCRC32
{
public:
    CCRC32() { initialize(); }

    unsigned long FullCRC(const unsigned char *sData, unsigned long ulDataLength)
    {
        unsigned long ulCRC = 0xffffffff;
        PartialCRC(&ulCRC, sData, ulDataLength);
        return(ulCRC ^ 0xffffffff);
    }

    void PartialCRC(unsigned long *ulCRC, const unsigned char *sData, unsigned long ulDataLength)
    {
        while(ulDataLength--) {
            *ulCRC = (*ulCRC >> 8) ^ ulTable[(*ulCRC & 0xFF) ^ *sData++];
        }
    }

private:
    void initialize(void)
    {
        unsigned long ulPolynomial = 0x04C11DB7;
        memset(&ulTable, 0, sizeof(ulTable));
        for(int iCodes = 0; iCodes <= 0xFF; iCodes++) {
            ulTable[iCodes] = Reflect(iCodes, 8) << 24;
            for(int iPos = 0; iPos < 8; iPos++) {
                ulTable[iCodes] = ((ulTable[iCodes] << 1) & 0xffffffff)
                    ^ ((ulTable[iCodes] & (1 << 31)) ? ulPolynomial : 0);
            }

            ulTable[iCodes] = Reflect(ulTable[iCodes], 32);
        }
    }
    unsigned long Reflect(unsigned long ulReflect, const char cChar)
    {
        unsigned long ulValue = 0;
        // Swap bit 0 for bit 7, bit 1 For bit 6, etc....
        for(int iPos = 1; iPos < (cChar + 1); iPos++) {
            if(ulReflect & 1) {
                ulValue |= (1ul << (cChar - iPos));
            }
            ulReflect >>= 1;
        }
        return ulValue;
    }
    unsigned long ulTable[256]; // CRC lookup table array.
};


int main(int argc, char **argv)
{
    CCRC32 crc;
    char *name;
    if (argc < 2) {
        std::cerr << "usage: crc <string>\n";
        return 0;
    } else {
        name = argv[1];
    }
    std::cout << crc.FullCRC((unsigned char *)name, strlen(name)) << std::endl;
}
