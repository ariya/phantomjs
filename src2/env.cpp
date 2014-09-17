/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2012 execjosh, http://execjosh.blogspot.com

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "env.h"

#include <QCoreApplication>
#include <QString>
#include <QVariantMap>

static Env *env_instance = NULL;

Env *Env::instance()
{
    if (NULL == env_instance)
        env_instance = new Env();

    return env_instance;
}

Env::Env()
    : QObject(QCoreApplication::instance())
{
}

// public:

void Env::parse(const char **envp)
{
    const char **env = (const char **)NULL;
    QString envvar, name, value;
    int indexOfEquals;
    // Loop for each of the <NAME>=<VALUE> pairs and split them into a map
    for (env = envp; *env != (const char *)NULL; env++) {
        envvar = QString(*env);
        indexOfEquals = envvar.indexOf('=');
        if (0 >= indexOfEquals) {
            // Should never happen because names cannot contain "=" and cannot
            // be empty. If it does happen, then just ignore this record.
            // See: http://pubs.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap08.html
            continue;
        }
        // Extract name and value (if it exists) from envvar
        // NOTE:
        //  QString::mid() will gracefully return an empty QString when the
        //  specified position index is >= the length() of the string
        name = envvar.left(indexOfEquals);
        value = envvar.mid(indexOfEquals + 1);
        m_map.insert(name, value);
    }
}

QVariantMap Env::asVariantMap() const
{
    return m_map;
}
