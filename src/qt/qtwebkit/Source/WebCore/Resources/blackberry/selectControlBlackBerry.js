/*
 * Copyright (C) Research In Motion Limited, 2012. All rights reserved.
 */

// Upon the user making a selection, I will call window.setValueAndClosePopup with a binary string where
// the character at index i being '1' means that the option at index i is selected.
(function (){

    var selectOption = function (event) {
        for (var option = document.getElementById('popup-content').firstChild; option; option = option.nextSibling) {
            if (option === this) {
                if (option.className.indexOf('selected') === -1) {
                    option.className += ' selected';
                }
            } else {
                option.className = option.className.replace('selected', '');
            }
        }
        done();
    };

    var toggleOption = function (event) {
        if (this.className.indexOf('selected') === -1) {
            this.className += ' selected';
        } else {
            this.className = this.className.replace('selected', '');
        }
    };

    var done = function () {
        var result = '';
        for (var option = document.getElementById('popup-content').firstChild; option; option = option.nextSibling) {
            if (option.className.indexOf('selected') === -1) {
                result += '0';
            } else {
                result += '1';
            }
        }
        document.body.innerHTML = '';
        window.setValueAndClosePopup(result, window.popUp);
    };

    var cancel = function () {
        document.body.innerHTML = '';
        window.setValueAndClosePopup('-1', window.popUp);
    };

    /* pickerParams is an object literal with the following properties:
     *
     * isMultiSelect - a boolean
     * labels - an array of strings
     * enableds - an array of booleans.
     *   -I will assume that the HTML "disabled optgroups disable all options in the optgroup" hasn't been applied,
     *    so if the index corresponds to an optgroup, I will render all of its options as disabled
     * itemTypes - an array of integers, 0 === separator, 1 === optgroup, 2 === option, 3 === option in optgroup
     * selecteds - an array of booleans
     * uiText - localized text to display to user
     * direction - layout direction to use, 'ltr' or 'rtl'
     */
    var show = function (pickerParams) {
        var i,
            size = pickerParams.labels.length,
            popup = document.createElement('div'),
            header = document.createElement('div'),
            select = document.createElement('div'),
            topSelected;

        popup.className = 'popup-area';
        popup.dir = pickerParams.direction;
        select.className = select.id = 'popup-content';
        header.className = 'popup-header';
        header.innerText = pickerParams.uiText.title;
        popup.appendChild(header);
        popup.appendChild(select);

        for (i = 0; i < size; i++) {
            var option = document.createElement('div');
            option.classList.add('option');

            if (!pickerParams.enableds[i]) {
                option.classList.add('disabled');
                if (pickerParams.itemTypes[i] === 1) {
                    // TODO: disabled optgroup
                }
            }
            if (pickerParams.selecteds[i]) {
                option.classList.add('selected');
                if (!topSelected) {
                    topSelected = option;
                }
            }
            switch(pickerParams.itemTypes[i]) {
            case 1:
                option.classList.add('optgroup');
                break;
            case 3:
                option.classList.add('optgroup-option');
                break;
            }
            var contents = document.createElement('div');
            contents.classList.add('contents');
            var textContainer = document.createElement('div');
            textContainer.classList.add('text');
            // wrap label in <bdi> and don't set any dir attribute on it,
            // so that its directionality will be determined heuristically and isolated from the rest of the DOM
            var text = document.createElement('bdi');
            text.innerText = pickerParams.labels[i];
            textContainer.appendChild(text);
            contents.appendChild(textContainer);
            option.appendChild(contents);

            if (!option.classList.contains('disabled') && !option.classList.contains('optgroup')) {
                if (!pickerParams.isMultiSelect) {
                    option.addEventListener('click', selectOption);
                } else if (pickerParams.enableds[i]) {
                    option.addEventListener('click', toggleOption);
                }
            }

            select.appendChild(option);
        }

        var cancelButton = document.createElement('button'),
            buttons = document.createElement('div');
        buttons.className = 'popup-buttons';
        cancelButton.className = 'popup-button';
        cancelButton.addEventListener('click', cancel);
        cancelButton.appendChild(document.createTextNode(pickerParams.uiText.cancelButtonLabel));
        buttons.appendChild(cancelButton);
        if (pickerParams.isMultiSelect) {
            var okButton = document.createElement('button');
            okButton.className = 'popup-button';
            okButton.addEventListener('click', done);
            okButton.appendChild(document.createTextNode(pickerParams.uiText.doneButtonLabel));
            buttons.appendChild(okButton);
        }
        popup.appendChild(buttons);
        document.body.appendChild(popup);
        if (topSelected) {
            topSelected.scrollIntoView(true);
            if (topSelected.nextSibling) { // Unless the selected element is the last one in the list...
                // ...scroll the selected element further down a bit to show that we're not at the top of the list
                select.scrollTop -= 50;
            }
        }

        var _keyupTimeout,
            _searchText = '',
            clearSearch = function () { _searchText = ''; };
        document.addEventListener('keyup', function (evt) {
            _searchText += String.fromCharCode(evt.keyCode);
            window.clearTimeout(_keyupTimeout);
            _keyupTimeout = window.setTimeout(clearSearch, 1000); // Reset search string if there is no...
            // ...input within 1 second. 1 second is sufficient for one hand input for most cases from my
            // experiments. Let us set the time for now, and change later if someone has a better idea.

            Array.prototype.some.call(document.getElementsByClassName('text'),
            function (textDiv) {
                if (textDiv.innerText.toLowerCase().indexOf(_searchText.toLowerCase()) === 0) {
                    textDiv.scrollIntoView(true);
                    return true;
                }
            });
            evt.preventDefault();
        });
    };

    window.select = window.select || {};
    window.select.show = show;
}());
