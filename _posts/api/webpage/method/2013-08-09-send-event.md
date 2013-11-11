---
layout: post
title:  sendEvent
categories: api webpage webpage-method
permalink: api/webpage/method/send-event.html
---

Sends an event to the web page.

[1.7 implementation source](https://github.com/ariya/phantomjs/blob/63e06cb/src/webpage.cpp#L1015).

The events are **not** synthetic [DOM events](http://www.w3.org/TR/DOM-Level-2-Events/events.html), each event is sent to the web page as if it comes as part of user interaction.

## Mouse events

`sendEvent(mouseEventType[, mouseX, mouseY, button='left'])`

The first argument is the event type. Supported types are `'mouseup'`, `'mousedown'`, `'mousemove'`, `'doubleclick'` and `'click'`. The next two arguments are optional but represent the mouse position for the event.

The button parameter (defaults to `left`) specifies the button to push.

For `'mousemove'`, however, there is no button pressed (i.e. it is not dragging).

## Keyboard events

`sendEvent(keyboardEventType, keyOrKeys, [null, null, modifier])`

The first argument is the event type. The supported types are: `keyup`, `keypress` and `keydown`. The second parameter is a key (from [page.event.key](https://github.com/ariya/phantomjs/commit/cab2635e66d74b7e665c44400b8b20a8f225153a)), or a string.

You can also indicate a fifth argument, which is an integer indicating the modifier key.

* 0: No modifier key is pressed
* 0x02000000: A Shift key on the keyboard is pressed
* 0x04000000: A Ctrl key on the keyboard is pressed
* 0x08000000: An Alt key on the keyboard is pressed
* 0x10000000: A Meta key on the keyboard is pressed
* 0x20000000: A keypad button is pressed

Third and fourth argument are not taken account for keyboard events. Just give null for them.

## Examples

### Simulate a shift+alt+A keyboard combination

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.sendEvent('keypress', page.event.key.A, null, null, 0x02000000 | 0x08000000);
```








