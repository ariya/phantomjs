dygraphs JavaScript charting library
Copyright (c) 2006-, Dan Vanderkam.

Support: http://groups.google.com/group/dygraphs-users
Source: http://github.com/danvk/dygraphs
Issues: http://code.google.com/p/dygraphs/


The dygraphs JavaScript library produces produces interactive, zoomable charts of time series.

Features
- Plots time series without using an external server or Flash
- Supports multiple data series
- Supports error bands around data series
- Displays values on mouseover
- Interactive zoom
- Adjustable averaging period
- Customizable click-through actions
- Compatible with the Google Visualization API

Demo
For a gallery and documentation, see http://danvk.org/dygraphs/

Minimal Example
<html>
<head>
<script type="text/javascript" src="dygraph-combined.js"></script>
</head>
<body>
<div id="graphdiv"></div>
<script type="text/javascript">
  g = new Dygraph(
        document.getElementById("graphdiv"),  // containing div
        "Date,Temperature\n" +                // the data series
        "2008-05-07,75\n" +
        "2008-05-08,70\n" +
        "2008-05-09,80\n"
      );
</script>
</body>
</html>

License(s)
dygraphs uses:
 - rgbcolor.js (Public Domain)
 - strftime.js (BSD License)
 - excanvas.js (Apache License)
 - YUI compressor (BSD License)

rgbcolor: http://www.phpied.com/rgb-color-parser-in-javascript/
strftime: http://tech.bluesmoon.info/2008/04/strftime-in-javascript.html
excanvas: http://code.google.com/p/explorercanvas/
yui compressor: http://developer.yahoo.com/yui/compressor/

dygraphs is available under the MIT license, included in LICENSE.txt.
