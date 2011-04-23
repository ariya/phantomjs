###
Extract text from a webpage using CSS selectors to include or exclude elements
By @westonruter
###

# Trim whitespace around an element and normalize the whitespace inside
# @param {String} s
# @returns {String}
trim = (s) ->
    s.replace(/\s+/g, ' ').replace(/^\s+/, '').replace(/\s+$/, '')

presetSelectors =
    wikipedia:
        excludeSelectors: "table.metadata, #bodyContent sup, #toc, .editsection, .rellink.relarticle.mainarticle, .rellink.boilerplate.seealso, table.vertical-navbox, .dablink, table.navbox, div.reflist, #catlinks, #mw-articlefeedback, table.infobox"
        includeSelectors: "#content p, #content h1, #content h2, #content h3, #content h4, #content h5, #content h6, #content li"
    _default:
        includeSelectors: 'h1, h2, h3, h4, h5, h6, p, li'

if not phantom.state
    if phantom.args.length < 1
        console.log 'Usage: extract-text.js [options] URL'
        console.log 'Try piping the output into the `say` command!'
        console.log 'Options:'
        console.log '--include SELECTORS'
        console.log '--exclude SELECTORS'
        console.log '--preset NAME (only "wikipedia" right now; default values for include/exclude)'
        phantom.exit()
    else
        state =
            url: null
            includeSelectors: null
            excludeSelectors: null

        # Parse the command-line arguments
        for arg in phantom.args
            matches = arg.match(/^--(\w+)(?:=(.+))?/)
            if matches
                name = matches[1]
                if matches[2]
                    value = matches[2].replace(/^["']/).replace(/["']$/)
                else
                    value = args.shift()

                switch name
                    when 'include'
                        state.includeSelectors = value
                    when 'exclude'
                        state.excludeSelectors = value
                    when 'preset'
                        if value not in presetSelectors
                            console.log "Error: Unrecognized preset selector set named '" + value + "'."
                            phantom.exit(1)
                        else
                            state.includeSelectors = presetSelectors[value].includeSelectors
                            state.excludeSelectors = presetSelectors[value].excludeSelectors
            else
                state.url = arg

        # Provide default selectors
        if not state.includeSelectors
            state.includeSelectors = presetSelectors._default.includeSelectors

        phantom.state = JSON.stringify(state)
        phantom.open(state.url)
else
    state = JSON.parse(phantom.state)

    # Remove the elements that we want excluded
    if state.excludeSelectors
        excludedElements = document.querySelectorAll(state.excludeSelectors)
        for el in excludedElements
            el.parentNode.removeChild(el)

    # Select only the remaining that we want included, and then print them out
    includedElements = document.querySelectorAll(state.includeSelectors)
    for el in includedElements
        text = trim(el.textContent)
        if text
            console.log text + "\n"

    phantom.exit()
