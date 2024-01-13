create function artifact(self requirement, requirements requirement[]) returns artifact
    immutable
    language sql
as
$$
select row (self, requirements)::omni_manifest.artifact
$$;