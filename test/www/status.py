import cStringIO as StringIO
import urlparse

def handle_request(req):
    url = urlparse.urlparse(req.path)

    body = "<!doctype html><h1>Status: {}</h1>".format(url.query)

    req.send_response(int(url.query))
    req.send_header('Content-Type', 'text/html')
    req.send_header('Content-Length', str(len(body)))
    req.end_headers()
    return StringIO.StringIO(body)
