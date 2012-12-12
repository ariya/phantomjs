
/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QPRINTENGINE_PDF_P_H
#define QPRINTENGINE_PDF_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtGui/qprintengine.h"

#ifndef QT_NO_PRINTER
#include "QtCore/qmap.h"
#include "QtGui/qmatrix.h"
#include "QtCore/qstring.h"
#include "QtCore/qvector.h"
#include "QtGui/qpaintengine.h"
#include "QtGui/qpainterpath.h"
#include "QtCore/qdatastream.h"

#include "private/qfontengine_p.h"
#include "private/qpdf_p.h"
#include "private/qpaintengine_p.h"

QT_BEGIN_NAMESPACE

// #define USE_NATIVE_GRADIENTS

class QImage;
class QDataStream;
class QPen;
class QPointF;
class QRegion;
class QFile;
class QPdfEngine;

class QPdfEnginePrivate;

class QPdfEngine : public QPdfBaseEngine
{
    Q_DECLARE_PRIVATE(QPdfEngine)
public:
    QPdfEngine(QPrinter::PrinterMode m);
    virtual ~QPdfEngine();

    // reimplementations QPaintEngine
    bool begin(QPaintDevice *pdev);
    bool end();

    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr, const QByteArray * data=0);
    void drawPixmap(const QRectF & rectangle, const QPixmap & pixmap, const QRectF & sr) {
        drawPixmap(rectangle, pixmap, sr, 0);
    }

    void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                   Qt::ImageConversionFlags flags = Qt::AutoColor);
    void drawTiledPixmap (const QRectF & rectangle, const QPixmap & pixmap, const QPointF & point);

    Type type() const;
    // end reimplementations QPaintEngine

    // reimplementations QPrintEngine
    bool abort() {return false;}
    bool newPage();
    QPrinter::PrinterState printerState() const {return state;}
    // end reimplementations QPrintEngine

    void setBrush();

    virtual void addHyperlink(const QRectF &r, const QUrl &url);
    virtual void addAnchor(const QRectF &r, const QString &name);
    virtual void addLink(const QRectF &r, const QString &anchor);
    virtual void addTextField(const QRectF &r, const QString &text, const QString &name, bool multiLine, bool password, bool readOnly, int maxLength);
    virtual void addCheckBox(const QRectF &r, bool checked, const QString &name, bool readOnly);

    // ### unused, should have something for this in QPrintEngine
    void setAuthor(const QString &author);
    QString author() const;

    void setDevice(QIODevice* dev);

    void beginSectionOutline(const QString &text, const QString &anchor);
    void endSectionOutline();

    void setProperty(PrintEnginePropertyKey key, const QVariant &value);
    QVariant property(PrintEnginePropertyKey key) const;
private:
    Q_DISABLE_COPY(QPdfEngine)

    QPrinter::PrinterState state;
};

class QPdfEnginePrivate : public QPdfBaseEnginePrivate
{
    Q_DECLARE_PUBLIC(QPdfEngine)
public:
  
    class OutlineItem {
    public:
        OutlineItem *parent;
        OutlineItem *next;
        OutlineItem *prev;
        OutlineItem *firstChild;
        OutlineItem *lastChild;
        uint obj;
        QString text;
        QString anchor;
        
        OutlineItem(const QString &t, const QString &a): 
            parent(NULL), next(NULL), prev(NULL), firstChild(NULL), lastChild(NULL),
            obj(0), text(t), anchor(a) {}
        ~OutlineItem() {
            OutlineItem *i = firstChild;
            while(i != NULL) { 
                OutlineItem *n = i->next;
                delete i;
                i=n;
            }
        }
    };
    
    OutlineItem *outlineRoot;
    OutlineItem *outlineCurrent;
    void writeOutlineChildren(OutlineItem *node);
    
    QPdfEnginePrivate(QPrinter::PrinterMode m);
    ~QPdfEnginePrivate();

    void newPage();

    int width() const {
        QRect r = paperRect();
        return qRound(r.width()*72./resolution);
    }
    int height() const {
        QRect r = paperRect();
        return qRound(r.height()*72./resolution);
    }

    void writeHeader();
    void writeTail();

    void convertImage(const QImage & image, QByteArray & imageData);

    int addImage(const QImage &image, bool *bitmap, qint64 serial_no, const QImage * noneScaled=0, const QByteArray * data=0, bool * useScaled=0);
    int addConstantAlphaObject(int brushAlpha, int penAlpha = 255);
    int addBrushPattern(const QTransform &matrix, bool *specifyColor, int *gStateObject);

    void drawTextItem(const QPointF &p, const QTextItemInt &ti);

    QTransform pageMatrix() const;

private:
    Q_DISABLE_COPY(QPdfEnginePrivate)

#ifdef USE_NATIVE_GRADIENTS
    int gradientBrush(const QBrush &b, const QMatrix &matrix, int *gStateObject);
#endif

    void writeInfo();
    void writePageRoot();
    void writeFonts();
    void embedFont(QFontSubset *font);

    int formFieldList;
    QVector<uint> formFields;
    QVector<int> xrefPositions;
    QDataStream* stream;
    int streampos;
    bool doCompress;
    int imageDPI;
    int imageQuality;

    int writeImage(const QByteArray &data, int width, int height, int depth,
                   int maskObject, int softMaskObject, bool dct = false);
    void writePage();

    int addXrefEntry(int object, bool printostr = true);

    void printString(const QString &string);
    void printAnchor(const QString &name);
    
    void xprintf(const char* fmt, ...);
    inline void write(const QByteArray &data) {
        stream->writeRawData(data.constData(), data.size());
        streampos += data.size();
    }

    int writeCompressed(const char *src, int len);
    inline int writeCompressed(const QByteArray &data) { return writeCompressed(data.constData(), data.length()); }
    int writeCompressed(QIODevice *dev);

    // various PDF objects
    int pageRoot, catalog, info, graphicsState, patternColorSpace;
    QVector<uint> dests;
    QHash<QString, uint> anchors;
    QVector<uint> pages;
    QHash<qint64, uint> imageCache;
    QHash<QPair<uint, uint>, uint > alphaCache;
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER

#endif // QPRINTENGINE_PDF_P_H
