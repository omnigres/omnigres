## Requirements

`omni_httpd` requires `omni` extension to be preloaded. To achieve this, execute the following
shell command to install `omni`:

```shell
curl -s https://raw.githubusercontent.com/omnigres/omnigres/master/download-omnigres-extension.sh \
| bash -s install omni 0.1.3
```

After this, you need to add `omni--0.1.3.so` from the installation to `postgresql.conf`'s `shared_preloaded_libraries`:

```ini
shared_preloaded_libraries = 'omni--0.1.3'
```

Subsequently, this instance of Postgres needs to be restarted.

Following this, you can proceed with the installation of `omni_httpd`:

## Extension Installation

```shell
curl -s https://raw.githubusercontent.com/omnigres/omnigres/master/download-omnigres-extension.sh \
| bash -s install omni_httpd 0.1.2
```

It can be then installed as an extension:

```postgresql
select *
from
    omni_manifest.install(
            'omni_httpd=0.1.2#omni_types=0.1.0,omni_http=0.1.0'
                ::text::omni_manifest.artifact[]);
```

!!! tip

    The above instruction is provided by the shell script above. It is only provided for reference
    purposes in this section.