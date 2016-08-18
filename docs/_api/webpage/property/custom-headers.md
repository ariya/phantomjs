---
layout: post
title:  customHeaders
section: webpage
kind: property
permalink: api/webpage/property/custom-headers.html
---

`customHeaders` {object}

**Introduced:** PhantomJS 1.5

This property specifies additional HTTP request headers that will be sent to the server for every request issued (for pages _and_ resources). The default value is an empty object `{}`. Headers names and values get encoded in US-ASCII before being sent. Please note that the 'User-Agent' should be set using the `page.settings`, setting the 'User-Agent' property in this property will _overwrite_ the value set via `page.settings`.

## Examples

### Send two additional headers 'X-Test' and 'DNT'

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.customHeaders = {
  "X-Test": "foo",
  "DNT": "1"
};
```

Do you only want these `customHeaders` passed to the initial `page.open` request?

Here's the recommended workaround:

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.customHeaders = {
  "X-Test": "foo",
  "DNT": "1"
};

page.onInitialized = function() {
  page.customHeaders = {};
};
```








