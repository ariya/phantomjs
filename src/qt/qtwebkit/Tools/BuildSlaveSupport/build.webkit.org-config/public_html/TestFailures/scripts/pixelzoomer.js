/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

// pixelzoomer is shared with LayoutTests/fast/harness/results.html
// Unfortuantely, there's no good way to share code between these two uses.

var pixelzoomer = pixelzoomer || {};

(function() {

var kZoomFactor = 6;
var kDelayTimeoutMS = 400;

var kResultWidth = 800;
var kResultHeight = 600;

var kZoomedResultWidth = kResultWidth * kZoomFactor;
var kZoomedResultHeight = kResultHeight * kZoomFactor;

function matchesSelector(node, selector)
{
    if (node.webkitMatchesSelector)
        return node.webkitMatchesSelector(selector);

    if (node.mozMatchesSelector)
        return node.mozMatchesSelector(selector);
}

function parentOfType(node, selector)
{
    while (node = node.parentNode) {
        if (matchesSelector(node, selector))
            return node;
    }
    return null;
}

function zoomImageContainer(url)
{
    var container = document.createElement('div');
    container.className = 'zoom-image-container';

    var title = url.match(/\-([^\-]*)\.png/)[1];
    
    var label = document.createElement('div');
    label.className = 'label';
    label.appendChild(document.createTextNode(title));
    container.appendChild(label);

    var imageContainer = document.createElement('div');
    imageContainer.className = 'scaled-image-container';

    var image = new Image();
    image.src = url;
    image.style.width = kZoomedResultWidth + 'px';
    image.style.height = kZoomedResultHeight + 'px';
    image.style.border = '1px solid black';
    imageContainer.appendChild(image);
    container.appendChild(imageContainer);

    return container;
}

function createContainer(e)
{
    var tbody = parentOfType(e.target, 'tbody');
    var row = tbody.querySelector('tr');
    var images = row.querySelectorAll('img[src$=".png"]');

    var container = document.createElement('div');
    container.className = 'pixel-zoom-container';

    for (var i = 0; i < images.length; i++)
        container.appendChild(zoomImageContainer(images[i].src));

    document.body.appendChild(container);
    drawAll();
}

function draw(imageContainer)
{
    var image = imageContainer.querySelector('img');
    var containerBounds = imageContainer.getBoundingClientRect();
    image.style.left = (containerBounds.width / 2 - pixelzoomer._percentX * kZoomedResultWidth) + 'px';
    image.style.top = (containerBounds.height / 2 - pixelzoomer._percentY * kZoomedResultHeight) + 'px';
}

function drawAll()
{
    Array.prototype.forEach.call(document.querySelectorAll('.pixel-zoom-container .scaled-image-container'), draw);
}

function handleMouseOut(e)
{
    if (e.relatedTarget && e.relatedTarget.tagName != 'IFRAME')
        return;

    // If e.relatedTarget is null, we've moused out of the document.
    $('pixel-zoom-container').detach();
}

function handleMouseMove(e)
{
    if (pixelzoomer._mouseMoveTimeout)
        clearTimeout(pixelzoomer._mouseMoveTimeout);

    if (parentOfType(e.target, '.pixel-zoom-container'))
        return;

    var container = document.querySelector('.pixel-zoom-container');

    var resultContainer = (e.target.className == 'result-container') ?
        e.target : parentOfType(e.target, '.result-container');
    if (!resultContainer || !resultContainer.querySelector('img')) {
        $(container).detach();
        return;
    }

    var targetLocation = e.target.getBoundingClientRect();
    pixelzoomer._percentX = (e.clientX - targetLocation.left) / targetLocation.width;
    pixelzoomer._percentY = (e.clientY - targetLocation.top) / targetLocation.height;

    if (!container) {
        if (pixelzoomer.showOnDelay) {
            pixelzoomer._mouseMoveTimeout = setTimeout(function() {
                createContainer(e);
            }, kDelayTimeoutMS);
            return;
        }

        createContainer(e);
        return;
    }

    drawAll();
}

pixelzoomer.showOnDelay = true;

pixelzoomer.installEventListeners = function()
{
    document.addEventListener('mousemove', handleMouseMove, false);
    document.addEventListener('mouseout', handleMouseOut, false);
};

})();
