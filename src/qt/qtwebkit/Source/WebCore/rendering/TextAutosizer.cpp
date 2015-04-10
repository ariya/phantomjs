/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(TEXT_AUTOSIZING)

#include "TextAutosizer.h"

#include "Document.h"
#include "HTMLElement.h"
#include "InspectorInstrumentation.h"
#include "IntSize.h"
#include "RenderObject.h"
#include "RenderStyle.h"
#include "RenderText.h"
#include "RenderView.h"
#include "Settings.h"
#include "StyleInheritedData.h"

#include <algorithm>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

namespace WebCore {

using namespace HTMLNames;

struct TextAutosizingWindowInfo {
    IntSize windowSize;
    IntSize minLayoutSize;
};

// Represents cluster related data. Instances should not persist between calls to processSubtree.
struct TextAutosizingClusterInfo {
    explicit TextAutosizingClusterInfo(RenderBlock* root)
        : root(root)
        , blockContainingAllText(0)
        , maxAllowedDifferenceFromTextWidth(150)
    {
    }

    RenderBlock* root;
    const RenderBlock* blockContainingAllText;

    // Upper limit on the difference between the width of the cluster's block containing all
    // text and that of a narrow child before the child becomes a separate cluster.
    float maxAllowedDifferenceFromTextWidth;

    // Descendants of the cluster that are narrower than the block containing all text and must be
    // processed together.
    Vector<TextAutosizingClusterInfo> narrowDescendants;
};


static const Vector<QualifiedName>& formInputTags()
{
    // Returns the tags for the form input elements.
    DEFINE_STATIC_LOCAL(Vector<QualifiedName>, formInputTags, ());
    if (formInputTags.isEmpty()) {
        formInputTags.append(inputTag);
        formInputTags.append(buttonTag);
        formInputTags.append(selectTag);
    }
    return formInputTags;
}

TextAutosizer::TextAutosizer(Document* document)
    : m_document(document)
{
}

TextAutosizer::~TextAutosizer()
{
}

void TextAutosizer::recalculateMultipliers()
{
    RenderObject* renderer = m_document->renderer();
    while (renderer) {
        if (renderer->style() && renderer->style()->textAutosizingMultiplier() != 1)
            setMultiplier(renderer, 1);
        renderer = renderer->nextInPreOrder();
    }
}

bool TextAutosizer::processSubtree(RenderObject* layoutRoot)
{
    // FIXME: Text Autosizing should only be enabled when m_document->page()->mainFrame()->view()->useFixedLayout()
    // is true, but for now it's useful to ignore this so that it can be tested on desktop.
    if (!m_document->settings() || !m_document->settings()->textAutosizingEnabled() || layoutRoot->view()->printing() || !m_document->page())
        return false;

    Frame* mainFrame = m_document->page()->mainFrame();

    TextAutosizingWindowInfo windowInfo;

    // Window area, in logical (density-independent) pixels.
    windowInfo.windowSize = m_document->settings()->textAutosizingWindowSizeOverride();
    if (windowInfo.windowSize.isEmpty()) {
        bool includeScrollbars = !InspectorInstrumentation::shouldApplyScreenWidthOverride(mainFrame);
        windowInfo.windowSize = mainFrame->view()->unscaledVisibleContentSize(includeScrollbars ? ScrollableArea::IncludeScrollbars : ScrollableArea::ExcludeScrollbars);
    }

    // Largest area of block that can be visible at once (assuming the main
    // frame doesn't get scaled to less than overview scale), in CSS pixels.
    windowInfo.minLayoutSize = mainFrame->view()->layoutSize();
    for (Frame* frame = m_document->frame(); frame; frame = frame->tree()->parent()) {
        if (!frame->view()->isInChildFrameWithFrameFlattening())
            windowInfo.minLayoutSize = windowInfo.minLayoutSize.shrunkTo(frame->view()->layoutSize());
    }

    // The layoutRoot could be neither a container nor a cluster, so walk up the tree till we find each of these.
    RenderBlock* container = layoutRoot->isRenderBlock() ? toRenderBlock(layoutRoot) : layoutRoot->containingBlock();
    while (container && !isAutosizingContainer(container))
        container = container->containingBlock();

    RenderBlock* cluster = container;
    while (cluster && (!isAutosizingContainer(cluster) || !isIndependentDescendant(cluster)))
        cluster = cluster->containingBlock();

    TextAutosizingClusterInfo clusterInfo(cluster);
    processCluster(clusterInfo, container, layoutRoot, windowInfo);
    return true;
}

float TextAutosizer::clusterMultiplier(WritingMode writingMode, const TextAutosizingWindowInfo& windowInfo, float textWidth) const
{
    int logicalWindowWidth = isHorizontalWritingMode(writingMode) ? windowInfo.windowSize.width() : windowInfo.windowSize.height();
    int logicalLayoutWidth = isHorizontalWritingMode(writingMode) ? windowInfo.minLayoutSize.width() : windowInfo.minLayoutSize.height();
    // Ignore box width in excess of the layout width, to avoid extreme multipliers.
    float logicalClusterWidth = std::min<float>(textWidth, logicalLayoutWidth);

    float multiplier = logicalClusterWidth / logicalWindowWidth;
    multiplier *= m_document->settings()->textAutosizingFontScaleFactor();
    return std::max(1.0f, multiplier);
}

void TextAutosizer::processClusterInternal(TextAutosizingClusterInfo& clusterInfo, RenderBlock* container, RenderObject* subtreeRoot, const TextAutosizingWindowInfo& windowInfo, float multiplier)
{
    processContainer(multiplier, container, clusterInfo, subtreeRoot, windowInfo);

    Vector<Vector<TextAutosizingClusterInfo> > narrowDescendantsGroups;
    getNarrowDescendantsGroupedByWidth(clusterInfo, narrowDescendantsGroups);
    for (size_t i = 0; i < narrowDescendantsGroups.size(); ++i)
        processCompositeCluster(narrowDescendantsGroups[i], windowInfo);
}

void TextAutosizer::processCluster(TextAutosizingClusterInfo& clusterInfo, RenderBlock* container, RenderObject* subtreeRoot, const TextAutosizingWindowInfo& windowInfo)
{
    // Many pages set a max-width on their content. So especially for the RenderView, instead of
    // just taking the width of |cluster| we find the lowest common ancestor of the first and last
    // descendant text node of the cluster (i.e. the deepest wrapper block that contains all the
    // text), and use its width instead.
    clusterInfo.blockContainingAllText = findDeepestBlockContainingAllText(clusterInfo.root);
    float textWidth = clusterInfo.blockContainingAllText->contentLogicalWidth();
    float multiplier =  1.0;
    if (clusterShouldBeAutosized(clusterInfo, textWidth))
        multiplier = clusterMultiplier(clusterInfo.root->style()->writingMode(), windowInfo, textWidth);
    processClusterInternal(clusterInfo, container, subtreeRoot, windowInfo, multiplier);
}

void TextAutosizer::processCompositeCluster(Vector<TextAutosizingClusterInfo>& clusterInfos, const TextAutosizingWindowInfo& windowInfo)
{
    if (clusterInfos.isEmpty())
        return;

    float maxTextWidth = 0;
    for (size_t i = 0; i < clusterInfos.size(); ++i) {
        TextAutosizingClusterInfo& clusterInfo = clusterInfos[i];
        clusterInfo.blockContainingAllText = findDeepestBlockContainingAllText(clusterInfo.root);
        maxTextWidth = max<float>(maxTextWidth, clusterInfo.blockContainingAllText->contentLogicalWidth());
    }

    float multiplier = 1.0;
    if (compositeClusterShouldBeAutosized(clusterInfos, maxTextWidth))
        multiplier = clusterMultiplier(clusterInfos[0].root->style()->writingMode(), windowInfo, maxTextWidth);
    for (size_t i = 0; i < clusterInfos.size(); ++i) {
        ASSERT(clusterInfos[i].root->style()->writingMode() == clusterInfos[0].root->style()->writingMode());
        processClusterInternal(clusterInfos[i], clusterInfos[i].root, clusterInfos[i].root, windowInfo, multiplier);
    }
}

void TextAutosizer::processContainer(float multiplier, RenderBlock* container, TextAutosizingClusterInfo& clusterInfo, RenderObject* subtreeRoot, const TextAutosizingWindowInfo& windowInfo)
{
    ASSERT(isAutosizingContainer(container));

    float localMultiplier = containerShouldBeAutosized(container) ? multiplier: 1;

    RenderObject* descendant = nextInPreOrderSkippingDescendantsOfContainers(subtreeRoot, subtreeRoot);
    while (descendant) {
        if (descendant->isText()) {
            if (localMultiplier != 1 && descendant->style()->textAutosizingMultiplier() == 1) {
                setMultiplier(descendant, localMultiplier);
                setMultiplier(descendant->parent(), localMultiplier); // Parent does line spacing.
            }
            // FIXME: Increase list marker size proportionately.
        } else if (isAutosizingContainer(descendant)) {
            RenderBlock* descendantBlock = toRenderBlock(descendant);
            TextAutosizingClusterInfo descendantClusterInfo(descendantBlock);
            if (isWiderDescendant(descendantBlock, clusterInfo) || isIndependentDescendant(descendantBlock))
                processCluster(descendantClusterInfo, descendantBlock, descendantBlock, windowInfo);
            else if (isNarrowDescendant(descendantBlock, clusterInfo)) {
                // Narrow descendants are processed together later to be able to apply the same multiplier
                // to each of them if necessary.
                clusterInfo.narrowDescendants.append(descendantClusterInfo);
            } else
                processContainer(multiplier, descendantBlock, clusterInfo, descendantBlock, windowInfo);
        }
        descendant = nextInPreOrderSkippingDescendantsOfContainers(descendant, subtreeRoot);
    }
}

void TextAutosizer::setMultiplier(RenderObject* renderer, float multiplier)
{
    RefPtr<RenderStyle> newStyle = RenderStyle::clone(renderer->style());
    newStyle->setTextAutosizingMultiplier(multiplier);
    renderer->setStyle(newStyle.release());
}

float TextAutosizer::computeAutosizedFontSize(float specifiedSize, float multiplier)
{
    // Somewhat arbitrary "pleasant" font size.
    const float pleasantSize = 16;

    // Multiply fonts that the page author has specified to be larger than
    // pleasantSize by less and less, until huge fonts are not increased at all.
    // For specifiedSize between 0 and pleasantSize we directly apply the
    // multiplier; hence for specifiedSize == pleasantSize, computedSize will be
    // multiplier * pleasantSize. For greater specifiedSizes we want to
    // gradually fade out the multiplier, so for every 1px increase in
    // specifiedSize beyond pleasantSize we will only increase computedSize
    // by gradientAfterPleasantSize px until we meet the
    // computedSize = specifiedSize line, after which we stay on that line (so
    // then every 1px increase in specifiedSize increases computedSize by 1px).
    const float gradientAfterPleasantSize = 0.5;

    float computedSize;
    if (specifiedSize <= pleasantSize)
        computedSize = multiplier * specifiedSize;
    else {
        computedSize = multiplier * pleasantSize + gradientAfterPleasantSize * (specifiedSize - pleasantSize);
        if (computedSize < specifiedSize)
            computedSize = specifiedSize;
    }
    return computedSize;
}

bool TextAutosizer::isAutosizingContainer(const RenderObject* renderer)
{
    // "Autosizing containers" are the smallest unit for which we can
    // enable/disable Text Autosizing.
    // - Must not be inline, as different multipliers on one line looks terrible.
    //   Exceptions are inline-block and alike elements (inline-table, -webkit-inline-*),
    //   as they often contain entire multi-line columns of text.
    // - Must not be list items, as items in the same list should look consistent (*).
    // - Must not be normal list items, as items in the same list should look
    //   consistent, unless they are floating or position:absolute/fixed.
    if (!renderer->isRenderBlock() || (renderer->isInline() && !renderer->style()->isDisplayReplacedType()))
        return false;
    if (renderer->isListItem())
        return renderer->isFloating() || renderer->isOutOfFlowPositioned();
    // Avoid creating containers for text within text controls, buttons, or <select> buttons.
    Node* parentNode = renderer->parent() ? renderer->parent()->generatingNode() : 0;
    if (parentNode && parentNode->isElementNode() && formInputTags().contains(toElement(parentNode)->tagQName()))
        return false;

    return true;
}

bool TextAutosizer::isNarrowDescendant(const RenderBlock* renderer, TextAutosizingClusterInfo& parentClusterInfo)
{
    ASSERT(isAutosizingContainer(renderer));

    // Autosizing containers that are significantly narrower than the |blockContainingAllText| of
    // their enclosing cluster may be acting as separate columns, hence must be autosized
    // separately. For example the 2nd div in:
    // <body>
    //     <div style="float: right; width: 50%"></div>
    //     <div style="width: 50%"></div>
    // <body>
    // is the left column, and should be autosized differently from the body.
    // If however the container is only narrower by 150px or less, it's considered part of
    // the enclosing cluster. This 150px limit is adjusted whenever a descendant container is
    // less than 50px narrower than the current limit.
    const float differenceFromMaxWidthDifference = 50;
    float contentWidth = renderer->contentLogicalWidth();
    float clusterTextWidth = parentClusterInfo.blockContainingAllText->contentLogicalWidth();
    float widthDifference = clusterTextWidth - contentWidth;

    if (widthDifference - parentClusterInfo.maxAllowedDifferenceFromTextWidth > differenceFromMaxWidthDifference)
        return true;

    parentClusterInfo.maxAllowedDifferenceFromTextWidth = std::max(widthDifference, parentClusterInfo.maxAllowedDifferenceFromTextWidth);
    return false;
}

bool TextAutosizer::isWiderDescendant(const RenderBlock* renderer, const TextAutosizingClusterInfo& parentClusterInfo)
{
    ASSERT(isAutosizingContainer(renderer));

    // Autosizing containers that are wider than the |blockContainingAllText| of their enclosing
    // cluster are treated the same way as autosizing clusters to be autosized separately.
    float contentWidth = renderer->contentLogicalWidth();
    float clusterTextWidth = parentClusterInfo.blockContainingAllText->contentLogicalWidth();
    return contentWidth > clusterTextWidth;
}

bool TextAutosizer::isIndependentDescendant(const RenderBlock* renderer)
{
    ASSERT(isAutosizingContainer(renderer));

    // "Autosizing clusters" are special autosizing containers within which we
    // want to enforce a uniform text size multiplier, in the hopes of making
    // the major sections of the page look internally consistent.
    // All their descendants (including other autosizing containers) must share
    // the same multiplier, except for subtrees which are themselves clusters,
    // and some of their descendant containers might not be autosized at all
    // (for example if their height is constrained).
    // Additionally, clusterShouldBeAutosized requires each cluster to contain a
    // minimum amount of text, without which it won't be autosized.
    //
    // Clusters are chosen using very similar criteria to CSS flow roots, aka
    // block formatting contexts (http://w3.org/TR/css3-box/#flow-root), since
    // flow roots correspond to box containers that behave somewhat
    // independently from their parent (for example they don't overlap floats).
    // The definition of a flow root also conveniently includes most of the
    // ways that a box and its children can have significantly different width
    // from the box's parent (we want to avoid having significantly different
    // width blocks within a cluster, since the narrower blocks would end up
    // larger than would otherwise be necessary).
    return renderer->isRenderView()
        || renderer->isFloating()
        || renderer->isOutOfFlowPositioned()
        || renderer->isTableCell()
        || renderer->isTableCaption()
        || renderer->isFlexibleBoxIncludingDeprecated()
        || renderer->hasColumns()
        || renderer->containingBlock()->isHorizontalWritingMode() != renderer->isHorizontalWritingMode()
        || renderer->style()->isDisplayReplacedType();
    // FIXME: Tables need special handling to multiply all their columns by
    // the same amount even if they're different widths; so do hasColumns()
    // containers, and probably flexboxes...
}

bool TextAutosizer::isAutosizingCluster(const RenderBlock* renderer, TextAutosizingClusterInfo& parentClusterInfo)
{
    ASSERT(isAutosizingContainer(renderer));

    return isNarrowDescendant(renderer, parentClusterInfo)
        || isWiderDescendant(renderer, parentClusterInfo)
        || isIndependentDescendant(renderer);
}

bool TextAutosizer::containerShouldBeAutosized(const RenderBlock* container)
{
    if (containerContainsOneOfTags(container, formInputTags()))
        return false;

    if (containerIsRowOfLinks(container))
        return false;

    // Don't autosize block-level text that can't wrap (as it's likely to
    // expand sideways and break the page's layout).
    if (!container->style()->autoWrap())
        return false;

    return !contentHeightIsConstrained(container);
}

bool TextAutosizer::containerContainsOneOfTags(const RenderBlock* container, const Vector<QualifiedName>& tags)
{
    const RenderObject* renderer = container;
    while (renderer) {
        const Node* rendererNode = renderer->node();
        if (rendererNode && rendererNode->isElementNode()) {
            if (tags.contains(toElement(rendererNode)->tagQName()))
                return true;
        }
        renderer = nextInPreOrderSkippingDescendantsOfContainers(renderer, container);
    }

    return false;
}

bool TextAutosizer::containerIsRowOfLinks(const RenderObject* container)
{
    // A "row of links" is a container for which holds:
    //  1. it should not contain non-link text elements longer than 3 characters
    //  2. it should contain min. 3 inline links and all links should
    //     have the same specified font size
    //  3. it should not contain <br> elements
    //  4. it should contain only inline elements unless they are containers,
    //     children of link elements or children of sub-containers.
    int linkCount = 0;
    RenderObject* renderer = container->nextInPreOrder(container);
    float matchingFontSize = -1;

    while (renderer) {
        if (!isAutosizingContainer(renderer)) {
            if (renderer->isText() && toRenderText(renderer)->text()->stripWhiteSpace()->length() > 3)
                return false;
            if (!renderer->isInline())
                return false;
            if (renderer->isBR())
                return false;
        }
        if (renderer->style()->isLink()) {
            if (matchingFontSize < 0)
                matchingFontSize = renderer->style()->specifiedFontSize();
            else {
                if (matchingFontSize != renderer->style()->specifiedFontSize())
                    return false;
            }

            linkCount++;
            // Skip traversing descendants of the link.
            renderer = renderer->nextInPreOrderAfterChildren(container);
        } else
            renderer = nextInPreOrderSkippingDescendantsOfContainers(renderer, container);
    }

    return (linkCount >= 3);
}

bool TextAutosizer::contentHeightIsConstrained(const RenderBlock* container)
{
    // FIXME: Propagate constrainedness down the tree, to avoid inefficiently walking back up from each box.
    // FIXME: This code needs to take into account vertical writing modes.
    // FIXME: Consider additional heuristics, such as ignoring fixed heights if the content is already overflowing before autosizing kicks in.
    for (; container; container = container->containingBlock()) {
        RenderStyle* style = container->style();
        if (style->overflowY() >= OSCROLL)
            return false;
        if (style->height().isSpecified() || style->maxHeight().isSpecified()) {
            // Some sites (e.g. wikipedia) set their html and/or body elements to height:100%,
            // without intending to constrain the height of the content within them.
            return !container->isRoot() && !container->isBody();
        }
        if (container->isFloatingOrOutOfFlowPositioned())
            return false;
    }
    return false;
}

bool TextAutosizer::clusterShouldBeAutosized(TextAutosizingClusterInfo& clusterInfo, float blockWidth)
{
    Vector<TextAutosizingClusterInfo> clusterInfos(1, clusterInfo);
    return compositeClusterShouldBeAutosized(clusterInfos, blockWidth);
}

bool TextAutosizer::compositeClusterShouldBeAutosized(Vector<TextAutosizingClusterInfo>& clusterInfos, float blockWidth)
{
    // Don't autosize clusters that contain less than 4 lines of text (in
    // practice less lines are required, since measureDescendantTextWidth
    // assumes that characters are 1em wide, but most characters are narrower
    // than that, so we're overestimating their contribution to the linecount).
    //
    // This is to reduce the likelihood of autosizing things like headers and
    // footers, which can be quite visually distracting. The rationale is that
    // if a cluster contains very few lines of text then it's ok to have to zoom
    // in and pan from side to side to read each line, since if there are very
    // few lines of text you'll only need to pan across once or twice.
    float totalTextWidth = 0;
    const float minLinesOfText = 4;
    float minTextWidth = blockWidth * minLinesOfText;
    for (size_t i = 0; i < clusterInfos.size(); ++i) {
        measureDescendantTextWidth(clusterInfos[i].blockContainingAllText, clusterInfos[i], minTextWidth, totalTextWidth);
        if (totalTextWidth >= minTextWidth)
            return true;
    }
    return false;
}

void TextAutosizer::measureDescendantTextWidth(const RenderBlock* container, TextAutosizingClusterInfo& clusterInfo, float minTextWidth, float& textWidth)
{
    bool skipLocalText = !containerShouldBeAutosized(container);

    RenderObject* descendant = nextInPreOrderSkippingDescendantsOfContainers(container, container);
    while (descendant) {
        if (!skipLocalText && descendant->isText()) {
            textWidth += toRenderText(descendant)->renderedTextLength() * descendant->style()->specifiedFontSize();
        } else if (isAutosizingContainer(descendant)) {
            RenderBlock* descendantBlock = toRenderBlock(descendant);
            if (!isAutosizingCluster(descendantBlock, clusterInfo))
                measureDescendantTextWidth(descendantBlock, clusterInfo, minTextWidth, textWidth);
        }
        if (textWidth >= minTextWidth)
            return;
        descendant = nextInPreOrderSkippingDescendantsOfContainers(descendant, container);
    }
}

RenderObject* TextAutosizer::nextInPreOrderSkippingDescendantsOfContainers(const RenderObject* current, const RenderObject* stayWithin)
{
    if (current == stayWithin || !isAutosizingContainer(current))
        return current->nextInPreOrder(stayWithin);
    return current->nextInPreOrderAfterChildren(stayWithin);
}

const RenderBlock* TextAutosizer::findDeepestBlockContainingAllText(const RenderBlock* cluster)
{
    size_t firstDepth = 0;
    const RenderObject* firstTextLeaf = findFirstTextLeafNotInCluster(cluster, firstDepth, FirstToLast);
    if (!firstTextLeaf)
        return cluster;

    size_t lastDepth = 0;
    const RenderObject* lastTextLeaf = findFirstTextLeafNotInCluster(cluster, lastDepth, LastToFirst);
    ASSERT(lastTextLeaf);

    // Equalize the depths if necessary. Only one of the while loops below will get executed.
    const RenderObject* firstNode = firstTextLeaf;
    const RenderObject* lastNode = lastTextLeaf;
    while (firstDepth > lastDepth) {
        firstNode = firstNode->parent();
        --firstDepth;
    }
    while (lastDepth > firstDepth) {
        lastNode = lastNode->parent();
        --lastDepth;
    }

    // Go up from both nodes until the parent is the same. Both pointers will point to the LCA then.
    while (firstNode != lastNode) {
        firstNode = firstNode->parent();
        lastNode = lastNode->parent();
    }

    if (firstNode->isRenderBlock())
        return toRenderBlock(firstNode);

    // containingBlock() should never leave the cluster, since it only skips ancestors when finding the
    // container of position:absolute/fixed blocks, and those cannot exist between a cluster and its text
    // nodes lowest common ancestor as isAutosizingCluster would have made them into their own independent
    // cluster.
    RenderBlock* containingBlock = firstNode->containingBlock();
    ASSERT(containingBlock->isDescendantOf(cluster));

    return containingBlock;
}

const RenderObject* TextAutosizer::findFirstTextLeafNotInCluster(const RenderObject* parent, size_t& depth, TraversalDirection direction)
{
    if (parent->isEmpty())
        return parent->isText() ? parent : 0;

    ++depth;
    const RenderObject* child = (direction == FirstToLast) ? parent->firstChild() : parent->lastChild();
    while (child) {
        if (!isAutosizingContainer(child) || !isIndependentDescendant(toRenderBlock(child))) {
            const RenderObject* leaf = findFirstTextLeafNotInCluster(child, depth, direction);
            if (leaf)
                return leaf;
        }
        child = (direction == FirstToLast) ? child->nextSibling() : child->previousSibling();
    }
    --depth;

    return 0;
}

namespace {

// Compares the width of the specified cluster's roots in descending order.
bool clusterWiderThanComparisonFn(const TextAutosizingClusterInfo& first, const TextAutosizingClusterInfo& second)
{
    return first.root->contentLogicalWidth() > second.root->contentLogicalWidth();
}

} // namespace

void TextAutosizer::getNarrowDescendantsGroupedByWidth(const TextAutosizingClusterInfo& parentClusterInfo, Vector<Vector<TextAutosizingClusterInfo> >& groups)
{
    ASSERT(parentClusterInfo.blockContainingAllText);
    ASSERT(groups.isEmpty());

    Vector<TextAutosizingClusterInfo> clusterInfos(parentClusterInfo.narrowDescendants);
    if (clusterInfos.isEmpty())
        return;

    std::sort(clusterInfos.begin(), clusterInfos.end(), &clusterWiderThanComparisonFn);
    groups.grow(1);

    // If the width difference between two consecutive elements of |clusterInfos| is greater than
    // this empirically determined value, the next element should start a new group.
    const float maxWidthDifferenceWithinGroup = 100;
    for (size_t i = 0; i < clusterInfos.size(); ++i) {
        groups.last().append(clusterInfos[i]);

        if (i + 1 < clusterInfos.size()) {
            float currentWidth = clusterInfos[i].root->contentLogicalWidth();
            float nextWidth = clusterInfos[i + 1].root->contentLogicalWidth();
            if (currentWidth - nextWidth > maxWidthDifferenceWithinGroup)
                groups.grow(groups.size() + 1);
        }
    }
}

} // namespace WebCore

#endif // ENABLE(TEXT_AUTOSIZING)
