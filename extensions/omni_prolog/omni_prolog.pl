:- use_module(library(clpb)).
:- use_module(library(clpfd)).

:- load_foreign_library(foreign('omni_prolog_stub.so'),install).

'$omni_load_code'(Name, String) :-
  open_string(String, Stream),
  load_files(Name, [stream(Stream)]).

sandbox:safe_primitive(user:arg(_, _)).
sandbox:safe_primitive(user:query(_, _, _)).

'$omni_sandbox_load_code'(Name, String) :-
  open_string(String, Stream),
  load_files(Name, [stream(Stream), sandboxed(true)]).

:- multifile result/1.

result(_) :- fail.

'$omni_sandbox_result'(Res) :-
  sandbox:safe_call(user:result(Res)).

prolog:message(unsupported_pg_type(Type)) -->
  [ 'Unsupported Postgres type \'', Type, '\'' ].