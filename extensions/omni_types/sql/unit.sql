select omni_types.unit();

select pg_column_size(omni_types.unit());

-- in/out
select ''::omni_types.unit;
select ''::omni_types.unit::text;

-- binary send/recv
select omni_types.unit_send(omni_types.unit());
