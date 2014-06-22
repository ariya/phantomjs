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

var ui = ui || {};
ui.actions = ui.actions || {};

(function() {

var Action = base.extends('button', {
    init: function() {
        this._eventName = null;
        $(this).addClass('action');
        this.addEventListener('click', function(event) {
            if (this._eventName) {
                $(this).trigger(this._eventName);
                event.stopPropagation();
            }
        }.bind(this));
    },
    makeDefault: function() {
        $(this).addClass('default');
        return this;
    }
});

ui.actions.Blame = base.extends(Action, {
    init: function() {
        this.textContent = 'Blame';
        this._eventName = 'blame';
        this.title = 'Blames this failure on this revision.'
    }
});

ui.actions.Close = base.extends(Action, {
    init: function() {
        this.textContent = 'Close';
        this._eventName = 'close';
    }
});

ui.actions.Rollout = base.extends(Action, {
    init: function() {
        this.textContent = 'Roll out';
        this._eventName = 'rollout';
        this.title = 'Rolls out this revision.'
    }
});

ui.actions.Examine = base.extends(Action, {
    init: function() {
        this.textContent = 'Examine';
        this._eventName = 'examine';
        this.title = 'Examine these failures in detail.'
    }
});

ui.actions.Rebaseline = base.extends(Action, {
    init: function() {
        this.textContent = 'Rebaseline';
        this._eventName = 'rebaseline';
    }
});

ui.actions.ExpectFailure = base.extends(Action, {
    init: function() {
        this.textContent = 'Expect Failure';
        this._eventName = 'expectfailure';
    }
});

ui.actions.Next = base.extends(Action, {
    init: function() {
        this.innerHTML = '&#9654;';
        this._eventName = 'next';
        $(this).addClass('next');
    }
});

ui.actions.Previous = base.extends(Action, {
    init: function() {
        this.innerHTML = '&#9664;';
        this._eventName = 'previous';
        $(this).addClass('previous');
    }
});

ui.actions.List = base.extends('ul', {
    init: function(actions) {
        this.className = 'actions';
        if (!actions)
            return;
        actions.forEach(this.add.bind(this));
    },
    add: function(action)
    {
        var item = document.createElement('li');
        item.appendChild(action);
        $(this).append(item);
    }
});

})();
