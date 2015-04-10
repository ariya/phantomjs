/*
 * Copyright (C) Research In Motion Limited, 2012. All rights reserved.
 */

(function (){
var _dragStartPoint,
    _dragDelta = 0,
    COLOR_WELL_WIDTH = (screen.width === 720 && screen.height === 720) ? 130 : 170, // keep in sync with colorControlBlackBerry.css
    MARGIN_OFFSET = 14;

var setColorWellColor = function (value) {
    document.getElementById('color-picker-current').style.backgroundColor = '#' + value;
};

var hexToR = function (h) {return parseInt(h.substring(0, 2), 16);};

var hexToG = function (h) {return parseInt(h.substring(2, 4), 16);};

var hexToB = function (h) {return parseInt(h.substring(4, 6), 16);};

var cutHex = function (h) {return (h.charAt(0) === '#') ? h.substring(1, 7) : h;};

var toHex = function (n) {
    n = parseInt(n, 10);
    if (isNaN(n)) {
        return '00';
    }
    n = Math.max(0, Math.min(n, 255));
    return '0123456789ABCDEF'.charAt((n - n % 16 ) / 16) + '0123456789ABCDEF'.charAt(n % 16);
};

var rgbToHex = function () {
    var color,
        red = document.getElementById('red-range'),
        green = document.getElementById('green-range'),
        blue = document.getElementById('blue-range');

    return color = toHex(red.value) + toHex(green.value) + toHex(blue.value);
};

var swatchUpdateColor = function (item, checkMark) {
    updateHex(item.mycolor);
    setColorWellColor(item.mycolor);
    item.appendChild(checkMark);
    var change = document.createEvent('HTMLEvents');
    change.initEvent('change', true, true);
    document.getElementById('color-picker-hex-input').dispatchEvent(change);
    ok();
};

var hexUpdateColor = function () {
    var colorString = cutHex(this.value);
    // Only valid 3 digits input and 6 digits input.
    if ((colorString.length === 3 || colorString.length === 6) && (/^([0-9A-F]{3,6}$)/i.test(colorString))) {
        // Auto fill 3 digits input. #ABC to #AABBCC
        if (colorString.length === 3) {
            colorString = colorString[0] + colorString[0] + colorString[1] + colorString[1] + colorString[2] + colorString[2];
        }
        setColorWellColor(colorString);
        document.getElementById('red-slider-output').value = document.getElementById('red-range').value = hexToR(colorString);
        document.getElementById('green-slider-output').value = document.getElementById('green-range').value = hexToG(colorString);
        document.getElementById('blue-slider-output').value = document.getElementById('blue-range').value = hexToB(colorString);
    }
    var color = rgbToHex();
    updateHex(color);
};

var updateHex = function (color) {
    document.getElementById('color-picker-hex-input').value = '#' + color;
};

var rangeUpdateColor = function (output, input) {
    if (input.value < 0) {
        input.value = 0;
    } else if (input.value > 255) {
        input.value = 255;
    }

    //update output
    output.value = input.value;
    //update hex colors
    var color = rgbToHex();
    updateHex(color);
    //update color well
    setColorWellColor(color);
};

var calculateSwatchBlockPadding = function () {
    var swatchBlock = document.getElementById('color-picker-swatch-block')
        padding = (swatchBlock.clientWidth - 4 * COLOR_WELL_WIDTH) / 2 + 'px';
    swatchBlock.style.paddingLeft = padding;
    swatchBlock.style.paddingRight = padding;
}

var openSwatches = function () {
    document.getElementById('swatches').classList.remove('color-picker-inactive-font');
    document.getElementById('sliders').classList.add('color-picker-inactive-font');
    document.getElementById('color-picker-swatch-block').classList.remove('color-picker-off');
    document.getElementById('color-picker-slider-block').classList.add('color-picker-off');
    var midpoint = document.getElementById('slide-button').offsetWidth;
    document.getElementById('slide-button').style['-webkit-transform'] = 'translate3d(' + midpoint + 'px, 0, 0)';
    document.getElementById('slide-button').translation = midpoint;
    calculateSwatchBlockPadding();
};

var openSliders = function () {
    document.getElementById('sliders').classList.remove('color-picker-inactive-font');
    document.getElementById('swatches').classList.add('color-picker-inactive-font');
    document.getElementById('color-picker-swatch-block').classList.add('color-picker-off');
    document.getElementById('color-picker-slider-block').classList.remove('color-picker-off');
    document.getElementById('slide-button').style.webkitTransform = 'translate3d(0, 0, 0)';
    document.getElementById('slide-button').translation = 0;
};

var rotationHandler = function () {
    if (document.getElementById('slide-button').translation) {
        var midpoint = document.getElementById('slide-button').offsetWidth;
        document.getElementById('slide-button').style['-webkit-transform'] = 'translate3d(' + midpoint + 'px, 0, 0)';
        document.getElementById('slide-button').translation = midpoint;
    }
    calculateSwatchBlockPadding();
};

var touchStartHandler = function (evt) {
    evt.stopPropagation();
    _dragDelta = 0;
    _dragStartPoint = evt.touches[0].pageX;
    document.getElementById('slide-highlight').classList.add('color-picker-highlight');
};

var touchMoveHandler = function (evt) {
    evt.stopPropagation();
    if (!_dragStartPoint) {
        return;
    }
    _dragDelta = evt.touches[0].pageX - _dragStartPoint;
    var button = document.getElementById('slide-button');
    if (button.translation + _dragDelta > 0 && button.translation + button.offsetWidth + _dragDelta < document.getElementById('switcher').offsetWidth - MARGIN_OFFSET) {
        button.style['-webkit-transform'] = 'translate3d(' + (button.translation + _dragDelta) + 'px, 0, 0)';
    }
}

var touchEndHandler = function (evt) {
    evt.preventDefault();
    evt.stopPropagation();
    var midpoint = document.getElementById('slide-button').offsetWidth;
    if (_dragDelta === 0) {
        if (_dragStartPoint > midpoint) {
            openSwatches();
        } else {
            openSliders();
        }
    } else if (Math.abs(_dragDelta) > 8) { // not a valid touch move if _dragDelta <= 8
        var button = document.getElementById('slide-button');
        if (button.translation + button.offsetWidth / 2 + _dragDelta > midpoint) {
            openSwatches();
        } else {
            openSliders();
        }
    }
    _dragStartPoint = undefined;
    _dragDelta = 0;
    document.getElementById('slide-highlight').classList.remove('color-picker-highlight');
}

var ok = function () {
    var result = document.getElementById('color-picker-hex-input').value;
    document.body.innerHTML = '';
    window.setValueAndClosePopup(result, window.popUp);
};

var cancel = function () {
    document.body.innerHTML = '';
    window.setValueAndClosePopup('-1', window.popUp);
};

var createRange = function (color) {
    var range = document.createElement('input');
    range.setAttribute('type', 'range');
    range.setAttribute('id', color + '-range');
    range.setAttribute('min', '0');
    range.setAttribute('max', '255');
    range.classList.add('color-picker-range');
    range.classList.add(color + '-ranger-bg');
    return range;
};

var createOutput = function (color) {
    var output = document.createElement('input');
    output.setAttribute('type', 'number');
    output.setAttribute('min', '0');
    output.setAttribute('max', '255');
    output.setAttribute('id', color + '-slider-output');
    output.classList.add('color-picker-slider-output');
    return output;
};

var layout = function (pickerParams) {
    var popup = document.createElement('div'),
        header = document.createElement('div'),
        body = document.createElement('div');
    popup.className = 'popup-area';
    popup.classList.add('color-picker-popup-area');
    popup.id = 'popup-area';
    popup.dir = pickerParams.direction;
    header.className = 'popup-header';
    header.innerText = pickerParams.uiText.title;
    popup.appendChild(header);
    body.className = 'popup-content';
    body.id = 'popup-content';
    body.dir = 'ltr'; // FIXME: RTL UI specifications not done yet
    popup.appendChild(body);

    var fragment = document.createDocumentFragment();

    var content = fragment.appendChild(document.createElement('div'));
    content.classList.add('color-picker-content');

    // Two buttons at top.
    var switcher = document.createElement('div');
    switcher.classList.add('color-picker-switcher');
    switcher.id = 'switcher';
    content.appendChild(switcher);

    var sliderButton = document.createElement('div');
    sliderButton.classList.add('color-picker-button');
    sliderButton.id = 'slide-button';
    var sliderHighlight = document.createElement('div');
    sliderHighlight.id = 'slide-highlight';
    sliderButton.appendChild(sliderHighlight);
    switcher.appendChild(sliderButton);

    switcher.addEventListener('touchstart', touchStartHandler);
    switcher.addEventListener('touchmove', touchMoveHandler);
    switcher.addEventListener('touchend', touchEndHandler);

    var sliders = document.createElement('div');
    sliders.classList.add('color-picker-option');
    sliders.id = 'sliders';
    switcher.appendChild(sliders);
    sliders.appendChild(document.createTextNode(pickerParams.uiText.slidersLabel));

    var swatches = document.createElement('div');
    swatches.classList.add('color-picker-option');
    swatches.id = 'swatches';
    switcher.appendChild(swatches);
    swatches.appendChild(document.createTextNode(pickerParams.uiText.swatchesLabel));

    // Swatches block
    var swatchBlock = document.createElement('div');
    swatchBlock.classList.add('color-picker-block');
    swatchBlock.id = 'color-picker-swatch-block';
    content.appendChild(swatchBlock);

    var SWATCH_COLORS = [['#ffffff', '#ffff00', '#00ff00', '#00ffff'], ['#ff00ff', '#0000ff', '#ff0000', '#000080'], ['#008080', '#008000', '#800080', '#800000'], ['#808000', '#808080', '#c0c0c0', '#000000']];

    var checkMarkDivTag = document.createElement('div');
    checkMarkDivTag.classList.add('color-picker-check-mark');

    for (var row = 1; row <= 4; row++) {
        var rowElt = document.createElement('div');
        rowElt.classList.add('color-picker-row');
        swatchBlock.appendChild(rowElt);
        for (var col = 1; col <= 4; col++) {
            var cell = document.createElement('div');
            cell.classList.add('color-picker-well');
            cell.id = row + '' + col;
            cell.style.backgroundColor = SWATCH_COLORS[row - 1][col - 1];
            cell.mycolor = cutHex(SWATCH_COLORS[row - 1][col - 1]);
            cell.addEventListener('click', function () { swatchUpdateColor(this, checkMarkDivTag); }, false);
            rowElt.appendChild(cell);
        }
    }

    // Sliders
    var sliderBlock = document.createElement('div');
    sliderBlock.classList.add('color-picker-block');
    sliderBlock.id = 'color-picker-slider-block';
    content.appendChild(sliderBlock);

    var hexInput = document.createElement('div');
    hexInput.classList.add('color-picker-input');
    sliderBlock.appendChild(hexInput);

    var currentColorWell = document.createElement('div');
    currentColorWell.classList.add('color-picker-well');
    currentColorWell.id = 'color-picker-current';
    hexInput.appendChild(currentColorWell);

    var userInput = document.createElement('input');
    userInput.setAttribute('type', 'text');
    userInput.setAttribute('id', 'color-picker-hex-input');
    userInput.setAttribute('maxlength', '7');
    userInput.classList.add('color-picker-hex-style');
    userInput.addEventListener('change', hexUpdateColor, false);
    hexInput.appendChild(userInput);

    var redSliderRow = document.createElement('div');
    redSliderRow.classList.add('color-picker-slider');
    sliderBlock.appendChild(redSliderRow);

    var redRange = createRange('red');
    redSliderRow.appendChild(redRange);

    var redOutput = createOutput('red');
    redSliderRow.appendChild(redOutput);

    var greenSliderRow = document.createElement('div');
    greenSliderRow.classList.add('color-picker-slider');
    sliderBlock.appendChild(greenSliderRow);

    var greenRange = createRange('green');
    greenSliderRow.appendChild(greenRange);

    var greenOutput = createOutput('green');
    greenSliderRow.appendChild(greenOutput);

    var blueSliderRow = document.createElement('div');
    blueSliderRow.classList.add('color-picker-slider');
    sliderBlock.appendChild(blueSliderRow);

    var blueRange = createRange('blue');
    blueSliderRow.appendChild(blueRange);

    var blueOutput = createOutput('blue');
    blueSliderRow.appendChild(blueOutput);

    redRange.addEventListener('change', function () { rangeUpdateColor(redOutput, redRange); }, false);
    greenRange.addEventListener('change', function () { rangeUpdateColor(greenOutput, greenRange); }, false);
    blueRange.addEventListener('change', function () { rangeUpdateColor(blueOutput, blueRange); }, false);
    redOutput.addEventListener('change', function () { rangeUpdateColor(redRange, redOutput); }, false);
    greenOutput.addEventListener('change', function () { rangeUpdateColor(greenRange, greenOutput); }, false);
    blueOutput.addEventListener('change', function () { rangeUpdateColor(blueRange, blueOutput); }, false);

    body.appendChild(fragment);

    var buttons = document.createElement('div'),
        cancelButton = document.createElement('button'),
        okButton = document.createElement('button');
    buttons.className = 'popup-buttons';
    cancelButton.className = 'popup-button';
    cancelButton.addEventListener('click', cancel);
    cancelButton.appendChild(document.createTextNode(pickerParams.uiText.cancelButtonLabel));
    buttons.appendChild(cancelButton);
    okButton.className = 'popup-button';
    okButton.addEventListener('click', ok);
    okButton.appendChild(document.createTextNode(pickerParams.uiText.doneButtonLabel));
    buttons.appendChild(okButton);
    popup.appendChild(buttons);

    document.body.appendChild(popup);
};

var show = function (pickerParams) {
    layout(pickerParams);
    openSliders();
    if (pickerParams.initialValue) {
        document.getElementById('color-picker-hex-input').value = pickerParams.initialValue;
    }

    var change = document.createEvent('HTMLEvents');
    change.initEvent('change', true, true);
    document.getElementById('color-picker-hex-input').dispatchEvent(change);
    window.addEventListener('orientationchange', rotationHandler, false);
};

window.popupcontrol = window.popupcontrol || {};
window.popupcontrol.show = show;
}());
