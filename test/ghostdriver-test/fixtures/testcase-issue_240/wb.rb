require 'json'
require 'shellwords'

def c (method, url, data)
  ret = `curl -s -X #{method} http://localhost:10000/#{url} --data-binary #{data.to_json.shellescape}`
  return JSON.parse ret rescue ret
end

def g (url, data={})
  c "GET", url, data
end

def o (url, data)
  c "POST", url, data
end

def a (session, action, data = {})
  o "session/#{session}/#{action}", data
end

def exec (session, js)
  a(session, "execute", { script: "return #{js}", args: [] })["value"]
end

def css (session, selector)
  a(session, "elements", { using: 'css selector', value: selector })["value"].map { |x| x["ELEMENT"] }
end

def click (session, element)
  a(session, "element/#{element}/click")
end

session_data = {
  desiredCapabilities: {
    browserName: "phantomjs",
    platform: "ANY",
    javascriptEnabled: true,
    cssSelectorsEnabled: true,
    takesScreenshot: true,
    nativeEvents: false,
    rotatable: false
    }
}

sessions = g("sessions", {})["value"]
if sessions.empty?
  o("session", session_data)
  sleep 0.5
end
session = g("sessions", {})["value"][0]["id"]

puts "Got session: #{session}"
a session, "url", { url: "http://localhost:8000/test.html" }
print "Going to do it now..."


css(session, "a").each do |i|
  click session, i
  # puts "Clicked on #{i}"
end

puts exec session, "results"
