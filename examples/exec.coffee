if phantom.args.length is 0
  console.log 'Try to pass some args when invoking this script!'
  phantom.exit()
else
  argstr = ''
  for arg, i in phantom.args
    argstr += arg + ' '
  phantom.exec(argstr)
