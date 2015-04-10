/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

static String writeHeader(const String& title)
{
    return "<!DOCTYPE html><html>"
        "<head>"
        "    <style>.title{text-align:center;color:white;font-size:28pt;}.box{padding:10px;border:2px solid gray;margin:0px;background:black;color:white;-webkit-border-radius: 10px;}.box-title{text-align:center;font-weight:bold;}.true {color:green;}.false {color: red;text-decoration: line-through;}.fixed-table{color:white;border-collapse:collapse;width:100%} tr:nth-child(2n){color:#A8A8A8;}</style>"
        "    <title>"+title+"</title>"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, maximum-scale=1\">"
        "    <style>@media all and (orientation:landscape) { .main { -webkit-column-count:2; -webkit-column-rule:solid; font-size:12px; } h1 { -webkit-column-span: all; } ul { font-size: 75%; } } td,li { text-overflow: ellipsis; overflow: hidden; }</style>"
        "</head>"
        "<body topmargin='10'>"
        "    <div class='box'><div class='title'><img alt='BlackBerry Browser Logo' src='data:image/png;base64,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' /> "+title+"</div></div><br>"
        "    <div class='main'>";
}
