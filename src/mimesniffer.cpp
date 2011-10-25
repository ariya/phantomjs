/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Erik Dubbelboer <erik@dubbelboer.com>

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


#include "mimesniffer.h"


#define CHECK_START(magic, type) if (checkStart(data, magic)) { return type; }
#define CHECK_CONTAIN(magic, type) if (ldata.contains(magic) != NULL) { return type; }



static bool checkStart(const QString data, const QString magic)
{
    for (int i = 0; i < magic.length(); ++i)
    {
        if (magic[i] == '.')
        {
            continue;
        }

        if (data[i] != magic[i])
        {
            return false;
        }
    }

    return true;
}



QString mimeSniff(const QByteArray &data)
{
    // Only check the first 512 bytes.
    QByteArray ldata = data.left(512).toLower();


    // HTML Tags

    // XML processing directive. Although this is not an HTML mime type,
    // we sniff for this in the HTML phase because text/xml is just as powerful as HTML.
    CHECK_CONTAIN("<?xml", "text/xml") // # Mozilla
    // DOCTYPEs
    CHECK_CONTAIN("<!DOCTYPE html", "text/html") // # HTML5 spec
    // Sniffable tags, ordered by how often they occur in sniffable documents.
    CHECK_CONTAIN("<script", "text/html") // HTML5 spec, Mozilla
    CHECK_CONTAIN("<html", "text/html") // HTML5 spec, Mozilla
    CHECK_CONTAIN("<!--", "text/html")
    CHECK_CONTAIN("<head", "text/html") // HTML5 spec, Mozilla
    CHECK_CONTAIN("<iframe", "text/html") // Mozilla
    CHECK_CONTAIN("<h1", "text/html") // Mozilla
    CHECK_CONTAIN("<div", "text/html") // Mozilla
    CHECK_CONTAIN("<font", "text/html") // Mozilla
    CHECK_CONTAIN("<table", "text/html") // Mozilla
    CHECK_CONTAIN("<a", "text/html") // Mozilla
    CHECK_CONTAIN("<style", "text/html") // Mozilla
    CHECK_CONTAIN("<title", "text/html") // Mozilla
    CHECK_CONTAIN("<b", "text/html") // Mozilla
    CHECK_CONTAIN("<body", "text/html") // Mozilla
    CHECK_CONTAIN("<br", "text/html")
    CHECK_CONTAIN("<p", "text/html") // Mozilla


    // XML Tags

    // We want to be very conservative in interpreting text/xml content as
    // XHTML -- we just want to sniff enough to make things pass.
    // So we match explicitly on this, and don't match other ways of writing
    // it in semantically-equivalent ways.
    CHECK_CONTAIN("<html xmlns=\"http://www.w3.org/1999/xhtml\"", "application/xhtml+xml")
    CHECK_CONTAIN("<feed", "application/atom+xml")
    CHECK_CONTAIN("<rss", "application/rss+xml") // UTF-8


    // Images

    // Source: HTML 5 specification
    CHECK_START("GIF87a", "image/gif")
    CHECK_START("GIF89a", "image/gif")
    CHECK_START("\x89PNG\x0D\x0A\x1A\x0A", "image/png")
    CHECK_START("\xFF\xD8\xFF", "image/jpeg")
    CHECK_START("BM", "image/bmp")
    // Source: Chrome
    CHECK_START("I I", "image/tiff")
    CHECK_START("II*", "image/tiff")
    CHECK_START("MM\x00*", "image/tiff")
    CHECK_START("RIFF....WEBPVP8 ", "image/webp") // The space at the end is no error


    // General magic numbers

    // Source: HTML 5 specification
    CHECK_START("%PDF-", "application/pdf")
    CHECK_START("%!PS-Adobe-", "application/postscript")
    // Source: Mozilla
    CHECK_START("#!", "text/plain") // Script
    CHECK_START("%!", "text/plain") // Script, similar to PS
    CHECK_START("From", "text/plain")
    CHECK_START(">From", "text/plain")
    // Source: Chrome
    CHECK_START("\x1F\x8B\x08", "application/x-gzip")
    CHECK_START("PK\x03\x04", "application/zip")
    CHECK_START("Rar!\x1A\x07\x00", "application/x-rar-compressed")
    CHECK_START("\xD7\xCD\xC6\x9A", "application/x-msmetafile")
    CHECK_START("MZ", "application/octet-stream") // Exe


    // Audio

    // Source: Chrome
    CHECK_START("\x2E\x52\x4D\x46", "audio/x-pn-realaudio")
    CHECK_START("ID3", "audio/mpeg")


    // Video

    // Source: Chrome
    CHECK_START("\x30\x26\xB2\x75\x8E\x66\xCF\x11\xA6\xD9\x00\xAA\x00\x62\xCE\x6C", "video/x-ms-asf")
    CHECK_START("\x1A\x45\xDF\xA3", "video/webm")


    // FLash

    CHECK_START("CWS", "application/x-shockwave-flash")


    // Byte order marks
    CHECK_CONTAIN("\xFE\xFF", "text/plain") // UTF-16BE
    CHECK_CONTAIN("\xFF\xFE", "text/plain") // UTF-16LE
    CHECK_CONTAIN("\xEF\xBB\xBF", "text/plain") // UTF-8


    // Whether a given byte looks like it might be part of binary content.
    // Source: HTML5 specification
    bool magic_byte_looks_binary[256] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, // 0x00 - 0x0F
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, // 0x10 - 0x1F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x20 - 0x2F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x30 - 0x3F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x40 - 0x4F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x50 - 0x5F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x60 - 0x6F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x70 - 0x7F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x80 - 0x8F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x90 - 0x9F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xA0 - 0xAF
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xB0 - 0xBF
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xC0 - 0xCF
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xD0 - 0xDF
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xE0 - 0xEF
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // 0xF0 - 0xFF
    };

    for (int i = 0; i < data.length(); ++i)
    {
      if (magic_byte_looks_binary[(int)data.at(i)])
      {
        return "application/octet-stream";
      }
    }


    // fall back to text/plain
    return "text/plain";
}


bool mimeIsText(QString mime)
{
    return (mime == "text/plain");
}


bool mimeIsHtml(QString mime)
{
    return ((mime == "text/html") ||
            (mime == "text/xml"));
}


bool mimeIsXml(QString mime)
{
    return ((mime == "application/xhtml+xml") ||
            (mime == "application/atom+xml") ||
            (mime == "application/rss+xml"));
}


bool mimeIsImage(QString mime)
{
    return ((mime == "image/gif") ||
            (mime == "image/png") ||
            (mime == "image/jpeg") ||
            (mime == "image/bmp") ||
            (mime == "image/tiff") ||
            (mime == "image/webp"));
}


bool mimeIsAudio(QString mime)
{
    return ((mime == "udio/x-pn-realaudio") ||
            (mime == "audio/mpeg"));
}


bool mimeIsVideo(QString mime)
{
    return ((mime == "video/x-ms-asf") ||
            (mime == "video/webm"));
}

