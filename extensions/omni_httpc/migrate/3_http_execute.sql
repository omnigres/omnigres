create type http_execute_options as
(
    http2_ratio           smallint,
    http3_ratio           smallint,
    force_cleartext_http2 bool,
    timeout               int,
    first_byte_timeout    int,
    follow_redirects      bool
);

create domain valid_http_execute_options as http_execute_options
    check ( (value).http2_ratio >= 0 and (value).http2_ratio <= 100 and
            (value).http3_ratio >= 0 and (value).http3_ratio <= 100 and
            ((value).http2_ratio + (value).http3_ratio) <= 100
        );

/*{% include "../src/http_execute_options.sql" %}*/
/*{% include "../src/http_execute.sql" %}*/
/*{% include "../src/http_execute_with_options.sql" %}*/
