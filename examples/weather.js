if (phantom.state.length === 0) {
    var city = 'Mountain View';
    if (phantom.arguments.length > 0) {
        city = phantom.arguments.join(' ');
    }
    phantom.state = city;
    console.log('Loading ' + city);
    phantom.open(encodeURI('http://www.google.com/ig/api?weather=' + city));
} else {

    if (phantom.loadStatus === 'fail') {
        console.log('Unable to access network');
        phantom.exit(1);
    }

    if (document.querySelectorAll('problem_cause').length > 0) {
        console.log('No data available for ' + phantom.state);
        phantom.exit(1);
    }

    var data = function (s, e) {
        e = e || document;
        return e.querySelector(s).attributes.data.value;
    };

    console.log('');
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
    phantom.exit();
}
