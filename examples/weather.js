// Get weather info for given address (or for the default one, "Mountain View")

var page = require('webpage').create(),
    address = "Mountain View"; //< default value

// Route "console.log()" calls from within the Page context to the main Phantom context (i.e. current "this")
page.onConsoleMessage = function(msg) {
    console.log(msg);
};

// Print usage message, if no twitter ID is passed
if (phantom.args.length < 1) {
    console.log("Usage: weather.js [address]");
} else {
    address = phantom.args.join(' ');
}

// Heading
console.log("*** Loading weather information for '" + address + "' ***\n");

// Open Google "secret" Weather API and, onPageLoad, do...
page.open(encodeURI('http://www.google.com/ig/api?weather=' + address), function (status) {
    // Check for page load success
    if (status !== "success") {
        console.log("Unable to access network");
    } else {
        // Execute some DOM inspection within the page context
        page.evaluate(function() {
            if (document.querySelectorAll('problem_cause').length > 0) {
                console.log('No data available for ' + address);
            } else {
                function data (s, e) {
                    var el;
                    e = e || document;
                    el = e.querySelector(s);
                    return el ? el.attributes.data.value : undefined;
                };

                console.log('City: ' + data('weather > forecast_information > city'));
                console.log('Current condition: ' + data('weather > current_conditions > condition'));
                console.log('Temperature: ' + data('weather > current_conditions > temp_f') + ' F');
                console.log(data('weather > current_conditions > humidity'));
                console.log(data('weather > current_conditions > wind_condition'));
                console.log('');

                var forecasts = document.querySelectorAll('weather > forecast_conditions');
                for (var i = 0; i < forecasts.length; ++i) {
                    var f = forecasts[i];
                    console.log(data('day_of_week', f) + ': ' +
                        data('low', f) + '-' + data('high', f) + ' F  ' +
                        data('condition', f));
                }
            }
        });
    }
    phantom.exit();
});
