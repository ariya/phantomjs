/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Cycle array of URLs and process with phantom.js
 Adds Array.prototype.forEachWebPage() iterator.

 EXAMPLE:
 Save screenshots. Command line:
 phantomjs phantom_js_url_cycle.js ./screenshots
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    [
    
      'www.google.com',
      'www.bbc.co.uk',
      'www.phantomjs.org' 
      
    ].forEachWebPage(function (url) {
      var out_dir = phantom.args[0] || '.';
      var file_name = [out_dir, url + '.png'].join('/'); // ./screenshots/www.google.com.png
      phantom.render(file_name);
      console.log('Generated ' + file_name);
    });
    
-------------------------- */
var UrlCycle = (function () {
  
  function extend (target, source) {
    for(var i in source) {
      target[i] = source[i];
    }
  }
  
  var prot = 'http://', urls, opts = {width: 800, height: 600};
  
  function cycle (urls, o, callback) {
    if(phantom.state.length === 0){ // first pass
      urls = urls;
      extend(opts, o || {});
      phantom.viewportSize = { width: opts.width, height: opts.height };
      phantom.state = 0;
      phantom.userAgent = 'Phantom.js bot';
      phantom.open(prot + urls[phantom.state]);
    } else { // page open
      
      callback(urls[phantom.state]);
      
      if(next_url = urls[++phantom.state]) {
        console.log('opening '+next_url)
        phantom.open(prot+next_url);
      } else {
        console.log('Done. Bye!')
        phantom.exit();
      }
    }
  }
  
  Array.prototype.forEachWebPage = function (callback, opts) {
    cycle(this, opts, callback);
  }
  
  return {
    cycle: cycle
  };

})();