/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Size of border around nodes.
// We could support arbitrary borders using getComputedStyle(), but I am
// skeptical the extra complexity (and performance hit) is worth it.
var kBorderWidth = 1;

// Padding around contents.
// TODO: do this with a nested div to allow it to be CSS-styleable.
var kPadding = 4;

var focused = null;

// Callback for embedding page to update after a focus.
function handleFocus(tree) {}

function focus(tree) {
  focused = tree;

  // Hide all visible siblings of all our ancestors by lowering them.
  var level = 0;
  var root = tree;
  while (root.parent) {
    root = root.parent;
    level += 1;
    for (var i = 0, sibling; sibling = root.children[i]; ++i) {
      if (sibling.dom)
        sibling.dom.style.zIndex = 0;
    }
  }
  var width = root.dom.offsetWidth;
  var height = root.dom.offsetHeight;
  // Unhide (raise) and maximize us and our ancestors.
  for (var t = tree; t.parent; t = t.parent) {
    // Shift off by border so we don't get nested borders.
    // TODO: actually make nested borders work (need to adjust width/height).
    position(t.dom, -kBorderWidth, -kBorderWidth, width, height);
    t.dom.style.zIndex = 1;
  }
  // And layout into the topmost box.
  layout(tree, level, width, height);
  handleFocus(tree);
}

function makeDom(tree, level) {
  var dom = document.createElement('div');
  dom.style.zIndex = 1;
  dom.className = 'webtreemap-node webtreemap-level' + Math.min(level, 4);

  dom.onmousedown = function(e) {
    if (e.button == 0) {
      if (focused && tree == focused && focused.parent) {
        focus(focused.parent);
      } else {
        focus(tree);
      }
    }
    e.stopPropagation();
    return true;
  };

  var caption = document.createElement('div');
  caption.className = 'webtreemap-caption';
  caption.innerHTML = tree.name;
  dom.appendChild(caption);

  tree.dom = dom;
  return dom;
}

function position(dom, x, y, width, height) {
  // CSS width/height does not include border.
  width -= kBorderWidth*2;
  height -= kBorderWidth*2;

  dom.style.left   = x + 'px';
  dom.style.top    = y + 'px';
  dom.style.width  = Math.max(width, 0) + 'px';
  dom.style.height = Math.max(height, 0) + 'px';
}

// Given a list of rectangles |nodes|, the 1-d space available
// |space|, and a starting rectangle index |start|, compute an span of
// rectangles that optimizes a pleasant aspect ratio.
//
// Returns [end, sum], where end is one past the last rectangle and sum is the
// 2-d sum of the rectangles' areas.
function selectSpan(nodes, space, start) {
  // Add rectangle one by one, stopping when aspect ratios begin to go
  // bad.  Result is [start,end) covering the best run for this span.
  // http://scholar.google.com/scholar?cluster=5972512107845615474
  var node = nodes[start];
  var rmin = node.data['$area'];  // Smallest seen child so far.
  var rmax = rmin;                // Largest child.
  var rsum = 0;                   // Sum of children in this span.
  var last_score = 0;             // Best score yet found.
  for (var end = start; node = nodes[end]; ++end) {
    var size = node.data['$area'];
    if (size < rmin)
      rmin = size;
    if (size > rmax)
      rmax = size;
    rsum += size;

    // This formula is from the paper, but you can easily prove to
    // yourself it's taking the larger of the x/y aspect ratio or the
    // y/x aspect ratio.  The additional magic fudge constant of 5
    // makes us prefer wider rectangles to taller ones.
    var score = Math.max(5*space*space*rmax / (rsum*rsum),
                         1*rsum*rsum / (space*space*rmin));
    if (last_score && score > last_score) {
      rsum -= size;  // Undo size addition from just above.
      break;
    }
    last_score = score;
  }
  return [end, rsum];
}

function layout(tree, level, width, height) {
  if (!('children' in tree))
    return;

  var total = tree.data['$area'];

  // XXX why do I need an extra -1/-2 here for width/height to look right?
  var x1 = 0, y1 = 0, x2 = width - 1, y2 = height - 2;
  x1 += kPadding; y1 += kPadding;
  x2 -= kPadding; y2 -= kPadding;
  y1 += 14;  // XXX get first child height for caption spacing

  var pixels_to_units = Math.sqrt(total / ((x2 - x1) * (y2 - y1)));

  for (var start = 0, child; child = tree.children[start]; ++start) {
    if (x2 - x1 < 60 || y2 - y1 < 40) {
      if (child.dom) {
        child.dom.style.zIndex = 0;
        position(child.dom, -2, -2, 0, 0);
      }
      continue;
    }

    // In theory we can dynamically decide whether to split in x or y based
    // on aspect ratio.  In practice, changing split direction with this
    // layout doesn't look very good.
    //   var ysplit = (y2 - y1) > (x2 - x1);
    var ysplit = true;

    var space;  // Space available along layout axis.
    if (ysplit)
      space = (y2 - y1) * pixels_to_units;
    else
      space = (x2 - x1) * pixels_to_units;

    var span = selectSpan(tree.children, space, start);
    var end = span[0], rsum = span[1];

    // Now that we've selected a span, lay out rectangles [start,end) in our
    // available space.
    var x = x1, y = y1;
    for (var i = start; i < end; ++i) {
      child = tree.children[i];
      if (!child.dom) {
        child.parent = tree;
        child.dom = makeDom(child, level + 1);
        tree.dom.appendChild(child.dom);
      } else {
        child.dom.style.zIndex = 1;
      }
      var size = child.data['$area'];
      var frac = size / rsum;
      if (ysplit) {
        width = rsum / space;
        height = size / width;
      } else {
        height = rsum / space;
        width = size / height;
      }
      width /= pixels_to_units;
      height /= pixels_to_units;
      width = Math.round(width);
      height = Math.round(height);
      position(child.dom, x, y, width, height);
      if ('children' in child) {
        layout(child, level + 1, width, height);
      }
      if (ysplit)
        y += height;
      else
        x += width;
    }

    // Shrink our available space based on the amount we used.
    if (ysplit)
      x1 += Math.round((rsum / space) / pixels_to_units);
    else
      y1 += Math.round((rsum / space) / pixels_to_units);

    // end points one past where we ended, which is where we want to
    // begin the next iteration, but subtract one to balance the ++ in
    // the loop.
    start = end - 1;
  }
}

function appendTreemap(dom, data) {
  var style = getComputedStyle(dom, null);
  var width = parseInt(style.width);
  var height = parseInt(style.height);
  if (!data.dom)
    makeDom(data, 0);
  dom.appendChild(data.dom);
  position(data.dom, 0, 0, width, height);
  layout(data, 0, width, height);
}