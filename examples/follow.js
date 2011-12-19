// List following and followers from several accounts

var users = ['sencha',
        'aconran',
        'adityabansod',
        'ambisinister',
        'arnebech',
        'ariyahidayat',
        'arthurakay',
        'bmoeskau',
        'darrellmeyer',
        'davidfoelber',
        'DavidKaneda',
        'donovanerba',
        'edspencer',
        'evantrimboli',
        'ExtAnimal',
        'jamieavins',
        'jarrednicholls',
        'jayrobinson',
        'lojjic',
        'luckymethod',
        'merrells',
        'mmullany',
        'philogb',
        'philstrong',
        'rdougan',
        'SubtleGradient',
        '__ted__',
        'tmaintz',
        'WesleyMoy',
        'whereisthysting'];

function follow(user, callback) {
    var page = require('webpage').create();
    page.open('http://mobile.twitter.com/' + user, function (status) {
        if (status === 'fail') {
            console.log(user + ': ?');
        } else {
            var data = page.evaluate(function () {
                return document.querySelector('div.timeline-following').innerText;
            });
            console.log(user + ': ' + data);
        }
        callback.apply();
    });
}

function process() {
    if (users.length > 0) {
        var user = users[0];
        users.splice(0, 1);
        follow(user, process);
    } else {
        phantom.exit();
    }
}

process();
