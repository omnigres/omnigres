create type statement_type as enum (
    'MultiStmt', -- special type for multiple statements
    'InsertStmt', 'DeleteStmt', 'UpdateStmt', 'MergeStmt', 'SelectStmt', 'SetOperationStmt', 'ReturnStmt',
    'PLAssignStmt', 'CreateSchemaStmt', 'AlterTableStmt', 'ReplicaIdentityStmt', 'AlterCollationStmt', 'AlterDomainStmt',
    'GrantStmt', 'GrantRoleStmt', 'AlterDefaultPrivilegesStmt', 'CopyStmt', 'VariableSetStmt', 'VariableShowStmt',
    'CreateStmt', 'CreateTableSpaceStmt', 'DropTableSpaceStmt', 'AlterTableSpaceOptionsStmt', 'AlterTableMoveAllStmt',
    'CreateExtensionStmt', 'AlterExtensionStmt', 'AlterExtensionContentsStmt', 'CreateFdwStmt', 'AlterFdwStmt',
    'CreateForeignServerStmt', 'AlterForeignServerStmt', 'CreateForeignTableStmt', 'CreateUserMappingStmt',
    'AlterUserMappingStmt', 'DropUserMappingStmt', 'ImportForeignSchemaStmt', 'CreatePolicyStmt', 'AlterPolicyStmt',
    'CreateAmStmt', 'CreateTrigStmt', 'CreateEventTrigStmt', 'AlterEventTrigStmt', 'CreatePLangStmt', 'CreateRoleStmt',
    'AlterRoleStmt', 'AlterRoleSetStmt', 'DropRoleStmt', 'CreateSeqStmt', 'AlterSeqStmt', 'DefineStmt', 'CreateDomainStmt',
    'CreateOpClassStmt', 'CreateOpFamilyStmt', 'AlterOpFamilyStmt', 'DropStmt', 'TruncateStmt', 'CommentStmt', 'SecLabelStmt',
    'DeclareCursorStmt', 'ClosePortalStmt', 'FetchStmt', 'IndexStmt', 'CreateStatsStmt', 'AlterStatsStmt', 'CreateFunctionStmt',
    'AlterFunctionStmt', 'DoStmt', 'CallStmt', 'RenameStmt', 'AlterObjectDependsStmt', 'AlterObjectSchemaStmt',
    'AlterOwnerStmt', 'AlterOperatorStmt', 'AlterTypeStmt', 'RuleStmt', 'NotifyStmt', 'ListenStmt', 'UnlistenStmt',
    'TransactionStmt', 'CompositeTypeStmt', 'CreateEnumStmt', 'CreateRangeStmt', 'AlterEnumStmt', 'ViewStmt', 'LoadStmt',
    'CreatedbStmt', 'AlterDatabaseStmt', 'AlterDatabaseRefreshCollStmt', 'AlterDatabaseSetStmt', 'DropdbStmt',
    'AlterSystemStmt', 'ClusterStmt', 'VacuumStmt', 'ExplainStmt', 'CreateTableAsStmt', 'RefreshMatViewStmt',
    'CheckPointStmt', 'DiscardStmt', 'LockStmt', 'ConstraintsSetStmt', 'ReindexStmt', 'CreateConversionStmt', 'CreateCastStmt',
    'CreateTransformStmt', 'PrepareStmt', 'ExecuteStmt', 'DeallocateStmt', 'DropOwnedStmt', 'ReassignOwnedStmt',
    'AlterTSDictionaryStmt', 'AlterTSConfigurationStmt', 'CreatePublicationStmt', 'AlterPublicationStmt',
    'CreateSubscriptionStmt', 'AlterSubscriptionStmt', 'DropSubscriptionStmt', 'PlannedStmt'
    );

create function statement_type(statement) returns statement_type
    language sql
    strict immutable as
$$
select omni_sql._statement_type($1)::text::omni_sql.statement_type
$$;

create function _statement_type(statement) returns cstring
as
'MODULE_PATHNAME',
'statement_type'
    language c strict
               immutable;