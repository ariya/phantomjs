/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformdefs.h"

#include <private/qprintengine_ps_p.h>
#include <private/qpainter_p.h>
#include <private/qfontengine_p.h>
#include <private/qpaintengine_p.h>
#include <private/qpdf_p.h>

#ifndef QT_NO_PRINTER

#include "qprinter.h"
#include "qpainter.h"
#include "qapplication.h"
#include "qpixmap.h"
#include "qimage.h"
#include "qdatetime.h"
#include "qstring.h"
#include "qbytearray.h"
#include "qhash.h"
#include "qbuffer.h"
#include "qsettings.h"
#include "qmap.h"
#include "qbitmap.h"
#include "qregion.h"
#include "qimagewriter.h"
#include <private/qpainterpath_p.h>
#include <qdebug.h>
#include <private/qdrawhelper_p.h>
#include <private/qmutexpool_p.h>

#ifndef Q_OS_WIN
#include <unistd.h>
#endif
#include <stdlib.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

static bool qt_gen_epsf = false;

void qt_generate_epsf(bool b)
{
    qt_gen_epsf = b;
}

static const char *const ps_header =
"/BD{bind def}bind def/d2{dup dup}BD/ED{exch def}BD/D0{0 ED}BD/F{setfont}BD\n"
"/RL{rlineto}BD/CM{currentmatrix}BD/SM{setmatrix}BD/TR{translate}BD/SD\n"
"{setdash}BD/SC{aload pop setrgbcolor}BD/CR{currentfile read pop}BD/i{index}\n"
"BD/scs{setcolorspace}BD/DB{dict dup begin}BD/DE{end def}BD/ie{ifelse}BD/gs\n"
"{gsave}BD/gr{grestore}BD/w{setlinewidth}BD/d{setdash}BD/J{setlinecap}BD/j\n"
"{setlinejoin}BD/scn{3 array astore/BCol exch def}BD/SCN{3 array astore/PCol\n"
"exch def}BD/cm{6 array astore concat}BD/m{moveto}BD/l{lineto}BD/c{curveto}BD\n"
"/h{closepath}BD/W{clip}BD/W*{eoclip}BD/n{newpath}BD/q{gsave 10 dict begin}BD\n"
"/Q{end grestore}BD/re{4 2 roll m dup 0 exch RL exch 0 RL 0 exch neg RL h}BD\n"
"/S{gs PCol SC stroke gr n}BD/BT{gsave 10 dict begin/_m matrix CM def BCol\n"
"SC}BD/ET{end grestore}BD/Tf{/_fs ED findfont[_fs 0 0 _fs 0 0]makefont F}BD\n"
"/Tm{6 array astore concat}BD/Td{translate}BD/Tj{0 0 m show}BD/BDC{pop pop}BD\n"
"/EMC{}BD/BSt 0 def/WFi false def/BCol[1 1 1]def/PCol[0 0 0]def/BDArr[0.94\n"
"0.88 0.63 0.50 0.37 0.12 0.06]def/level3{/languagelevel where{pop\n"
"languagelevel 3 ge}{false}ie}BD/QCIgray D0/QCIcolor D0/QCIindex D0/QCI{\n"
"/colorimage where{pop false 3 colorimage}{exec/QCIcolor ED/QCIgray QCIcolor\n"
"length 3 idiv string def 0 1 QCIcolor length 3 idiv 1 sub{/QCIindex ED/_x\n"
"QCIindex 3 mul def QCIgray QCIindex QCIcolor _x get 0.30 mul QCIcolor _x 1\n"
"add get 0.59 mul QCIcolor _x 2 add get 0.11 mul add add cvi put}for QCIgray\n"
"image}ie}BD/di{gs TR 1 i 1 eq{pop pop false 3 1 roll BCol SC imagemask}{dup\n"
"false ne{level3}{false}ie{/_ma ED 8 eq{/_dc[0 1]def/DeviceGray}{/_dc[0 1 0 1\n"
"0 1]def/DeviceRGB}ie scs/_im ED/_mt ED/_h ED/_w ED <</ImageType 3/DataDict\n"
"<</ImageType 1/Width _w/Height _h/ImageMatrix _mt/DataSource _im\n"
"/BitsPerComponent 8/Decode _dc >>/MaskDict <</ImageType 1/Width _w/Height _h\n"
"/ImageMatrix _mt/DataSource _ma/BitsPerComponent 1/Decode[0 1]>>\n"
"/InterleaveType 3 >> image}{pop 8 4 1 roll 8 eq{image}{QCI}ie}ie}ie gr}BD/BF\n"
"{gs BSt 1 eq{BCol SC WFi{fill}{eofill}ie}if BSt 2 ge BSt 8 le and{BDArr BSt\n"
"2 sub get/_sc ED BCol{1. exch sub _sc mul 1. exch sub}forall 3 array astore\n"
"SC WFi{fill}{eofill}ie}if BSt 9 ge BSt 14 le and{WFi{W}{W*}ie pathbbox 3 i 3\n"
"i TR 4 2 roll 3 2 roll exch sub/_h ED sub/_w ED BCol SC 0.3 w n BSt 9 eq BSt\n"
"11 eq or{0 4 _h{dup 0 exch m _w exch l}for}if BSt 10 eq BSt 11 eq or{0 4 _w{\n"
"dup 0 m _h l}for}if BSt 12 eq BSt 14 eq or{_w _h gt{0 6 _w _h add{dup 0 m _h\n"
"sub _h l}for}{0 6 _w _h add{dup 0 exch m _w sub _w exch l}for}ie}if BSt 13\n"
"eq BSt 14 eq or{_w _h gt{0 6 _w _h add{dup _h m _h sub 0 l}for}{0 6 _w _h\n"
"add{dup _w exch m _w sub 0 exch l}for}ie}if stroke}if BSt 15 eq{}if BSt 24\n"
"eq{}if gr}BD/f{/WFi true def BF n}BD/f*{/WFi false def BF n}BD/B{/WFi true\n"
"def BF S n}BD/B*{/WFi false def BF S n}BD/QI{/C save def pageinit q n}BD/QP{\n"
"Q C restore showpage}BD/SPD{/setpagedevice where{<< 3 1 roll >>\n"
"setpagedevice}{pop pop}ie}BD/T1AddMapping{10 dict begin/glyphs ED/fnt ED\n"
"/current fnt/NumGlyphs get def/CMap fnt/CMap get def 0 1 glyphs length 1 sub\n"
"{glyphs exch get/gn ED current dup 256 mod/min ED 256 idiv/maj ED CMap dup\n"
"maj get dup null eq{pop 256 array 0 1 255{1 i exch/.notdef put}for}if dup\n"
"min gn put maj exch put/current current 1 add def}for fnt/CMap CMap put fnt\n"
"/NumGlyphs current put end}def/T1AddGlyphs{10 dict begin/glyphs ED/fnt ED\n"
"/current fnt/NumGlyphs get def/CMap fnt/CMap get def/CharStrings fnt\n"
"/CharStrings get def 0 1 glyphs length 2 idiv 1 sub{2 mul dup glyphs exch\n"
"get/gn ED 1 add glyphs exch get/cs ED current dup 256 mod/min ED 256 idiv\n"
"/maj ED CMap dup maj get dup null eq{pop 256 array 0 1 255{1 i exch/.notdef\n"
"put}for}if dup min gn put maj exch put CharStrings gn cs put/current current\n"
"1 add def}for fnt/CharStrings CharStrings put fnt/CMap CMap put fnt\n"
"/NumGlyphs current put end}def/StringAdd{1 i length 1 i length add string 3\n"
"1 roll 2 i 0 3 i putinterval 2 i 2 i length 2 i putinterval pop pop}def\n"
"/T1Setup{10 dict begin dup/FontName ED (-Base) StringAdd cvx cvn/Font ED\n"
"/MaxPage Font/NumGlyphs get 1 sub 256 idiv def/FDepVector MaxPage 1 add\n"
"array def/Encoding MaxPage 1 add array def 0 1 MaxPage{dup Encoding exch dup\n"
"put dup/Page ED FontName (-) StringAdd exch 20 string cvs StringAdd cvn Font\n"
"0 dict copy d2/CMap get Page get/Encoding exch put definefont FDepVector\n"
"exch Page exch put}for FontName cvn <</FontType 0/FMapType 2/FontMatrix[1 0\n"
"0 1 0 0]/Encoding Encoding/FDepVector FDepVector >> definefont pop end}def\n";



// ------------------------------End of static data ----------------------------------

// make sure DSC comments are not longer than 255 chars per line.
static QByteArray wrapDSC(const QByteArray &str)
{
    QByteArray dsc = str.simplified();
    const int wrapAt = 254;
    QByteArray wrapped;
    if (dsc.length() < wrapAt)
        wrapped = dsc;
    else {
        wrapped = dsc.left(wrapAt);
        QByteArray tmp = dsc.mid(wrapAt);
        while (tmp.length() > wrapAt-3) {
            wrapped += "\n%%+" + tmp.left(wrapAt-3);
            tmp = tmp.mid(wrapAt-3);
        }
        wrapped += "\n%%+" + tmp;
    }
    return wrapped + '\n';
}

// ----------------------------- Internal class declarations -----------------------------

QPSPrintEnginePrivate::QPSPrintEnginePrivate(QPrinter::PrinterMode m)
    : QPdfBaseEnginePrivate(m),
      printerState(QPrinter::Idle), hugeDocument(false), headerDone(false)
{
    useAlphaEngine = true;
    postscript = true;

    firstPage = true;

#ifndef QT_NO_SETTINGS
    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
    settings.beginGroup(QLatin1String("Qt"));
    embedFonts = settings.value(QLatin1String("embedFonts"), true).toBool();
#else
    embedFonts = true;
#endif
}

QPSPrintEnginePrivate::~QPSPrintEnginePrivate()
{
}

QT_BEGIN_INCLUDE_NAMESPACE
#include <qdebug.h>
QT_END_INCLUDE_NAMESPACE

static void ps_r7(QPdf::ByteStream& stream, const char * s, int l)
{
    int i = 0;
    uchar line[84];
    int col = 0;

    while(i < l) {
        line[col++] = s[i++];
        if (i < l - 1 && col >= 76) {
            line[col++] = '\n';
            line[col++] = '\0';
            stream << (const char *)line;
            col = 0;
        }
    }
    if (col > 0) {
        while((col&3) != 0)
            line[col++] = '%'; // use a comment as padding
        line[col++] = '\n';
        line[col++] = '\0';
        stream << (const char *)line;
    }
}

static QByteArray runlengthEncode(const QByteArray &input)
{
    if (!input.length())
        return input;

    const char *data = input.constData();

    QByteArray out;
    int start = 0;
    char last = *data;

    enum State {
        Undef,
        Equal,
        Diff
    };
    State state = Undef;

    int i = 1;
    int written = 0;
    while (1) {
        bool flush = (i == input.size());
        if (!flush) {
            switch(state) {
            case Undef:
                state = (last == data[i]) ? Equal : Diff;
                break;
            case Equal:
                if (data[i] != last)
                    flush = true;
                break;
            case Diff:
                if (data[i] == last) {
                    --i;
                    flush = true;
                }
            }
        }
        if (flush || i - start == 128) {
            int size = i - start;
            if (state == Equal) {
                out.append((char)(uchar)(257-size));
                out.append(last);
                written += size;
            } else {
                out.append((char)(uchar)size-1);
                while (start < i)
                    out.append(data[start++]);
                written += size;
            }
            state = Undef;
            start = i;
            if (i == input.size())
                break;
        }
        last = data[i];
        ++i;
    };
    out.append((char)(uchar)128);
    return out;
}

enum format {
    Raw,
    Runlength,
    DCT
};
static const char *const filters[3] = {
    " ",
    "/RunLengthDecode filter ",
    "/DCTDecode filter "
};

static QByteArray compressHelper(const QImage &image, bool gray, int *format)
{
    // we can't use premultiplied here
    QByteArray pixelData;
    int depth = image.depth();

    Q_ASSERT(image.format() != QImage::Format_ARGB32_Premultiplied);

    if (depth != 1 && !gray && QImageWriter::supportedImageFormats().contains("jpeg")) {
        QBuffer buffer(&pixelData);
        QImageWriter writer(&buffer, "jpeg");
        writer.setQuality(94);
        writer.write(image);
        *format = DCT;
    } else {
        int width = image.width();
        int height = image.height();
        int size = width*height;

        if (depth == 1)
            size = (width+7)/8*height;
        else if (!gray)
            size = size*3;

        pixelData.resize(size);
        uchar *pixel = (uchar *)pixelData.data();
        int i = 0;
        if (depth == 1) {
            QImage::Format format = image.format();
            memset(pixel, 0xff, size);
            for(int y=0; y < height; y++) {
                const uchar * s = image.scanLine(y);
                for(int x=0; x < width; x++) {
                    // need to copy bit for bit...
                    bool b = (format == QImage::Format_MonoLSB) ?
                             (*(s + (x >> 3)) >> (x & 7)) & 1 :
                             (*(s + (x >> 3)) << (x & 7)) & 0x80 ;
                    if (b)
                        pixel[i >> 3] ^= (0x80 >> (i & 7));
                    i++;
                }
                // we need to align to 8 bit here
                i = (i+7) & 0xffffff8;
            }
        } else if (depth == 8) {
            for(int y=0; y < height; y++) {
                const uchar * s = image.scanLine(y);
                for(int x=0; x < width; x++) {
                    QRgb rgb = image.color(s[x]);
                    if (gray) {
                        pixel[i] = (unsigned char) qGray(rgb);
                        i++;
                    } else {
                        pixel[i] = (unsigned char) qRed(rgb);
                        pixel[i+1] = (unsigned char) qGreen(rgb);
                        pixel[i+2] = (unsigned char) qBlue(rgb);
                        i += 3;
                    }
                }
            }
        } else {
            for(int y=0; y < height; y++) {
                QRgb * s = (QRgb*)(image.scanLine(y));
                for(int x=0; x < width; x++) {
                    QRgb rgb = (*s++);
                    if (gray) {
                        pixel[i] = (unsigned char) qGray(rgb);
                        i++;
                    } else {
                        pixel[i] = (unsigned char) qRed(rgb);
                        pixel[i+1] = (unsigned char) qGreen(rgb);
                        pixel[i+2] = (unsigned char) qBlue(rgb);
                        i += 3;
                    }
                }
            }
        }
        *format = Raw;
        if (depth == 1) {
            pixelData = runlengthEncode(pixelData);
            *format = Runlength;
        }
    }
    QByteArray outarr = QPdf::ascii85Encode(pixelData);
    return outarr;
}

void QPSPrintEnginePrivate::drawImageHelper(qreal x, qreal y, qreal w, qreal h, const QImage &img,
                                            const QImage &mask, bool gray, qreal scaleX, qreal scaleY)
{
    Q_UNUSED(h);
    Q_UNUSED(w);
    int width = img.width();
    int height = img.height();

    QByteArray out;
    int size = 0;
    const char *bits;

    if (!mask.isNull()) {
        int format;
        out = compressHelper(mask, true, &format);
        size = (width+7)/8*height;
        *currentPage << "/mask currentfile/ASCII85Decode filter"
                     << filters[format]
                     << size << " string readstring\n";
        ps_r7(*currentPage, out, out.size());
        *currentPage << " pop def\n";
    }
    if (img.depth() == 1) {
        size = (width+7)/8*height;
        bits = "1 ";
    } else if (gray) {
        size = width*height;
        bits = "8 ";
    } else {
        size = width*height*3;
        bits = "24 ";
    }

    int format;
    out = compressHelper(img, gray, &format);
    *currentPage << "/sl currentfile/ASCII85Decode filter"
                 << filters[format]
                 << size << " string readstring\n";
    ps_r7(*currentPage, out, out.size());
    *currentPage << " pop def\n";
    *currentPage << width << ' ' << height << '[' << scaleX << " 0 0 " << scaleY << " 0 0]sl "
                 << bits << (!mask.isNull() ? "mask " : "false ")
                 << x << ' ' << y << " di\n";
}


void QPSPrintEnginePrivate::drawImage(qreal x, qreal y, qreal w, qreal h,
                                      const QImage &image, const QImage &msk)
{
    if (!w || !h || image.isNull()) return;

    QImage img(image);
    QImage mask(msk);

    if (image.format() == QImage::Format_ARGB32_Premultiplied)
        img = image.convertToFormat(QImage::Format_ARGB32);

    if (!msk.isNull() && msk.format() == QImage::Format_ARGB32_Premultiplied)
        mask = msk.convertToFormat(QImage::Format_ARGB32);

    int width  = img.width();
    int height = img.height();
    qreal scaleX = width/w;
    qreal scaleY = height/h;

    bool gray = (colorMode == QPrinter::GrayScale) || img.allGray();
    int splitSize = 21830 * (gray ? 3 : 1);
    if (width * height > splitSize) { // 65535/3, tolerance for broken printers
        int images, subheight;
        images = (width * height + splitSize - 1) / splitSize;
        subheight = (height + images-1) / images;
        while (subheight * width > splitSize) {
            images++;
            subheight = (height + images-1) / images;
        }
        int suby = 0;
        const QImage constImg(img);
        const QImage constMask(mask);
        while(suby < height) {
            qreal subImageHeight = qMin(subheight, height-suby);
            const QImage subImage(constImg.scanLine(suby), width, subImageHeight,
                                  constImg.bytesPerLine(), constImg.format());
            const QImage subMask = mask.isNull() ? mask : QImage(constMask.scanLine(suby), width, subImageHeight,
                                                                 constMask.bytesPerLine(), constMask.format());
            drawImageHelper(x, y + suby/scaleY, w, subImageHeight/scaleY,
                            subImage, subMask, gray, scaleX, scaleY);
            suby += subheight;
        }
    } else {
        drawImageHelper(x, y, width, height, img, mask, gray, scaleX, scaleY);
    }
}

void QPSPrintEnginePrivate::emitHeader(bool finished)
{
    QPSPrintEngine *q = static_cast<QPSPrintEngine *>(q_ptr);
    QPrinter *printer = static_cast<QPrinter*>(pdev);

    if (creator.isEmpty())
        creator = QLatin1String("Qt " QT_VERSION_STR);

    QByteArray header;
    QPdf::ByteStream s(&header);

    qreal scale = 72. / ((qreal) q->metric(QPaintDevice::PdmDpiY));
    QRect pageRect = this->pageRect();
    QRect paperRect = this->paperRect();
    int mtop = pageRect.top() - paperRect.top();
    int mleft = pageRect.left() - paperRect.left();
    int mbottom = paperRect.bottom() - pageRect.bottom();
    int mright = paperRect.right() - pageRect.right();
    int width = pageRect.width();
    int height = pageRect.height();
    if (finished && pageCount == 1 && copies == 1 &&
        ((fullPage && qt_gen_epsf) || (outputFileName.endsWith(QLatin1String(".eps")))))
    {
        // According to the EPSF 3.0 spec it is required that the PS
        // version is PS-Adobe-3.0
        s << "%!PS-Adobe-3.0";
        if (!boundingBox.isValid())
            boundingBox.setRect(0, 0, width, height);
        if (orientation == QPrinter::Landscape) {
            if (!fullPage)
                boundingBox.translate(-mleft, -mtop);
            s << " EPSF-3.0\n%%BoundingBox: "
              << int((printer->height() - boundingBox.bottom())*scale) // llx
              << int((printer->width() - boundingBox.right())*scale - 1) // lly
              << int((printer->height() - boundingBox.top())*scale + 1) // urx
              << int((printer->width() - boundingBox.left())*scale); // ury
        } else {
            if (!fullPage)
                boundingBox.translate(mleft, -mtop);
            s << " EPSF-3.0\n%%BoundingBox: "
              << int((boundingBox.left())*scale)
              << int((printer->height() - boundingBox.bottom())*scale - 1)
              << int((boundingBox.right())*scale + 1)
              << int((printer->height() - boundingBox.top())*scale);
        }
    } else {
        s << "%!PS-Adobe-1.0";
        int w = width + (fullPage ? 0 : mleft + mright);
        int h = height + (fullPage ? 0 : mtop + mbottom);
        w = (int)(w*scale);
        h = (int)(h*scale);
        // set a bounding box according to the DSC
        if (orientation == QPrinter::Landscape)
            s << "\n%%BoundingBox: 0 0 " << h << w;
        else
            s << "\n%%BoundingBox: 0 0 " << w << h;
    }
    s << '\n' << wrapDSC("%%Creator: " + creator.toUtf8());
    if (!title.isEmpty())
        s << wrapDSC("%%Title: " + title.toUtf8());
#ifndef QT_NO_DATESTRING
    s << "%%CreationDate: " << QDateTime::currentDateTime().toString().toUtf8();
#endif
    s << "\n%%Orientation: ";
    if (orientation == QPrinter::Landscape)
        s << "Landscape";
    else
        s << "Portrait";

    s << "\n%%Pages: (atend)"
        "\n%%DocumentFonts: (atend)"
        "\n%%EndComments\n"

        "%%BeginProlog\n"
        "% Prolog copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).\n"
        "% You may copy this prolog in any way that is directly related to this document.\n"
        "% For other use of this prolog, see your licensing agreement for Qt.\n"
      << ps_header << '\n';


    s << "/pageinit {\n";
    if (!fullPage) {
        if (orientation == QPrinter::Portrait)
            s << mleft*scale << mbottom*scale << "translate\n";
        else
            s << mtop*scale << mleft*scale << "translate\n";
    }
    if (orientation == QPrinter::Portrait) {
        s << "% " << printer->widthMM() << '*' << printer->heightMM()
          << "mm (portrait)\n0 " << height*scale
          << "translate " << scale << '-' << scale << "scale } def\n";
    } else {
        s << "% " << printer->heightMM() << '*' << printer->widthMM()
          << " mm (landscape)\n 90 rotate " << scale << '-' << scale << "scale } def\n";
    }
    s << "%%EndProlog\n";

    outDevice->write(header);
    headerDone = true;
}


void QPSPrintEnginePrivate::emitPages()
{
    if (!hugeDocument) {
        for (QHash<QFontEngine::FaceId, QFontSubset *>::const_iterator it = fonts.constBegin();
             it != fonts.constEnd(); ++it)
            outDevice->write((*it)->toType1());
    }

    QIODevice *content = buffer.stream();
    // Write the page contents in chunks.
    while (!content->atEnd()) {
        QByteArray buf = content->read(currentPage->chunkSize());
        if (!buf.isEmpty())
            outDevice->write(buf);
    }
    content = currentPage->stream();
    // Write the page contents in chunks.
    while (!content->atEnd()) {
        QByteArray buf = content->read(currentPage->chunkSize());
        if (!buf.isEmpty())
            outDevice->write(buf);
    }
    outDevice->write(trailer);

    buffer.clear();
    currentPage->clear();
    trailer = QByteArray();
    hugeDocument = true;
}


#ifdef Q_WS_QWS
static const int max_in_memory_size = 2000000;
#else
static const int max_in_memory_size = 32000000;
#endif

void QPSPrintEnginePrivate::flushPage(bool last)
{
    if (!last && currentPage->stream()->size() == 0)
        return;

    QPdf::ByteStream e(&trailer);
    buffer << "%%Page: "
           << pageCount << pageCount << "\n"
           "%%BeginPageSetup\n"
           "QI\n";
    if (hugeDocument) {
        for (QHash<QFontEngine::FaceId, QFontSubset *>::const_iterator it = fonts.constBegin();
             it != fonts.constEnd(); ++it) {
            if (currentPage->fonts.contains((*it)->object_id)) {
                if ((*it)->downloaded_glyphs == 0) {
                    buffer << (*it)->toType1();
                    (*it)->downloaded_glyphs = 0;
                } else {
                    buffer << (*it)->type1AddedGlyphs();
                }
            }
        }
    }
    for (int i = 0; i < currentPage->fonts.size(); ++i)
        buffer << "(F" << QByteArray::number(currentPage->fonts.at(i)) << ") T1Setup\n";

    buffer << "%%EndPageSetup\nq\n";
    e << "\nQ QP\n";
    if (last || hugeDocument
        || buffer.stream()->size() + currentPage->stream()->size() > max_in_memory_size) {
//        qDebug("emiting header at page %d", pageCount);
        if (!headerDone)
            emitHeader(last);
        emitPages();
    } else {
        buffer << *currentPage << e;
        currentPage->clear();
        trailer.clear();
    }
    pageCount++;
}

// ================ PSPrinter class ========================

QPSPrintEngine::QPSPrintEngine(QPrinter::PrinterMode m)
    : QPdfBaseEngine(*(new QPSPrintEnginePrivate(m)),
                     PrimitiveTransform
                     | PatternTransform
                     | PixmapTransform
                     | PainterPaths
                     | PatternBrush
        )
{
}

static void ignoreSigPipe(bool b)
{
#ifndef QT_NO_LPR
    static struct sigaction *users_sigpipe_handler = 0;
    static int lockCount = 0;

#ifndef QT_NO_THREAD
    QMutexLocker locker(QMutexPool::globalInstanceGet(&users_sigpipe_handler));
#endif

    if (b) {
        if (lockCount++ > 0)
            return;

        if (users_sigpipe_handler != 0)
            return; // already ignoring sigpipe

        users_sigpipe_handler = new struct sigaction;
        struct sigaction tmp_sigpipe_handler;
        tmp_sigpipe_handler.sa_handler = SIG_IGN;
        sigemptyset(&tmp_sigpipe_handler.sa_mask);
        tmp_sigpipe_handler.sa_flags = 0;

        if (sigaction(SIGPIPE, &tmp_sigpipe_handler, users_sigpipe_handler) == -1) {
            delete users_sigpipe_handler;
            users_sigpipe_handler = 0;
        }
    }
    else {
        if (--lockCount > 0)
            return;

        if (users_sigpipe_handler == 0)
            return; // not ignoring sigpipe

        if (sigaction(SIGPIPE, users_sigpipe_handler, 0) == -1)
            qWarning("QPSPrintEngine: Could not restore SIGPIPE handler");

        delete users_sigpipe_handler;
        users_sigpipe_handler = 0;
    }
#else
    Q_UNUSED(b);
#endif
}
QPSPrintEngine::~QPSPrintEngine()
{
    Q_D(QPSPrintEngine);
    if (d->fd >= 0)
#if defined(Q_OS_WIN) && defined(_MSC_VER) && _MSC_VER >= 1400
        ::_close(d->fd);
#else
        ::close(d->fd);
#endif
}

bool QPSPrintEngine::begin(QPaintDevice *pdev)
{
    Q_D(QPSPrintEngine);

    if (d->fd >= 0)
        return true;

    if (d->useAlphaEngine) {
        QAlphaPaintEngine::begin(pdev);
        if (!continueCall())
            return true;
    }

    if(!QPdfBaseEngine::begin(pdev)) {
        d->printerState = QPrinter::Error;
        return false;
    }

    d->pageCount = 1;                // initialize state

    d->pen = QPen(Qt::black);
    d->brush = Qt::NoBrush;
    d->hasPen = true;
    d->hasBrush = false;
    d->clipEnabled = false;
    d->allClipped = false;
    d->boundingBox = QRect();
    d->fontsUsed = "";
    d->hugeDocument = false;
    d->simplePen = false;

    setActive(true);
    d->printerState = QPrinter::Active;

    newPage();

    return true;
}

bool QPSPrintEngine::end()
{
    Q_D(QPSPrintEngine);

    if (d->useAlphaEngine) {
        QAlphaPaintEngine::end();
        if (!continueCall())
            return true;
    }

    // we're writing to lp/lpr through a pipe, we don't want to crash with SIGPIPE
    // if lp/lpr dies
    ignoreSigPipe(true);
    d->flushPage(true);
    QByteArray trailer;
    QPdf::ByteStream s(&trailer);
    s << "%%Trailer\n"
         "%%Pages: " << d->pageCount - 1 << '\n' <<
        wrapDSC("%%DocumentFonts: " + d->fontsUsed);
    s << "%%EOF\n";
    d->outDevice->write(trailer);

    QPdfBaseEngine::end();
    ignoreSigPipe(false);

    d->firstPage = true;
    d->headerDone = false;

    setActive(false);
    d->printerState = QPrinter::Idle;
    d->pdev = 0;

    return true;
}

void QPSPrintEngine::setBrush()
{
    Q_D(QPSPrintEngine);
#if 0
    bool specifyColor;
    int gStateObject = 0;
    int patternObject = d->addBrushPattern(brush, d->stroker.matrix, brushOrigin, &specifyColor, &gStateObject);

    *d->currentPage << (patternObject ? "/PCSp cs " : "/CSp cs ");
    if (specifyColor) {
        QColor rgba = brush.color();
        *d->currentPage << rgba.redF()
                        << rgba.greenF()
                        << rgba.blueF();
    }
    if (patternObject)
        *d->currentPage << "/Pat" << patternObject;
    *d->currentPage << "scn\n";
#endif
    QColor rgba = d->brush.color();
    if (d->colorMode == QPrinter::GrayScale) {
        qreal gray = qGray(rgba.rgba())/255.;
        *d->currentPage << gray << gray << gray;
    } else {
        *d->currentPage << rgba.redF()
                        << rgba.greenF()
                        << rgba.blueF();
    }
    *d->currentPage << "scn\n"
                    << "/BSt " << d->brush.style() << "def\n";
}

void QPSPrintEngine::drawImageInternal(const QRectF &r, QImage image, bool bitmap)
{
    Q_D(QPSPrintEngine);
    if (d->clipEnabled && d->allClipped)
        return;
    if (bitmap && image.depth() != 1)
        bitmap = false;
    QImage mask;
    // the below is not necessary since it's handled by the alpha
    // engine
    if (!d->useAlphaEngine && !bitmap) {
        if (image.format() == QImage::Format_Mono || image.format() == QImage::Format_MonoLSB)
            image = image.convertToFormat(QImage::Format_Indexed8);
        if (image.hasAlphaChannel()) {
            // get better alpha dithering
            int xscale = image.width();
            xscale *= xscale <= 800 ? 4 : (xscale <= 1600 ? 2 : 1);
            int yscale = image.height();
            yscale *= yscale <= 800 ? 4 : (yscale <= 1600 ? 2 : 1);
            image = image.scaled(xscale, yscale);
            mask = image.createAlphaMask(Qt::OrderedAlphaDither);
        }
    }
    *d->currentPage << "q\n";
    if(!d->simplePen)
        *d->currentPage << QPdf::generateMatrix(d->stroker.matrix);
    QBrush b = d->brush;
    if (image.depth() == 1) {
        // set current pen as brush
        d->brush = d->pen.brush();
        setBrush();
    }
    d->drawImage(r.x(), r.y(), r.width(), r.height(), image, mask);
    *d->currentPage << "Q\n";
    d->brush = b;
}


void QPSPrintEngine::drawImage(const QRectF &r, const QImage &img, const QRectF &sr,
                               Qt::ImageConversionFlags)
{
    Q_D(QPSPrintEngine);

    if (d->useAlphaEngine) {
        QAlphaPaintEngine::drawImage(r, img, sr);
        if (!continueCall())
            return;
    }
    QImage image = img.copy(sr.toRect());
    drawImageInternal(r, image, false);
}

void QPSPrintEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
    Q_D(QPSPrintEngine);

    if (d->useAlphaEngine) {
        QAlphaPaintEngine::drawPixmap(r, pm, sr);
        if (!continueCall())
            return;
    }

    QImage img = pm.copy(sr.toRect()).toImage();
    drawImageInternal(r, img, true);
}

void QPSPrintEngine::drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &p)
{
    Q_D(QPSPrintEngine);

    if (d->useAlphaEngine) {
        QAlphaPaintEngine::drawTiledPixmap(r, pixmap, p);
        if (!continueCall())
            return;
    }

    if (d->clipEnabled && d->allClipped)
        return;
    // ### Optimize implementation!
    qreal yPos = r.y();
    qreal yOff = p.y();
    while(yPos < r.y() + r.height()) {
        qreal drawH = pixmap.height() - yOff;    // Cropping first row
        if (yPos + drawH > r.y() + r.height())        // Cropping last row
            drawH = r.y() + r.height() - yPos;
        qreal xPos = r.x();
        qreal xOff = p.x();
        while(xPos < r.x() + r.width()) {
            qreal drawW = pixmap.width() - xOff; // Cropping first column
            if (xPos + drawW > r.x() + r.width())    // Cropping last column
                drawW = r.x() + r.width() - xPos;
            // ########
            painter()->drawPixmap(QPointF(xPos, yPos).toPoint(), pixmap,
                                  QRectF(xOff, yOff, drawW, drawH).toRect());
            xPos += drawW;
            xOff = 0;
        }
        yPos += drawH;
        yOff = 0;
    }

}

bool QPSPrintEngine::newPage()
{
    Q_D(QPSPrintEngine);

    if (!d->firstPage && d->useAlphaEngine)
        flushAndInit();

    // we're writing to lp/lpr through a pipe, we don't want to crash with SIGPIPE
    // if lp/lpr dies
    ignoreSigPipe(true);
    if (!d->firstPage)
        d->flushPage();
    d->firstPage = false;
    ignoreSigPipe(false);

    delete d->currentPage;
    d->currentPage = new QPdfPage;
    d->stroker.stream = d->currentPage;

    return QPdfBaseEngine::newPage();
}

bool QPSPrintEngine::abort()
{
    // ### abort!?!
    return false;
}

QPrinter::PrinterState QPSPrintEngine::printerState() const
{
    Q_D(const QPSPrintEngine);
    return d->printerState;
}

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
