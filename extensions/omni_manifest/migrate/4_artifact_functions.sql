/*{% include "../src/text_to_artifact.sql" %}*/
/*{% include "../src/text_to_artifacts.sql" %}*/
/*{% include "../src/json_to_artifact.sql" %}*/
/*{% include "../src/jsonb_to_artifact.sql" %}*/
/*{% include "../src/json_to_artifacts.sql" %}*/
/*{% include "../src/jsonb_to_artifacts.sql" %}*/
/*{% include "../src/artifact_to_json.sql" %}*/
/*{% include "../src/artifacts_to_json.sql" %}*/
/*{% include "../src/artifact_to_jsonb.sql" %}*/
/*{% include "../src/artifacts_to_jsonb.sql" %}*/
/*{% include "../src/artifact_to_text.sql" %}*/
/*{% include "../src/artifacts_to_text.sql" %}*/

create cast (text as artifact) with function text_to_artifact as implicit;
create cast (text as artifact[]) with function text_to_artifacts as implicit;
create cast (json as artifact) with function json_to_artifact as implicit;
create cast (jsonb as artifact) with function jsonb_to_artifact as implicit;
create cast (json as artifact[]) with function json_to_artifacts as implicit;
create cast (jsonb as artifact[]) with function jsonb_to_artifacts as implicit;
create cast (artifact as json) with function artifact_to_json as implicit;
create cast (artifact[] as json) with function artifacts_to_json as implicit;
create cast (artifact as jsonb) with function artifact_to_jsonb as implicit;
create cast (artifact[] as jsonb) with function artifacts_to_jsonb as implicit;
create cast (artifact as text) with function artifact_to_text as implicit;
create cast (artifact[] as text) with function artifacts_to_text as implicit;