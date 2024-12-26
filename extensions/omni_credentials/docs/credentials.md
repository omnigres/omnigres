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
only contains `name` and `value` columns that represent credential name
and value.

You can simply query and update it as you see fit. Behind the scene, it will propage changes as necessary.

## Credential Encryption

By default, all credentials are stored into the _encryped credentials store_. The encryption key is derived from the
`OMNI_CREDENTIALS_MASTER_PASSWORD` environment variable[^env].

The store keeps the data in `encrypted_credentials` table.

## File Store

In development mode, it is practical to store encrypted files in the repository
(conceptually similar to what Ruby on Rails [does](https://guides.rubyonrails.org/security.html#custom-credentials)).

In order to use one, a file store must be instantiated:

```postgresql
select omni_credentials.instantiate_file_store(filename, [schema])
```

It will import any available records in this file into the encrypted store,
and export what's missing in it from the table. After that, every time the credentials are
updated and commited, the file will be updated.

To reload the credentials from the file (for example, if a new version of the code was pulled),
invoke `credential_file_store_reload(filename)`. All registered file stores are listed in the
`credential_file_stores` table.

[^env]:

     This is fine for development environment but may be limited beyond it. In staging and production, direct use of encrypted credentials or future integrated stores is recommended.