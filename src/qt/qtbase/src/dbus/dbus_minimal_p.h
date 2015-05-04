/****************************************************************************
**
** Copyright (C) 2014 Intel Corporation
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDBus module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef DBUS_MINIMAL_P_H
#define DBUS_MINIMAL_P_H

extern "C" {

// Equivalent to dbus-arch-deps.h
typedef qint64 dbus_int64_t;
typedef quint64 dbus_uint64_t;
typedef qint32 dbus_int32_t;
typedef quint32 dbus_uint32_t;
typedef qint16 dbus_int16_t;
typedef quint16 dbus_uint16_t;

// simulate minimum version we support
#define DBUS_MAJOR_VERSION 1
#define DBUS_MINOR_VERSION 2
#define DBUS_VERSION ((1 << 16) | (2 << 8))

// forward declaration to opaque types we use
struct DBusConnection;
struct DBusMessage;
struct DBusPendingCall;
struct DBusServer;
struct DBusTimeout;
struct DBusWatch;

// This file contains constants and typedefs from libdbus-1 headers,
// which carry the following copyright:
/*
 * Copyright (C) 2002, 2003  CodeFactory AB
 * Copyright (C) 2004, 2005 Red Hat, Inc.
 *
 * Licensed under the Academic Free License version 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* dbus-types.h */
typedef dbus_uint32_t  dbus_unichar_t;
typedef dbus_uint32_t  dbus_bool_t;

/* dbus-shared.h */
#define DBUS_SERVICE_DBUS      "org.freedesktop.DBus"
#define DBUS_PATH_DBUS  "/org/freedesktop/DBus"
#define DBUS_INTERFACE_DBUS           "org.freedesktop.DBus"
#define DBUS_INTERFACE_INTROSPECTABLE "org.freedesktop.DBus.Introspectable"
#define DBUS_INTERFACE_PROPERTIES     "org.freedesktop.DBus.Properties"

#define DBUS_NAME_FLAG_ALLOW_REPLACEMENT 0x1 /**< Allow another service to become the primary owner if requested */
#define DBUS_NAME_FLAG_REPLACE_EXISTING  0x2 /**< Request to replace the current primary owner */
#define DBUS_NAME_FLAG_DO_NOT_QUEUE      0x4 /**< If we can not become the primary owner do not place us in the queue */

#define DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER  1 /**< Service has become the primary owner of the requested name */
#define DBUS_REQUEST_NAME_REPLY_IN_QUEUE       2 /**< Service could not become the primary owner and has been placed in the queue */
#define DBUS_REQUEST_NAME_REPLY_EXISTS         3 /**< Service is already in the queue */
#define DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER  4 /**< Service is already the primary owner */

#define DBUS_RELEASE_NAME_REPLY_RELEASED        1 /**< Service was released from the given name */
#define DBUS_RELEASE_NAME_REPLY_NON_EXISTENT    2 /**< The given name does not exist on the bus */
#define DBUS_RELEASE_NAME_REPLY_NOT_OWNER       3 /**< Service is not an owner of the given name */

typedef enum
{
  DBUS_BUS_SESSION,    /**< The login session bus */
  DBUS_BUS_SYSTEM,     /**< The systemwide bus */
  DBUS_BUS_STARTER     /**< The bus that started us, if any */
} DBusBusType;

typedef enum
{
  DBUS_HANDLER_RESULT_HANDLED,         /**< Message has had its effect - no need to run more handlers. */
  DBUS_HANDLER_RESULT_NOT_YET_HANDLED, /**< Message has not had any effect - see if other handlers want it. */
  DBUS_HANDLER_RESULT_NEED_MEMORY      /**< Need more memory in order to return #DBUS_HANDLER_RESULT_HANDLED or #DBUS_HANDLER_RESULT_NOT_YET_HANDLED. Please try again later with more memory. */
} DBusHandlerResult;

/* dbus-memory.h */
typedef void (* DBusFreeFunction) (void *memory);

/* dbus-connection.h */
typedef enum
{
  DBUS_WATCH_READABLE = 1 << 0, /**< As in POLLIN */
  DBUS_WATCH_WRITABLE = 1 << 1, /**< As in POLLOUT */
  DBUS_WATCH_ERROR    = 1 << 2, /**< As in POLLERR (can't watch for
                                 *   this, but can be present in
                                 *   current state passed to
                                 *   dbus_watch_handle()).
                                 */
  DBUS_WATCH_HANGUP   = 1 << 3  /**< As in POLLHUP (can't watch for
                                 *   it, but can be present in current
                                 *   state passed to
                                 *   dbus_watch_handle()).
                                 */
  /* Internal to libdbus, there is also _DBUS_WATCH_NVAL in dbus-watch.h */
} DBusWatchFlags;

typedef enum
{
  DBUS_DISPATCH_DATA_REMAINS,  /**< There is more data to potentially convert to messages. */
  DBUS_DISPATCH_COMPLETE,      /**< All currently available data has been processed. */
  DBUS_DISPATCH_NEED_MEMORY    /**< More memory is needed to continue. */
} DBusDispatchStatus;

typedef dbus_bool_t (* DBusAddWatchFunction)       (DBusWatch      *watch,
                                                    void           *data);
typedef void        (* DBusWatchToggledFunction)   (DBusWatch      *watch,
                                                    void           *data);
typedef void        (* DBusRemoveWatchFunction)    (DBusWatch      *watch,
                                                    void           *data);
typedef dbus_bool_t (* DBusAddTimeoutFunction)     (DBusTimeout    *timeout,
                                                    void           *data);
typedef void        (* DBusTimeoutToggledFunction) (DBusTimeout    *timeout,
                                                    void           *data);
typedef void        (* DBusRemoveTimeoutFunction)  (DBusTimeout    *timeout,
                                                    void           *data);
typedef void        (* DBusDispatchStatusFunction) (DBusConnection *connection,
                                                    DBusDispatchStatus new_status,
                                                    void           *data);
typedef void        (* DBusWakeupMainFunction)     (void           *data);
typedef void (* DBusPendingCallNotifyFunction) (DBusPendingCall *pending,
                                                void            *user_data);
typedef DBusHandlerResult (* DBusHandleMessageFunction) (DBusConnection     *connection,
                                                         DBusMessage        *message,
                                                         void               *user_data);

/* dbus-errors.h */
struct DBusError
{
  const char *name;    /**< public error name field */
  const char *message; /**< public error message field */

  unsigned int dummy1 : 1; /**< placeholder */
  unsigned int dummy2 : 1; /**< placeholder */
  unsigned int dummy3 : 1; /**< placeholder */
  unsigned int dummy4 : 1; /**< placeholder */
  unsigned int dummy5 : 1; /**< placeholder */

  void *padding1; /**< placeholder */
};

/* dbus-message.h */
struct DBusMessageIter
{
  void *dummy1;         /**< Don't use this */
  void *dummy2;         /**< Don't use this */
  dbus_uint32_t dummy3; /**< Don't use this */
  int dummy4;           /**< Don't use this */
  int dummy5;           /**< Don't use this */
  int dummy6;           /**< Don't use this */
  int dummy7;           /**< Don't use this */
  int dummy8;           /**< Don't use this */
  int dummy9;           /**< Don't use this */
  int dummy10;          /**< Don't use this */
  int dummy11;          /**< Don't use this */
  int pad1;             /**< Don't use this */
  int pad2;             /**< Don't use this */
  void *pad3;           /**< Don't use this */
};

/* dbus-protocol.h */
#define DBUS_TYPE_INVALID       ((int) '\0')
#define DBUS_TYPE_INVALID_AS_STRING        "\0"
#define DBUS_TYPE_BYTE          ((int) 'y')
#define DBUS_TYPE_BYTE_AS_STRING           "y"
#define DBUS_TYPE_BOOLEAN       ((int) 'b')
#define DBUS_TYPE_BOOLEAN_AS_STRING        "b"
#define DBUS_TYPE_INT16         ((int) 'n')
#define DBUS_TYPE_INT16_AS_STRING          "n"
#define DBUS_TYPE_UINT16        ((int) 'q')
#define DBUS_TYPE_UINT16_AS_STRING         "q"
#define DBUS_TYPE_INT32         ((int) 'i')
#define DBUS_TYPE_INT32_AS_STRING          "i"
#define DBUS_TYPE_UINT32        ((int) 'u')
#define DBUS_TYPE_UINT32_AS_STRING         "u"
#define DBUS_TYPE_INT64         ((int) 'x')
#define DBUS_TYPE_INT64_AS_STRING          "x"
#define DBUS_TYPE_UINT64        ((int) 't')
#define DBUS_TYPE_UINT64_AS_STRING         "t"
#define DBUS_TYPE_DOUBLE        ((int) 'd')
#define DBUS_TYPE_DOUBLE_AS_STRING         "d"
#define DBUS_TYPE_STRING        ((int) 's')
#define DBUS_TYPE_STRING_AS_STRING         "s"
#define DBUS_TYPE_OBJECT_PATH   ((int) 'o')
#define DBUS_TYPE_OBJECT_PATH_AS_STRING    "o"
#define DBUS_TYPE_SIGNATURE     ((int) 'g')
#define DBUS_TYPE_SIGNATURE_AS_STRING      "g"
#define DBUS_TYPE_UNIX_FD      ((int) 'h')
#define DBUS_TYPE_UNIX_FD_AS_STRING        "h"
#define DBUS_TYPE_ARRAY         ((int) 'a')
#define DBUS_TYPE_ARRAY_AS_STRING          "a"
#define DBUS_TYPE_VARIANT       ((int) 'v')
#define DBUS_TYPE_VARIANT_AS_STRING        "v"

#define DBUS_TYPE_STRUCT        ((int) 'r')
#define DBUS_TYPE_STRUCT_AS_STRING         "r"
#define DBUS_TYPE_DICT_ENTRY    ((int) 'e')
#define DBUS_TYPE_DICT_ENTRY_AS_STRING     "e"

#define DBUS_STRUCT_BEGIN_CHAR   ((int) '(')
#define DBUS_STRUCT_BEGIN_CHAR_AS_STRING   "("
#define DBUS_STRUCT_END_CHAR     ((int) ')')
#define DBUS_STRUCT_END_CHAR_AS_STRING     ")"
#define DBUS_DICT_ENTRY_BEGIN_CHAR   ((int) '{')
#define DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING   "{"
#define DBUS_DICT_ENTRY_END_CHAR     ((int) '}')
#define DBUS_DICT_ENTRY_END_CHAR_AS_STRING     "}"

#define DBUS_MESSAGE_TYPE_INVALID       0
#define DBUS_MESSAGE_TYPE_METHOD_CALL   1
#define DBUS_MESSAGE_TYPE_METHOD_RETURN 2
#define DBUS_MESSAGE_TYPE_ERROR         3
#define DBUS_MESSAGE_TYPE_SIGNAL        4

#define DBUS_MAXIMUM_NAME_LENGTH 255

#define DBUS_INTROSPECT_1_0_XML_NAMESPACE         "http://www.freedesktop.org/standards/dbus"
#define DBUS_INTROSPECT_1_0_XML_PUBLIC_IDENTIFIER "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
#define DBUS_INTROSPECT_1_0_XML_SYSTEM_IDENTIFIER "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd"
#define DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE "<!DOCTYPE node PUBLIC \"" DBUS_INTROSPECT_1_0_XML_PUBLIC_IDENTIFIER "\"\n\"" DBUS_INTROSPECT_1_0_XML_SYSTEM_IDENTIFIER "\">\n"

/* dbus-server.h */
typedef void (* DBusNewConnectionFunction) (DBusServer     *server,
                                            DBusConnection *new_connection,
                                            void           *data);

} // extern "C"

#endif // DBUS_MINIMAL_P_H

