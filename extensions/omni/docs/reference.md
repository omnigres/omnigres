Review [omni_v0.h](https://github.com/omnigres/omnigres/blob/master/omni/omni/omni_v0.h) for the declaration of types such as `omni_handle`, `omni_guc_variable`, `OMNI_MAGIC`, `_Omni_init` etc.

## Useful views:

When `omni` is created as an extension, it creates additional database objects such as views that can provide useful
information.

`omni.shmem_allocations`:

|         Column | Type      | Description                                     |
|---------------:|-----------|-------------------------------------------------|
|       **name** | text      | Name of the allocation                          |
|  **module_id** | int       | Id of the module that allocated                 |
|       **size** | int       | The amount of shared memory allocated           |
| **refcounter** | timestamp | Number of times a same allocation is referenced |

`omni.hooks`:

|        Column | Type | Description                                    |
|--------------:|------|------------------------------------------------|
|      **hook** | text | Type of the hook                               |
|      **name** | text | Name of the hook                               |
| **module_id** | int  | Id of the module that created                  |
|       **pos** | int  | Position of occurrence for the particular hook |

`omni.modules`:

|             Column | Type | Description                                            |
|-------------------:|------|--------------------------------------------------------|
|             **id** | int  | Id of the module that is loaded                        |
|           **path** | text | Path on the filesystem where the module is loaded from |
| **omni_interface** | int  | Version of the omni interface                          |