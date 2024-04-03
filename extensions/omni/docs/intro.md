# Intro to omni

`omni` is a Postgres shared library (and also available as an extension) that helps managing and developing Postgres
extensions easier. Think of it as an extension for extensions !

## Why?

The ecosystem of Postgres extensions is expanding at a fast pace, but the supporting tools and infrastructure are
lagging.
For context, an extension that allocates shared memory or initiates background workers necessitates a Postgres server
restart.
Another challenge is setting up multiple hooks of the same type and properly accessing them in the right order.

## Lifecycle of Omni

###  when and how Omni modules loaded and how they are initialized and how this taps into the lifecycle of things like hook registration lifecycle (when it goes into effect, out, etc.), shared memory (how do we know who needs to initialize the memory?), similar situation for bgworker, how do we use it for different “topologies”: which Omni_init starts, which one stops, what’s “timing” there, etc.

An extension that’s built using Omni, we call it an Omni extension. It’s the same as a vanilla Postgres extension, but the life cycle of an Omni extension depends on how Omni works internally. It is useful for extensions with C language functions.

Every Omni extension should have a SQL script that creates a Postgres procedure (or function). When the SQL script is executed Omni uses Postgres hooks to load the shared object file of the extension, also referred to as the Omni module.

Every Omni extension should have a _Omni_init() (instead of _PG_init()) where the necessary Postgres resources like Background workers, Shared memory, GUC, and Hooks can be configured. There is also a _Omni_deinit() which can be used for cleaning of these Postgres resources.


## Hooks:

Omni has default hooks for the currently [supported hook types](https://github.com/omnigres/omnigres/issues/488). The default hook calls the default execution path (e.g. standard_executor(), planner()), if there is no existing hook for that type. The hooks registered by Omni can be listed through `Omni.hooks` view

```postgresql
postgres=# select * from omni.hooks;
      hook       |       name        | module_id | pos
-----------------+-------------------+-----------+-----
 executor_start  | default           |           |   1
 executor_run    | default           |           |   1
 executor_finish | default           |           |   1
 executor_end    | default           |           |   1
 process_utility | extension upgrade |         1 |   1
 process_utility | default           |           |   2
 process_utility | extension upgrade |         1 |   3
(9 rows)

```
Once the Omni module is loaded, the hooks goes into effect. They stay in-effect until the module is removed (through `drop extension` command). 

The following snippet registers a ExecutorRun_hook_type function

```yourextension.c
void _Omni_init(const omni_handle *handle){ 
  omni_hook executor_hook; // config of the hook
  executor_hook.type = omni_hook_executor_run; // set type of hook
  executor_hook.name = "pg_savior_executor_run_hook"; // name of hook
  executor_hook.fn.executor_run = ExecutorRun_hook_savior; // function to be called for executor_run stage
  handle->register_hook(handle, &executor_hook); // register the hook
}
```

## Shared Memory:

Omni extensions can allocate shared memory for sharing data between multiple processes. Each allocation is associated with a name. If allocation is requested with the same name multiple times, it will be allocated only once and the subsequent requests return the allocation from the first request.

## Background Worker:

Omni extensions can start Postgres background workers without restarting Postgres.

Background workers spun with Omni can be spun at different "timing" - after commit, at commit, immediately

## Features

* Hooks initialization
* Allocate/Deallocate shared memory
* Managing background workers
* Upgrading extensions without downtime
* Setting custom GUC

## Installing omni

### Download omni

```shell
$ export omni_latest_version=$(curl -s https://index.omnigres.com/16/Release/ubuntu-x86-64/index.json | jq -r '.extensions.omni | keys | sort | .[-1]')

$ curl -s https://raw.githubusercontent.com/omnigres/omnigres/master/download-omnigres-extension.sh | bash -s install omni $omni_latest_version
```

Option: Refer [omni_manifest](/omni_manifest/usage/#install) for more on installing extensions

## Modify shared_preload_libraries in your postgresql.conf
```shell
$ sed -i.bak "s/^shared_preload_libraries =.*/shared_preload_libraries = '$omni_latest_version,\0'/" /path/to/postgresql.conf
```

## Create extension omni
```sql
postgres=# create extension omni;
```

## Integrating omni in your extension

- Include omni header file [omni_v0.h](https://github.com/omnigres/omnigres/blob/master/omni/omni/omni_v0.h) in your extension's `.c` file
- Copy the [omni_v0.h](https://github.com/omnigres/omnigres/blob/master/omni/omni/omni_v0.h) to the source directory
- Declare `OMNI_MAGIC` in your extension's `.c` file
- Define the callback function `_Omni_init` (refer the header file for the signature)
- Restart Postgres since omni itself is a shared preload library

## Using omni

## Hook capabilities

Declare a hook variable that is of type `omni_hook` and register that with the handle (of type `omni_handle`) in the
callback function `_Omni_init`.

**Usage:**

```c
void planner_hook_fn(omni_hook_handle *handle, Query *parse, const char *query_string,
                     int cursorOptions, ParamListInfo boundParams) {
  ereport(NOTICE, errmsg("planner_hook_fn %p", handle->returns.PlannedStmt_value));
}
void _Omni_init(const omni_handle *handle) {
  omni_hook planner_hook = {.type = omni_hook_planner,
                            .name = "omni_guard planner hook",
                            .fn = {.planner = planner_hook_fn},
                            .wrap = true};
  handle->register_hook(handle, &planner_hook);
}
```

### Deciding a hook's next action

`omni_handle` has a field `hook_next_action` which can be used to determine what to do after the successful execution of
a hook

If you decide not to run the default hook, you can set handle's `next_action` field to `hook_next_action_finish`

### Wrapping a hook

Setting `omni_hook`'s wrap field to true runs a hook before and after the default hook. You can check the order of the
hooks from the helper view [`omni.hooks`](./reference.md).

## Shared memory allocation

Inside the callback `_Omni_init`, invoke the `allocate_shmem` method of `omni_handle` type.

```c
void _Omni_init(const omni_handle *handle) {
  bool found;
  int a = handle->allocate_shmem(handle, "shmem example using omni", sizeof(int), NULL, NULL, &found);
}
```

## Shared memory deallocation

Inside the callback `_Omni_deinit`, call `deallocate_shmem` method of `omni_handle` type.

```c
void _Omni_deinit(const omni_handle *handle) {
    handle->deallocate_shmem(handle, "shmem example using omni", NULL);
}
```

## Setting GUC's for your extension

Inside the callback `_Omni_init`, invoke `declare_guc_variable` method of `omni_handle` type.

```c
void _Omni_init(const omni_handle *handle) {
  omni_guc_variable guc_example_variable = {
      .name = "guc_example_variable",
      .long_desc = "Example variable to set GUC with omni",
      .type = PGC_STRING,
      .typed = {.string_val = {.boot_value = "hello"}},
      .context = PGC_SIGHUP};
}
```

## Starting background workers

Generally, starting [background workers](https://www.postgresql.org/docs/current/bgworker.html) from an extension
require Postgres restart.

Inside the callback `_Omni_init`, invoke `request_bgworker_start` method of `omni_handle` type.

`request_bgworker_start` takes an argument of type [`BackgroundWorker`](https://www.postgresql.org/docs/current/bgworker.html)

```c
void example_entry_point() {
  elog(INFO, "do something useful here");
}
void _Omni_init(const omni_handle *handle) {
omni_bgworker_handle example_bgworker_handle = handle->request_bgworker_start(handle, {
    .bgw_name = "example bgw worker from omni"
    .bgw_type = "omni"
    .bgw_function_name = "example_entry_point"
}, NULL);
}
```

## Terminating background workers

Inside the callback `_Omni_deinit`, invoke `request_bgworker_termination` method of `omni_handle` type.

```c
void _Omni_deinit(const omni_handle *handle) {
  if (example_bgworker_handle != NULL) {
    handle->request_bgworker_termination(
      handle,
      example_bgworker_handle
  );
  }
}
```

## Examples

For an end-to-end implementation of Omni in extensions checkout [omni_httpd](https://github.com/omnigres/omnigres/blob/master/extensions/omni_httpd/omni_httpd.c),
[omni_tests](https://github.com/omnigres/omnigres/blob/master/extensions/omni/test/omni_test.c) & [pg_savior](https://github.com/viggy28/pg_savior/tree/port-to-omni)