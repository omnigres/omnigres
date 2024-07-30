--- Supporting types

-- SQL expression (domain over text)
create domain sql_expression as text;

-- A policy is a single source of truth for all access decisions
-- Can be represented with a document (such as YAML or JSON document)
create table policies
(
    id     integer primary key generated always as identity,
    -- Name of the policy
    name    text    not null,
    -- Version of the policy
    version text    not null,
    -- Only one policy can be active at a time
    ---
    -- Mostly for clarity of policy applications. If we want to "merge" policies,
    -- this needs to be done at the policy "document" level
    active boolean not null default false
);

-- Only one policy can be active at a time
create unique index policy_active on policies (name) where active;

-- Name/version pairs of the policy must be unique to ensure we're tracking
-- them correctly. TODO: do we really need this?
create unique index policy_unique_version on policies (name, version);

-- Every policy must define a set of Postgres roles that the policy will apply to
create table roles
(
    id          integer primary key generated always as identity,
    -- Reference to the policy
    policy_id   integer not null references policies (id),
    -- Postgres role name
    name name not null,
    -- Description of the role
    description text
);

-- Every policy can only list a given role once
create unique index roles_unique on roles (policy_id, name);

-- Every policy must define a set of relations (tables, views) that the policy will apply to
-- (and permit acccess to)
create table relations
(
    id        integer primary key generated always as identity,
    -- Reference to the policy
    policy_id integer not null references policies (id),
    -- Policy's name of the relation
    name      text    not null,
    -- Description of the relation
    description text,
    -- Underlying relation's name
    relation  text    not null,
    -- Underlying relation's columns
    columns   text[]  not null
);

-- TODO: "virtual relations" -- do we want to support them? or should we force users to create views

-- Every relation should declare joins to be used in scopes
create table relation_joins
(
    id          integer primary key generated always as identity,
    -- Reference to the relation being joined
    relation_id integer        not null references relations (id),
    -- Relation's name of the join
    name        text           not null,
    -- Name of the relation to join with
    -- TODO: should we limit the relations to be joined with to
    -- those expressly listed in the policy? If so, then we can use
    -- a reference here instead
    target text           not null,
    -- An expression to join on
    "on"   sql_expression not null
);

-- Predicates are critical to policies as they effectively allow to filter on
-- the context in a terms of business domain logic.
--
-- A predicate is a generalized qualifying statement effectively expressed as
-- a join.
--
-- Does this mean that predicates are just relations?
create table predicates
(
    id        integer primary key generated always as identity,
    -- Reference to the policy
    policy_id integer        not null references policies (id),
    -- Name of the predicate.
    name      text           not null,
    -- Joining on a relation
    --
    -- If null, will be joining on `(select)`
    target    text,
    -- SQL expression to compute the policy
    "on"      sql_expression not null
);

-- Compartmentalizing and sectioning policies into manageable scopes
create table scopes
(
    id   integer primary key generated always as identity,
    -- Name of the scope
    name text not null,
    -- Description of the scope
    description text,
    -- Scopes can be nested:
    -- Reference to a parent scope
    parent_id   integer references scopes (id)
);

-- Scopes are limited by applicable predicates
create table scope_predicates
(
    id           integer primary key generated always as identity,
    -- Reference to a scope
    scope_id     integer not null references scopes (id),
    -- Reference to an predicate
    predicate_id integer not null references predicates (id)
);

-- Scopes are further limited by Postgres roles that they apply to.
-- They could have been implemented using predicates, but they kind of have a special meaning
-- because we use these to give grants to the roles.
create table scope_roles
(
    -- Reference to a scope
    scope_id integer not null references scopes (id),
    -- Reference to an role
    role_id  integer not null references roles (id)
);

-- A scope can only have one instance of every role
create unique index scope_roles_unique on scope_roles (scope_id, role_id);

-- Operation is what can be done with a relation
create type relation_operation as enum ('select','update','insert','delete');

-- "Top-level" scopes are those that are attached to a policy
create table scopes_policies
(
    -- Reference to a scope
    scope_id  integer not null references scopes (id),
    -- Reference to a policy
    policy_id integer not null references policies (id)
);

-- A scope can only be attached to a policy once
create unique index scopes_policies_unique on scopes_policies (scope_id, policy_id);

-- Scope can be narrowed down by relations
create table scope_relation_targets
(
    id      integer primary key generated always as identity,
    -- Reference to a scope
    scope_id    integer              not null references scopes (id),
    -- Reference to a relation
    relation_id integer              not null references relations (id),
    -- Boolean filter on the relation to limit the application of the
    -- scope to the subset of data in the relation
    "where" sql_expression,
    -- Array of operations to be considered for this scope
    operations  relation_operation[] not null default '{}'
);

-- Scope can include joins
create table scope_relation_target_joins
(
    -- Reference to a scope relation target
    scope_relation_target_id integer not null references scope_relation_targets (id),
    -- Reference to a relation
    relation_join_id         integer not null references relation_joins (id)
);

-- A scope can only have one instance of every join
create unique index scope_relation_target_joins_unique on scope_relation_target_joins (scope_relation_target_id, relation_join_id);

-- Relation rule is the ultimate way to make a determination whether an operation on
-- a relational target can succeed.
create table scope_relation_rules
(
    id                       integer primary key generated always as identity,
    -- Relational target
    scope_relation_target_id integer              not null references scope_relation_targets (id),
    -- Rule name
    name                     text                 not null,
    -- Rule description
    description              text,
    -- Expression that would permit the scope
    "when"                   sql_expression,
    -- A rule may have exceptions. Exception belongs to one and only one rule
    exception_for_rule_id    integer references scope_relation_rules (id),
    -- Array of operations to be considered for this rule
    operations               relation_operation[] not null default '{}'
);

-- Scope can include joins
create table scope_relation_rule_joins
(
    -- Reference to a scope relation target
    scope_relation_rule_id integer not null references scope_relation_rules (id),
    -- Reference to a relation join
    relation_join_id       integer not null references relation_joins (id)
);

-- A scope can only have one instance of every join
create unique index scope_relation_rule_joins_unique on scope_relation_rule_joins (scope_relation_rule_id, relation_join_id);
