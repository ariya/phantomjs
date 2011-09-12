# Upload an image to imagebin.org

page = require('webpage').create()

if phantom.args.length isnt 1
    console.log 'Usage: imagebin.coffee filename'
    phantom.exit()
else
    fname = phantom.args[0]
    page.open 'http://imagebin.org/index.php?page=add', ->
        page.uploadFile 'input[name=image]', fname
        page.evaluate ->
            document.querySelector('input[name=nickname]').value = 'phantom'
            document.querySelector('input[name=disclaimer_agree]').click()
            document.querySelector('form').submit()

        window.setTimeout ->
            phantom.exit()
        , 3000
