
#include "config.h"

#if ENABLE(SMOOTH_SCROLLING)

#include "ScrollAnimatorBlackBerry.h"

#include "ScrollableArea.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

PassOwnPtr<ScrollAnimator> ScrollAnimator::create(ScrollableArea* scrollableArea)
{
    if (scrollableArea && scrollableArea->scrollAnimatorEnabled())
        return adoptPtr(new ScrollAnimatorBlackBerry(scrollableArea));
    return adoptPtr(new ScrollAnimator(scrollableArea));
}

ScrollAnimatorBlackBerry::ScrollAnimatorBlackBerry(ScrollableArea* scrollableArea)
    : ScrollAnimatorNone(scrollableArea)
    , m_disableConstrainsScrollingToContentEdgeWhileAnimating(false)
    , m_originalConstransScrollingToContentEdge(true)
{
}

void ScrollAnimatorBlackBerry::animationWillStart()
{
    if (m_disableConstrainsScrollingToContentEdgeWhileAnimating) {
        m_originalConstransScrollingToContentEdge = scrollableArea()->constrainsScrollingToContentEdge();
        scrollableArea()->setConstrainsScrollingToContentEdge(false);
    }
}

void ScrollAnimatorBlackBerry::animationDidFinish()
{
    if (m_disableConstrainsScrollingToContentEdgeWhileAnimating) {
        scrollableArea()->setConstrainsScrollingToContentEdge(m_originalConstransScrollingToContentEdge);
        m_disableConstrainsScrollingToContentEdgeWhileAnimating = false;
    }
}

void ScrollAnimatorBlackBerry::setDisableConstrainsScrollingToContentEdgeWhileAnimating(bool value)
{
    m_disableConstrainsScrollingToContentEdgeWhileAnimating = value;
}

} // namespace WebCore

#endif // ENABLE(SMOOTH_SCROLLING)
