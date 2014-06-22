/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

// Bump this version when making changes that affect the storage format.
const _imageStorageFormatVersion = 1;

try {
    var _generatedImageCacheDatabase = openDatabase("com.apple.WebInspector", 1, "Web Inspector Storage Database", 5 * 1024 * 1024);
} catch (e) {
    // If we can't open the database it isn't the end of the world, we just will always generate
    // the images and not cache them for better load times.
    console.warn("Can't open database due to: " + e + ". Images will be generated instead of loaded from cache.");
}

var _initialPrefetchComplete = false;
var _fetchedCachedImages = {};

var _generatedImageUpdateFunctions = [];

_prefetchCachedImagesAndUpdate();

// Updates each image when the device pixel ratio changes to redraw at the new resolution.
window.matchMedia("(-webkit-device-pixel-ratio: 1)").addListener(_devicePixelRatioChanged);

// Delete old cached images from localStorage to free up space.
// FIXME: Remove this once it has been in the builds for a while.
try {
    const processedFlagKey = "com.apple.WebInspector.deleted-generated-images";

    if (!window.localStorage[processedFlagKey]) {
        for (var key in window.localStorage) {
            if (/^com\.apple\.WebInspector\.generated-(?:colored|embossed)-image-/.test(key))
                delete window.localStorage[key];
        }

        window.localStorage[processedFlagKey] = true;
    }
} catch (e) {
    // Ignore.
}

function _devicePixelRatioChanged()
{
    _prefetchCachedImagesAndUpdate();
}

function _registerGeneratedImageUpdateFunction(update)
{
    console.assert(typeof update === "function");

    _generatedImageUpdateFunctions.push(update);

    if (_initialPrefetchComplete)
        update();
}

function _logSQLError(tx, error)
{
    console.error(error.code, error.message);
}

function _logSQLTransactionError(error)
{
    console.error(error.code, error.message);
}

function _prefetchCachedImagesAndUpdate()
{
    _fetchedCachedImages = {};

    function complete()
    {
        _initialPrefetchComplete = true;

        for (var i = 0; i < _generatedImageUpdateFunctions.length; ++i)
            _generatedImageUpdateFunctions[i]();
    }

    if (!_generatedImageCacheDatabase) {
        complete();
        return;
    }

    _generatedImageCacheDatabase.transaction(function(tx) {
        tx.executeSql("SELECT key, imageVersion, data FROM CachedImages WHERE pixelRatio = ? AND formatVersion = ?", [window.devicePixelRatio, _imageStorageFormatVersion], function(tx, result) {
            for (var i = 0; i < result.rows.length; ++i) {
                var row = result.rows.item(i);
                _fetchedCachedImages[row.key] = {data: row.data, imageVersion: row.imageVersion};
            }

            complete();
        }, function(tx, error) {
            // The select failed. That could be because the schema changed or this is the first time.
            // Drop the table and recreate it fresh.

            tx.executeSql("DROP TABLE IF EXISTS CachedImages");
            tx.executeSql("CREATE TABLE CachedImages (key TEXT, pixelRatio INTEGER, formatVersion NUMERIC, imageVersion NUMERIC, data BLOB, UNIQUE(key, pixelRatio))", [], null, _logSQLError);

            complete();
        });
    }, _logSQLTransactionError);
}

function saveImageToStorage(storageKey, context, width, height, imageVersion)
{
    console.assert(storageKey);
    console.assert(context);
    console.assert(typeof width === "number");
    console.assert(typeof height === "number");
    console.assert(typeof imageVersion === "number");

    if (!_generatedImageCacheDatabase)
        return;

    var imageData = context.getImageData(0, 0, width, height);
    var imageDataPixels = new Uint32Array(imageData.data.buffer);

    var imageDataString = "";
    for (var i = 0; i < imageDataPixels.length; ++i)
        imageDataString += (i ? ":" : "") + (imageDataPixels[i] ? imageDataPixels[i].toString(36) : "");

    _generatedImageCacheDatabase.transaction(function(tx) {
        tx.executeSql("INSERT OR REPLACE INTO CachedImages (key, pixelRatio, imageVersion, formatVersion, data) VALUES (?, ?, ?, ?, ?)", [storageKey, window.devicePixelRatio, imageVersion, _imageStorageFormatVersion, imageDataString], null, _logSQLError);
    }, _logSQLTransactionError);
}

function restoreImageFromStorage(storageKey, context, width, height, imageVersion, generateCallback)
{
    console.assert(storageKey);
    console.assert(context);
    console.assert(typeof width === "number");
    console.assert(typeof height === "number");
    console.assert(typeof imageVersion === "number");
    console.assert(typeof generateCallback === "function");

    if (!_generatedImageCacheDatabase) {
        generateCallback();
        return;
    }

    var imageInfo = _fetchedCachedImages[storageKey];

    if (imageInfo) {
        // We only want to keep the data around for the first use. These images
        // are typically only used in one place. This keeps performance good
        // during page load and frees memory that typically won't be reused.
        delete _fetchedCachedImages[storageKey];
    }

    if (imageInfo && (!imageInfo.data || imageInfo.imageVersion !== imageVersion)) {
        generateCallback();
        return;
    }

    if (imageInfo) {
        // Restore the image from the memory cache.
        restoreImageData(imageInfo.data);
    } else {
        // Try fetching the image data from the database.
        _generatedImageCacheDatabase.readTransaction(function(tx) {
            tx.executeSql("SELECT data FROM CachedImages WHERE key = ? AND pixelRatio = ? AND imageVersion = ? AND formatVersion = ?", [storageKey, window.devicePixelRatio, imageVersion, _imageStorageFormatVersion], function(tx, result) {
                if (!result.rows.length) {
                    generateCallback();
                    return;
                }

                console.assert(result.rows.length === 1);

                restoreImageData(result.rows.item(0).data);
            }, function(tx, error) {
                _logSQLError(tx, error);

                generateCallback();
            });
        }, _logSQLTransactionError);
    }

    function restoreImageData(imageDataString)
    {
        var imageData = context.createImageData(width, height);
        var imageDataPixels = new Uint32Array(imageData.data.buffer);

        var imageDataArray = imageDataString.split(":");
        if (imageDataArray.length !== imageDataPixels.length) {
            generateCallback();
            return;
        }

        for (var i = 0; i < imageDataArray.length; ++i) {
            var pixelString = imageDataArray[i];
            imageDataPixels[i] = pixelString ? parseInt(pixelString, 36) : 0;
        }

        context.putImageData(imageData, 0, 0);
    }
}

function generateColoredImage(inputImage, red, green, blue, alpha, width, height)
{
    console.assert(inputImage);

    if (alpha === undefined)
        alpha = 1;

    if (width === undefined)
        width = inputImage.width;

    if (height === undefined)
        height = inputImage.height;

    if (inputImage instanceof HTMLCanvasElement) {
        // The input is already a canvas, so we can use its context directly.
        var inputContext = inputImage.getContext("2d");
    } else {
        console.assert(inputImage instanceof HTMLImageElement || inputImage instanceof HTMLVideoElement);

        // The input is an image/video element, so we need to draw it into
        // a canvas to get the pixel data.
        var inputCanvas = document.createElement("canvas");
        inputCanvas.width = width;
        inputCanvas.height = height;

        var inputContext = inputCanvas.getContext("2d");
        inputContext.drawImage(inputImage, 0, 0, width, height);
    }

    var imageData = inputContext.getImageData(0, 0, width, height);
    var imageDataPixels = new Uint32Array(imageData.data.buffer);

    var isLittleEndian = Uint32Array.isLittleEndian();

    // Loop over the image data and set the color channels while preserving the alpha.
    for (var i = 0; i < imageDataPixels.length; ++i) {
        if (isLittleEndian) {
            var existingAlpha = 0xff & (imageDataPixels[i] >> 24);
            imageDataPixels[i] = red | green << 8 | blue << 16 | (existingAlpha * alpha) << 24;
        } else {
            var existingAlpha = 0xff & imageDataPixels[i];
            imageDataPixels[i] = red << 24 | green << 16 | blue << 8 | existingAlpha * alpha;
        }
    }

    // Make a canvas that will be returned as the result.
    var resultCanvas = document.createElement("canvas");
    resultCanvas.width = width;
    resultCanvas.height = height;

    var resultContext = resultCanvas.getContext("2d");

    resultContext.putImageData(imageData, 0, 0);

    return resultCanvas;
}

function generateColoredImagesForCSS(imagePath, specifications, width, height, canvasIdentifierPrefix)
{
    console.assert(imagePath);
    console.assert(specifications);
    console.assert(typeof width === "number");
    console.assert(typeof height === "number");

    var scaleFactor = window.devicePixelRatio;
    var scaledWidth = width * scaleFactor;
    var scaledHeight = height * scaleFactor;

    canvasIdentifierPrefix = canvasIdentifierPrefix || "";

    const storageKeyPrefix = "generated-colored-image-";

    var imageElement = null;
    var pendingImageLoadCallbacks = [];

    _registerGeneratedImageUpdateFunction(update);

    function imageLoaded()
    {
        console.assert(imageElement);
        console.assert(imageElement.complete);
        for (var i = 0; i < pendingImageLoadCallbacks.length; ++i)
            pendingImageLoadCallbacks[i]();
        pendingImageLoadCallbacks = null;
    }

    function ensureImageIsLoaded(callback)
    {
        if (imageElement && imageElement.complete) {
            callback();
            return;
        }

        console.assert(pendingImageLoadCallbacks);
        pendingImageLoadCallbacks.push(callback);

        if (imageElement)
            return;

        imageElement = document.createElement("img");
        imageElement.addEventListener("load", imageLoaded);
        imageElement.width = width;
        imageElement.height = height;
        imageElement.src = imagePath;
    }

    function restoreImages()
    {
        for (var canvasIdentifier in specifications) {
            // Don't restore active images yet.
            if (canvasIdentifier.indexOf("active") !== -1)
                continue;

            var specification = specifications[canvasIdentifier];
            restoreImage(canvasIdentifier, specification);
        }

        function restoreActiveImages()
        {
            for (var canvasIdentifier in specifications) {
                // Only restore active images here.
                if (canvasIdentifier.indexOf("active") === -1)
                    continue;

                var specification = specifications[canvasIdentifier];
                restoreImage(canvasIdentifier, specification);
            }
        }

        // Delay restoring the active states until later to improve the initial page load time.
        setTimeout(restoreActiveImages, 500);
    }

    function restoreImage(canvasIdentifier, specification)
    {
        const storageKey = storageKeyPrefix + canvasIdentifierPrefix + canvasIdentifier;
        const context = document.getCSSCanvasContext("2d", canvasIdentifierPrefix + canvasIdentifier, scaledWidth, scaledHeight);
        restoreImageFromStorage(storageKey, context, scaledWidth, scaledHeight, specification.imageVersion || 0, function() {
            ensureImageIsLoaded(generateImage.bind(null, canvasIdentifier, specification));
        });
    }

    function update()
    {
        restoreImages();
    }

    function generateImage(canvasIdentifier, specification)
    {
        console.assert(specification.fillColor instanceof Array);
        console.assert(specification.fillColor.length === 3 || specification.fillColor.length === 4);

        const context = document.getCSSCanvasContext("2d", canvasIdentifierPrefix + canvasIdentifier, scaledWidth, scaledHeight);
        context.save();
        context.scale(scaleFactor, scaleFactor);

        if (specification.shadowColor) {
            context.shadowOffsetX = specification.shadowOffsetX || 0;
            context.shadowOffsetY = specification.shadowOffsetY || 0;
            context.shadowBlur = specification.shadowBlur || 0;

            if (specification.shadowColor instanceof Array) {
                if (specification.shadowColor.length === 3)
                    context.shadowColor = "rgb(" + specification.shadowColor.join(", ") + ")";
                else if (specification.shadowColor.length === 4)
                    context.shadowColor = "rgba(" + specification.shadowColor.join(", ") + ")";
            } else
                context.shadowColor = specification.shadowColor;
        }

        var coloredImage = generateColoredImage(imageElement, specification.fillColor[0], specification.fillColor[1], specification.fillColor[2], specification.fillColor[3], scaledWidth, scaledHeight);
        context.drawImage(coloredImage, 0, 0, width, height);

        const storageKey = storageKeyPrefix + canvasIdentifierPrefix + canvasIdentifier;
        saveImageToStorage(storageKey, context, scaledWidth, scaledHeight, specification.imageVersion || 0);
        context.restore();
    }
}

function generateEmbossedImages(src, width, height, states, canvasIdentifierCallback, ignoreCache)
{
    console.assert(src);
    console.assert(typeof width === "number");
    console.assert(typeof height === "number");
    console.assert(states);
    console.assert(states.Normal);
    console.assert(states.Active);
    console.assert(typeof canvasIdentifierCallback === "function");

    var scaleFactor = window.devicePixelRatio;
    var scaledWidth = width * scaleFactor;
    var scaledHeight = height * scaleFactor;

    // Bump this version when making changes that affect the result image.
    const imageVersion = 2;

    const storageKeyPrefix = "generated-embossed-image-";

    var image = null;
    var pendingImageLoadCallbacks = [];

    _registerGeneratedImageUpdateFunction(update);

    function imageLoaded()
    {
        console.assert(image);
        console.assert(image.complete);
        for (var i = 0; i < pendingImageLoadCallbacks.length; ++i)
            pendingImageLoadCallbacks[i]();
        pendingImageLoadCallbacks = null;
    }

    function ensureImageIsLoaded(callback)
    {
        if (image && image.complete) {
            callback();
            return;
        }

        console.assert(pendingImageLoadCallbacks);
        pendingImageLoadCallbacks.push(callback);

        if (image)
            return;

        image = document.createElement("img");
        image.addEventListener("load", imageLoaded);
        image.width = width;
        image.height = height;
        image.src = src;
    }

    function restoreImages()
    {
        restoreImage(states.Normal);
        if (states.Focus)
            restoreImage(states.Focus);

        function restoreActiveImages()
        {
            restoreImage(states.Active);
            if (states.ActiveFocus)
                restoreImage(states.ActiveFocus);
        }

        // Delay restoring the active states until later to improve the initial page load time.
        setTimeout(restoreActiveImages, 500);
    }

    function restoreImage(state)
    {
        const storageKey = storageKeyPrefix + canvasIdentifierCallback(state);
        const context = document.getCSSCanvasContext("2d", canvasIdentifierCallback(state), scaledWidth, scaledHeight);
        restoreImageFromStorage(storageKey, context, scaledWidth, scaledHeight, imageVersion, function() {
            ensureImageIsLoaded(generateImage.bind(null, state));
        });
    }

    function update()
    {
        if (ignoreCache)
            generateImages();
        else
            restoreImages();
    }

    function generateImages()
    {
        ensureImageIsLoaded(generateImage.bind(null, states.Normal));

        if (states.Focus)
            ensureImageIsLoaded(generateImage.bind(null, states.Focus));

        function generateActiveImages()
        {
            ensureImageIsLoaded(generateImage.bind(null, states.Active));

            if (states.ActiveFocus)
                ensureImageIsLoaded(generateImage.bind(null, states.ActiveFocus));
        }

        // Delay generating the active states until later to improve the initial page load time.
        setTimeout(generateActiveImages, 500);
    }

    function generateImage(state)
    {
        const depth = 1 * scaleFactor;
        const shadowDepth = depth;
        const shadowBlur = depth - 1;
        const glowBlur = 2;

        const context = document.getCSSCanvasContext("2d", canvasIdentifierCallback(state), scaledWidth, scaledHeight);
        context.save();
        context.scale(scaleFactor, scaleFactor);

        context.clearRect(0, 0, width, height);

        if (depth > 0) {
            // Use scratch canvas so we can apply the draw the white drop shadow
            // to the whole glyph at the end.

            var scratchCanvas = document.createElement("canvas");
            scratchCanvas.width = scaledWidth;
            scratchCanvas.height = scaledHeight;

            var scratchContext = scratchCanvas.getContext("2d");
            scratchContext.scale(scaleFactor, scaleFactor);
        } else
            var scratchContext = context;

        var gradient = scratchContext.createLinearGradient(0, 0, 0, height);
        if (state === states.Active) {
            gradient.addColorStop(0, "rgb(60, 60, 60)");
            gradient.addColorStop(1, "rgb(100, 100, 100)");
        } else if (state === states.Focus) {
            gradient.addColorStop(0, "rgb(50, 135, 200)");
            gradient.addColorStop(1, "rgb(60, 155, 225)");
        } else if (state === states.ActiveFocus) {
            gradient.addColorStop(0, "rgb(30, 115, 185)");
            gradient.addColorStop(1, "rgb(40, 135, 200)");
        } else {
            gradient.addColorStop(0, "rgb(90, 90, 90)");
            gradient.addColorStop(1, "rgb(145, 145, 145)");
        }

        scratchContext.fillStyle = gradient;
        scratchContext.fillRect(0, 0, width, height);

        if (depth > 0) {
            // Invert the image to use as a reverse image mask for the inner shadows.
            // Pass in the color to use for the opaque areas to prevent "black halos"
            // later when applying the final image mask.

            if (state === states.Active)
                var invertedImage = _invertMaskImage(image, 60, 60, 60);
            else if (state === states.Focus)
                var invertedImage = _invertMaskImage(image, 45, 145, 210);
            else if (state === states.ActiveFocus)
                var invertedImage = _invertMaskImage(image, 35, 125, 195);
            else
                var invertedImage = _invertMaskImage(image, 90, 90, 90);

            if (state === states.Focus) {
                // Double draw the blurry inner shadow to get the right effect.
                _drawImageShadow(scratchContext, 0, 0, shadowDepth, "rgb(10, 95, 150)", invertedImage);
                _drawImageShadow(scratchContext, 0, 0, shadowDepth, "rgb(10, 95, 150)", invertedImage);

                // Draw the inner shadow.
                _drawImageShadow(scratchContext, 0, shadowDepth, shadowBlur, "rgb(0, 80, 170)", invertedImage);
            } else if (state === states.ActiveFocus) {
                // Double draw the blurry inner shadow to get the right effect.
                _drawImageShadow(scratchContext, 0, 0, shadowDepth, "rgb(0, 80, 100)", invertedImage);
                _drawImageShadow(scratchContext, 0, 0, shadowDepth, "rgb(0, 80, 100)", invertedImage);

                // Draw the inner shadow.
                _drawImageShadow(scratchContext, 0, shadowDepth, shadowBlur, "rgb(0, 65, 150)", invertedImage);
            } else {
                // Double draw the blurry inner shadow to get the right effect.
                _drawImageShadow(scratchContext, 0, 0, shadowDepth, "rgba(0, 0, 0, 1)", invertedImage);
                _drawImageShadow(scratchContext, 0, 0, shadowDepth, "rgba(0, 0, 0, 1)", invertedImage);

                // Draw the inner shadow.
                _drawImageShadow(scratchContext, 0, shadowDepth, shadowBlur, "rgba(0, 0, 0, 0.6)", invertedImage);
            }
        }

        // Apply the mask to keep just the inner shape of the glyph.
        _applyImageMask(scratchContext, image);

        // Draw the white drop shadow.
        if (depth > 0)
            _drawImageShadow(context, 0, shadowDepth, shadowBlur, "rgba(255, 255, 255, 0.6)", scratchCanvas);

        // Draw a subtle glow for the focus states.
        if (state === states.Focus || state === states.ActiveFocus)
            _drawImageShadow(context, 0, 0, glowBlur, "rgba(20, 100, 220, 0.4)", scratchCanvas);

        if (!ignoreCache) {
            const storageKey = storageKeyPrefix + canvasIdentifierCallback(state);
            saveImageToStorage(storageKey, context, scaledWidth, scaledHeight, imageVersion);
        }

        context.restore();
    }

    function _drawImageShadow(context, xOffset, yOffset, blur, color, image) {
        context.save();

        context.shadowOffsetX = xOffset || 0;
        context.shadowOffsetY = yOffset || 0;
        context.shadowBlur = blur || 0;
        context.shadowColor = color || "black";

        context.drawImage(image, 0, 0, width, height);

        context.restore();
    }

    function _invertMaskImage(image, red, green, blue) {
        var bufferCanvas = document.createElement("canvas");
        bufferCanvas.width = scaledWidth;
        bufferCanvas.height = scaledHeight;

        var buffer = bufferCanvas.getContext("2d");
        buffer.scale(scaleFactor, scaleFactor);
        buffer.drawImage(image, 0, 0, width, height);

        var imageData = buffer.getImageData(0, 0, scaledWidth, scaledHeight);
        var imageDataPixels = new Uint32Array(imageData.data.buffer);

        red = red || 0;
        green = green || 0;
        blue = blue || 0;

        var isLittleEndian = Uint32Array.isLittleEndian();

        for (var i = 0; i < imageDataPixels.length; ++i) {
            if (isLittleEndian) {
                var existingAlpha = 0xff & (imageDataPixels[i] >> 24);
                imageDataPixels[i] = red | green << 8 | blue << 16 | (255 - existingAlpha) << 24;
            } else {
                var existingAlpha = 0xff & imageDataPixels[i];
                imageDataPixels[i] = red << 24 | green << 16 | blue << 8 | 255 - existingAlpha;
            }
        }

        buffer.putImageData(imageData, 0, 0);

        return bufferCanvas;
    }

    function _applyImageMask(context, image) {
        var maskCanvas = document.createElement("canvas");
        maskCanvas.width = scaledWidth;
        maskCanvas.height = scaledHeight;

        var mask = maskCanvas.getContext("2d");
        mask.scale(scaleFactor, scaleFactor);
        mask.drawImage(image, 0, 0, width, height);

        var imageData = context.getImageData(0, 0, scaledWidth, scaledHeight);
        var imageDataPixels = imageData.data;

        var maskImageDataPixels = mask.getImageData(0, 0, scaledWidth, scaledHeight).data;

        for (var i = 3; i < imageDataPixels.length; i += 4)
            imageDataPixels[i] = maskImageDataPixels[i] * (imageDataPixels[i] / 255);

        context.putImageData(imageData, 0, 0);
    }
}
