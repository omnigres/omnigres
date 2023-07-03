create function plprologu_call_handler() returns language_handler
as
'MODULE_PATHNAME' language c;

create language plprologu handler plprologu_call_handler;

alter language plprologu owner to @extowner@;

comment on language plprologu is 'PL/Prolog language (untrusted)';

create function plprolog_call_handler() returns language_handler
as
'MODULE_PATHNAME' language c;

create trusted language plprolog handler plprolog_call_handler;

alter language plprolog owner to @extowner@;

comment on language plprolog is 'PL/Prolog language (trusted)';