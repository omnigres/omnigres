/*{% include "../src/sum_type_provision_trigger.sql" %}*/

create trigger sum_type_provision_trigger
    after insert
    on sum_types
    for each row
execute function sum_type_provision_trigger();
