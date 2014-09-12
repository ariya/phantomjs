import cStringIO as StringIO
import urlparse

def handle_request(req):
    url = urlparse.urlparse(req.path)

    body = "<!doctype html><h1>Status: {}</h1>".format(url.query)

    self.send_response(int(url.query))
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', str(len(body)))
    self.end_headers()
    return StringIO(body)
