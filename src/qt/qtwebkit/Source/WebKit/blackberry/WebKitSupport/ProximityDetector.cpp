/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
 */

#include "config.h"
#include "ProximityDetector.h"

#include "Document.h"
#include "Element.h"
#include "ExceptionCode.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "RenderLayer.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "WebPage_p.h"

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

static int getPriorityLevel(Node* node)
{
    // Priority level is ascending with zero being the lowest.
    if (node->isTextNode())
        return 1;

    if (!node->isElementNode())
        return 0;

    Element* element = toElement(node);
    ASSERT(element);
    ExceptionCode ec = 0;

    if (element->webkitMatchesSelector("img,obj", ec))
        return 1;

    if (element->hasTagName(HTMLNames::pTag))
        return 2;

    if (element->webkitMatchesSelector("h1,h2,h3,h4,h5,h6", ec))
        return 3;

    return 0;
}

ProximityDetector::ProximityDetector(WebPagePrivate* webPage)
    : m_webPage(webPage)
{
}

ProximityDetector::~ProximityDetector()
{
}

IntPoint ProximityDetector::findBestPoint(const IntPoint& documentPos, const IntRect& documentPaddingRect)
{
    ASSERT(m_webPage);

    if (!m_webPage->m_mainFrame)
        return documentPos;

    Document* document = m_webPage->m_mainFrame->document();

    if (!document || !document->frame()->view())
        return documentPos;

    unsigned left = -documentPaddingRect.x();
    unsigned top = -documentPaddingRect.y();
    unsigned right = documentPaddingRect.maxX();
    unsigned bottom = documentPaddingRect.maxY();

    // Adjust hit point to frame
    IntPoint frameContentPos(document->frame()->view()->windowToContents(m_webPage->m_mainFrame->view()->contentsToWindow(documentPos)));
    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::IgnoreClipping | HitTestRequest::DisallowShadowContent);
    HitTestResult result(frameContentPos, top, right, bottom, left);
    document->renderView()->layer()->hitTest(request, result);

    IntPoint bestPoint = documentPos;
    int bestPriority = 0;

    // Iterate over the list of nodes checking both priority and location
    ListHashSet<RefPtr<Node> > intersectedNodes = result.rectBasedTestResult();
    ListHashSet<RefPtr<Node> >::const_iterator it = intersectedNodes.begin();
    ListHashSet<RefPtr<Node> >::const_iterator end = intersectedNodes.end();
    for ( ; it != end; ++it) {

        Node* curNode = (*it).get();
        if (!curNode || !curNode->renderer())
            continue;

        IntRect curRect = curNode->renderer()->absoluteBoundingBoxRect(true /*use transforms*/);
        IntRect hitTestRect = HitTestLocation::rectForPoint(documentPos, top, right, bottom, left);

        // Check that top corner does not exceed padding
        if (!hitTestRect.contains(curRect.location()))
            continue;

        int priority = getPriorityLevel(curNode);
        if (!priority)
            continue;

        bool equalPriorityAndCloser = (priority == bestPriority) && (documentPos.distanceSquaredToPoint(bestPoint) > documentPos.distanceSquaredToPoint(curRect.location()));
        if (priority > bestPriority || equalPriorityAndCloser) {
            bestPoint = curRect.location(); // use top left
            bestPriority = priority;
        }
    }

    return bestPoint;
}

}
}
