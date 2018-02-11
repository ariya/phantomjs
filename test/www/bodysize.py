import time
import cStringIO as StringIO

def handle_request(req):
    req.send_response(200)
    req.end_headers()
    req.wfile.write('test')
    req.wfile.flush()
    time.sleep(1)
    req.wfile.write('test')

    return StringIO.StringIO('')
