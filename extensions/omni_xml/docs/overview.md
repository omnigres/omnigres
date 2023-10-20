# Overview

`omni_xml` extension is an XML toolkit that provides barebones 
XML-related functionality. It is particularly useful when Postgres is
built without XML support or `xml2` extension.

Since Omnigres users may be using different builds of Postgres, we can't rely
on the availability of XML support. This extension closes the gap.

!!! warning "It is not fully W3C-compliant"

    The underlying library providing XML/XPath functionality is [not fully
    W3C-conformant](https://pugixml.org/docs/manual.html#xpath.w3c).

    In particular, support for the `namespace::` axis is unavailable.