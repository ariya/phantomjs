# Get weather info for given address (or for the default one, "Mountain View")

page = require('webpage').create()
system = require 'system'
address = 'Mountain View' #< default value

# Route "console.log()" calls from within the Page context to the main Phantom context (i.e. current "this")
page.onConsoleMessage = (msg) ->
    console.log msg

# Print usage message, if no address is passed
if system.args.length < 2
    console.log 'Usage: weather.coffee [address]'
else
    address = Array.prototype.slice.call(system.args, 1).join(' ')

# Heading
console.log "*** Loading weather information for '#{address}' ***\n"

# Open Google "secret" Weather API and, onPageLoad, do...
page.open encodeURI("http://www.google.com/ig/api?weather=#{address}"), (status) ->
    # Check for page load success
    if status isnt 'success'
        console.log 'Unable to access network'
    else
        # Execute some DOM inspection within the page context
        page.evaluate ->
            if document.querySelectorAll('problem_cause').length > 0
                console.log "No data available for #{address}"
            else
                data = (s, e) ->
                    e = e or document
                    el = e.querySelector s
                    if el then el.attributes.data.value else undefined

                console.log """City: #{data 'weather > forecast_information > city'}
                               Current condition: #{data 'weather > current_conditions > condition'}
                               Temperature: #{data 'weather > current_conditions > temp_f'} F
                               #{data 'weather > current_conditions > humidity'}
                               #{data 'weather > current_conditions > wind_condition'}\n
                            """

                forecasts = document.querySelectorAll 'weather > forecast_conditions'
                for i in forecasts
                    console.log "#{ data 'day_of_week', i }: " +
                                "#{ data 'low', i }-" +
                                "#{ data 'high', i } F  " +
                                "#{ data 'condition', i }"
    phantom.exit()
