if phantom.state.length is 0
    city = 'Mountain View'
    if phantom.args.length > 0
        city = phantom.args.join ' '
    phantom.state = city
    console.log "Loading #{ city }"
    phantom.open encodeURI "http://www.google.com/ig/api?weather=#{ city }"
else
    if phantom.loadStatus is 'fail'
        console.log 'Unable to access network'
    else
        if document.querySelectorAll('problem_cause').length > 0
            console.log "No data available for #{ phantom.state }"
        else
            data = (s, e) ->
                e = e or document
                el = e.querySelector s
                if el then el.attributes.data.value else null

            console.log ''
            console.log 'City: ' + data 'weather > forecast_information > city'
            console.log 'Current condition ' + data 'weather > current_conditions > condition'
            console.log 'Temperature: ' + data('weather > current_conditions > temp_f') + ' F'
            console.log data 'weather > current_conditions > humidity'
            console.log data 'weather > current_conditions > wind_condition'
            console.log ''

            forecasts = document.querySelectorAll 'weather > forecast_conditions'
            for f in forecasts
                console.log "#{ data 'day_of_week', f }: " +
                            "#{ data 'low', f }-" +
                            "#{ data 'high', f } F  " +
                            "#{ data 'condition', f }"

    phantom.exit()
