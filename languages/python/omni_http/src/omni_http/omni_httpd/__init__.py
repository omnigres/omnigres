import json
from omni_python import Composite, Custom
from ..omni_http import HttpHeader
from typing import TypedDict, Optional, Any
from dataclasses import dataclass

import sys
import io

@dataclass
class HTTPRequest:
    method: str
    path: str
    query_string: str
    body: Optional[bytes]
    headers: list[HttpHeader]

    def wsgi_environ(self) -> dict:
        environ = {'wsgi.version': (1, 0), 'wsgi.url_scheme': 'http', 'wsgi.input': io.BytesIO(self.body or b""), 'wsgi.multithread': False,
                   'REQUEST_METHOD': self.method, 'PATH_INFO': self.path, 'QUERY_STRING': self.query_string or ''}
        for header in self.headers:
            environ[f"HTTP_{header['name'].upper().replace('-', '_')}"] = header['value']
        
        # set content-length header to enable body stream
        if not environ.get('CONTENT_LENGTH'): # TODO: how to handle chunked data?
            environ['CONTENT_LENGTH'] = str(len(self.body or ""))
        return environ

    @classmethod
    def __pg_type_hint__(cls):
        from omni_python import Composite, Custom
        return Composite(cls, "omni_httpd.http_request")

HTTPOutcome = Custom(Any, "omni_httpd.http_outcome")

@dataclass
class HTTPResponse:
    headers: list[HttpHeader] = None
    body: bytes = b""
    status: int = 200

    def outcome(self) -> HTTPOutcome:
      __plpy = sys.modules['__main__'].plpy
      return __plpy.execute(__plpy.prepare("select omni_httpd.http_response(body => $1, status => $2, headers => $3) as result",
                                           ["bytea", "int","omni_http.http_headers"]), [self.body, self.status, self.headers])[0]["result"]


    @classmethod
    def __pg_type_hint__(cls):
        from omni_python import Composite, Custom
        return Composite(cls, "omni_httpd.http_response")
