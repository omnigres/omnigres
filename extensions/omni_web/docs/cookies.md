# Cookies

## Getting cookies

One can use `omni_web.cookies(text)` to convert a `Cookie` header string 
into a table of key/value pairs of cookies.


```postgresql
omni_web=# select * from omni_web.cookies('PHPSESSID=298zf09hf012fh2; csrftoken=u32t4o3tb3gg43; _gat=1'::text);
   name    |      value      
-----------+-----------------
 PHPSESSID | 298zf09hf012fh2
 csrftoken | u32t4o3tb3gg43
 _gat      | 1
(3 rows)
```