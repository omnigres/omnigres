from typing import Optional

from . import HTTPResponse, HTTPRequest, HTTPOutcome
from ..omni_http import HttpHeader

try:
    import flask
except ImportError:
    raise RuntimeError("To use this feature, please install the optional dependency by 'pip install omni_http[Flask]'.")

class responder:
    status: int = 200
    headers: list[HttpHeader] = []

    def __call__(self, status, headers):
        self.status = int(status.split()[0])
        self.headers = headers

class Adapter:
    app: Optional[flask.Flask] = None

    """
    Wraps Flask application into a callable that can respond to HTTP requests
    from Omnigres' omni_httpd.
    """

    def __init__(self, app: flask.Flask):
        self.app = app

    def __call__(self, req: HTTPRequest) -> HTTPOutcome:
        res = responder()
        response = self.app.wsgi_app(req.wsgi_environ(), res)

        body = b""
        for chunk in response:
            body += chunk

        return HTTPResponse(status = res.status, body = body, headers = res.headers).outcome()