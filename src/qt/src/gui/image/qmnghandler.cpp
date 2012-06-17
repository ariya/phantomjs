/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qmnghandler_p.h"

#include "qimage.h"
#include "qvariant.h"
#include "qcolor.h"

#define MNG_USE_SO
#include <libmng.h>

QT_BEGIN_NAMESPACE

class QMngHandlerPrivate
{
    Q_DECLARE_PUBLIC(QMngHandler)
    public:
    bool haveReadNone;
    bool haveReadAll;
    mng_handle hMNG;
    QImage image;
    int elapsed;
    int nextDelay;
    int iterCount;
    int frameIndex;
    int nextIndex;
    int frameCount;
    mng_uint32 iStyle;
    mng_bool readData(mng_ptr pBuf, mng_uint32 iSize, mng_uint32p pRead);
    mng_bool writeData(mng_ptr pBuf, mng_uint32 iSize, mng_uint32p pWritten);
    mng_bool processHeader(mng_uint32 iWidth, mng_uint32 iHeight);
    QMngHandlerPrivate(QMngHandler *q_ptr);
    ~QMngHandlerPrivate();
    bool getNextImage(QImage *result);
    bool writeImage(const QImage &image);
    int currentImageNumber() const;
    int imageCount() const;
    bool jumpToImage(int imageNumber);
    bool jumpToNextImage();
    int nextImageDelay() const;
    bool setBackgroundColor(const QColor &color);
    QColor backgroundColor() const;
    QMngHandler *q_ptr;
};

static mng_bool myerror(mng_handle /*hMNG*/,
    mng_int32   iErrorcode,
    mng_int8    /*iSeverity*/,
    mng_chunkid iChunkname,
    mng_uint32  /*iChunkseq*/,
    mng_int32   iExtra1,
    mng_int32   iExtra2,
    mng_pchar   zErrortext)
{
    qWarning("MNG error %d: %s; chunk %c%c%c%c; subcode %d:%d",
        iErrorcode,zErrortext,
        (iChunkname>>24)&0xff,
        (iChunkname>>16)&0xff,
        (iChunkname>>8)&0xff,
        (iChunkname>>0)&0xff,
        iExtra1,iExtra2);
    return TRUE;
}

static mng_ptr myalloc(mng_size_t iSize)
{
#if defined(Q_OS_WINCE)
    mng_ptr ptr = malloc(iSize);
    memset(ptr, 0, iSize);
    return ptr;
#else
    return (mng_ptr)calloc(1, iSize);
#endif
}

static void myfree(mng_ptr pPtr, mng_size_t /*iSize*/)
{
    free(pPtr);
}

static mng_bool myopenstream(mng_handle)
{
    return MNG_TRUE;
}

static mng_bool myclosestream(mng_handle hMNG)
{
    QMngHandlerPrivate *pMydata = reinterpret_cast<QMngHandlerPrivate *>(mng_get_userdata(hMNG));
    pMydata->haveReadAll = true;
    return MNG_TRUE;
}

static mng_bool myreaddata(mng_handle hMNG,
                    mng_ptr    pBuf,
                    mng_uint32 iSize,
                    mng_uint32p pRead)
{
    QMngHandlerPrivate *pMydata = reinterpret_cast<QMngHandlerPrivate *>(mng_get_userdata(hMNG));
    return pMydata->readData(pBuf, iSize, pRead);
}

static mng_bool mywritedata(mng_handle hMNG,
		     mng_ptr pBuf,
		     mng_uint32 iSize,
		     mng_uint32p pWritten)
{
    QMngHandlerPrivate *pMydata = reinterpret_cast<QMngHandlerPrivate *>(mng_get_userdata(hMNG));
    return pMydata->writeData(pBuf, iSize, pWritten);
}

static mng_bool myprocessheader(mng_handle hMNG,
                                mng_uint32 iWidth,
                                mng_uint32 iHeight)
{
    QMngHandlerPrivate *pMydata = reinterpret_cast<QMngHandlerPrivate *>(mng_get_userdata(hMNG));
    return pMydata->processHeader(iWidth, iHeight);
}

static mng_ptr mygetcanvasline(mng_handle hMNG,
			mng_uint32 iLinenr)
{
    QMngHandlerPrivate *pMydata = reinterpret_cast<QMngHandlerPrivate *>(mng_get_userdata(hMNG));
    return (mng_ptr)pMydata->image.scanLine(iLinenr);
}

static mng_bool myrefresh(mng_handle /*hMNG*/,
                          mng_uint32 /*iX*/,
                          mng_uint32 /*iY*/,
                          mng_uint32 /*iWidth*/,
                          mng_uint32 /*iHeight*/)
{
    return MNG_TRUE;
}

static mng_uint32 mygettickcount(mng_handle hMNG)
{
    QMngHandlerPrivate *pMydata = reinterpret_cast<QMngHandlerPrivate *>(mng_get_userdata(hMNG));
    return pMydata->elapsed++;
}

static mng_bool mysettimer(mng_handle hMNG,
		    mng_uint32 iMsecs)
{
    QMngHandlerPrivate *pMydata = reinterpret_cast<QMngHandlerPrivate *>(mng_get_userdata(hMNG));
    pMydata->elapsed += iMsecs;
    pMydata->nextDelay = iMsecs;
    return MNG_TRUE;
}

static mng_bool myprocessterm(mng_handle hMNG,
                        mng_uint8   iTermaction,
                        mng_uint8   /*iIteraction*/,
                        mng_uint32  /*iDelay*/,
                        mng_uint32  iItermax)
{
    QMngHandlerPrivate *pMydata = reinterpret_cast<QMngHandlerPrivate *>(mng_get_userdata(hMNG));
    if (iTermaction == 3)
        pMydata->iterCount = iItermax;
    return MNG_TRUE;
}

static mng_bool mytrace(mng_handle,
                        mng_int32   iFuncnr,
                        mng_int32   iFuncseq,
                        mng_pchar   zFuncname)
{
    qDebug("mng trace: iFuncnr: %d iFuncseq: %d zFuncname: %s", iFuncnr, iFuncseq, zFuncname);
    return MNG_TRUE;
}

QMngHandlerPrivate::QMngHandlerPrivate(QMngHandler *q_ptr)
    : haveReadNone(true), haveReadAll(false), elapsed(0), nextDelay(0), iterCount(1),
      frameIndex(-1), nextIndex(0), frameCount(0), q_ptr(q_ptr)
{
    iStyle = (QSysInfo::ByteOrder == QSysInfo::LittleEndian) ? MNG_CANVAS_BGRA8 : MNG_CANVAS_ARGB8;
    // Initialize libmng
    hMNG = mng_initialize((mng_ptr)this, myalloc, myfree, mytrace);
    if (hMNG) {
	// Set callback functions
        mng_setcb_errorproc(hMNG, myerror);
        mng_setcb_openstream(hMNG, myopenstream);
        mng_setcb_closestream(hMNG, myclosestream);
        mng_setcb_readdata(hMNG, myreaddata);
        mng_setcb_writedata(hMNG, mywritedata);
        mng_setcb_processheader(hMNG, myprocessheader);
        mng_setcb_getcanvasline(hMNG, mygetcanvasline);
        mng_setcb_refresh(hMNG, myrefresh);
        mng_setcb_gettickcount(hMNG, mygettickcount);
        mng_setcb_settimer(hMNG, mysettimer);
        mng_setcb_processterm(hMNG, myprocessterm);
        mng_set_doprogressive(hMNG, MNG_FALSE);
	mng_set_suspensionmode(hMNG, MNG_TRUE);
    }
}

QMngHandlerPrivate::~QMngHandlerPrivate()
{
    mng_cleanup(&hMNG);
}

mng_bool QMngHandlerPrivate::readData(mng_ptr pBuf, mng_uint32 iSize, mng_uint32p pRead)
{
    Q_Q(QMngHandler);
    *pRead = q->device()->read((char *)pBuf, iSize);
    return (*pRead > 0) ? MNG_TRUE : MNG_FALSE;
}

mng_bool QMngHandlerPrivate::writeData(mng_ptr pBuf, mng_uint32 iSize, mng_uint32p pWritten)
{
    Q_Q(QMngHandler);
    *pWritten = q->device()->write((char *)pBuf, iSize);
    return MNG_TRUE;
}

mng_bool QMngHandlerPrivate::processHeader(mng_uint32 iWidth, mng_uint32 iHeight)
{
    if (mng_set_canvasstyle(hMNG, iStyle) != MNG_NOERROR)
        return MNG_FALSE;
    image = QImage(iWidth, iHeight, QImage::Format_ARGB32);
    image.fill(0);
    return MNG_TRUE;
}

bool QMngHandlerPrivate::getNextImage(QImage *result)
{
    mng_retcode ret;
    if (haveReadNone) {
        haveReadNone = false;
        ret = mng_readdisplay(hMNG);
    } else {
        ret = mng_display_resume(hMNG);
    }
    if ((MNG_NOERROR == ret) || (MNG_NEEDTIMERWAIT == ret)) {
        *result = image;
        frameIndex = nextIndex++;
        if (haveReadAll && (frameCount == 0))
            frameCount = nextIndex;
        return true;
    }
    return false;
}

bool QMngHandlerPrivate::writeImage(const QImage &image)
{
    mng_reset(hMNG);
    if (mng_create(hMNG) != MNG_NOERROR)
	return false;

    this->image = image.convertToFormat(QImage::Format_ARGB32);
    int w = image.width();
    int h = image.height();

    if (
    // width, height, ticks, layercount, framecount, playtime, simplicity
         (mng_putchunk_mhdr(hMNG, w, h, 1000, 0, 0, 0, 7) == MNG_NOERROR) &&
    // termination_action, action_after_iterations, delay, iteration_max
         (mng_putchunk_term(hMNG, 3, 0, 1, 0x7FFFFFFF) == MNG_NOERROR) &&
    // width, height, bitdepth, colortype, compression, filter, interlace
         (mng_putchunk_ihdr(hMNG, w, h, 8, 6, 0, 0, 0) == MNG_NOERROR) &&
    // width, height, colortype, bitdepth, compression, filter, interlace, canvasstyle, getcanvasline
         (mng_putimgdata_ihdr(hMNG, w, h, 6, 8, 0, 0, 0, iStyle, mygetcanvasline) == MNG_NOERROR) &&
         (mng_putchunk_iend(hMNG) == MNG_NOERROR) &&
         (mng_putchunk_mend(hMNG) == MNG_NOERROR) &&
         (mng_write(hMNG) == MNG_NOERROR)
        )
        return true;
    return false;
}

int QMngHandlerPrivate::currentImageNumber() const
{
//    return mng_get_currentframe(hMNG) % imageCount(); not implemented, apparently
    return frameIndex;
}

int QMngHandlerPrivate::imageCount() const
{
//    return mng_get_totalframes(hMNG); not implemented, apparently
    if (haveReadAll)
        return frameCount;
    return 0; // Don't know
}

bool QMngHandlerPrivate::jumpToImage(int imageNumber)
{
    if (imageNumber == nextIndex)
        return true;

    if ((imageNumber == 0) && haveReadAll && (nextIndex == frameCount)) {
        // Loop!
        nextIndex = 0;
        return true;
    }
    if (mng_display_freeze(hMNG) == MNG_NOERROR) {
        if (mng_display_goframe(hMNG, imageNumber) == MNG_NOERROR) {
            nextIndex = imageNumber;
            return true;
        }
    }
    return false;
}

bool QMngHandlerPrivate::jumpToNextImage()
{
    return jumpToImage((currentImageNumber()+1) % imageCount());
}

int QMngHandlerPrivate::nextImageDelay() const
{
    return nextDelay;
}

bool QMngHandlerPrivate::setBackgroundColor(const QColor &color)
{
    mng_uint16 iRed = (mng_uint16)(color.red() << 8);
    mng_uint16 iBlue = (mng_uint16)(color.blue() << 8);
    mng_uint16 iGreen = (mng_uint16)(color.green() << 8);
    return (mng_set_bgcolor(hMNG, iRed, iBlue, iGreen) == MNG_NOERROR);
}

QColor QMngHandlerPrivate::backgroundColor() const
{
    mng_uint16 iRed;
    mng_uint16 iBlue;
    mng_uint16 iGreen;
    if (mng_get_bgcolor(hMNG, &iRed, &iBlue, &iGreen) == MNG_NOERROR)
        return QColor((iRed >> 8) & 0xFF, (iGreen >> 8) & 0xFF, (iBlue >> 8) & 0xFF);
    return QColor();
}

QMngHandler::QMngHandler()
    : d_ptr(new QMngHandlerPrivate(this))
{
}

QMngHandler::~QMngHandler()
{
}

/*! \reimp */
bool QMngHandler::canRead() const
{
    Q_D(const QMngHandler);
    if ((!d->haveReadNone
         && (!d->haveReadAll || (d->haveReadAll && (d->nextIndex < d->frameCount))))
        || canRead(device()))
    {
        setFormat("mng");
        return true;
    }
    return false;
}

/*! \internal */
bool QMngHandler::canRead(QIODevice *device)
{
    if (!device) {
        qWarning("QMngHandler::canRead() called with no device");
        return false;
    }

    return device->peek(8) == "\x8A\x4D\x4E\x47\x0D\x0A\x1A\x0A";
}

/*! \reimp */
QByteArray QMngHandler::name() const
{
    return "mng";    
}

/*! \reimp */
bool QMngHandler::read(QImage *image)
{
    Q_D(QMngHandler);
    return canRead() ? d->getNextImage(image) : false;
}

/*! \reimp */
bool QMngHandler::write(const QImage &image)
{
    Q_D(QMngHandler);
    return d->writeImage(image);
}

/*! \reimp */
int QMngHandler::currentImageNumber() const
{
    Q_D(const QMngHandler);
    return d->currentImageNumber();
}

/*! \reimp */
int QMngHandler::imageCount() const
{
    Q_D(const QMngHandler);
    return d->imageCount();
}

/*! \reimp */
bool QMngHandler::jumpToImage(int imageNumber)
{
    Q_D(QMngHandler);
    return d->jumpToImage(imageNumber);
}

/*! \reimp */
bool QMngHandler::jumpToNextImage()
{
    Q_D(QMngHandler);
    return d->jumpToNextImage();
}

/*! \reimp */
int QMngHandler::loopCount() const
{
    Q_D(const QMngHandler);
    if (d->iterCount == 0x7FFFFFFF)
        return -1; // infinite loop
    return d->iterCount-1;
}

/*! \reimp */
int QMngHandler::nextImageDelay() const
{
    Q_D(const QMngHandler);
    return d->nextImageDelay();
}

/*! \reimp */
QVariant QMngHandler::option(ImageOption option) const
{
    Q_D(const QMngHandler);
    if (option == QImageIOHandler::Animation)
        return true;
    else if (option == QImageIOHandler::BackgroundColor)
        return d->backgroundColor();
    return QVariant();
}

/*! \reimp */
void QMngHandler::setOption(ImageOption option, const QVariant & value)
{
    Q_D(QMngHandler);
    if (option == QImageIOHandler::BackgroundColor)
        d->setBackgroundColor(qvariant_cast<QColor>(value));
}

/*! \reimp */
bool QMngHandler::supportsOption(ImageOption option) const
{
    if (option == QImageIOHandler::Animation)
        return true;
    else if (option == QImageIOHandler::BackgroundColor)
        return true;
    return false;
}

QT_END_NAMESPACE
