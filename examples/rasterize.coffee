if phantom.state.length is 0
    if phantom.args.length < 2 or phantom.args.length > 3
        console.log 'Usage: rasterize.js URL filename [paperwidth*paperheight|paperformat]'
        console.log '  paper (pdf output) examples: "5in*7.5in", "10cm*20cm", "A4", "Letter"'
        phantom.exit()
    else
        address = phantom.args[0]
        phantom.state = 'rasterize'
        phantom.viewportSize = { width: 600, height: 600 }
        if phantom.args.length is 3 and phantom.args[1].substr(-4) is ".pdf"
            size = phantom.args[2].split '*'
            if size.length is 2
                phantom.paperSize = { width: size[0], height: size[1], border: '0px' }
            else
                phantom.paperSize = { format: phantom.args[2], orientation: 'portrait', border: '1cm' }
        phantom.open(address)
else
    output = phantom.args[1]
    phantom.sleep 200
    phantom.render output
    phantom.exit()
