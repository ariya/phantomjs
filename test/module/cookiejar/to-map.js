
var assert = require('../../assert');

var cookies = [{
                'name':     'beforeExpires',        
                'value':    'expireValue',              
                'domain':   '.abc.com',             
                'path':     '/',
                'httponly': false,
                'secure':   false,
                'expires':  'Tue, 10 Jun 2025 12:28:29 GMT'
            },
            {
                'name':     'noExpiresDate',            
                'value':    'value',                
                'domain':   '.abc.com',             
                'path':     '/',
                'httponly': false,
                'secure':   false,
                'expires':  null
            },
            {
                'name':     'afterExpires',         
                'value':    'value',                
                'domain':   '.abc.com',             
                'path':     '/',
                'httponly': false,
                'secure':   false,
                'expires':  'Mon, 10 Jun 2024 12:28:29 GMT'
            }];

for(var cookieIdx in cookies)
    phantom.addCookie(cookies[cookieIdx]);

var phantomCookies = phantom.cookies;

for(var cookieIdx in phantomCookies)
{
    if( phantomCookies[cookieIdx].name === "noExpiresDate" )
    {
        var expiresDate = phantomCookies[cookieIdx].expires;
                assert.isTrue(expiresDate === undefined)
    }
}