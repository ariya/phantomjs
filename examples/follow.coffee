# List following and followers from several accounts

if phantom.state.length == 0
    phantom.state = [
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
    ].join ':'
    phantom.open 'http://mobile.twitter.com/sencha'
else
    users = phantom.state.split ':'
    id = users[0]
    next = users[1]
    data = document.querySelector 'div.timeline-following'
    phantom.state = users.slice(1).join ':'
    console.log id + ': ' + data.innerText
    if next
        phantom.open 'http://mobile.twitter.com/' + next
    else
        phantom.exit 1
