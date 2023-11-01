# URL/URI Encoding

Functionality for [URL encoding](https://en.wikipedia.org/wiki/Percent-encoding)

## String encoding for a URL

To encode a string to be safely included as part of a URL:

```postgresql
select omni_web.url_encode('Hello World')
```

You will get `Hello%20World`

To decode and get back to the original string:

```postgresql
select omni_web.url_encode('Hello%20World')
```

### Encoding a URI

Similar to JavaScript's `encodeUri`/`decodeUri`, you can also encode/decode
a URI without encoding the "unreserved marks":

```postgresql
select omni_web.uri_encode('http://hu.wikipedia.org/wiki/São_Paulo')
```

The above results in `http://hu.wikipedia.org/wiki/S%C3%A3o_Paulo`

A counterpart function to that is `uri_decode`.