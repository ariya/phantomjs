if phantom.args.length is 0
    console.log 'Try to pass some args when invoking this script!'
else
    for arg, i in phantom.args
        console.log i + ': ' + arg
phantom.exit()
