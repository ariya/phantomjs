/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "XSLTUnicodeSort.h"

#if ENABLE(XSLT)

#include "PlatformString.h"
#include <libxslt/templates.h>
#include <libxslt/xsltutils.h>
#include <wtf/unicode/Collator.h>

#if PLATFORM(MAC)
#include "SoftLinking.h"
#endif

#if PLATFORM(MAC)

SOFT_LINK_LIBRARY(libxslt)
SOFT_LINK(libxslt, xsltComputeSortResult, xmlXPathObjectPtr*, (xsltTransformContextPtr ctxt, xmlNodePtr sort), (ctxt, sort))
SOFT_LINK(libxslt, xsltEvalAttrValueTemplate, xmlChar*, (xsltTransformContextPtr ctxt, xmlNodePtr node, const xmlChar *name, const xmlChar *ns), (ctxt, node, name, ns))

static void xsltTransformErrorTrampoline(xsltTransformContextPtr, xsltStylesheetPtr, xmlNodePtr, const char* message, ...) WTF_ATTRIBUTE_PRINTF(4, 5);

void xsltTransformErrorTrampoline(xsltTransformContextPtr context, xsltStylesheetPtr style, xmlNodePtr node, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    char* messageWithArgs;
    vasprintf(&messageWithArgs, message, args);
    va_end(args);

    static void (*xsltTransformErrorPointer)(xsltTransformContextPtr, xsltStylesheetPtr, xmlNodePtr, const char*, ...) WTF_ATTRIBUTE_PRINTF(4, 5)
        = reinterpret_cast<void (*)(xsltTransformContextPtr, xsltStylesheetPtr, xmlNodePtr, const char*, ...)>(dlsym(libxsltLibrary(), "xsltTransformError"));
    xsltTransformErrorPointer(context, style, node, "%s", messageWithArgs);

    free(messageWithArgs);
}

#define xsltTransformError xsltTransformErrorTrampoline

#endif

namespace WebCore {

// Based on default implementation from libxslt 1.1.22 and xsltICUSort.c example.
void xsltUnicodeSortFunction(xsltTransformContextPtr ctxt, xmlNodePtr *sorts, int nbsorts)
{
#ifdef XSLT_REFACTORED
    xsltStyleItemSortPtr comp;
#else
    xsltStylePreCompPtr comp;
#endif
    xmlXPathObjectPtr *resultsTab[XSLT_MAX_SORT];
    xmlXPathObjectPtr *results = NULL, *res;
    xmlNodeSetPtr list = NULL;
    int descending, number, desc, numb;
    int len = 0;
    int i, j, incr;
    int tst;
    int depth;
    xmlNodePtr node;
    xmlXPathObjectPtr tmp;    
    int tempstype[XSLT_MAX_SORT], temporder[XSLT_MAX_SORT];

    if ((ctxt == NULL) || (sorts == NULL) || (nbsorts <= 0) ||
        (nbsorts >= XSLT_MAX_SORT))
        return;
    if (sorts[0] == NULL)
        return;
    comp = static_cast<xsltStylePreComp*>(sorts[0]->psvi);
    if (comp == NULL)
        return;

    list = ctxt->nodeList;
    if ((list == NULL) || (list->nodeNr <= 1))
        return; /* nothing to do */

    for (j = 0; j < nbsorts; j++) {
        comp = static_cast<xsltStylePreComp*>(sorts[j]->psvi);
        tempstype[j] = 0;
        if ((comp->stype == NULL) && (comp->has_stype != 0)) {
            comp->stype =
                xsltEvalAttrValueTemplate(ctxt, sorts[j],
                                          (const xmlChar *) "data-type",
                                          XSLT_NAMESPACE);
            if (comp->stype != NULL) {
                tempstype[j] = 1;
                if (xmlStrEqual(comp->stype, (const xmlChar *) "text"))
                    comp->number = 0;
                else if (xmlStrEqual(comp->stype, (const xmlChar *) "number"))
                    comp->number = 1;
                else {
                    xsltTransformError(ctxt, NULL, sorts[j],
                          "xsltDoSortFunction: no support for data-type = %s\n",
                                     comp->stype);
                    comp->number = 0; /* use default */
                }
            }
        }
        temporder[j] = 0;
        if ((comp->order == NULL) && (comp->has_order != 0)) {
            comp->order = xsltEvalAttrValueTemplate(ctxt, sorts[j],
                                                    (const xmlChar *) "order",
                                                    XSLT_NAMESPACE);
            if (comp->order != NULL) {
                temporder[j] = 1;
                if (xmlStrEqual(comp->order, (const xmlChar *) "ascending"))
                    comp->descending = 0;
                else if (xmlStrEqual(comp->order,
                                     (const xmlChar *) "descending"))
                    comp->descending = 1;
                else {
                    xsltTransformError(ctxt, NULL, sorts[j],
                             "xsltDoSortFunction: invalid value %s for order\n",
                                     comp->order);
                    comp->descending = 0; /* use default */
                }
            }
        }
    }

    len = list->nodeNr;

    resultsTab[0] = xsltComputeSortResult(ctxt, sorts[0]);
    for (i = 1;i < XSLT_MAX_SORT;i++)
        resultsTab[i] = NULL;

    results = resultsTab[0];

    comp = static_cast<xsltStylePreComp*>(sorts[0]->psvi);
    descending = comp->descending;
    number = comp->number;
    if (results == NULL)
        return;

    // We are passing a language identifier to a function that expects a locale identifier.
    // The implementation of Collator should be lenient, and accept both "en-US" and "en_US", for example.
    // This lets an author to really specify sorting rules, e.g. "de_DE@collation=phonebook", which isn't
    // possible with language alone.
    Collator collator(comp->has_lang ? (const char*)comp->lang : "en");
    collator.setOrderLowerFirst(comp->lower_first);

    /* Shell's sort of node-set */
    for (incr = len / 2; incr > 0; incr /= 2) {
        for (i = incr; i < len; i++) {
            j = i - incr;
            if (results[i] == NULL)
                continue;
            
            while (j >= 0) {
                if (results[j] == NULL)
                    tst = 1;
                else {
                    if (number) {
                        /* We make NaN smaller than number in accordance
                           with XSLT spec */
                        if (xmlXPathIsNaN(results[j]->floatval)) {
                            if (xmlXPathIsNaN(results[j + incr]->floatval))
                                tst = 0;
                            else
                                tst = -1;
                        } else if (xmlXPathIsNaN(results[j + incr]->floatval))
                            tst = 1;
                        else if (results[j]->floatval ==
                                results[j + incr]->floatval)
                            tst = 0;
                        else if (results[j]->floatval > 
                                results[j + incr]->floatval)
                            tst = 1;
                        else tst = -1;
                    } else {
                        String str1 = String::fromUTF8((const char*)results[j]->stringval);
                        String str2 = String::fromUTF8((const char*)results[j + incr]->stringval);
                        tst = collator.collate(str1.characters(), str1.length(), str2.characters(), str2.length());
                    }
                    if (descending)
                        tst = -tst;
                }
                if (tst == 0) {
                    /*
                     * Okay we need to use multi level sorts
                     */
                    depth = 1;
                    while (depth < nbsorts) {
                        if (sorts[depth] == NULL)
                            break;
                        comp = static_cast<xsltStylePreComp*>(sorts[depth]->psvi);
                        if (comp == NULL)
                            break;
                        desc = comp->descending;
                        numb = comp->number;

                        /*
                         * Compute the result of the next level for the
                         * full set, this might be optimized ... or not
                         */
                        if (resultsTab[depth] == NULL) 
                            resultsTab[depth] = xsltComputeSortResult(ctxt,
                                                        sorts[depth]);
                        res = resultsTab[depth];
                        if (res == NULL) 
                            break;
                        if (res[j] == NULL) {
                            if (res[j+incr] != NULL)
                                tst = 1;
                        } else {
                            if (numb) {
                                /* We make NaN smaller than number in
                                   accordance with XSLT spec */
                                if (xmlXPathIsNaN(res[j]->floatval)) {
                                    if (xmlXPathIsNaN(res[j +
                                                    incr]->floatval))
                                        tst = 0;
                                    else
                                        tst = -1;
                                } else if (xmlXPathIsNaN(res[j + incr]->
                                                floatval))
                                    tst = 1;
                                else if (res[j]->floatval == res[j + incr]->
                                                floatval)
                                    tst = 0;
                                else if (res[j]->floatval > 
                                        res[j + incr]->floatval)
                                    tst = 1;
                                else tst = -1;
                            } else {
                                String str1 = String::fromUTF8((const char*)res[j]->stringval);
                                String str2 = String::fromUTF8((const char*)res[j + incr]->stringval);
                                tst = collator.collate(str1.characters(), str1.length(), str2.characters(), str2.length());
                            }
                            if (desc)
                                tst = -tst;
                        }

                        /*
                         * if we still can't differenciate at this level
                         * try one level deeper.
                         */
                        if (tst != 0)
                            break;
                        depth++;
                    }
                }
                if (tst == 0) {
                    tst = results[j]->index > results[j + incr]->index;
                }
                if (tst > 0) {
                    tmp = results[j];
                    results[j] = results[j + incr];
                    results[j + incr] = tmp;
                    node = list->nodeTab[j];
                    list->nodeTab[j] = list->nodeTab[j + incr];
                    list->nodeTab[j + incr] = node;
                    depth = 1;
                    while (depth < nbsorts) {
                        if (sorts[depth] == NULL)
                            break;
                        if (resultsTab[depth] == NULL)
                            break;
                        res = resultsTab[depth];
                        tmp = res[j];
                        res[j] = res[j + incr];
                        res[j + incr] = tmp;
                        depth++;
                    }
                    j -= incr;
                } else
                    break;
            }
        }
    }

    for (j = 0; j < nbsorts; j++) {
        comp = static_cast<xsltStylePreComp*>(sorts[j]->psvi);
        if (tempstype[j] == 1) {
            /* The data-type needs to be recomputed each time */
            xmlFree((void *)(comp->stype));
            comp->stype = NULL;
        }
        if (temporder[j] == 1) {
            /* The order needs to be recomputed each time */
            xmlFree((void *)(comp->order));
            comp->order = NULL;
        }
        if (resultsTab[j] != NULL) {
            for (i = 0;i < len;i++)
                xmlXPathFreeObject(resultsTab[j][i]);
            xmlFree(resultsTab[j]);
        }
    }
}

}

#endif
