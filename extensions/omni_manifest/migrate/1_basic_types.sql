create type requirement as
(
    name    text,
    version text
);

/*{% include "../src/requirement.sql" %}*/

/*{% include "../src/requirement_to_text.sql" %}*/
/*{% include "../src/requirement_to_json.sql" %}*/
/*{% include "../src/requirement_to_jsonb.sql" %}*/
/*{% include "../src/requirements_to_text.sql" %}*/
/*{% include "../src/requirements_to_json.sql" %}*/
/*{% include "../src/requirements_to_jsonb.sql" %}*/
/*{% include "../src/text_to_requirement.sql" %}*/
/*{% include "../src/json_to_requirement.sql" %}*/
/*{% include "../src/jsonb_to_requirement.sql" %}*/
/*{% include "../src/text_to_requirements.sql" %}*/
/*{% include "../src/json_to_requirements.sql" %}*/
/*{% include "../src/jsonb_to_requirements.sql" %}*/

create cast (requirement as text) with function requirement_to_text as implicit;
create cast (requirement as json) with function requirement_to_json as implicit;
create cast (requirement as jsonb) with function requirement_to_jsonb as implicit;
create cast (requirement[] as text) with function requirements_to_text as implicit;
create cast (requirement[] as json) with function requirements_to_json as implicit;
create cast (requirement[] as jsonb) with function requirements_to_jsonb as implicit;
create cast (text as requirement) with function text_to_requirement as implicit;
create cast (json as requirement) with function json_to_requirement as implicit;
create cast (jsonb as requirement) with function jsonb_to_requirement as implicit;
create cast (text as requirement[]) with function text_to_requirements as implicit;
create cast (json as requirement[]) with function json_to_requirements as implicit;
create cast (jsonb as requirement[]) with function jsonb_to_requirements as implicit;

create type artifact as
(
    self requirement,
    requirements requirement[]
);

/*{% include "../src/artifact.sql" %}*/
