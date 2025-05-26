# Credential Management

??? tip "`omni_credentials` is a templated extension"

    `omni_credentials` is a templated extension. This means that by installing it, a default copy of it is instantiated into extension's schema. However, you can replicate it into any other schema, tune some of the parameters and make the database own the objects (as opposed to the extension itself):

     ```postgresql 
     select omni_credentials.instantiate(
        [schema => 'omni_credentials'],    
        [env_var => 'OMNI_CREDENTIALS_MASTER_PASSWORD'])
     ```

    This allows you to have multiple independent credential systems, even if
    using different versions of `omni_credentials`. 

## Core Architecture

The central object of interest is the `credentials` view (instantiated into `omni_credentials` schema by default), it
contains `name` and `value` columns that represent credential name and value. To help with producing a more unified,
shareable credential ecosystem, we add few more columns

|      **Name** | Type            | Description                                                                                                                                                                                                                   |
|--------------:|-----------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|      **name** | text            | An identifier for the credential.                                                                                                                                                                                             |
|     **value** | bytea           | Credential data (such as an API key, password, or token)                                                                                                                                                                      |
|      **kind** | credential_kind | An enumerated type that categorizes the credential. Common values might include `api_key`, `api_secret`, `password`, etc., with a default of `credential` for generic cases.                                                  |
| **principal** | regrole         | Specifies the authenticated entity (the principal) for whom the credential is intended. Uses Postgres rolese, defaulting to the current user, ensuring the credential is tied to an identity.                                 |
|     **scope** | jsonb           | A JSON object defining the domain or resource constraints where the credential is valid. For example, `{ "all": true }` (default) indicates a wildcard scope, while more structured scopes can specify domains or conditions. |

You can simply query and update it as you see fit. Behind the scene, it will propage changes as necessary.

## Encryption & Access Control

The `credentials` view is based on the `encrypted_credentials` view that has `value` encrypted and is a
Row-Level Security-enabled relation. The default policy on it requires current user to have a grant for the
role of `principal` (`encrypted_credentials_principal` policy). Additional policies can be provisioned.

## Credential Encryption

By default, all credentials are stored into the _encryped credentials store_. The encryption key is derived from the
`OMNI_CREDENTIALS_MASTER_PASSWORD` environment variable[^env].

The store keeps the data in `encrypted_credentials` table.

## File Store

File stores can be instantiated using any of the virtual file systems available through omni_vfs.

In development mode, it is practical to store encrypted files in the repository
(conceptually similar to what Ruby on Rails [does](https://guides.rubyonrails.org/security.html#custom-credentials)).

In order to use one, a file store must be instantiated:

```postgresql
select omni_credentials.instantiate_file_store(omni_vfs.local_fs('/path/to/dir'), 'creds.json', 'schema');
```

It will import any available records in this file into the encrypted store,
and export what's missing in it from the table. After that, every time the credentials are
updated and committed, the file will be updated.

To reload the credentials from the file (for example, if a new version of the code was pulled),
invoke `credential_file_store_reload(filename)`. All registered file stores are listed in the
`credential_file_stores` table, which now stores the VFS type and info for each file store.

This approach supports any VFS backend (local, table, remote, etc.) and is extensible for future types.

[^env]:

     This is fine for development environment but may be limited beyond it. In staging and production, direct use of encrypted credentials or integrated stores is recommended.
