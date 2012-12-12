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
#include <qdebug.h>
#include "qcups_p.h"

#ifndef QT_NO_CUPS

#ifndef QT_LINUXBASE // LSB merges everything into cups.h
# include <cups/language.h>
#endif
#include <qtextcodec.h>

QT_BEGIN_NAMESPACE

typedef int (*CupsGetDests)(cups_dest_t **dests);
typedef void (*CupsFreeDests)(int num_dests, cups_dest_t *dests);
typedef const char* (*CupsGetPPD)(const char *printer);
typedef int (*CupsMarkOptions)(ppd_file_t *ppd, int num_options, cups_option_t *options);
typedef ppd_file_t* (*PPDOpenFile)(const char *filename);
typedef void (*PPDMarkDefaults)(ppd_file_t *ppd);
typedef int (*PPDMarkOption)(ppd_file_t *ppd, const char *keyword, const char *option);
typedef void (*PPDClose)(ppd_file_t *ppd);
typedef int (*PPDMarkOption)(ppd_file_t *ppd, const char *keyword, const char *option);
typedef void (*CupsFreeOptions)(int num_options, cups_option_t *options);
typedef void (*CupsSetDests)(int num_dests, cups_dest_t *dests);
typedef cups_lang_t* (*CupsLangGet)(const char *language);
typedef const char* (*CupsLangEncoding)(cups_lang_t *language);
typedef int (*CupsAddOption)(const char *name, const char *value, int num_options, cups_option_t **options);
typedef int (*CupsTempFd)(char *name, int len);
typedef int (*CupsPrintFile)(const char * name, const char * filename, const char * title, int num_options, cups_option_t * options);

static bool cupsLoaded = false;
static int qt_cups_num_printers = 0;
static CupsGetDests _cupsGetDests = 0;
static CupsFreeDests _cupsFreeDests = 0;
static CupsGetPPD _cupsGetPPD = 0;
static PPDOpenFile _ppdOpenFile = 0;
static PPDMarkDefaults _ppdMarkDefaults = 0;
static PPDClose _ppdClose = 0;
static CupsMarkOptions _cupsMarkOptions = 0;
static PPDMarkOption _ppdMarkOption = 0;
static CupsFreeOptions _cupsFreeOptions = 0;
static CupsSetDests _cupsSetDests = 0;
static CupsLangGet _cupsLangGet = 0;
static CupsLangEncoding _cupsLangEncoding = 0;
static CupsAddOption _cupsAddOption = 0;
static CupsTempFd _cupsTempFd = 0;
static CupsPrintFile _cupsPrintFile = 0;

static void resolveCups()
{
    QLibrary cupsLib(QLatin1String("cups"), 2);
    if(cupsLib.load()) {
        _cupsGetDests = (CupsGetDests) cupsLib.resolve("cupsGetDests");
        _cupsFreeDests = (CupsFreeDests) cupsLib.resolve("cupsFreeDests");
        _cupsGetPPD = (CupsGetPPD) cupsLib.resolve("cupsGetPPD");
        _cupsLangGet = (CupsLangGet) cupsLib.resolve("cupsLangGet");
        _cupsLangEncoding = (CupsLangEncoding) cupsLib.resolve("cupsLangEncoding");
        _ppdOpenFile = (PPDOpenFile) cupsLib.resolve("ppdOpenFile");
        _ppdMarkDefaults = (PPDMarkDefaults) cupsLib.resolve("ppdMarkDefaults");
        _ppdClose = (PPDClose) cupsLib.resolve("ppdClose");
        _cupsMarkOptions = (CupsMarkOptions) cupsLib.resolve("cupsMarkOptions");
        _ppdMarkOption = (PPDMarkOption) cupsLib.resolve("ppdMarkOption");
        _cupsFreeOptions = (CupsFreeOptions) cupsLib.resolve("cupsFreeOptions");
        _cupsSetDests = (CupsSetDests) cupsLib.resolve("cupsSetDests");
        _cupsAddOption = (CupsAddOption) cupsLib.resolve("cupsAddOption");
        _cupsTempFd = (CupsTempFd) cupsLib.resolve("cupsTempFd");
        _cupsPrintFile = (CupsPrintFile) cupsLib.resolve("cupsPrintFile");

        if (_cupsGetDests && _cupsFreeDests) {
            cups_dest_t *printers;
            int num_printers = _cupsGetDests(&printers);
            if (num_printers)
                _cupsFreeDests(num_printers, printers);
            qt_cups_num_printers = num_printers;
        }
    }
    cupsLoaded = true;
}

// ================ CUPS Support class ========================

QCUPSSupport::QCUPSSupport()
    :
    prnCount(0),
    printers(0),
    page_sizes(0),
    currPrinterIndex(0),
    currPPD(0)
{
    if (!cupsLoaded)
        resolveCups();

    // getting all available printers
    if (!isAvailable())
        return;

    // Update the available printer count
    qt_cups_num_printers = prnCount = _cupsGetDests(&printers);

    for (int i = 0; i <  prnCount; ++i) {
        if (printers[i].is_default) {
            currPrinterIndex = i;
            setCurrentPrinter(i);
            break;
        }
    }

#ifndef QT_NO_TEXTCODEC
    cups_lang_t *cupsLang = _cupsLangGet(0);
    codec = QTextCodec::codecForName(_cupsLangEncoding(cupsLang));
    if (!codec)
        codec = QTextCodec::codecForLocale();
#endif
}

QCUPSSupport::~QCUPSSupport()
{
     if (currPPD)
        _ppdClose(currPPD);
     if (prnCount)
         _cupsFreeDests(prnCount, printers);
}

int QCUPSSupport::availablePrintersCount() const
{
    return prnCount;
}

const cups_dest_t* QCUPSSupport::availablePrinters() const
{
    return printers;
}

const ppd_file_t* QCUPSSupport::currentPPD() const
{
    return currPPD;
}

const ppd_file_t* QCUPSSupport::setCurrentPrinter(int index)
{
    Q_ASSERT(index >= 0 && index <= prnCount);
    if (index == prnCount)
        return 0;

    currPrinterIndex = index;

    if (currPPD)
        _ppdClose(currPPD);
    currPPD = 0;
    page_sizes = 0;

    const char *ppdFile = _cupsGetPPD(printers[index].name);

    if (!ppdFile)
      return 0;

    currPPD = _ppdOpenFile(ppdFile);
    unlink(ppdFile);

    // marking default options
    _ppdMarkDefaults(currPPD);

    // marking options explicitly set
    _cupsMarkOptions(currPPD, printers[currPrinterIndex].num_options, printers[currPrinterIndex].options);

    // getting pointer to page sizes
    page_sizes = ppdOption("PageSize");

    return currPPD;
}

int QCUPSSupport::currentPrinterIndex() const
{
    return currPrinterIndex;
}

bool QCUPSSupport::isAvailable()
{
    if(!cupsLoaded)
        resolveCups();

    return _cupsGetDests &&
        _cupsFreeDests &&
        _cupsGetPPD &&
        _ppdOpenFile &&
        _ppdMarkDefaults &&
        _ppdClose &&
        _cupsMarkOptions &&
        _ppdMarkOption &&
        _cupsFreeOptions &&
        _cupsSetDests &&
        _cupsLangGet &&
        _cupsLangEncoding &&
        _cupsAddOption &&
        (qt_cups_num_printers > 0);
}

const ppd_option_t* QCUPSSupport::ppdOption(const char *key) const
{
    if (currPPD) {
        for (int gr = 0; gr < currPPD->num_groups; ++gr) {
            for (int opt = 0; opt < currPPD->groups[gr].num_options; ++opt) {
                if (qstrcmp(currPPD->groups[gr].options[opt].keyword, key) == 0)
                    return &currPPD->groups[gr].options[opt];
            }
        }
    }
    return 0;
}

const cups_option_t* QCUPSSupport::printerOption(const QString &key) const
{
    for (int i = 0; i < printers[currPrinterIndex].num_options; ++i) {
        if (QLatin1String(printers[currPrinterIndex].options[i].name) == key)
            return &printers[currPrinterIndex].options[i];
    }
    return 0;
}

const ppd_option_t* QCUPSSupport::pageSizes() const
{
    return page_sizes;
}

int QCUPSSupport::markOption(const char* name, const char* value)
{
    return _ppdMarkOption(currPPD, name, value);
}

void QCUPSSupport::saveOptions(QList<const ppd_option_t*> options, QList<const char*> markedOptions)
{
    int oldOptionCount = printers[currPrinterIndex].num_options;
    cups_option_t* oldOptions = printers[currPrinterIndex].options;

    int newOptionCount = 0;
    cups_option_t* newOptions = 0;

    // copying old options that are not on the new list
    for (int i = 0; i < oldOptionCount; ++i) {
        bool contains = false;
        for (int j = 0; j < options.count(); ++j) {
            if (qstrcmp(options.at(j)->keyword, oldOptions[i].name) == 0) {
                contains = true;
                break;
            }
        }

        if (!contains) {
            newOptionCount = _cupsAddOption(oldOptions[i].name, oldOptions[i].value, newOptionCount, &newOptions);
        }
    }

    // we can release old option list
     _cupsFreeOptions(oldOptionCount, oldOptions);

    // adding marked options
    for (int i = 0; i < markedOptions.count(); ++i) {
        const char* name = markedOptions.at(i);
        ++i;
        newOptionCount = _cupsAddOption(name, markedOptions.at(i), newOptionCount, &newOptions);
    }

    // placing the new option list
    printers[currPrinterIndex].num_options = newOptionCount;
    printers[currPrinterIndex].options = newOptions;

    // saving new default values
    _cupsSetDests(prnCount, printers);
}

QRect QCUPSSupport::paperRect(const char *choice) const
{
    if (!currPPD)
        return QRect();
    for (int i = 0; i < currPPD->num_sizes; ++i) {
        if (qstrcmp(currPPD->sizes[i].name, choice) == 0)
            return QRect(0, 0, qRound(currPPD->sizes[i].width), qRound(currPPD->sizes[i].length));
    }
    return QRect();
}

QRect QCUPSSupport::pageRect(const char *choice) const
{
    if (!currPPD)
        return QRect();
    for (int i = 0; i < currPPD->num_sizes; ++i) {
        if (qstrcmp(currPPD->sizes[i].name, choice) == 0)
            return QRect(qRound(currPPD->sizes[i].left),
                         qRound(currPPD->sizes[i].length - currPPD->sizes[i].top),
                         qRound(currPPD->sizes[i].right - currPPD->sizes[i].left),
                         qRound(currPPD->sizes[i].top - currPPD->sizes[i].bottom));
    }
    return QRect();
}

QStringList QCUPSSupport::options() const
{
    QStringList list;
    collectMarkedOptions(list);
    return list;
}

bool QCUPSSupport::printerHasPPD(const char *printerName)
{
    if (!isAvailable())
        return false;
    const char *ppdFile = _cupsGetPPD(printerName);
    if (ppdFile)
        unlink(ppdFile);
    return (ppdFile != 0);
}

QString QCUPSSupport::unicodeString(const char *s)
{
#ifndef QT_NO_TEXTCODEC
    return codec->toUnicode(s);
#else
    return QLatin1String(s);
#endif
}

void QCUPSSupport::collectMarkedOptions(QStringList& list, const ppd_group_t* group) const
{
    if (group == 0) {
        if (!currPPD)
            return;
        for (int i = 0; i < currPPD->num_groups; ++i) {
            collectMarkedOptions(list, &currPPD->groups[i]);
            collectMarkedOptionsHelper(list, &currPPD->groups[i]);
        }
    } else {
        for (int i = 0; i < group->num_subgroups; ++i)
            collectMarkedOptionsHelper(list, &group->subgroups[i]);
    }
}

void QCUPSSupport::collectMarkedOptionsHelper(QStringList& list, const ppd_group_t* group) const
{
    for (int i = 0; i < group->num_options; ++i) {
        for (int j = 0; j < group->options[i].num_choices; ++j) {
            if (group->options[i].choices[j].marked == 1 && qstrcmp(group->options[i].choices[j].choice, group->options[i].defchoice) != 0)
                list << QString::fromLocal8Bit(group->options[i].keyword) << QString::fromLocal8Bit(group->options[i].choices[j].choice);
        }
    }
}

QPair<int, QString> QCUPSSupport::tempFd()
{
    char filename[512];
    int fd = _cupsTempFd(filename, 512);
    return QPair<int, QString>(fd, QString::fromLocal8Bit(filename));
}

// Prints the given file and returns a job id.
int QCUPSSupport::printFile(const char * printerName, const char * filename, const char * title,
                            int num_options, cups_option_t * options)
{
    return _cupsPrintFile(printerName, filename, title, num_options, options);
}

QT_END_NAMESPACE

#endif // QT_NO_CUPS
