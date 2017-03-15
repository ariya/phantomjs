//@author: fengidri@gmail.com
phantom.onError = function(msg, trace){
  var msgStack = ['PHANTOM ERROR: ' + msg];
  if (trace && trace.length) {
    msgStack.push('TRACE:');
    trace.forEach(function(t) {
      msgStack.push(' -> ' + (t.file || t.sourceURL) + ': ' + t.line + (t.function ? ' (in function ' + t.function +')' : ''));
    });
  }
  console.error(msgStack.join('\n'));
  phantom.exit(1);
};

function paser_args(args)
{
    var i = 1;
    var dns = [];
    var url = undefined;
    while(i<args.length)
    {
        if (args[i] == '-d')
        {
            var d = args[i+1];
            if (undefined == dns)
            {
                console.log('-d: except dns description: domain/ip')
                phantom.exit(-1);
            }
            d = d.split('/')
            if (2 != d.length)
            {
                console.log('-d: except dns description: domain/ip')
                phantom.exit(-1);
            }
            dns.push(d);
            i += 2;
            continue;
        }

        if (undefined == url)
        {
            url = args[i];
            i += 1;
            continue;
        }
        i += 1;
    }
    if (undefined == url)
    {
        console.log('Usage: netsniff.js <some URL>');
        phantom.exit(1);
    }
    return [url, dns];
}


function lookup_domain(dnsmap, requestData, networkRequest)
{
    var domain = requestData.url.split('/')[2];
    for (var i in dnsmap)
    {
        var m_domain = dnsmap[i][0];
        var m_ip = dnsmap[i][1];
        if (m_domain == domain)
        {
            var url = requestData.url.replace(m_domain, m_ip);
            networkRequest.changeUrl(url)
            networkRequest.setHeader('Host', domain);
            return
        }
        if (m_ip == domain)
        {
            networkRequest.setHeader('Host', domain);
            return
        }
    }


}


var webPage = require('webpage');
var system = require('system');

var page = webPage.create();

var args = paser_args(system.args)
var domain_map = args[1];
page.URL = args[0];


page.onResourceRequested = function(requestData, networkRequest) {
  var match = requestData.url.match(/^data:image/);
  if (match) {
      return
  }

  lookup_domain(domain_map, requestData, networkRequest);
};


function Over(status)
{
    console.log(status)
    phantom.exit(0)
}
page.open(page.URL, Over);
