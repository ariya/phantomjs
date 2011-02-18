phantom.addCtxVar('start', Date.now().toString());
phantom.open('http://freegeoip.net/xml/', function(err) {
    if (err) {
		console.log('Cannot get the location');
	} else {
		var city = document.querySelector('City').textContent + ', ' 
			+ document.querySelector('RegionCode').textContent;
		var elapsed = Date.now() - new Date().setTime(phantom.ctx.start);
		console.log('Geolocated city: ' + city + ' (in ' + elapsed + 'msec)');
		phantom.addCtxVar('city', city);
		phantom.addCtxVar('start', Date.now().toString());
		phantom.open(encodeURI('http://www.google.com/ig/api?weather=' + city), function(err) {
			if (err || document.querySelectorAll('problem_cause').length > 0) {
				console.log('Cannot get weather for ' + phantom.ctx.city);
			} else {
				function data (s, e) {
					var el;
					e = e || document;
					el = e.querySelector(s);
					return el ? el.attributes.data.value : undefined;
				};
	
				var elapsed = Date.now() - new Date().setTime(phantom.ctx.start);
				console.log('Forecast city: ' + data('weather > forecast_information > city') + ' (in ' + elapsed + 'msec)');
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
});
