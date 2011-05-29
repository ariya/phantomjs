# List following and followers from several accounts

users= [
  'sencha'
  'aconran'
  'ariyahidayat'
  'darrellmeyer'
  'DavidKaneda'
  'DmitryBaranovsk'
  'donovanerba'
  'edspencer'
  'helder_correia'
  'jamespearce'
  'jamieavins'
  'jarrednicholls'
  'jayrobinson'
  'lojjic'
  'mmullany'
  'philogb'
  'rdougan'
  'tmaintz'
  'whereisthysting'
  ]

follow = (user, callback) ->
  page = new WebPage()
  page.open 'http://mobile.twitter.com/' + user, (status) ->
    if status is 'fail'
      console.log user + ': ?'
    else
      data = page.evaluate -> document.querySelector('div.timeline-following').innerText
      console.log user + ': ' + data
    callback.apply()

process = () ->
  if (users.length > 0)
    user = users[0]
    users.splice(0, 1)
    follow(user, process)
  else
    phantom.exit()

process()
