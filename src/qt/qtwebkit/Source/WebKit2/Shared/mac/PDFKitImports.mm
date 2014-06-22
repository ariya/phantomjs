/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PDFKitImports.h"

namespace WebKit {

NSString *pdfKitFrameworkPath()
{
    NSString *systemLibraryPath = [NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSSystemDomainMask, NO) objectAtIndex:0];
    return [systemLibraryPath stringByAppendingPathComponent:@"Frameworks/Quartz.framework/Frameworks/PDFKit.framework"];
}

Class classFromPDFKit(NSString *className)
{
    static NSBundle *pdfKitBundle = [NSBundle bundleWithPath:pdfKitFrameworkPath()];
    [pdfKitBundle load];
    return [pdfKitBundle classNamed:className];
}

Class pdfAnnotationLinkClass()
{
    static Class pdfAnnotationLinkClass = classFromPDFKit(@"PDFAnnotationLink");
    ASSERT(pdfAnnotationLinkClass);
    return pdfAnnotationLinkClass;
}

Class pdfDocumentClass()
{
    static Class pdfDocumentClass = classFromPDFKit(@"PDFDocument");
    ASSERT(pdfDocumentClass);
    return pdfDocumentClass;
}
    
#if ENABLE(PDFKIT_PLUGIN)
Class pdfLayerControllerClass()
{
    static Class pdfLayerControllerClass = classFromPDFKit(@"PDFLayerController");
    ASSERT(pdfLayerControllerClass);
    return pdfLayerControllerClass;
}

Class pdfAnnotationTextWidgetClass()
{
    static Class pdfAnnotationTextWidgetClass = classFromPDFKit(@"PDFAnnotationTextWidget");
    ASSERT(pdfAnnotationTextWidgetClass);
    return pdfAnnotationTextWidgetClass;
}

Class pdfAnnotationChoiceWidgetClass()
{
    static Class pdfAnnotationChoiceWidgetClass = classFromPDFKit(@"PDFAnnotationChoiceWidget");
    ASSERT(pdfAnnotationChoiceWidgetClass);
    return pdfAnnotationChoiceWidgetClass;
}
#endif

}
