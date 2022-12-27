#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <commands/dbcommands.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <utils/builtins.h>
#include <utils/fmgrprotos.h>
#include <utils/guc.h>

#include <libgluepg_curl.h>
#include <libgluepg_yyjson.h>

#include "omni_containers.h"

PG_MODULE_MAGIC;

const char *get_docker_error(gluepg_curl_buffer *buf) {
  yyjson_doc *response = yyjson_read_opts(
      buf->data, buf->size, YYJSON_READ_NOFLAG, &gluepg_yyjson_allocator, NULL);
  yyjson_val *root = yyjson_doc_get_root(response);
  const char *message = yyjson_get_str(yyjson_obj_get(root, "message"));
  return message == NULL ? buf->data : message;
}

CURL *init_curl() {
  CURL *curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, "/var/run/docker.sock");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, gluepg_curl_buffer_write);
  return curl;
}

PG_FUNCTION_INFO_V1(docker_images_json);

Datum docker_images_json(PG_FUNCTION_ARGS) {
  gluepg_curl_init();

#ifdef DEBUG
  if (test_fixtures) {
    char *filename = psprintf("%s/%s/images.json",
                              getenv("EXTENSION_SOURCE_DIR"), test_fixtures);
    FILE *f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);
    char *content = palloc0(file_size + 1);
    fread(content, file_size, 1, f);
    fclose(f);
    return DirectFunctionCall1(jsonb_in, CStringGetDatum(content));
  } else
#endif
  {
    CURL *curl = init_curl();
    gluepg_curl_buffer buf;
    gluepg_curl_buffer_init(&buf);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_URL, "http://v1.41/images/json");
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return DirectFunctionCall1(jsonb_in, CStringGetDatum(buf.data));
  }
}

char *normalize_docker_image_name(char *image) {
  char *first_slash = strchr(image, '/');
  char *last_slash = strrchr(image, '/');
  if (first_slash == NULL) {
    // No slashes, it's a library
    return psprintf("docker.io/library/%s", image);
  }
  if (first_slash == last_slash) {
    // One slash, it's on docker.io
    return psprintf("docker.io/%s", image);
  }
  // More than one slash, it's a normalized URl
  return image;
}

PG_FUNCTION_INFO_V1(docker_container_create);

Datum docker_container_create(PG_FUNCTION_ARGS) {
  gluepg_curl_init();
  // Create a new memory context to avoid over-complicating
  // the code with releasing memory. Release it at once instead.
  MemoryContext context = AllocSetContextCreate(
      CurrentMemoryContext, "docker_container_create", ALLOCSET_DEFAULT_SIZES);
  MemoryContext old_context = MemoryContextSwitchTo(context);
  // Docker image
  char *image = text_to_cstring(PG_GETARG_TEXT_PP(0));
  char *normalized_image = normalize_docker_image_name(image);

  // Command (if not null)
  char *cmd = NULL;
  if (!PG_ARGISNULL(1)) {
    text *cmd_text = PG_GETARG_TEXT_PP(1);
    cmd = text_to_cstring(cmd_text);
  }

  // Attach local PostgreSQL instance as (if not null)
  char *attach = NULL;
  char *host_alias = NULL;
  if (!PG_ARGISNULL(2)) {
    text *attach_text = PG_GETARG_TEXT_PP(2);

    host_alias = text_to_cstring(attach_text);
    // This will alias docker's host (where PostgreSQL is) to `attach`
    // argument's value.
    attach = psprintf("%s:host-gateway", host_alias);
  }

  // Should it be started?
  bool start = true;
  if (!PG_ARGISNULL(3)) {
    start = PG_GETARG_BOOL(3);
  }

  // Should we wait until the container exits?
  bool wait = false;
  if (!PG_ARGISNULL(4)) {
    wait = PG_GETARG_BOOL(4);
  }

  // Should we attempt to pull the image if doesn't exist?
  bool pull = false;
  if (!PG_ARGISNULL(5)) {
    pull = PG_GETARG_BOOL(5);
  }

  // Prepare container creation request
  // https://docs.docker.com/engine/api/v1.41/#tag/Container/operation/ContainerCreate
  yyjson_mut_doc *request = yyjson_mut_doc_new(&gluepg_yyjson_allocator);
  yyjson_mut_val *obj = yyjson_mut_obj(request);
  yyjson_mut_doc_set_root(request, obj);
  yyjson_mut_obj_add_str(request, obj, "Image", normalized_image);
  yyjson_mut_val *env = yyjson_mut_arr(request);
  yyjson_mut_val *host_config = yyjson_mut_obj(request);
  yyjson_mut_val *extra_hosts = yyjson_mut_arr(request);
  yyjson_mut_obj_add_val(request, host_config, "ExtraHosts", extra_hosts);
  yyjson_mut_obj_add_val(request, obj, "Env", env);
  yyjson_mut_obj_add_val(request, obj, "HostConfig", host_config);

  if (attach) {
    yyjson_mut_arr_add_str(request, extra_hosts, attach);

    // Prepare environment variables to facilitate attachment to the database

    char *envvar;

    char *username = GetUserNameFromId(GetUserId(), false);
    envvar = psprintf("PGUSER=%s", username);
    yyjson_mut_arr_add_strcpy(request, env, envvar);

    envvar = psprintf("PGHOST=%s", host_alias);
    yyjson_mut_arr_add_strcpy(request, env, envvar);

    char *dbname = get_database_name(MyDatabaseId);
    envvar = psprintf("PGDATABASE=%s", dbname);
    yyjson_mut_arr_add_strcpy(request, env, envvar);

    const char *port = GetConfigOption("port", true, false);
    envvar = psprintf("PGPORT=%s", port);
    yyjson_mut_arr_add_strcpy(request, env, envvar);
  }

  if (cmd) {
    yyjson_mut_val *cmd_arr = yyjson_mut_arr(request);
    yyjson_mut_arr_add_str(request, cmd_arr, "sh");
    yyjson_mut_arr_add_str(request, cmd_arr, "-c");
    yyjson_mut_arr_add_str(request, cmd_arr, cmd);
    yyjson_mut_obj_add_val(request, obj, "Cmd", cmd_arr);
  }

  char *json = yyjson_mut_write_opts(request, YYJSON_WRITE_NOFLAG,
                                     &gluepg_yyjson_allocator, NULL, NULL);
  yyjson_mut_doc_free(request);

  CURL *curl = init_curl();
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  gluepg_curl_buffer buf;
  gluepg_curl_buffer_init(&buf);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);

  bool retry_creating;
  bool attempted_to_pull = false;
  do {
    retry_creating = false;
    // Attempt to create the container
    curl_easy_setopt(curl, CURLOPT_URL, "http://v1.41/containers/create");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_perform(curl);

    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // How did it go?
    switch (http_code) {
    // Created
    case 201:
      // We're done
      break;
    // Not found
    case 404:
      if (pull && !attempted_to_pull) {
        // Try to pull the image if allowed to
        ereport(NOTICE, errmsg("Pulling image %s", image));
        char *image_escaped = curl_easy_escape(curl, normalized_image, 0);
        char *url =
            psprintf("http://v1.41/images/create?fromImage=%s", image_escaped);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
        gluepg_curl_buffer_reset(&buf);

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);
        curl_easy_perform(curl);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        long http_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        attempted_to_pull = true;
        if (http_code == 200) {
          // Try to create the container again
          retry_creating = true;
        } else {
          const char *message = get_docker_error(&buf);
          ereport(ERROR, errmsg("Failed to pull image %s", image),
                  errdetail("Code %ld: %s", http_code,
                            MemoryContextStrdup(old_context, message)));
        }
      } else {
        // Attempt, wasn't found
        ereport(ERROR, errmsg("Docker image not found"),
                errdetail("%s", image));
      }
      break;
      // Other error
    default: {
      const char *message = get_docker_error(&buf);
      ereport(ERROR, errmsg("Can't create the container"),
              errdetail("Error code %ld: %s", http_code,
                        MemoryContextStrdup(old_context, message)));
    }
    }
  } while (retry_creating);

  yyjson_doc *response = yyjson_read_opts(
      buf.data, buf.size, YYJSON_READ_NOFLAG, &gluepg_yyjson_allocator, NULL);
  yyjson_val *root = yyjson_doc_get_root(response);
  const char *id = yyjson_get_str(yyjson_obj_get(root, "Id"));

  if (start) {
    char *url = psprintf("http://v1.41/containers/%s/start", id);

    gluepg_curl_buffer_reset(&buf);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");

    curl_easy_perform(curl);
  }

  if (wait) {
    char *url = psprintf("http://v1.41/containers/%s/wait", id);

    gluepg_curl_buffer_reset(&buf);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");

    curl_easy_perform(curl);
  }

  curl_easy_cleanup(curl);

  MemoryContextSwitchTo(old_context);
  MemoryContextDelete(context);
  text *id_text = cstring_to_text(id);
  PG_RETURN_TEXT_P(id_text);
}