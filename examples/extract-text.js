/**
 * Extract text from a webpage using CSS selectors to include or exclude elements
 * By @westonruter
 */

var presetSelectors = {
    wikipedia: {
        excludeSelectors: "table.metadata, #bodyContent sup, #toc, .editsection, .rellink.relarticle.mainarticle, .rellink.boilerplate.seealso, table.vertical-navbox, .dablink, table.navbox, div.reflist, #catlinks, #mw-articlefeedback, table.infobox",
        includeSelectors: "#content p, #content h1, #content h2, #content h3, #content h4, #content h5, #content h6, #content li"
    },
    _default: {
        includeSelectors: 'h1, h2, h3, h4, h5, h6, p, li'
    }
};

if(!phantom.state) {
    if(phantom.args.length < 1){
        console.log('Usage: extract-text.js [options] URL');
        console.log('Try piping the output into the `say` command!');
        console.log('Options:');
        console.log('--include SELECTORS');
        console.log('--exclude SELECTORS');
        console.log('--preset NAME (only "wikipedia" right now; default values for include/exclude)');
        phantom.exit();
    }
    else {
        var state = {
            url: null,
            includeSelectors: null,
            excludeSelectors: null
        };
        
        // Parse the command-line arguments
        var args = toArray(phantom.args);
        while(args.length){
            var arg = args.shift();
            var matches = arg.match(/^--(\w+)(?:=(.+))?/);
            if(matches){
                var name = matches[1];
                var value;
                if(matches[2]){
                    value = matches[2].replace(/^["']/).replace(/["']$/);
                }
                else {
                    value = args.shift();
                }
                
                switch(name){
                    case 'include':
                        state.includeSelectors = value;
                        break;
                    case 'exclude':
                        state.excludeSelectors = value;
                        break;
                    case 'preset':
                        if(!(value in presetSelectors)){
                            console.log("Error: Unrecognized preset selector set named '" + value + "'.");
                            phantom.exit(1);
                        }
                        else {
                            state.includeSelectors = presetSelectors[value].includeSelectors;
                            state.excludeSelectors = presetSelectors[value].excludeSelectors;
                        }
                        break;
                }
            }
            else {
                state.url = arg;
            }
        }
        
        // Provide default selectors
        if(!state.includeSelectors){
            state.includeSelectors = presetSelectors._default.includeSelectors;
        }
        
        phantom.state = JSON.stringify(state);
        phantom.open(state.url);
    }
}
else {
    var state = JSON.parse(phantom.state);
    
    // Remove the elements that we want excluded
    if(state.excludeSelectors){
        var excludedElements = document.querySelectorAll(state.excludeSelectors);
        toArray(excludedElements).forEach(function(el){
            el.parentNode.removeChild(el);
        });
    }
    
    // Select only the remaining that we want included, and then print them out
    var includedElements = document.querySelectorAll(state.includeSelectors);
    toArray(includedElements).forEach(function(el){
        var text = trim(el.textContent);
        if(text){
            console.log(text + "\n");
        }
    });
    
    phantom.exit();
}


/**
 * Coerce an array-like object into an array and/or copy an array
 * @param {Object} list
 * @returns {Array}
 */
function toArray(list){
    return Array.prototype.slice.call(list);
}

/**
 * Trim whitespace around an element and normalize the whitespace inside
 * @param {String} s
 * @returns {String}
 */
function trim(s){
    return s.replace(/\s+/g, ' ').replace(/^\s+/, '').replace(/\s+$/, '');
}
