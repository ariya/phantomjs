/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <private/qcore_mac_p.h>
#include <new>

QT_BEGIN_NAMESPACE

QString QCFString::toQString(CFStringRef str)
{
    if (!str)
        return QString();

    CFIndex length = CFStringGetLength(str);
    if (length == 0)
        return QString();

    QString string(length, Qt::Uninitialized);
    CFStringGetCharacters(str, CFRangeMake(0, length), reinterpret_cast<UniChar *>(const_cast<QChar *>(string.unicode())));

    return string;
}

QCFString::operator QString() const
{
    if (string.isEmpty() && type)
        const_cast<QCFString*>(this)->string = toQString(type);
    return string;
}

CFStringRef QCFString::toCFStringRef(const QString &string)
{
    return CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar *>(string.unicode()),
                                        string.length());
}

QCFString::operator CFStringRef() const
{
    if (!type) {
        if (string.d->data != string.d->array)
            const_cast<QCFString*>(this)->string.realloc(); // ### Qt5: do we really need this stupid user protection?
        const_cast<QCFString*>(this)->type =
            CFStringCreateWithCharactersNoCopy(0,
                                               reinterpret_cast<const UniChar *>(string.unicode()),
                                               string.length(),
                                               kCFAllocatorNull);
    }
    return type;
}


#if !defined(Q_OS_IOS)
void qt_mac_to_pascal_string(const QString &s, Str255 str, TextEncoding encoding, int len)
{
    if(len == -1)
        len = s.length();
#if 0
    UnicodeMapping mapping;
    mapping.unicodeEncoding = CreateTextEncoding(kTextEncodingUnicodeDefault,
                                                 kTextEncodingDefaultVariant,
                                                 kUnicode16BitFormat);
    mapping.otherEncoding = (encoding ? encoding : );
    mapping.mappingVersion = kUnicodeUseLatestMapping;

    UnicodeToTextInfo info;
    OSStatus err = CreateUnicodeToTextInfo(&mapping, &info);
    if(err != noErr) {
        qDebug("Qt: internal: Unable to create pascal string '%s'::%d [%ld]",
               s.left(len).latin1(), (int)encoding, err);
        return;
    }
    const int unilen = len * 2;
    const UniChar *unibuf = (UniChar *)s.unicode();
    ConvertFromUnicodeToPString(info, unilen, unibuf, str);
    DisposeUnicodeToTextInfo(&info);
#else
    Q_UNUSED(encoding);
    CFStringGetPascalString(QCFString(s), str, 256, CFStringGetSystemEncoding());
#endif
}

QString qt_mac_from_pascal_string(const Str255 pstr)
{
    return QCFString(CFStringCreateWithPascalString(0, pstr, CFStringGetSystemEncoding()));
}

OSErr qt_mac_create_fsref(const QString &file, FSRef *fsref)
{
    return FSPathMakeRef(reinterpret_cast<const UInt8 *>(file.toUtf8().constData()), fsref, 0);
}

// Don't use this function, it won't work in 10.5 (Leopard) and up
OSErr qt_mac_create_fsspec(const QString &file, FSSpec *spec)
{
    FSRef fsref;
    OSErr ret = qt_mac_create_fsref(file, &fsref);
    if (ret == noErr)
        ret = FSGetCatalogInfo(&fsref, kFSCatInfoNone, 0, 0, spec, 0);
    return ret;
}
#endif // !defined(Q_OS_IOS)

QT_END_NAMESPACE
