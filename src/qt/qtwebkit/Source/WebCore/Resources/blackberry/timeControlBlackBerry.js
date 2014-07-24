/*
 * Copyright (C) Research In Motion Limited, 2012. All rights reserved.
 */
(function (){
var _type,
    _currentDate,
    _tzHours,
    _tzMinutes,
    _tzOffset = 0,
    _min,
    _max,
    _step,
    _shortMonthLabels,
    _longMonthLabels,
    _daysOfWeekLabels,
    _amPmLabels,
    _baseOffset,
    _dragging = false,
    _firstY = 0,
    _scrollListener,
    _endScrollListener;

var ITEM_HEIGHT = (screen.width === 720 && screen.height === 720) ? 91 : 99; // in px, no border

var TIME = 'Time',
    DATE_TIME = 'DateTime',
    DATE_TIME_LOCAL = 'DateTimeLocal',
    DATE = 'Date',
    MONTH = 'Month';

var HOURS = 'Hours',
    MINUTES = 'Minutes',
    YEARMONTHDATE = 'YearMonthDate',
    YEAR = 'Year';

var COLUMNS = {};
COLUMNS[TIME] = [HOURS, MINUTES];
COLUMNS[DATE_TIME] = [YEARMONTHDATE, HOURS, MINUTES];
COLUMNS[DATE_TIME_LOCAL] = [YEARMONTHDATE, HOURS, MINUTES];
COLUMNS[DATE] = [DATE, MONTH, YEAR];
COLUMNS[MONTH] = [MONTH, YEAR];

// Hard limits for internal type. See WebCore/platform/DateComponents.h
var LIMIT_MIN = new Date(-62135596800000.0),
    LIMIT_MAX = new Date(8640000000000000.0);

function parseFromDateString(str) {
    var now = new Date(),
        match;
    if (str) {
        switch (_type) {
        case TIME:
            match = str.match(/(\d+):(\d+)/);
            _tzHours = match[1];
            _tzMinutes = match[2];
            _currentDate = new Date(now.getFullYear(), now.getMonth(), now.getDate(), _tzHours, _tzMinutes);
            break;
        case DATE:
            match = str.match(/(\d+)-(\d+)-(\d+)/);
            _currentDate = new Date(match[1], match[2] - 1, match[3]);
            break;
        case MONTH:
            match = str.match(/(\d+)-(\d+)/);
            _currentDate = new Date(match[1], match[2] - 1);
            break;
        case DATE_TIME: // fall-through
        case DATE_TIME_LOCAL:
            match = str.match(/(\d+)-(\d+)-(\d+)[T ](\d+):(\d+)/);
            var utcTime = new Date(match[1], match[2] - 1, match[3], match[4], match[5]);
            if (match = str.match(/(\d+)-(\d+)-(\d+)[T ](\d+):(\d+)+(\d+):(\d+)/)) { // positive timezone offset
                _tzHours = match[6];
                _tzMinutes = match[7];
                _tzOffset = (match[6] * 60 + match[7]) * 60000;
            } else if (result = str.match(/(\d+)-(\d+)-(\d+)[T ](\d+):(\d+)-(\d+):(\d+)/)) { // negative timezone offset
                _tzHours = match[6];
                _tzMinutes = match[7];
                _tzOffset = -(match[6] * 60 + match[7]) * 60000;
            }
            _currentDate = new Date(utcTime.getTime() + _tzOffset);
            break;
        }
    } else {
        switch (_type) {
        case TIME:
        case DATE_TIME:
        case DATE_TIME_LOCAL:
            _currentDate = new Date(now.getFullYear(), now.getMonth(), now.getDate(), now.getHours(), now.getMinutes());
            break;
        case DATE:
            _currentDate = new Date(now.getFullYear(), now.getMonth(), now.getDate());
            break;
        case MONTH:
            _currentDate = new Date(now.getFullYear(), now.getMonth());
            break;
        }
    }
}

function padTwo(v) { return ('0' + v).substr(-2, 2); }
function padFour(v) { return ('000' + v).substr(-4, 4); }

function getResultString() {
    switch(_type) {
    case TIME:
        return padTwo(_currentDate.getHours()) + ':' + padTwo(_currentDate.getMinutes());
    case DATE_TIME: // fall-through
    case DATE_TIME_LOCAL: // works because _tzOffset for dtlocal is 0
        var adjustedDate = new Date(_currentDate.getTime() - _tzOffset),
            datetime = padFour(adjustedDate.getFullYear()) + '-' + padTwo(adjustedDate.getMonth() + 1) + '-' + padTwo(adjustedDate.getDate()) + 'T' + padTwo(adjustedDate.getHours()) + ':' + padTwo(adjustedDate.getMinutes());
        if (_type === DATE_TIME_LOCAL) {
            return datetime;
        } else if (!_tzOffset) {
            return datetime + 'Z';
        } else if (_tzOffset > 0) {
            return datetime + '+' + padTwo(_tzHours) + ':' + padTwo(_tzMinutes);
        } else {
            return datetime + '-' + padTwo(_tzHours) + ':' + padTwo(_tzMinutes);
        }
    case DATE:
        return padFour(_currentDate.getFullYear()) + '-' + padTwo(_currentDate.getMonth() + 1) + '-' + padTwo(_currentDate.getDate());
    case MONTH:
        return padFour(_currentDate.getFullYear()) + '-' + padTwo(_currentDate.getMonth() + 1);
    }
    return '-1';
}

function ok() {
    document.body.innerHTML = '';
    window.setValueAndClosePopup(getResultString(), window.popUp);
}

function cancel() {
    document.body.innerHTML = '';
    window.setValueAndClosePopup('-1', window.popUp);
}

function adjustColumnLabelsForValue(col) {
    var cloneDate = new Date(_currentDate.getTime()),
        offset = Math.floor(col.children.length / 2);
    switch(col.type) {
    case HOURS:
        // FIXME: 12h/24h
        cloneDate.setHours(cloneDate.getHours() - offset);
        Array.prototype.forEach.call(col.children, function (child) {
            child.innerText = padTwo(cloneDate.getHours());
            cloneDate.setHours(cloneDate.getHours() + 1);
        });
        break;
    case MINUTES:
        cloneDate.setMinutes(cloneDate.getMinutes() - offset);
        Array.prototype.forEach.call(col.children, function (child) {
            child.innerText = padTwo(cloneDate.getMinutes());
            cloneDate.setMinutes(cloneDate.getMinutes() + 1);
        });
        break;
    case YEARMONTHDATE:
        // FIXME: show the year
        cloneDate.setDate(cloneDate.getDate() - offset);
        Array.prototype.forEach.call(col.children, function (child) {
            var dateMonth = document.createElement('div'),
                weekday = document.createElement('div');
            dateMonth.innerText = cloneDate.getDate() + ' ' + _shortMonthLabels[cloneDate.getMonth()]; // FIXME: should localize date as a whole instead of concatenating individual components
            weekday.innerText = _daysOfWeekLabels[cloneDate.getDay()];
            child.innerHTML = '';
            child.appendChild(dateMonth);
            child.appendChild(weekday);
            cloneDate.setDate(cloneDate.getDate() + 1);
        });
        break;
    case DATE:
        cloneDate.setDate(cloneDate.getDate() - offset);
        Array.prototype.forEach.call(col.children, function (child) {
            child.innerText = cloneDate.getDate();
            cloneDate.setDate(cloneDate.getDate() + 1);
        });
        break;
    case MONTH:
        cloneDate.setDate(1); // Otherwise the loop might skip shorter months
        cloneDate.setMonth(cloneDate.getMonth() - offset);
        Array.prototype.forEach.call(col.children, function (child) {
            child.innerText = _longMonthLabels[cloneDate.getMonth()];
            cloneDate.setMonth(cloneDate.getMonth() + 1);
        });
        break;
    case YEAR:
        cloneDate.setYear(cloneDate.getFullYear() - offset);
        Array.prototype.forEach.call(col.children, function (child) {
            child.innerText = padFour(cloneDate.getFullYear());
            cloneDate.setYear(cloneDate.getFullYear() + 1);
        });
        break;
    }
}

function beginScroll(evt) {
    _dragging = true;
    _firstY = evt.clientY || evt.touches[0].screenY;
    _scrollListener = scroll.bind(this);
    _endScrollListener = endScroll.bind(this);
    document.addEventListener('mousemove', _scrollListener);
    document.addEventListener('touchmove', _scrollListener);
    document.addEventListener('mouseup', _endScrollListener);
    document.addEventListener('touchend', _endScrollListener);
}

function scroll(evt) {
    if (_dragging) {
        var clientY = evt.clientY || evt.touches[0].screenY;
        this.getElementsByClassName('tall-bit')[0].style.webkitTransform = 'translateY(' + (clientY - _firstY + _baseOffset) + 'px) translateZ(0)';
    }
}

function endScroll(evt) {
    if (!_dragging) {
        return;
    }

    _dragging = false;
    document.removeEventListener('mousemove', _scrollListener);
    document.removeEventListener('touchmove', _scrollListener);
    document.removeEventListener('mouseup', _endScrollListener);
    document.removeEventListener('touchend', _endScrollListener);
    var clientY = evt.clientY || evt.changedTouches[0].screenY;
    var dy = clientY - _firstY,
        dItems = Math.floor(dy / ITEM_HEIGHT),
        extra = dy % ITEM_HEIGHT;
    if (extra < 0) {
        dItems++;
    }
    // Fudge: which item is actually mostly in the window?
    if (extra > ITEM_HEIGHT / 2) {
        // you've moved down more than half the next element, move some more
        dItems++;
        extra -= ITEM_HEIGHT;
    } else if (extra < -ITEM_HEIGHT / 2) {
        dItems--;
        extra += ITEM_HEIGHT;
    }
    var oldDate = new Date(_currentDate.getTime());
    switch(this.type) {
    case HOURS:
    case MINUTES:
    case DATE:
    case MONTH:
        _currentDate['set' + this.type](_currentDate['get' + this.type]() - dItems);
        break;
    case YEAR:
        _currentDate.setFullYear(_currentDate.getFullYear() - dItems);
        break;
    case YEARMONTHDATE:
        _currentDate.setDate(_currentDate.getDate() - dItems);
        break;
    }

    // TODO: animate the extraneous column changes
    // animate the one changed tallbit manually
    var tallBit = this.getElementsByClassName('tall-bit')[0];
    tallBit.style.webkitTransition = '-webkit-transform ' + Math.abs(extra * 2) + 'ms ease-out';
    tallBit.style.webkitTransform = 'translateY(' + (_baseOffset + dy - extra) + 'px) translateZ(0)';
    // use a timer instead of webkitTransitionEnd in case webkitTransitionEnd doesn't fire (happens if it skips all frames)
    setTimeout(function () {
        tallBit.style.webkitTransition = '';
        var tallBits = document.getElementsByClassName('tall-bit');
        Array.prototype.forEach.call(tallBits, function (tall) {
            switch(tall.type) {
            case HOURS:
            case MINUTES:
            case DATE:
            case MONTH:
                if (oldDate['get' + tall.type]() !== _currentDate['get' + tall.type]()) {
                    adjustColumnLabelsForValue(tall);
                }
                break;
            case YEAR:
                if (oldDate.getFullYear() !== _currentDate.getFullYear()) {
                    adjustColumnLabelsForValue(tall);
                }
                break;
            case YEARMONTHDATE:
                if (oldDate.getFullYear() !== _currentDate.getFullYear()
                    || oldDate.getMonth() !== _currentDate.getMonth()
                    || oldDate.getDate() !== _currentDate.getDate()
                ) {
                    adjustColumnLabelsForValue(tall);
                }
                break;
            }
        });
        resetPositions();
    }, Math.abs(extra * 2) + 1);
}

function createColumn(type) {
    var col = document.createElement('div'),
        tallBit = document.createElement('div');
    col.className = 'column ' + type;
    col.type = type;
    tallBit.className = 'tall-bit';
    tallBit.type = type;
    col.appendChild(tallBit);

    var numCells = Math.ceil(Math.max(screen.height, screen.width) / ITEM_HEIGHT) * 2 + 1;
    for (var i = 0; i < numCells; ++i) {
        var cell = document.createElement('div');
        cell.className = 'cell';
        tallBit.appendChild(cell);
    }

    adjustColumnLabelsForValue(tallBit);

    col.addEventListener('mousedown', beginScroll);
    col.addEventListener('touchstart', beginScroll);

    return col;
}

function resetPositions() {
    var tallBits = document.getElementsByClassName('tall-bit');
    Array.prototype.forEach.call(tallBits, function (tallBit) {
        tallBit.style.webkitTransform = 'translateY(' + _baseOffset + 'px) translateZ(0)';
    });
}

function show(pickerParams) {
    _type = pickerParams.type;
    parseFromDateString(pickerParams.initialValue);
    _min = pickerParams.min; // FIXME: min not supported
    _max = pickerParams.max; // FIXME: max not supported
    _step = pickerParams.step; // FIXME: step not supported
    // FIXME: disabled not supported (need to get this state passed in)

    _shortMonthLabels = pickerParams.uiText.shortMonthLabels;
    _longMonthLabels = pickerParams.uiText.monthLabels;
    _daysOfWeekLabels = pickerParams.uiText.daysOfWeekLabels;
    _amPmLabels = pickerParams.uiText.amPmLabels;

    if (!_currentDate) {
        // Parse failed
        cancel();
    }

    if (_currentDate < LIMIT_MIN) {
        _currentDate = new Date(LIMIT_MIN.getTime());
    } else if (_currentDate > LIMIT_MAX) {
        _currentDate = new Date(LIMIT_MAX.getTime());
    }

    // Set up the UI
    var popup = document.createElement('div');
    popup.className = popup.id = 'popup-area';
    popup.dir = pickerParams.direction;
    var header = document.createElement('div');
    header.className = 'popup-header';
    header.innerText = pickerParams.uiText.title;
    popup.appendChild(header);
    var content = document.createElement('div');
    content.className = 'popup-content';
    content.dir = 'ltr'; // FIXME: RTL UI specifications not done yet
    content.id = 'popup-content-time';
    popup.appendChild(content);

    COLUMNS[_type].forEach(function (type) {
        content.appendChild(createColumn(type));
    });

    var rowHighlightContainer = document.createElement('div'),
        filler1 = document.createElement('div'),
        selectedRowHighlight = document.createElement('div'),
        filler2 = document.createElement('div');
    rowHighlightContainer.className = 'row-highlight-container';
    selectedRowHighlight.className = 'row-highlight';
    filler1.className = filler2.className = 'row-highlight-filler';
    rowHighlightContainer.appendChild(filler1);
    rowHighlightContainer.appendChild(selectedRowHighlight);
    rowHighlightContainer.appendChild(filler2);
    content.appendChild(rowHighlightContainer);

    var okButton = document.createElement('button'),
        cancelButton = document.createElement('button'),
        buttons = document.createElement('div');
    buttons.className = 'popup-buttons';
    okButton.className = 'popup-button';
    okButton.innerText = pickerParams.uiText.doneButtonLabel;
    okButton.addEventListener('click', ok);
    cancelButton.className = 'popup-button';
    cancelButton.innerText = pickerParams.uiText.cancelButtonLabel;
    cancelButton.addEventListener('click', cancel);
    buttons.appendChild(cancelButton);
    buttons.appendChild(okButton);

    popup.appendChild(buttons);

    document.body.appendChild(popup);

    _baseOffset = Math.floor(document.getElementsByClassName('column')[0].clientHeight / 2 - document.getElementsByClassName('tall-bit')[0].clientHeight / 2);
    resetPositions();
}

window.popupcontrol = window.popupcontrol || {};
window.popupcontrol.show = show;
}());
