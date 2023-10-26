# Usage

To make your extension use the interface, include the header file and declare magic signature:

```c
#include <dynpgext.h>

DYNPGEXT_MAGIC;
```

### Allocating shared memory

You can request shared memory in `_Dynpgext_init` callback you can define:

```c linenums="1" title="Shared memory allocation"
void _Dynpgext_init(const dynpgext_handle *handle) {
      handle->allocate_shmem(handle, /* name */ "my_ext:0.1:value", /* size */ 1024,
                         /* callback */ cb, /* callback payload */ NULL,
                         /* flags */ DYNPGEXT_SCOPE_DATABASE_LOCAL /* or DYNPGEXT_SCOPE_GLOBAL */);
}
```

A particularly useful feature is the ability to scope the allocation per
database so that every database provisioned will get its own allocation of the
specified size.

Once allocated, the pointer to allocated memory can be found using `dynpgext_lookup_shmem(name)`:

```c
void *ptr = dynpgext_lookup_shmem("my_ext:0.1:value");
```

### Registering background worker

You can register a background worker to be started in `_Dynpgext_init:

```c linenums="1" title="Background worker registration"
BackgroundWorker bgw = {.bgw_name = "worker",
                          .bgw_type = "worker",
                          .bgw_function_name = "worker",
                          .bgw_flags = BGWORKER_SHMEM_ACCESS,
                          .bgw_start_time = BgWorkerStart_RecoveryFinished};
strncpy(bgw.bgw_library_name, handle->library_name, BGW_MAXLEN);
handle->register_bgworker(handle, &bgw, NULL, NULL,
                           DYNPGEXT_REGISTER_BGWORKER_NOTIFY | DYNPGEXT_SCOPE_GLOBAL /* can also take DYNPGEXT_SCOPE_DATABASE_LOCAL */);
```

Just like shared memory allocations, background workers can be either global or provisioned per database.

!!! tip "Caveat: bgw_restart_time is always BGW_NEVER_RESTART"

    Since omni_ext manages the startup of the background workers, 
    `BackgroundWorker.bgw_restart_time` value is ignored and is always 
    effectively set to `BGW_NEVER_RESTART` so that Postgres never attempts
    to restart them itself.
