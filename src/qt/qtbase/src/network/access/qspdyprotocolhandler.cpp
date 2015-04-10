/****************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include <private/qspdyprotocolhandler_p.h>
#include <private/qnoncontiguousbytedevice_p.h>
#include <private/qhttpnetworkconnectionchannel_p.h>
#include <QtCore/QtEndian>

#if !defined(QT_NO_HTTP) && !defined(QT_NO_SSL)

QT_BEGIN_NAMESPACE

static const char spdyDictionary[] = {
    0x00, 0x00, 0x00, 0x07, 0x6f, 0x70, 0x74, 0x69,  // ....opti
    0x6f, 0x6e, 0x73, 0x00, 0x00, 0x00, 0x04, 0x68,  // ons....h
    0x65, 0x61, 0x64, 0x00, 0x00, 0x00, 0x04, 0x70,  // ead....p
    0x6f, 0x73, 0x74, 0x00, 0x00, 0x00, 0x03, 0x70,  // ost....p
    0x75, 0x74, 0x00, 0x00, 0x00, 0x06, 0x64, 0x65,  // ut....de
    0x6c, 0x65, 0x74, 0x65, 0x00, 0x00, 0x00, 0x05,  // lete....
    0x74, 0x72, 0x61, 0x63, 0x65, 0x00, 0x00, 0x00,  // trace...
    0x06, 0x61, 0x63, 0x63, 0x65, 0x70, 0x74, 0x00,  // .accept.
    0x00, 0x00, 0x0e, 0x61, 0x63, 0x63, 0x65, 0x70,  // ...accep
    0x74, 0x2d, 0x63, 0x68, 0x61, 0x72, 0x73, 0x65,  // t-charse
    0x74, 0x00, 0x00, 0x00, 0x0f, 0x61, 0x63, 0x63,  // t....acc
    0x65, 0x70, 0x74, 0x2d, 0x65, 0x6e, 0x63, 0x6f,  // ept-enco
    0x64, 0x69, 0x6e, 0x67, 0x00, 0x00, 0x00, 0x0f,  // ding....
    0x61, 0x63, 0x63, 0x65, 0x70, 0x74, 0x2d, 0x6c,  // accept-l
    0x61, 0x6e, 0x67, 0x75, 0x61, 0x67, 0x65, 0x00,  // anguage.
    0x00, 0x00, 0x0d, 0x61, 0x63, 0x63, 0x65, 0x70,  // ...accep
    0x74, 0x2d, 0x72, 0x61, 0x6e, 0x67, 0x65, 0x73,  // t-ranges
    0x00, 0x00, 0x00, 0x03, 0x61, 0x67, 0x65, 0x00,  // ....age.
    0x00, 0x00, 0x05, 0x61, 0x6c, 0x6c, 0x6f, 0x77,  // ...allow
    0x00, 0x00, 0x00, 0x0d, 0x61, 0x75, 0x74, 0x68,  // ....auth
    0x6f, 0x72, 0x69, 0x7a, 0x61, 0x74, 0x69, 0x6f,  // orizatio
    0x6e, 0x00, 0x00, 0x00, 0x0d, 0x63, 0x61, 0x63,  // n....cac
    0x68, 0x65, 0x2d, 0x63, 0x6f, 0x6e, 0x74, 0x72,  // he-contr
    0x6f, 0x6c, 0x00, 0x00, 0x00, 0x0a, 0x63, 0x6f,  // ol....co
    0x6e, 0x6e, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e,  // nnection
    0x00, 0x00, 0x00, 0x0c, 0x63, 0x6f, 0x6e, 0x74,  // ....cont
    0x65, 0x6e, 0x74, 0x2d, 0x62, 0x61, 0x73, 0x65,  // ent-base
    0x00, 0x00, 0x00, 0x10, 0x63, 0x6f, 0x6e, 0x74,  // ....cont
    0x65, 0x6e, 0x74, 0x2d, 0x65, 0x6e, 0x63, 0x6f,  // ent-enco
    0x64, 0x69, 0x6e, 0x67, 0x00, 0x00, 0x00, 0x10,  // ding....
    0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x2d,  // content-
    0x6c, 0x61, 0x6e, 0x67, 0x75, 0x61, 0x67, 0x65,  // language
    0x00, 0x00, 0x00, 0x0e, 0x63, 0x6f, 0x6e, 0x74,  // ....cont
    0x65, 0x6e, 0x74, 0x2d, 0x6c, 0x65, 0x6e, 0x67,  // ent-leng
    0x74, 0x68, 0x00, 0x00, 0x00, 0x10, 0x63, 0x6f,  // th....co
    0x6e, 0x74, 0x65, 0x6e, 0x74, 0x2d, 0x6c, 0x6f,  // ntent-lo
    0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x00,  // cation..
    0x00, 0x0b, 0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e,  // ..conten
    0x74, 0x2d, 0x6d, 0x64, 0x35, 0x00, 0x00, 0x00,  // t-md5...
    0x0d, 0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74,  // .content
    0x2d, 0x72, 0x61, 0x6e, 0x67, 0x65, 0x00, 0x00,  // -range..
    0x00, 0x0c, 0x63, 0x6f, 0x6e, 0x74, 0x65, 0x6e,  // ..conten
    0x74, 0x2d, 0x74, 0x79, 0x70, 0x65, 0x00, 0x00,  // t-type..
    0x00, 0x04, 0x64, 0x61, 0x74, 0x65, 0x00, 0x00,  // ..date..
    0x00, 0x04, 0x65, 0x74, 0x61, 0x67, 0x00, 0x00,  // ..etag..
    0x00, 0x06, 0x65, 0x78, 0x70, 0x65, 0x63, 0x74,  // ..expect
    0x00, 0x00, 0x00, 0x07, 0x65, 0x78, 0x70, 0x69,  // ....expi
    0x72, 0x65, 0x73, 0x00, 0x00, 0x00, 0x04, 0x66,  // res....f
    0x72, 0x6f, 0x6d, 0x00, 0x00, 0x00, 0x04, 0x68,  // rom....h
    0x6f, 0x73, 0x74, 0x00, 0x00, 0x00, 0x08, 0x69,  // ost....i
    0x66, 0x2d, 0x6d, 0x61, 0x74, 0x63, 0x68, 0x00,  // f-match.
    0x00, 0x00, 0x11, 0x69, 0x66, 0x2d, 0x6d, 0x6f,  // ...if-mo
    0x64, 0x69, 0x66, 0x69, 0x65, 0x64, 0x2d, 0x73,  // dified-s
    0x69, 0x6e, 0x63, 0x65, 0x00, 0x00, 0x00, 0x0d,  // ince....
    0x69, 0x66, 0x2d, 0x6e, 0x6f, 0x6e, 0x65, 0x2d,  // if-none-
    0x6d, 0x61, 0x74, 0x63, 0x68, 0x00, 0x00, 0x00,  // match...
    0x08, 0x69, 0x66, 0x2d, 0x72, 0x61, 0x6e, 0x67,  // .if-rang
    0x65, 0x00, 0x00, 0x00, 0x13, 0x69, 0x66, 0x2d,  // e....if-
    0x75, 0x6e, 0x6d, 0x6f, 0x64, 0x69, 0x66, 0x69,  // unmodifi
    0x65, 0x64, 0x2d, 0x73, 0x69, 0x6e, 0x63, 0x65,  // ed-since
    0x00, 0x00, 0x00, 0x0d, 0x6c, 0x61, 0x73, 0x74,  // ....last
    0x2d, 0x6d, 0x6f, 0x64, 0x69, 0x66, 0x69, 0x65,  // -modifie
    0x64, 0x00, 0x00, 0x00, 0x08, 0x6c, 0x6f, 0x63,  // d....loc
    0x61, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x00, 0x00,  // ation...
    0x0c, 0x6d, 0x61, 0x78, 0x2d, 0x66, 0x6f, 0x72,  // .max-for
    0x77, 0x61, 0x72, 0x64, 0x73, 0x00, 0x00, 0x00,  // wards...
    0x06, 0x70, 0x72, 0x61, 0x67, 0x6d, 0x61, 0x00,  // .pragma.
    0x00, 0x00, 0x12, 0x70, 0x72, 0x6f, 0x78, 0x79,  // ...proxy
    0x2d, 0x61, 0x75, 0x74, 0x68, 0x65, 0x6e, 0x74,  // -authent
    0x69, 0x63, 0x61, 0x74, 0x65, 0x00, 0x00, 0x00,  // icate...
    0x13, 0x70, 0x72, 0x6f, 0x78, 0x79, 0x2d, 0x61,  // .proxy-a
    0x75, 0x74, 0x68, 0x6f, 0x72, 0x69, 0x7a, 0x61,  // uthoriza
    0x74, 0x69, 0x6f, 0x6e, 0x00, 0x00, 0x00, 0x05,  // tion....
    0x72, 0x61, 0x6e, 0x67, 0x65, 0x00, 0x00, 0x00,  // range...
    0x07, 0x72, 0x65, 0x66, 0x65, 0x72, 0x65, 0x72,  // .referer
    0x00, 0x00, 0x00, 0x0b, 0x72, 0x65, 0x74, 0x72,  // ....retr
    0x79, 0x2d, 0x61, 0x66, 0x74, 0x65, 0x72, 0x00,  // y-after.
    0x00, 0x00, 0x06, 0x73, 0x65, 0x72, 0x76, 0x65,  // ...serve
    0x72, 0x00, 0x00, 0x00, 0x02, 0x74, 0x65, 0x00,  // r....te.
    0x00, 0x00, 0x07, 0x74, 0x72, 0x61, 0x69, 0x6c,  // ...trail
    0x65, 0x72, 0x00, 0x00, 0x00, 0x11, 0x74, 0x72,  // er....tr
    0x61, 0x6e, 0x73, 0x66, 0x65, 0x72, 0x2d, 0x65,  // ansfer-e
    0x6e, 0x63, 0x6f, 0x64, 0x69, 0x6e, 0x67, 0x00,  // ncoding.
    0x00, 0x00, 0x07, 0x75, 0x70, 0x67, 0x72, 0x61,  // ...upgra
    0x64, 0x65, 0x00, 0x00, 0x00, 0x0a, 0x75, 0x73,  // de....us
    0x65, 0x72, 0x2d, 0x61, 0x67, 0x65, 0x6e, 0x74,  // er-agent
    0x00, 0x00, 0x00, 0x04, 0x76, 0x61, 0x72, 0x79,  // ....vary
    0x00, 0x00, 0x00, 0x03, 0x76, 0x69, 0x61, 0x00,  // ....via.
    0x00, 0x00, 0x07, 0x77, 0x61, 0x72, 0x6e, 0x69,  // ...warni
    0x6e, 0x67, 0x00, 0x00, 0x00, 0x10, 0x77, 0x77,  // ng....ww
    0x77, 0x2d, 0x61, 0x75, 0x74, 0x68, 0x65, 0x6e,  // w-authen
    0x74, 0x69, 0x63, 0x61, 0x74, 0x65, 0x00, 0x00,  // ticate..
    0x00, 0x06, 0x6d, 0x65, 0x74, 0x68, 0x6f, 0x64,  // ..method
    0x00, 0x00, 0x00, 0x03, 0x67, 0x65, 0x74, 0x00,  // ....get.
    0x00, 0x00, 0x06, 0x73, 0x74, 0x61, 0x74, 0x75,  // ...statu
    0x73, 0x00, 0x00, 0x00, 0x06, 0x32, 0x30, 0x30,  // s....200
    0x20, 0x4f, 0x4b, 0x00, 0x00, 0x00, 0x07, 0x76,  // .OK....v
    0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x00, 0x00,  // ersion..
    0x00, 0x08, 0x48, 0x54, 0x54, 0x50, 0x2f, 0x31,  // ..HTTP.1
    0x2e, 0x31, 0x00, 0x00, 0x00, 0x03, 0x75, 0x72,  // .1....ur
    0x6c, 0x00, 0x00, 0x00, 0x06, 0x70, 0x75, 0x62,  // l....pub
    0x6c, 0x69, 0x63, 0x00, 0x00, 0x00, 0x0a, 0x73,  // lic....s
    0x65, 0x74, 0x2d, 0x63, 0x6f, 0x6f, 0x6b, 0x69,  // et-cooki
    0x65, 0x00, 0x00, 0x00, 0x0a, 0x6b, 0x65, 0x65,  // e....kee
    0x70, 0x2d, 0x61, 0x6c, 0x69, 0x76, 0x65, 0x00,  // p-alive.
    0x00, 0x00, 0x06, 0x6f, 0x72, 0x69, 0x67, 0x69,  // ...origi
    0x6e, 0x31, 0x30, 0x30, 0x31, 0x30, 0x31, 0x32,  // n1001012
    0x30, 0x31, 0x32, 0x30, 0x32, 0x32, 0x30, 0x35,  // 01202205
    0x32, 0x30, 0x36, 0x33, 0x30, 0x30, 0x33, 0x30,  // 20630030
    0x32, 0x33, 0x30, 0x33, 0x33, 0x30, 0x34, 0x33,  // 23033043
    0x30, 0x35, 0x33, 0x30, 0x36, 0x33, 0x30, 0x37,  // 05306307
    0x34, 0x30, 0x32, 0x34, 0x30, 0x35, 0x34, 0x30,  // 40240540
    0x36, 0x34, 0x30, 0x37, 0x34, 0x30, 0x38, 0x34,  // 64074084
    0x30, 0x39, 0x34, 0x31, 0x30, 0x34, 0x31, 0x31,  // 09410411
    0x34, 0x31, 0x32, 0x34, 0x31, 0x33, 0x34, 0x31,  // 41241341
    0x34, 0x34, 0x31, 0x35, 0x34, 0x31, 0x36, 0x34,  // 44154164
    0x31, 0x37, 0x35, 0x30, 0x32, 0x35, 0x30, 0x34,  // 17502504
    0x35, 0x30, 0x35, 0x32, 0x30, 0x33, 0x20, 0x4e,  // 505203.N
    0x6f, 0x6e, 0x2d, 0x41, 0x75, 0x74, 0x68, 0x6f,  // on-Autho
    0x72, 0x69, 0x74, 0x61, 0x74, 0x69, 0x76, 0x65,  // ritative
    0x20, 0x49, 0x6e, 0x66, 0x6f, 0x72, 0x6d, 0x61,  // .Informa
    0x74, 0x69, 0x6f, 0x6e, 0x32, 0x30, 0x34, 0x20,  // tion204.
    0x4e, 0x6f, 0x20, 0x43, 0x6f, 0x6e, 0x74, 0x65,  // No.Conte
    0x6e, 0x74, 0x33, 0x30, 0x31, 0x20, 0x4d, 0x6f,  // nt301.Mo
    0x76, 0x65, 0x64, 0x20, 0x50, 0x65, 0x72, 0x6d,  // ved.Perm
    0x61, 0x6e, 0x65, 0x6e, 0x74, 0x6c, 0x79, 0x34,  // anently4
    0x30, 0x30, 0x20, 0x42, 0x61, 0x64, 0x20, 0x52,  // 00.Bad.R
    0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x34, 0x30,  // equest40
    0x31, 0x20, 0x55, 0x6e, 0x61, 0x75, 0x74, 0x68,  // 1.Unauth
    0x6f, 0x72, 0x69, 0x7a, 0x65, 0x64, 0x34, 0x30,  // orized40
    0x33, 0x20, 0x46, 0x6f, 0x72, 0x62, 0x69, 0x64,  // 3.Forbid
    0x64, 0x65, 0x6e, 0x34, 0x30, 0x34, 0x20, 0x4e,  // den404.N
    0x6f, 0x74, 0x20, 0x46, 0x6f, 0x75, 0x6e, 0x64,  // ot.Found
    0x35, 0x30, 0x30, 0x20, 0x49, 0x6e, 0x74, 0x65,  // 500.Inte
    0x72, 0x6e, 0x61, 0x6c, 0x20, 0x53, 0x65, 0x72,  // rnal.Ser
    0x76, 0x65, 0x72, 0x20, 0x45, 0x72, 0x72, 0x6f,  // ver.Erro
    0x72, 0x35, 0x30, 0x31, 0x20, 0x4e, 0x6f, 0x74,  // r501.Not
    0x20, 0x49, 0x6d, 0x70, 0x6c, 0x65, 0x6d, 0x65,  // .Impleme
    0x6e, 0x74, 0x65, 0x64, 0x35, 0x30, 0x33, 0x20,  // nted503.
    0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x20,  // Service.
    0x55, 0x6e, 0x61, 0x76, 0x61, 0x69, 0x6c, 0x61,  // Unavaila
    0x62, 0x6c, 0x65, 0x4a, 0x61, 0x6e, 0x20, 0x46,  // bleJan.F
    0x65, 0x62, 0x20, 0x4d, 0x61, 0x72, 0x20, 0x41,  // eb.Mar.A
    0x70, 0x72, 0x20, 0x4d, 0x61, 0x79, 0x20, 0x4a,  // pr.May.J
    0x75, 0x6e, 0x20, 0x4a, 0x75, 0x6c, 0x20, 0x41,  // un.Jul.A
    0x75, 0x67, 0x20, 0x53, 0x65, 0x70, 0x74, 0x20,  // ug.Sept.
    0x4f, 0x63, 0x74, 0x20, 0x4e, 0x6f, 0x76, 0x20,  // Oct.Nov.
    0x44, 0x65, 0x63, 0x20, 0x30, 0x30, 0x3a, 0x30,  // Dec.00.0
    0x30, 0x3a, 0x30, 0x30, 0x20, 0x4d, 0x6f, 0x6e,  // 0.00.Mon
    0x2c, 0x20, 0x54, 0x75, 0x65, 0x2c, 0x20, 0x57,  // ..Tue..W
    0x65, 0x64, 0x2c, 0x20, 0x54, 0x68, 0x75, 0x2c,  // ed..Thu.
    0x20, 0x46, 0x72, 0x69, 0x2c, 0x20, 0x53, 0x61,  // .Fri..Sa
    0x74, 0x2c, 0x20, 0x53, 0x75, 0x6e, 0x2c, 0x20,  // t..Sun..
    0x47, 0x4d, 0x54, 0x63, 0x68, 0x75, 0x6e, 0x6b,  // GMTchunk
    0x65, 0x64, 0x2c, 0x74, 0x65, 0x78, 0x74, 0x2f,  // ed.text.
    0x68, 0x74, 0x6d, 0x6c, 0x2c, 0x69, 0x6d, 0x61,  // html.ima
    0x67, 0x65, 0x2f, 0x70, 0x6e, 0x67, 0x2c, 0x69,  // ge.png.i
    0x6d, 0x61, 0x67, 0x65, 0x2f, 0x6a, 0x70, 0x67,  // mage.jpg
    0x2c, 0x69, 0x6d, 0x61, 0x67, 0x65, 0x2f, 0x67,  // .image.g
    0x69, 0x66, 0x2c, 0x61, 0x70, 0x70, 0x6c, 0x69,  // if.appli
    0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2f, 0x78,  // cation.x
    0x6d, 0x6c, 0x2c, 0x61, 0x70, 0x70, 0x6c, 0x69,  // ml.appli
    0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2f, 0x78,  // cation.x
    0x68, 0x74, 0x6d, 0x6c, 0x2b, 0x78, 0x6d, 0x6c,  // html.xml
    0x2c, 0x74, 0x65, 0x78, 0x74, 0x2f, 0x70, 0x6c,  // .text.pl
    0x61, 0x69, 0x6e, 0x2c, 0x74, 0x65, 0x78, 0x74,  // ain.text
    0x2f, 0x6a, 0x61, 0x76, 0x61, 0x73, 0x63, 0x72,  // .javascr
    0x69, 0x70, 0x74, 0x2c, 0x70, 0x75, 0x62, 0x6c,  // ipt.publ
    0x69, 0x63, 0x70, 0x72, 0x69, 0x76, 0x61, 0x74,  // icprivat
    0x65, 0x6d, 0x61, 0x78, 0x2d, 0x61, 0x67, 0x65,  // emax-age
    0x3d, 0x67, 0x7a, 0x69, 0x70, 0x2c, 0x64, 0x65,  // .gzip.de
    0x66, 0x6c, 0x61, 0x74, 0x65, 0x2c, 0x73, 0x64,  // flate.sd
    0x63, 0x68, 0x63, 0x68, 0x61, 0x72, 0x73, 0x65,  // chcharse
    0x74, 0x3d, 0x75, 0x74, 0x66, 0x2d, 0x38, 0x63,  // t.utf-8c
    0x68, 0x61, 0x72, 0x73, 0x65, 0x74, 0x3d, 0x69,  // harset.i
    0x73, 0x6f, 0x2d, 0x38, 0x38, 0x35, 0x39, 0x2d,  // so-8859-
    0x31, 0x2c, 0x75, 0x74, 0x66, 0x2d, 0x2c, 0x2a,  // 1.utf-..
    0x2c, 0x65, 0x6e, 0x71, 0x3d, 0x30, 0x2e         // .enq.0.
};

// uncomment to debug
//static void printHex(const QByteArray &ba)
//{
//    QByteArray hex;
//    QByteArray clearText;
//    for (int a = 0; a < ba.count(); ++a) {
//        QByteArray currentHexChar = QByteArray(1, ba.at(a)).toHex().rightJustified(2, ' ');
//        QByteArray currentChar;
//        if (ba.at(a) >= 32 && ba.at(a) < 126) { // if ASCII, print the letter
//            currentChar = QByteArray(1, ba.at(a));
//        } else {
//            currentChar = " ";
//        }
//        clearText.append(currentChar.rightJustified(2, ' '));
//        hex.append(currentHexChar);
//            hex.append(' ');
//            clearText.append(' ');
//    }
//    int chunkSize = 102; // 12 == 4 bytes per line
//    for (int a = 0; a < hex.count(); a += chunkSize) {
//        qDebug() << hex.mid(a, chunkSize);
//        qDebug() << clearText.mid(a, chunkSize);
//    }
//}

QSpdyProtocolHandler::QSpdyProtocolHandler(QHttpNetworkConnectionChannel *channel)
    : QObject(0), QAbstractProtocolHandler(channel),
      m_nextStreamID(-1),
      m_maxConcurrentStreams(100), // 100 is recommended in the SPDY RFC
      m_initialWindowSize(0),
      m_waitingForCompleteStream(false)
{
    m_inflateStream.zalloc = Z_NULL;
    m_inflateStream.zfree = Z_NULL;
    m_inflateStream.opaque = Z_NULL;
    int zlibRet = inflateInit(&m_inflateStream);
    Q_ASSERT(zlibRet == Z_OK);

    m_deflateStream.zalloc = Z_NULL;
    m_deflateStream.zfree = Z_NULL;
    m_deflateStream.opaque = Z_NULL;

    // Do actually not compress (i.e. compression level = 0)
    // when sending the headers because of the CRIME attack
    zlibRet = deflateInit(&m_deflateStream, /* compression level = */ 0);
    Q_ASSERT(zlibRet == Z_OK);
    Q_UNUSED(zlibRet); // silence -Wunused-variable
}

QSpdyProtocolHandler::~QSpdyProtocolHandler()
{
    deflateEnd(&m_deflateStream);
    deflateEnd(&m_inflateStream);
}

bool QSpdyProtocolHandler::sendRequest()
{
    Q_ASSERT(!m_reply);

    int maxPossibleRequests = m_maxConcurrentStreams - m_inFlightStreams.count();
    Q_ASSERT(maxPossibleRequests >= 0);
    if (maxPossibleRequests == 0)
        return true; // return early if max concurrent requests are exceeded

    m_channel->state = QHttpNetworkConnectionChannel::WritingState;

    // requests will be ordered by priority (see QMultiMap doc)
    QList<HttpMessagePair> requests = m_channel->spdyRequestsToSend.values();
    QList<int> priorities = m_channel->spdyRequestsToSend.keys();

    int requestsToSend = qMin(requests.count(), maxPossibleRequests);

    for (int a = 0; a < requestsToSend; ++a) {
        HttpMessagePair currentPair = requests.at(a);
        QHttpNetworkRequest currentRequest = requests.at(a).first;
        QHttpNetworkReply *currentReply = requests.at(a).second;

        currentReply->setSpdyWasUsed(true);
        qint32 streamID = generateNextStreamID();
        currentReply->setProperty("SPDYStreamID", streamID);

        currentReply->setRequest(currentRequest);
        currentReply->d_func()->connection = m_connection;
        currentReply->d_func()->connectionChannel = m_channel;
        m_inFlightStreams.insert(streamID, currentPair);
        connect(currentReply, SIGNAL(destroyed(QObject*)), this, SLOT(_q_replyDestroyed(QObject*)));

        sendSYN_STREAM(currentPair, streamID, /* associatedToStreamID = */ 0);
        int requestsRemoved = m_channel->spdyRequestsToSend.remove(
                    priorities.at(a), currentPair);
        Q_ASSERT(requestsRemoved == 1);
        Q_UNUSED(requestsRemoved); // silence -Wunused-variable
    }
    m_channel->state = QHttpNetworkConnectionChannel::IdleState;
    return true;
}

void QSpdyProtocolHandler::_q_replyDestroyed(QObject* reply)
{
    qint32 streamID = reply->property("SPDYStreamID").toInt();
    if (m_inFlightStreams.remove(streamID))
        sendRST_STREAM(streamID, RST_STREAM_CANCEL);
}

void QSpdyProtocolHandler::_q_receiveReply()
{
    Q_ASSERT(m_socket);

    // only run when the QHttpNetworkConnection is not currently being destructed, e.g.
    // this function is called from _q_disconnected which is called because
    // of ~QHttpNetworkConnectionPrivate
    if (!qobject_cast<QHttpNetworkConnection*>(m_connection)) {
        return;
    }

    if (bytesAvailable() < 8)
        return; // cannot read frame headers, wait for more data

    char frameHeadersRaw[8];
    if (!readNextChunk(8, frameHeadersRaw))
        return; // this should not happen, we just checked

    const QByteArray frameHeaders(frameHeadersRaw, 8); // ### try without memcpy
    if (frameHeadersRaw[0] & 0x80) {
        handleControlFrame(frameHeaders);
    } else {
        handleDataFrame(frameHeaders);
    }

    // after handling the current frame, check whether there is more data waiting
    if (m_socket->bytesAvailable() > 0)
        QMetaObject::invokeMethod(m_channel, "_q_receiveReply", Qt::QueuedConnection);
}

void QSpdyProtocolHandler::_q_readyRead()
{
    _q_receiveReply();
}

static qint16 twoBytesToInt(const char *bytes)
{
    return qFromBigEndian<qint16>(reinterpret_cast<const uchar *>(bytes));
}

static qint32 threeBytesToInt(const char *bytes)
{
    return qFromBigEndian<qint32>(reinterpret_cast<const uchar *>(bytes)) >> 8;
}

static qint32 fourBytesToInt(const char *bytes)
{
    return qFromBigEndian<qint32>(reinterpret_cast<const uchar *>(bytes));
}

static void appendIntToThreeBytes(char *output, qint32 number)
{
    qToBigEndian<qint16>(number, reinterpret_cast<uchar *>(output + 1));
    qToBigEndian<qint8>(number >> 16, reinterpret_cast<uchar *>(output));
}

static void appendIntToFourBytes(char *output, qint32 number)
{
    qToBigEndian<qint32>(number, reinterpret_cast<uchar *>(output));
}

static QByteArray intToFourBytes(qint32 number) // ### try to use appendIntToFourBytes where possible
{
    uchar data[4];
    qToBigEndian<qint32>(number, data);
    QByteArray ret(reinterpret_cast<char *>(data), 4);
    return ret;
}

static QByteArray intToThreeBytes(qint32 number)
{
    uchar data[4];
    qToBigEndian<qint32>(number << 8, data);
    QByteArray ret(reinterpret_cast<char *>(data), 3);
    return ret;
}

static qint32 getStreamID(const char *bytes)
{
    // eliminate most significant bit; it might be 0 or 1 depending on whether
    // we are dealing with a control or data frame
    return fourBytesToInt(bytes) & 0x3fffffff;
}

static QByteArray headerField(const QByteArray &name, const QByteArray &value)
{
    QByteArray ret;
    ret.reserve(name.count() + value.count() + 8); // 4 byte for length each
    ret.append(intToFourBytes(name.count()));
    ret.append(name);
    ret.append(intToFourBytes(value.count()));
    ret.append(value);
    return ret;
}

bool QSpdyProtocolHandler::uncompressHeader(const QByteArray &input, QByteArray *output)
{
    const size_t chunkSize = 1024;
    char outputRaw[chunkSize];
    // input bytes will not be changed by zlib, so it is safe to const_cast here
    m_inflateStream.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(input.constData()));
    m_inflateStream.avail_in = input.count();
    m_inflateStream.total_in = input.count();
    int zlibRet;

    do {
        m_inflateStream.next_out = reinterpret_cast<Bytef *>(outputRaw);
        m_inflateStream.avail_out = chunkSize;
        zlibRet = inflate(&m_inflateStream, Z_SYNC_FLUSH);
        if (zlibRet == Z_NEED_DICT) {
            zlibRet = inflateSetDictionary(&m_inflateStream,
                                           reinterpret_cast<const Bytef*>(spdyDictionary),
                                           /* dictionaryLength = */ 1423);
            Q_ASSERT(zlibRet == Z_OK);
            continue;
        }
        switch (zlibRet) {
        case Z_BUF_ERROR: {
            if (m_inflateStream.avail_in == 0) {
                int outputSize = chunkSize - m_inflateStream.avail_out;
                output->append(outputRaw, outputSize);
                m_inflateStream.avail_out = chunkSize;
            }
            break;
        }
        case Z_OK: {
            int outputSize = chunkSize - m_inflateStream.avail_out;
            output->append(outputRaw, outputSize);
            break;
        }
        default: {
            qWarning() << Q_FUNC_INFO << "got unexpected zlib return value:" << zlibRet;
            return false;
        }
        }
    } while (m_inflateStream.avail_in > 0 && zlibRet != Z_STREAM_END);

    Q_ASSERT(m_inflateStream.avail_in == 0);
    return true;
}

QByteArray QSpdyProtocolHandler::composeHeader(const QHttpNetworkRequest &request)
{
    QByteArray uncompressedHeader;
    uncompressedHeader.reserve(300); // rough estimate

    // calculate additional headers first, because we need to know the size
    // ### do not partially copy the list, but restrict the set header fields
    // in QHttpNetworkConnection
    QList<QPair<QByteArray, QByteArray> > additionalHeaders;
    for (int a = 0; a < request.header().count(); ++a) {
        QByteArray key = request.header().at(a).first;
        if (key == "Connection" || key == "Host" || key == "Keep-Alive"
                || key == "Proxy-Connection" || key == "Transfer-Encoding")
            continue; // those headers are not valid (section 3.2.1)
        additionalHeaders.append(request.header().at(a));
    }

    qint32 numberOfHeaderPairs = 5 + additionalHeaders.count(); // 5 mandatory below + the additional ones
    uncompressedHeader.append(intToFourBytes(numberOfHeaderPairs));

    // mandatory header fields:

    uncompressedHeader.append(headerField(":method", request.methodName()));
#ifndef QT_NO_NETWORKPROXY
    bool useProxy = m_connection->d_func()->networkProxy.type() != QNetworkProxy::NoProxy;
    uncompressedHeader.append(headerField(":path", request.uri(useProxy)));
#else
    uncompressedHeader.append(headerField(":path", request.uri(false)));
#endif
    uncompressedHeader.append(headerField(":version", "HTTP/1.1"));

    QHostAddress add; // ### unify with the host parsing from QHttpNetworkConnection
    QByteArray host;
    QString hostName = m_connection->hostName();
    if (add.setAddress(hostName)) {
        if (add.protocol() == QAbstractSocket::IPv6Protocol)
            host = "[" + hostName.toLatin1() + "]"; //format the ipv6 in the standard way
        else
            host = hostName.toLatin1();

    } else {
        host = QUrl::toAce(hostName);
    }

    int port = request.url().port();
    if (port != -1) {
        host += ':';
        host += QByteArray::number(port);
    }
    uncompressedHeader.append(headerField(":host", host));

    uncompressedHeader.append(headerField(":scheme", request.url().scheme().toLatin1()));

    // end of mandatory header fields

    // now add the additional headers
    for (int a = 0; a < additionalHeaders.count(); ++a) {
        uncompressedHeader.append(headerField(additionalHeaders.at(a).first.toLower(),
                                              additionalHeaders.at(a).second));
    }

    m_deflateStream.total_in = uncompressedHeader.count();
    m_deflateStream.avail_in = uncompressedHeader.count();
    m_deflateStream.next_in = reinterpret_cast<unsigned char *>(uncompressedHeader.data());
    int outputBytes = uncompressedHeader.count() + 30; // 30 bytes of compression header overhead
    m_deflateStream.avail_out = outputBytes;
    unsigned char *out = new unsigned char[outputBytes];
    m_deflateStream.next_out = out;
    int availOutBefore = m_deflateStream.avail_out;
    int zlibRet = deflate(&m_deflateStream, Z_SYNC_FLUSH); // do everything in one go since we use no compression
    int compressedHeaderSize = availOutBefore - m_deflateStream.avail_out;
    Q_ASSERT(zlibRet == Z_OK); // otherwise, we need to allocate more outputBytes
    Q_UNUSED(zlibRet); // silence -Wunused-variable
    Q_ASSERT(m_deflateStream.avail_in == 0);
    QByteArray compressedHeader(reinterpret_cast<char *>(out), compressedHeaderSize);
    delete[] out;

    return compressedHeader;
}

quint64 QSpdyProtocolHandler::bytesAvailable() const
{
    Q_ASSERT(m_socket);
    return m_spdyBuffer.byteAmount() + m_socket->bytesAvailable();
}

bool QSpdyProtocolHandler::readNextChunk(qint64 length, char *sink)
{
    qint64 expectedReadBytes = length;
    qint64 requiredBytesFromBuffer = 0;

    if (m_waitingForCompleteStream) {
        requiredBytesFromBuffer = qMin(length, m_spdyBuffer.byteAmount());
        // ### if next chunk from buffer bigger than what we want to read,
        // we have to call read() (which memcpy's). Otherwise, we can just
        // read the next chunk without memcpy'ing.
        qint64 bytesReadFromBuffer = m_spdyBuffer.read(sink, requiredBytesFromBuffer);
        Q_ASSERT(bytesReadFromBuffer == requiredBytesFromBuffer);
        if (length <= bytesReadFromBuffer) {
            return true; // buffer > required size -> no need to read from socket
        }
        expectedReadBytes -= requiredBytesFromBuffer;
    }
    qint64 readBytes = m_socket->read(sink + requiredBytesFromBuffer, expectedReadBytes);

    if (readBytes < expectedReadBytes) {
        m_waitingForCompleteStream = true;
        // ### this is inefficient, we should not put back so much data into the buffer
        QByteArray temp(sink, requiredBytesFromBuffer + readBytes);
        m_spdyBuffer.append(temp);
        return false;
    } else {
        return true; // buffer must be cleared by calling function
    }
}

void QSpdyProtocolHandler::sendControlFrame(FrameType type,
                                            ControlFrameFlags flags,
                                            const char *data,
                                            quint32 length)
{
    // frame type and stream ID
    char header[8];
    header[0] = 0x80; // leftmost bit == 1 -> is a control frame
    header[1] = 0x03; // 3 bit == version 3
    header[2] = 0;
    switch (type) {
    case FrameType_CREDENTIAL: {
        qWarning("sending SPDY CREDENTIAL frame is not yet implemented"); // QTBUG-36188
        return;
    }
    default:
        header[3] = type;
    }

    // flags
    header[4] = 0;
    if (flags & ControlFrame_FLAG_FIN || length == 0) {
        Q_ASSERT(type == FrameType_SYN_STREAM || type == FrameType_SYN_REPLY
                 || type == FrameType_HEADERS || length == 0);
        header[4] |= ControlFrame_FLAG_FIN;
    }
    if (flags & ControlFrame_FLAG_UNIDIRECTIONAL) {
        Q_ASSERT(type == FrameType_SYN_STREAM);
        header[4] |= ControlFrame_FLAG_UNIDIRECTIONAL;
    }

    // length
    appendIntToThreeBytes(header + 5, length);

    qint64 written = m_socket->write(header, 8);
    Q_ASSERT(written == 8);
    written = m_socket->write(data, length);
    Q_ASSERT(written == length);
    Q_UNUSED(written); // silence -Wunused-variable
}

void QSpdyProtocolHandler::sendSYN_STREAM(HttpMessagePair messagePair,
                                          qint32 streamID, qint32 associatedToStreamID)
{
    QHttpNetworkRequest request = messagePair.first;
    QHttpNetworkReply *reply = messagePair.second;

    ControlFrameFlags flags = 0;

    if (!request.uploadByteDevice()) {
        // no upload -> this is the last frame, send the FIN flag
        flags |= ControlFrame_FLAG_FIN;
        reply->d_func()->state = QHttpNetworkReplyPrivate::SPDYHalfClosed;
    } else {
        reply->d_func()->state = QHttpNetworkReplyPrivate::SPDYUploading;

        // hack: set the stream ID on the device directly, so when we get
        // the signal for uploading we know which stream we are sending on
        request.uploadByteDevice()->setProperty("SPDYStreamID", streamID);

        QObject::connect(request.uploadByteDevice(), SIGNAL(readyRead()), this,
                         SLOT(_q_uploadDataReadyRead()), Qt::QueuedConnection);
    }

    QByteArray namesAndValues = composeHeader(request);
    quint32 length = namesAndValues.count() + 10; // 10 == 4 for Stream-ID + 4 for Associated-To-Stream-ID
    // + 2 for Priority, Unused and Slot

    QByteArray wireData;
    wireData.reserve(length);
    wireData.append(intToFourBytes(streamID));
    wireData.append(intToFourBytes(associatedToStreamID));

    // priority (3 bits) / unused (5 bits) / slot (8 bits)
    char prioAndSlot[2];
    switch (request.priority()) {
    case QHttpNetworkRequest::HighPriority:
        prioAndSlot[0] = 0x00; // == prio 0 (highest)
        break;
    case QHttpNetworkRequest::NormalPriority:
        prioAndSlot[0] = 0x80; // == prio 4
        break;
    case QHttpNetworkRequest::LowPriority:
        prioAndSlot[0] = 0xe0; // == prio 7 (lowest)
        break;
    }
    prioAndSlot[1] = 0x00; // slot in client certificates (not supported currently)
    wireData.append(prioAndSlot, 2);

    wireData.append(namesAndValues);

    sendControlFrame(FrameType_SYN_STREAM, flags, wireData.constData(), length);

    if (reply->d_func()->state == QHttpNetworkReplyPrivate::SPDYUploading)
        uploadData(streamID);
}

void QSpdyProtocolHandler::sendRST_STREAM(qint32 streamID, RST_STREAM_STATUS_CODE statusCode)
{
    char wireData[8];
    appendIntToFourBytes(wireData, streamID);
    appendIntToFourBytes(wireData + 4, statusCode);
    sendControlFrame(FrameType_RST_STREAM, /* flags = */ 0, wireData, /* length = */ 8);
}

void QSpdyProtocolHandler::sendPING(quint32 pingID)
{
    char rawData[4];
    appendIntToFourBytes(rawData, pingID);
    sendControlFrame(FrameType_PING, /* flags = */ 0, rawData, /* length = */ 4);
}

bool QSpdyProtocolHandler::uploadData(qint32 streamID)
{
    // we only rely on SPDY flow control here and don't care about TCP buffers
    if (!m_inFlightStreams.contains(streamID)) {
        sendRST_STREAM(streamID, RST_STREAM_INVALID_STREAM);
        return false;
    }

    HttpMessagePair messagePair = m_inFlightStreams.value(streamID);
    QHttpNetworkRequest request = messagePair.first;
    QHttpNetworkReply *reply = messagePair.second;
    Q_ASSERT(reply);
    QHttpNetworkReplyPrivate *replyPrivate = reply->d_func();
    Q_ASSERT(replyPrivate);

    if (reply->d_func()->state == QHttpNetworkReplyPrivate::SPDYHalfClosed || reply->d_func()->state == QHttpNetworkReplyPrivate::SPDYClosed) {
        qWarning() << Q_FUNC_INFO << "Trying to upload to closed stream";
        return false;
    }

    qint32 dataLeftInWindow = replyPrivate->windowSizeUpload
            - replyPrivate->currentlyUploadedDataInWindow;

    while (dataLeftInWindow > 0 && !request.uploadByteDevice()->atEnd()) {

        // get pointer to upload data
        qint64 currentReadSize = 0;
        const char *readPointer = request.uploadByteDevice()->readPointer(dataLeftInWindow,
                                                                          currentReadSize);

        if (currentReadSize == -1) {
            // premature eof happened
            m_connection->d_func()->emitReplyError(m_socket, reply,
                                                   QNetworkReply::UnknownNetworkError);
            return false;
        } else if (readPointer == 0 || currentReadSize == 0) {
            // nothing to read currently, break the loop
            break;
        } else {
            DataFrameFlags flags = 0;
            // we will send the FIN flag later if appropriate
            qint64 currentWriteSize = sendDataFrame(streamID, flags, currentReadSize, readPointer);
            if (currentWriteSize == -1 || currentWriteSize != currentReadSize) {
                // socket broke down
                m_connection->d_func()->emitReplyError(m_socket, reply,
                                                       QNetworkReply::UnknownNetworkError);
                return false;
            } else {
                replyPrivate->currentlyUploadedDataInWindow += currentWriteSize;
                replyPrivate->totallyUploadedData += currentWriteSize;
                dataLeftInWindow = replyPrivate->windowSizeUpload
                        - replyPrivate->currentlyUploadedDataInWindow;
                request.uploadByteDevice()->advanceReadPointer(currentWriteSize);

                emit reply->dataSendProgress(replyPrivate->totallyUploadedData,
                                             request.contentLength());
            }
        }
    }
    if (replyPrivate->totallyUploadedData == request.contentLength()) {
        DataFrameFlags finFlag = DataFrame_FLAG_FIN;
        qint64 writeSize = sendDataFrame(streamID, finFlag, 0, 0);
        Q_ASSERT(writeSize == 0);
        Q_UNUSED(writeSize); // silence -Wunused-variable
        replyPrivate->state = QHttpNetworkReplyPrivate::SPDYHalfClosed;
        if (reply->request().uploadByteDevice())
            reply->request().uploadByteDevice()->disconnect(this);
        // ### this will not work if the content length is not known, but
        // then again many servers will fail in this case anyhow according
        // to the SPDY RFC
    }
    return true;
}

void QSpdyProtocolHandler::_q_uploadDataReadyRead()
{
    QNonContiguousByteDevice *device = qobject_cast<QNonContiguousByteDevice *>(sender());
    Q_ASSERT(device);
    qint32 streamID = device->property("SPDYStreamID").toInt();
    Q_ASSERT(streamID > 0);
    uploadData(streamID);
}

void QSpdyProtocolHandler::sendWINDOW_UPDATE(qint32 streamID, quint32 deltaWindowSize)
{
    char windowUpdateData[8];
    appendIntToFourBytes(windowUpdateData, streamID);
    appendIntToFourBytes(windowUpdateData + 4, deltaWindowSize);

    sendControlFrame(FrameType_WINDOW_UPDATE, /* flags = */ 0, windowUpdateData, /* length = */ 8);
}

qint64 QSpdyProtocolHandler::sendDataFrame(qint32 streamID, DataFrameFlags flags,
                                           quint32 length, const char *data)
{
    QByteArray wireData;
    wireData.reserve(8);

    wireData.append(intToFourBytes(streamID));
    wireData.append(flags);
    wireData.append(intToThreeBytes(length));

    Q_ASSERT(m_socket);
    m_socket->write(wireData);

    if (data) {
        qint64 ret = m_socket->write(data, length);
        return ret;
    } else {
        return 0; // nothing to write, e.g. FIN flag
    }
}

void QSpdyProtocolHandler::handleControlFrame(const QByteArray &frameHeaders) // ### make it char *
{
    Q_ASSERT(frameHeaders.count() >= 8);
    qint16 version = twoBytesToInt(frameHeaders.constData());
    version &= 0x3fff; // eliminate most significant bit to determine version
    Q_ASSERT(version == 3);

    qint16 type = twoBytesToInt(frameHeaders.constData() + 2);

    char flags = frameHeaders.at(4);
    qint32 length = threeBytesToInt(frameHeaders.constData() + 5);
    Q_ASSERT(length > 0);

    QByteArray frameData;
    frameData.resize(length);
    if (!readNextChunk(length, frameData.data())) {
        // put back the frame headers to the buffer
        m_spdyBuffer.prepend(frameHeaders);
        return; // we couldn't read the whole frame and need to wait
    } else {
        m_spdyBuffer.clear();
        m_waitingForCompleteStream = false;
    }

    switch (type) {
    case FrameType_SYN_STREAM: {
        handleSYN_STREAM(flags, length, frameData);
        break;
    }
    case FrameType_SYN_REPLY: {
        handleSYN_REPLY(flags, length, frameData);
        break;
    }
    case FrameType_RST_STREAM: {
        handleRST_STREAM(flags, length, frameData);
        break;
    }
    case FrameType_SETTINGS: {
        handleSETTINGS(flags, length, frameData);
        break;
    }
    case FrameType_PING: {
        handlePING(flags, length, frameData);
        break;
    }
    case FrameType_GOAWAY: {
        handleGOAWAY(flags, length, frameData);
        break;
    }
    case FrameType_HEADERS: {
        handleHEADERS(flags, length, frameData);
        break;
    }
    case FrameType_WINDOW_UPDATE: {
        handleWINDOW_UPDATE(flags, length, frameData);
        break;
    }
    default:
        qWarning() << Q_FUNC_INFO << "cannot handle frame of type" << type;
    }
}

void QSpdyProtocolHandler::handleSYN_STREAM(char /*flags*/, quint32 /*length*/,
                                            const QByteArray &frameData)
{
    // not implemented; will be implemented when servers start using it
    // we just tell the server that we do not accept that

    qint32 streamID = getStreamID(frameData.constData());

    sendRST_STREAM(streamID, RST_STREAM_REFUSED_STREAM);
}

void QSpdyProtocolHandler::handleSYN_REPLY(char flags, quint32 /*length*/, const QByteArray &frameData)
{
    parseHttpHeaders(flags, frameData);
}

void QSpdyProtocolHandler::parseHttpHeaders(char flags, const QByteArray &frameData)
{
    qint32 streamID = getStreamID(frameData.constData());
    if (!m_inFlightStreams.contains(streamID)) {
        sendRST_STREAM(streamID, RST_STREAM_INVALID_STREAM);
        return;
    }

    flags &= 0x3f;
    bool flag_fin = flags & 0x01;

    QByteArray headerValuePairs = frameData.mid(4);

    HttpMessagePair pair = m_inFlightStreams.value(streamID);
    QHttpNetworkReply *httpReply = pair.second;
    Q_ASSERT(httpReply != 0);

    if (httpReply->d_func()->state == QHttpNetworkReplyPrivate::SPDYClosed) {
        sendRST_STREAM(streamID, RST_STREAM_STREAM_ALREADY_CLOSED);
        return;
    }

    QByteArray uncompressedHeader;
    if (!uncompressHeader(headerValuePairs, &uncompressedHeader)) {
        qWarning() << Q_FUNC_INFO << "error reading header from SYN_REPLY message";
        return;
    }

    qint32 headerCount = fourBytesToInt(uncompressedHeader.constData());
    if (headerCount * 8 > uncompressedHeader.size()) {
        qWarning() << Q_FUNC_INFO << "error parsing header from SYN_REPLY message";
        sendRST_STREAM(streamID, RST_STREAM_PROTOCOL_ERROR);
        return;
    }
    qint32 readPointer = 4;
    for (qint32 a = 0; a < headerCount; ++a) {
        qint32 count = fourBytesToInt(uncompressedHeader.constData() + readPointer);
        readPointer += 4;
        QByteArray name = uncompressedHeader.mid(readPointer, count);
        readPointer += count;
        if (readPointer > uncompressedHeader.size()) {
            qWarning() << Q_FUNC_INFO << "error parsing header from SYN_REPLY message";
            sendRST_STREAM(streamID, RST_STREAM_PROTOCOL_ERROR);
            return;
        }
        count = fourBytesToInt(uncompressedHeader.constData() + readPointer);
        readPointer += 4;
        QByteArray value = uncompressedHeader.mid(readPointer, count);
        readPointer += count;
        if (readPointer > uncompressedHeader.size()) {
            qWarning() << Q_FUNC_INFO << "error parsing header from SYN_REPLY message";
            sendRST_STREAM(streamID, RST_STREAM_PROTOCOL_ERROR);
            return;
        }
        if (name == ":status") {
            httpReply->setStatusCode(value.left(3).toInt());
            httpReply->d_func()->reasonPhrase = QString::fromLatin1(value.mid(4));
        } else if (name == ":version") {
            int majorVersion = value.at(5) - 48;
            int minorVersion = value.at(7) - 48;
            httpReply->d_func()->majorVersion = majorVersion;
            httpReply->d_func()->minorVersion = minorVersion;
        } else if (name == "content-length") {
            httpReply->setContentLength(value.toLongLong());
        } else {
            if (value.contains('\0')) {
                QList<QByteArray> values = value.split('\0');
                QByteArray binder(", ");
                if (name == "set-cookie")
                    binder = "\n";
                value.clear();
                Q_FOREACH (const QByteArray& ivalue, values) {
                    if (value.isEmpty())
                        value = ivalue;
                    else
                        value += binder + ivalue;
                }
            }
            httpReply->setHeaderField(name, value);
        }
    }
    emit httpReply->headerChanged();

    if (flag_fin) {
        if (httpReply->d_func()->state != QHttpNetworkReplyPrivate::SPDYHalfClosed)
            sendDataFrame(streamID, DataFrame_FLAG_FIN, 0, 0);
        replyFinished(httpReply, streamID);
    }
}

void QSpdyProtocolHandler::handleRST_STREAM(char /*flags*/, quint32 length,
                                            const QByteArray &frameData)
{
    // flags are ignored

    Q_ASSERT(length == 8);
    Q_UNUSED(length); // silence -Wunused-parameter
    qint32 streamID = getStreamID(frameData.constData());
    QHttpNetworkReply *httpReply = m_inFlightStreams.value(streamID).second;

    qint32 statusCodeInt = fourBytesToInt(frameData.constData() + 4);
    RST_STREAM_STATUS_CODE statusCode = static_cast<RST_STREAM_STATUS_CODE>(statusCodeInt);
    QNetworkReply::NetworkError errorCode;
    QByteArray errorMessage;

    switch (statusCode) {
    case RST_STREAM_PROTOCOL_ERROR:
        errorCode = QNetworkReply::ProtocolFailure;
        errorMessage = "SPDY protocol error";
        break;
    case RST_STREAM_INVALID_STREAM:
        errorCode = QNetworkReply::ProtocolFailure;
        errorMessage = "SPDY stream is not active";
        break;
    case RST_STREAM_REFUSED_STREAM:
        errorCode = QNetworkReply::ProtocolFailure;
        errorMessage = "SPDY stream was refused";
        break;
    case RST_STREAM_UNSUPPORTED_VERSION:
        errorCode = QNetworkReply::ProtocolUnknownError;
        errorMessage = "SPDY version is unknown to the server";
        break;
    case RST_STREAM_CANCEL:
        errorCode = QNetworkReply::ProtocolFailure;
        errorMessage = "SPDY stream is no longer needed";
        break;
    case RST_STREAM_INTERNAL_ERROR:
        errorCode = QNetworkReply::InternalServerError;
        errorMessage = "Internal server error";
        break;
    case RST_STREAM_FLOW_CONTROL_ERROR:
        errorCode = QNetworkReply::ProtocolFailure;
        errorMessage = "peer violated the flow control protocol";
        break;
    case RST_STREAM_STREAM_IN_USE:
        errorCode = QNetworkReply::ProtocolFailure;
        errorMessage = "server received a SYN_REPLY for an already open stream";
        break;
    case RST_STREAM_STREAM_ALREADY_CLOSED:
        errorCode = QNetworkReply::ProtocolFailure;
        errorMessage = "server received data or a SYN_REPLY for an already half-closed stream";
        break;
    case RST_STREAM_INVALID_CREDENTIALS:
        errorCode = QNetworkReply::ContentAccessDenied;
        errorMessage = "server received invalid credentials";
        break;
    case RST_STREAM_FRAME_TOO_LARGE:
        errorCode = QNetworkReply::ProtocolFailure;
        errorMessage = "server cannot process the frame because it is too large";
        break;
    default:
        qWarning() << Q_FUNC_INFO << "could not understand servers RST_STREAM status code";
        errorCode = QNetworkReply::ProtocolFailure;
        errorMessage = "got SPDY RST_STREAM message with unknown error code";
    }
    if (httpReply)
        replyFinishedWithError(httpReply, streamID, errorCode, errorMessage.constData());
}

void QSpdyProtocolHandler::handleSETTINGS(char flags, quint32 /*length*/, const QByteArray &frameData)
{
    Q_ASSERT(frameData.count() > 0);

    SETTINGS_Flags settingsFlags = static_cast<SETTINGS_Flags>(flags);
    if (settingsFlags & FLAG_SETTINGS_CLEAR_SETTINGS) {
        // ### clear all persistent settings; since we do not persist settings
        // as of now, we don't need to clear anything either
    }

    qint32 numberOfEntries = fourBytesToInt(frameData.constData());
    Q_ASSERT(numberOfEntries > 0);
    for (int a = 0, frameDataIndex = 4; a < numberOfEntries; ++a, frameDataIndex += 8) {
        SETTINGS_ID_Flag idFlag = static_cast<SETTINGS_ID_Flag>(frameData[frameDataIndex]);
        if (idFlag & FLAG_SETTINGS_PERSIST_VALUE) {
            // ### we SHOULD persist the settings here according to the RFC, but we don't have to,
            // so implement that later
        } // the other value is only sent by us, but not received

        quint32 uniqueID = static_cast<SETTINGS_ID>(
                    threeBytesToInt(frameData.constData() + frameDataIndex + 1));
        quint32 value = fourBytesToInt(frameData.constData() + frameDataIndex + 4);
        switch (uniqueID) {
        case SETTINGS_UPLOAD_BANDWIDTH: {
            // ignored for now, just an estimated informative value
            break;
        }
        case SETTINGS_DOWNLOAD_BANDWIDTH: {
            // ignored for now, just an estimated informative value
            break;
        }
        case SETTINGS_ROUND_TRIP_TIME: {
            // ignored for now, just an estimated informative value
            break;
        }
        case SETTINGS_MAX_CONCURRENT_STREAMS: {
            m_maxConcurrentStreams = value;
            break;
        }
        case SETTINGS_CURRENT_CWND: {
            // ignored for now, just an informative value
            break;
        }
        case SETTINGS_DOWNLOAD_RETRANS_RATE: {
            // ignored for now, just an estimated informative value
            break;
        }
        case SETTINGS_INITIAL_WINDOW_SIZE: {
            m_initialWindowSize = value;
            break;
        }
        case SETTINGS_CLIENT_CERTIFICATE_VECTOR_SIZE: {
            // client certificates are not supported
            break;
        }
        default:
            qWarning() << Q_FUNC_INFO << "found unknown settings value" << value;
        }
    }
}

void QSpdyProtocolHandler::handlePING(char /*flags*/, quint32 length, const QByteArray &frameData)
{
    // flags are ignored

    Q_ASSERT(length == 4);
    Q_UNUSED(length); // silence -Wunused-parameter
    quint32 pingID = fourBytesToInt(frameData.constData());

    // odd numbered IDs must be ignored
    if ((pingID & 1) == 0) // is even?
        sendPING(pingID);
}

void QSpdyProtocolHandler::handleGOAWAY(char /*flags*/, quint32 /*length*/,
                                        const QByteArray &frameData)
{
    // flags are ignored

    qint32 statusCode = static_cast<GOAWAY_STATUS>(fourBytesToInt(frameData.constData() + 4));
    QNetworkReply::NetworkError errorCode;
    switch (statusCode) {
    case GOAWAY_OK: {
        errorCode = QNetworkReply::NoError;
        break;
    }
    case GOAWAY_PROTOCOL_ERROR: {
        errorCode = QNetworkReply::ProtocolFailure;
        break;
    }
    case GOAWAY_INTERNAL_ERROR: {
        errorCode = QNetworkReply::InternalServerError;
        break;
    }
    default:
        qWarning() << Q_FUNC_INFO << "unexpected status code" << statusCode;
        errorCode = QNetworkReply::ProtocolUnknownError;
    }

    qint32 lastGoodStreamID = getStreamID(frameData.constData());

    // emit errors for all replies after the last good stream ID
    Q_ASSERT(m_connection);
    for (qint32 currentStreamID = lastGoodStreamID + 2; currentStreamID <= m_nextStreamID;
         ++currentStreamID) {
        QHttpNetworkReply *reply = m_inFlightStreams.value(currentStreamID).second;
        Q_ASSERT(reply);
        m_connection->d_func()->emitReplyError(m_socket, reply, errorCode);
    }
    // ### we could make sure a new session is initiated anyhow
}

void QSpdyProtocolHandler::handleHEADERS(char flags, quint32 /*length*/,
                                         const QByteArray &frameData)
{
    parseHttpHeaders(flags, frameData);
}

void QSpdyProtocolHandler::handleWINDOW_UPDATE(char /*flags*/, quint32 /*length*/,
                                               const QByteArray &frameData)
{
    qint32 streamID = getStreamID(frameData.constData());
    qint32 deltaWindowSize = fourBytesToInt(frameData.constData() + 4);

    if (!m_inFlightStreams.contains(streamID)) {
        sendRST_STREAM(streamID, RST_STREAM_INVALID_STREAM);
        return;
    }

    QHttpNetworkReply *reply = m_inFlightStreams.value(streamID).second;
    Q_ASSERT(reply);
    QHttpNetworkReplyPrivate *replyPrivate = reply->d_func();
    Q_ASSERT(replyPrivate);

    // Ignore WINDOW_UPDATE if we are already done.
    if (replyPrivate->state == QHttpNetworkReplyPrivate::SPDYHalfClosed || replyPrivate->state == QHttpNetworkReplyPrivate::SPDYClosed)
        return;

    replyPrivate->currentlyUploadedDataInWindow = replyPrivate->windowSizeUpload - deltaWindowSize;
    uploadData(streamID); // we hopefully can continue to upload
}


void QSpdyProtocolHandler::handleDataFrame(const QByteArray &frameHeaders)
{
    Q_ASSERT(frameHeaders.count() >= 8);

    qint32 streamID = getStreamID(frameHeaders.constData());
    if (!m_inFlightStreams.contains(streamID)) {
        sendRST_STREAM(streamID, RST_STREAM_INVALID_STREAM);
        return;
    }

    unsigned char flags = static_cast<unsigned char>(frameHeaders.at(4));
    flags &= 0x3f;
    bool flag_fin = flags & 0x01;
    bool flag_compress = flags & 0x02;
    qint32 length = threeBytesToInt(frameHeaders.constData() + 5);

    QByteArray data;
    data.resize(length);
    if (!readNextChunk(length, data.data())) {
        // put back the frame headers to the buffer
        m_spdyBuffer.prepend(frameHeaders);
        return; // we couldn't read the whole frame and need to wait
    } else {
        m_spdyBuffer.clear();
        m_waitingForCompleteStream = false;
    }

    HttpMessagePair pair = m_inFlightStreams.value(streamID);
    QHttpNetworkRequest httpRequest = pair.first;
    QHttpNetworkReply *httpReply = pair.second;
    Q_ASSERT(httpReply != 0);

    QHttpNetworkReplyPrivate *replyPrivate = httpReply->d_func();

    if (replyPrivate->state == QHttpNetworkReplyPrivate::SPDYClosed) {
        sendRST_STREAM(streamID, RST_STREAM_STREAM_ALREADY_CLOSED);
        return;
    }

    // check whether we need to send WINDOW_UPDATE (i.e. tell the sender it can send more)
    replyPrivate->currentlyReceivedDataInWindow += length;
    qint32 dataLeftInWindow = replyPrivate->windowSizeDownload - replyPrivate->currentlyReceivedDataInWindow;

    if (replyPrivate->currentlyReceivedDataInWindow > 0
            && dataLeftInWindow < replyPrivate->windowSizeDownload / 2) {

        // socket read buffer size is 64K actually, hard coded in the channel
        // We can read way more than 64K per socket, because the window size
        // here is per stream.
        if (replyPrivate->windowSizeDownload >= m_socket->readBufferSize()) {
            replyPrivate->windowSizeDownload = m_socket->readBufferSize();
        } else {
            replyPrivate->windowSizeDownload *= 1.5;
        }
        QMetaObject::invokeMethod(this, "sendWINDOW_UPDATE", Qt::QueuedConnection,
                                  Q_ARG(qint32, streamID),
                                  Q_ARG(quint32, replyPrivate->windowSizeDownload));
        // setting the current data count to 0 is a race condition,
        // because we call sendWINDOW_UPDATE through the event loop.
        // But then again, the whole situation is a race condition because
        // we don't know when the packet will arrive at the server; so
        // this is most likely good enough here.
        replyPrivate->currentlyReceivedDataInWindow = 0;
    }

    httpReply->d_func()->compressedData.append(data);


    replyPrivate->totalProgress += length;

    if (httpRequest.d->autoDecompress && httpReply->d_func()->isCompressed()) {
        QByteDataBuffer inDataBuffer; // ### should we introduce one in the http reply?
        inDataBuffer.append(data);
        qint64 compressedCount = httpReply->d_func()->uncompressBodyData(&inDataBuffer,
                                                                         &replyPrivate->responseData);
        Q_ASSERT(compressedCount >= 0);
        Q_UNUSED(compressedCount); // silence -Wunused-variable
    } else {
        replyPrivate->responseData.append(data);
    }

    if (replyPrivate->shouldEmitSignals()) {
        emit httpReply->readyRead();
        emit httpReply->dataReadProgress(replyPrivate->totalProgress, replyPrivate->bodyLength);
    }

    if (flag_compress) {
        qWarning() << Q_FUNC_INFO << "SPDY level compression is not supported";
    }

    if (flag_fin) {
        if (httpReply->d_func()->state != QHttpNetworkReplyPrivate::SPDYHalfClosed)
            sendDataFrame(streamID, DataFrame_FLAG_FIN, 0, 0);
        replyFinished(httpReply, streamID);
    }
}

void QSpdyProtocolHandler::replyFinished(QHttpNetworkReply *httpReply, qint32 streamID)
{
    httpReply->d_func()->state = QHttpNetworkReplyPrivate::SPDYClosed;
    httpReply->disconnect(this);
    if (httpReply->request().uploadByteDevice())
        httpReply->request().uploadByteDevice()->disconnect(this);
    int streamsRemoved = m_inFlightStreams.remove(streamID);
    Q_ASSERT(streamsRemoved == 1);
    Q_UNUSED(streamsRemoved); // silence -Wunused-variable
    emit httpReply->finished();
}

void QSpdyProtocolHandler::replyFinishedWithError(QHttpNetworkReply *httpReply, qint32 streamID,
                                                  QNetworkReply::NetworkError errorCode, const char *errorMessage)
{
    Q_ASSERT(httpReply);
    httpReply->d_func()->state = QHttpNetworkReplyPrivate::SPDYClosed;
    httpReply->disconnect(this);
    if (httpReply->request().uploadByteDevice())
        httpReply->request().uploadByteDevice()->disconnect(this);
    int streamsRemoved = m_inFlightStreams.remove(streamID);
    Q_ASSERT(streamsRemoved == 1);
    Q_UNUSED(streamsRemoved); // silence -Wunused-variable
    emit httpReply->finishedWithError(errorCode, QSpdyProtocolHandler::tr(errorMessage));
}

qint32 QSpdyProtocolHandler::generateNextStreamID()
{
    // stream IDs initiated by the client must be odd
    m_nextStreamID += 2;
    return m_nextStreamID;
}

QT_END_NAMESPACE

#endif // !defined(QT_NO_HTTP) && !defined(QT_NO_SSL)
