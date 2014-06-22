/**
 * Copyright (C) 2001-2002 Thomas Broyer, Charlie Bozeman and Daniel Veillard.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is fur-
 * nished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
 * NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CON-
 * NECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Except as contained in this notice, the name of the authors shall not
 * be used in advertising or otherwise to promote the sale, use or other deal-
 * ings in this Software without prior written authorization from him.
 */

#include "config.h"

#if ENABLE(XSLT)
#include "XSLTExtensions.h"

#include <libxml/xpathInternals.h>

#include <libxslt/xsltutils.h>
#include <libxslt/extensions.h>
#include <libxslt/extra.h>

#if PLATFORM(MAC)
#include "SoftLinking.h"
#endif

#if PLATFORM(MAC)
SOFT_LINK_LIBRARY(libxslt)
SOFT_LINK(libxslt, xsltRegisterExtFunction, int, (xsltTransformContextPtr ctxt, const xmlChar *name, const xmlChar *URI, xmlXPathFunction function), (ctxt, name, URI, function))
SOFT_LINK(libxslt, xsltFunctionNodeSet, void, (xmlXPathParserContextPtr ctxt, int nargs), (ctxt, nargs))
#endif

namespace WebCore {

// FIXME: This code is taken from libexslt 1.1.11; should sync with newer versions.
static void exsltNodeSetFunction(xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *strval;
    xmlNodePtr retNode;
    xmlXPathObjectPtr ret;

    if (nargs != 1) {
        xmlXPathSetArityError(ctxt);
        return;
    }

    if (xmlXPathStackIsNodeSet(ctxt)) {
        xsltFunctionNodeSet(ctxt, nargs);
        return;
    }

    strval = xmlXPathPopString(ctxt);
    retNode = xmlNewDocText(NULL, strval);
    ret = xmlXPathNewValueTree(retNode);
    
    // FIXME: It might be helpful to push any errors from xmlXPathNewValueTree
    // up to the Javascript Console.
    if (ret != NULL) 
        ret->type = XPATH_NODESET;

    if (strval != NULL)
        xmlFree(strval);

    valuePush(ctxt, ret);
}

void registerXSLTExtensions(xsltTransformContextPtr ctxt)
{
    xsltRegisterExtFunction(ctxt, (const xmlChar*)"node-set", (const xmlChar*)"http://exslt.org/common", exsltNodeSetFunction);
}

}

#endif
