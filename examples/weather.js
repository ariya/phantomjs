if (phantom.storage.length === 0) {
    var city = 'Mountain View';
    if (phantom.arguments.length > 0) {
        city = phantom.arguments.join(' ');
    }
    phantom.storage = city;
    phantom.log('Loading ' + city);
    phantom.open(encodeURI('http://www.google.com/ig/api?weather=' + city));
} else {

    if (phantom.loadStatus === 'fail') {
        phantom.log('Unable to access network');
        phantom.exit(1);
    }

    if (document.querySelectorAll('problem_cause').length > 0) {
        phantom.log('No data available for ' + phantom.storage);
        phantom.exit(1);
    }

    var data = function (s, e) {
        e = e || document;
        return e.querySelector(s).attributes.data.value;
    };

    phantom.log('');
    phantom.log('City: ' + data('weather > forecast_information > city'));
    phantom.log('Current condition: ' + data('weather > current_conditions > condition'));
    phantom.log('Temperature: ' + data('weather > current_conditions > temp_f') + ' F');
    phantom.log(data('weather > current_conditions > humidity'));
    phantom.log(data('weather > current_conditions > wind_condition'));
    phantom.log('');

    var forecasts = document.querySelectorAll('weather > forecast_conditions');
    for (var i = 0; i < forecasts.length; ++i) {
        var f = forecasts[i];
        phantom.log(data('day_of_week', f) + ': ' +
            data('low', f) + '-' + data('high', f) + ' F  ' +
            data('condition', f));
    }
    phantom.exit();
}
