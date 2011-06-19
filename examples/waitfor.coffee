##
# Try 'utils.core.waitfor'
##

phantom.injectJs "../utils/core.js"

page = new WebPage()

# Route "console.log()" calls from within the Page context to the main Phantom context (i.e. current "this")
page.onConsoleMessage = (msg) ->
    console.log msg

# Open Twitter on 'sencha' profile and, onPageLoad, do...
page.open 'http://twitter.com/#!/sencha', (status) ->
    # Check for page load success
    if status isnt 'success'
        console.log 'Unable to access network'
    else
        # Wait for 'signin-dropdown' to be visible
        utils.core.waitfor ->
            # Check in the page if a specific element is now visible
            page.evaluate ->
                $('#signin-dropdown').is ':visible'
        , ->
           console.log 'The sign-in dialog should be visible now.'
           phantom.exit()
