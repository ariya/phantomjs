/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 University of Szeged
 *
 * All rights reserved.
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

#include "utils.h"

bool takeOptionFlag(QStringList* arguments, const QString& name)
{
    int index = arguments->indexOf(name);
    if (index != -1)
        arguments->removeAt(index);
    return index != -1;
}

QString takeOptionValue(QStringList* arguments, const QString& name)
{
    QString result;

    int index = arguments->indexOf(name);
    if (index != -1) {
        arguments->removeAt(index);
        if (index < arguments->count() && !arguments->at(index).startsWith("-"))
            result = arguments->takeAt(index);
    }

    return result;
}

QString formatKeys(QList<QString> keys)
{
    QString result;
    for (int i = 0; i < keys.count() - 1; i++)
        result.append(keys.at(i) + "|");
    result.append(keys.last());
    return result;
}

QList<QString> enumToKeys(const QMetaObject o, const QString& name, const QString& strip)
{
    QList<QString> list;

    int enumIndex = o.indexOfEnumerator(name.toLatin1().data());
    QMetaEnum enumerator = o.enumerator(enumIndex);

    if (enumerator.isValid()) {
        for (int i = 0; i < enumerator.keyCount(); i++) {
            QString key(enumerator.valueToKey(i));
            list.append(key.remove(strip));
        }
    }

    return list;
}

void appQuit(int exitCode, const QString& msg)
{
    if (!msg.isEmpty()) {
        if (exitCode > 0)
            qDebug("ERROR: %s", msg.toLatin1().data());
        else
            qDebug() << msg;
    }
    exit(exitCode);
}

QUrl Utils::urlFromUserInput(const QString& string)
{
    QString input(string);
    QFileInfo fi(input);
    if (fi.exists() && fi.isRelative())
        input = fi.absoluteFilePath();

    return QUrl::fromUserInput(input);
}
