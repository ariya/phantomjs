
phantom.injectJs "../utils/core.js"

if phantom.args.length isnt 1
    console.log 'Usage: run-jasmine.coffee URL'
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
                if document.body.querySelector '.finished-at'
                    return true
                return false
        , ->
            page.evaluate ->
                console.log document.body.querySelector('.description').innerText
                list = document.body.querySelectorAll 'div.jasmine_reporter > div.suite.failed'
                for i in list
                    el = i
                    desc = el.querySelectorAll '.description'
                    console.log ''
                    for j in desc
                        console.log j.innerText

            phantom.exit()
