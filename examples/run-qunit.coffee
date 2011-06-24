
phantom.injectJs "../utils/core.js"

if phantom.args.length isnt 1
    console.log 'Usage: run-qunit.coffee URL'
    phantom.exit()

page = new WebPage()

# Route "console.log()" calls from within the Page context to the main Phantom context (i.e. current "this")
page.onConsoleMessage = (msg) ->
    console.log msg

page.open phantom.args[0], (status) ->
    if status isnt 'success'
        console.log 'Unable to access network'
        phantom.exit()
    else
        utils.core.waitfor ->
            page.evaluate ->
                el = document.getElementById 'qunit-testresult'
                if el and el.innerText.match 'completed'
                    return true
                return false
        , ->
            failedNum = page.evaluate ->
                el = document.getElementById 'qunit-testresult'
                console.log el.innerText
                try
                    return el.getElementsByClassName('failed')[0].innerHTML
                catch e
                return 10000

            phantom.exit if parseInt(failedNum, 10) > 0 then 1 else 0
