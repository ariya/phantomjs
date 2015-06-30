# -*- encoding: utf-8 -*-
import cStringIO as StringIO
import urlparse
def handle_request(req):
    url = urlparse.urlparse(req.path)
    if url.query:
        req.send_response(404, u"不存在".encode("utf-8"))
    else:
        req.send_response(200, u"行".encode("utf-8"))
    req.send_header("Set-Cookie", u"κουλουράκι".encode("utf-8"))
    req.end_headers()

    if url.query:
        return StringIO.StringIO("")
    else:
        return StringIO.StringIO("""<!doctype html><img src="?1">""")
