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
#include <utils/jsonb.h>
#include <utils/timestamp.h>

#include <libgluepg_curl.h>
#include <libgluepg_yyjson.h>
#include <libpgaug.h>

#include "omni_containers.h"

PG_MODULE_MAGIC;

#define DEFAULT_SOCK "/var/run/docker.sock"

CURL *init_curl() {
  CURL *curl = curl_easy_init();
  char *sock = DEFAULT_SOCK;
  bool is_unix_sock = true;
  if (getenv("DOCKER_HOST")) {
    sock = getenv("DOCKER_HOST");
    if (strncmp(sock, "unix://", strlen("unix://")) == 0) {
      sock = sock + strlen("unix://");
    } else {
      is_unix_sock = false;
    }
  }
  curl_easy_setopt(curl, is_unix_sock ? CURLOPT_UNIX_SOCKET_PATH : CURLOPT_URL, sock);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, gluepg_curl_buffer_write);
  return curl;
}

/**
 * @brief Docker may send multiple progress JSONs, return the last one
 *
 * @param buf Docker response
 * @return yyjson_doc* last JSON
 */
static yyjson_doc *read_last_json(gluepg_curl_buffer *buf) {
  char *input = buf->data;
  uintptr_t size = buf->size;
  yyjson_doc *doc;
  while (true) {
    yyjson_doc *attempted_doc =
        yyjson_read_opts(input, size, YYJSON_READ_STOP_WHEN_DONE, &gluepg_yyjson_allocator, NULL);
    if (!attempted_doc) {
      return doc;
    }
    doc = attempted_doc;
    input += yyjson_doc_get_read_size(doc);
    size -= yyjson_doc_get_read_size(doc);
  }
}

/**
 * @brief Get Docker error message
 *
 * @param buf response
 * @return const char* error message
 */
static const char *get_docker_error(gluepg_curl_buffer *buf) {
  yyjson_doc *response = read_last_json(buf);
  yyjson_val *root = yyjson_doc_get_root(response);
  const char *message = yyjson_get_str(yyjson_obj_get(root, "message"));
  return message == NULL ? buf->data : message;
}

PG_FUNCTION_INFO_V1(docker_images_json);

Datum docker_images_json(PG_FUNCTION_ARGS) {
  gluepg_curl_init();

  CURL *curl = init_curl();
  gluepg_curl_buffer buf;
  gluepg_curl_buffer_init(&buf);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
  curl_easy_setopt(curl, CURLOPT_URL, "http://v1.41/images/json");
  curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  return DirectFunctionCall1(jsonb_in, CStringGetDatum(buf.data));
}

static char *normalize_docker_image_name(char *image) {
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

  const char *id;
  text *result;
  WITH_TEMP_MEMCXT {
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
      attach = psprintf("%s:%s", host_alias,
                        getenv("DOCKER_HOST_IP") ? getenv("DOCKER_HOST_IP") : "host-gateway");
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

    // Options object to pass extra options
    yyjson_val *options = NULL;
    if (!PG_ARGISNULL(6)) {
      Jsonb *jsonb = PG_GETARG_JSONB_P(6);
      char *json = JsonbToCString(NULL, &jsonb->root, VARSIZE(jsonb));
      yyjson_doc *doc =
          yyjson_read_opts(json, strlen(json), YYJSON_READ_NOFLAG, &gluepg_yyjson_allocator, NULL);
      if (doc) {
        options = yyjson_doc_get_root(doc);
      }
    }

    // Prepare container creation request
    // https://docs.docker.com/engine/api/v1.41/#tag/Container/operation/ContainerCreate
    yyjson_mut_doc *request = yyjson_mut_doc_new(&gluepg_yyjson_allocator);
    yyjson_mut_val *obj = yyjson_mut_obj(request);

    // Set options, if any
    if (options) {
      obj = yyjson_val_mut_copy(request, options);
    }

    yyjson_mut_doc_set_root(request, obj);
    yyjson_mut_obj_add_str(request, obj, "Image", normalized_image);
    yyjson_mut_val *env = yyjson_mut_obj_get(obj, "Env");
    if (!env) {
      env = yyjson_mut_arr(request);
      yyjson_mut_obj_add_val(request, obj, "Env", env);
    }
    yyjson_mut_val *host_config = yyjson_mut_obj_get(obj, "HostConfig");
    if (!host_config) {
      host_config = yyjson_mut_obj(request);
      yyjson_mut_obj_add_val(request, obj, "HostConfig", host_config);
    }
    yyjson_mut_val *extra_hosts = yyjson_mut_obj_get(obj, "ExtraHosts");
    if (!extra_hosts) {
      extra_hosts = yyjson_mut_arr(request);
      yyjson_mut_obj_add_val(request, host_config, "ExtraHosts", extra_hosts);
    }

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

    char *json =
        yyjson_mut_write_opts(request, YYJSON_WRITE_NOFLAG, &gluepg_yyjson_allocator, NULL, NULL);

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
          char *url = psprintf("http://v1.41/images/create?fromImage=%s&tag=latest", image_escaped);
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
                              MemoryContextStrdup(memory_context.old, message)));
          }
        } else {
          // Attempted, wasn't found
          ereport(ERROR, errmsg("Docker image not found"), errdetail("%s", image));
        }
        break;
        // Other error
      default:
        ereport(ERROR, errmsg("Can't create the container"),
                errdetail("Error code %ld: %s", http_code,
                          MemoryContextStrdup(memory_context.old, get_docker_error(&buf))));
      }
    } while (retry_creating);

    yyjson_doc *response = read_last_json(&buf);
    yyjson_val *root = yyjson_doc_get_root(response);

    id = yyjson_get_str(yyjson_obj_get(root, "Id"));

    if (start) {
      char *url = psprintf("http://v1.41/containers/%s/start", id);

      gluepg_curl_buffer_reset(&buf);
      curl_easy_setopt(curl, CURLOPT_URL, url);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");

      curl_easy_perform(curl);

      long http_code;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

      // How did it go?
      switch (http_code) {
      // Created
      case 204:
        // We're done
        break;
      default:
        ereport(ERROR, errmsg("Can't start the container"),
                errdetail("Error code %ld: %s", http_code,
                          MemoryContextStrdup(memory_context.old, get_docker_error(&buf))));
      }
    }

    if (wait) {
      char *url = psprintf("http://v1.41/containers/%s/wait", id);

      gluepg_curl_buffer_reset(&buf);
      curl_easy_setopt(curl, CURLOPT_URL, url);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");

      curl_easy_perform(curl);

      long http_code;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

      // How did it go?
      switch (http_code) {
      // Created
      case 200:
        // We're done
        break;
      default:
        ereport(ERROR, errmsg("Can't wait for the container"),
                errdetail("Error code %ld: %s", http_code,
                          MemoryContextStrdup(memory_context.old, get_docker_error(&buf))));
      }
    }

    curl_easy_cleanup(curl);
  }
  MEMCXT_FINALIZE { result = cstring_to_text(id); }

  PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(docker_container_inspect);

Datum docker_container_inspect(PG_FUNCTION_ARGS) {
  gluepg_curl_init();

  Datum result;
  gluepg_curl_buffer buf;

  WITH_TEMP_MEMCXT {

    // Container ID
    char *id = text_to_cstring(PG_GETARG_TEXT_PP(0));

    CURL *curl = init_curl();

    gluepg_curl_buffer_init(&buf);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);

    curl_easy_setopt(curl, CURLOPT_URL, psprintf("http://v1.41/containers/%s/json", id));
    curl_easy_perform(curl);

    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // How did it go?
    switch (http_code) {
    // Got it
    case 200:
      // We're done
      break;
    // Error
    default:
      ereport(ERROR, errmsg("Can't inspect the container"),
              errdetail("Error code %ld: %s", http_code,
                        MemoryContextStrdup(memory_context.old, get_docker_error(&buf))));
    }
    curl_easy_cleanup(curl);
  }
  MEMCXT_FINALIZE { result = DirectFunctionCall1(jsonb_in, CStringGetDatum(buf.data)); }

  return result;
}

text *docker_stream_to_text(gluepg_curl_buffer *buf) {
  // The actual output will be smaller, but this is the absolute
  // upper bound and we don't want to constantly re-allocate it to grow
  char *output = palloc0(buf->size);
  char *current = buf->data;
  long i = 0;
  // If there's any stream data at all
  if (buf->size >= 8) {
    // Keep reading frames
    while (true) {
      uint32_t sz = pg_ntoh32(*((uint32_t *)(current + 4)));
      memcpy(output + i, current + 8, sz);
      i += sz;
      current += 8 + sz;
      // until we run out of data
      if (current - buf->data == buf->size) {
        break;
      }
    }
  }

  return cstring_to_text_with_len(output, i);
}

PG_FUNCTION_INFO_V1(docker_container_logs);

Datum docker_container_logs(PG_FUNCTION_ARGS) {
  gluepg_curl_init();
  gluepg_curl_buffer buf;
  text *logs;
  WITH_TEMP_MEMCXT {

    // Container ID
    char *id = text_to_cstring(PG_GETARG_TEXT_PP(0));

    // Include stdout
    bool stdout = false;
    if (!PG_ARGISNULL(1)) {
      stdout = PG_GETARG_BOOL(1);
    }

    // Include stderr
    bool stderr = false;
    if (!PG_ARGISNULL(2)) {
      stdout = PG_GETARG_BOOL(2);
    }

    Timestamp epoch = SetEpochTimestamp();

    long since = 0;
    if (!PG_ARGISNULL(3)) {
      Timestamp ts = PG_GETARG_TIMESTAMP(3);
      since = (ts - epoch) / 1000000;
    }

    long until = 0;
    if (!PG_ARGISNULL(4)) {
      Timestamp ts = PG_GETARG_TIMESTAMP(4);
      until = (ts - epoch) / 1000000;
    }

    bool timestamps = false;
    if (!PG_ARGISNULL(5)) {
      timestamps = PG_GETARG_BOOL(5);
    }

    int tail = -1;
    if (!PG_ARGISNULL(6)) {
      tail = PG_GETARG_INT32(tail);
    }

    CURL *curl = init_curl();

    gluepg_curl_buffer_init(&buf);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);

    curl_easy_setopt(curl, CURLOPT_URL,
                     psprintf("http://v1.41/containers/%s/"
                              "logs?stdout=%s&stderr=%s&since=%ld&until=%ld&"
                              "timestamps=%s&tail=%s",
                              id, stdout ? "true" : "false", stderr ? "true" : "false", since,
                              until, timestamps ? "true" : "false",
                              tail == -1 ? "all" : psprintf("%d", tail)));
    curl_easy_perform(curl);

    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // How did it go?
    switch (http_code) {
    // Created
    case 200:
      // We're done
      break;
    // Error
    default:
      ereport(ERROR, errmsg("Can't get logs from the container"),
              errdetail("Error code %ld: %s", http_code,
                        MemoryContextStrdup(memory_context.old, get_docker_error(&buf))));
    }
    curl_easy_cleanup(curl);
  }
  MEMCXT_FINALIZE { logs = docker_stream_to_text(&buf); }
  PG_RETURN_TEXT_P(logs);
}

PG_FUNCTION_INFO_V1(docker_container_stop);

Datum docker_container_stop(PG_FUNCTION_ARGS) {
  gluepg_curl_init();

  Datum result;
  gluepg_curl_buffer buf;

  WITH_TEMP_MEMCXT {

    // Container ID
    char *id = text_to_cstring(PG_GETARG_TEXT_PP(0));

    CURL *curl = init_curl();

    gluepg_curl_buffer_init(&buf);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);

    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, psprintf("http://v1.41/containers/%s/stop", id));
    curl_easy_perform(curl);

    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // How did it go?
    switch (http_code) {
    // Got it
    case 204:
      // We're done
      break;
    case 304:
      ereport(WARNING, errmsg("Can't stop the container"), errdetail("Container already stopped"));
      break;
    case 404:
      ereport(ERROR, errmsg("Can't stop the container"), errdetail("No such container"));
      break;
    case 500:
      ereport(ERROR, errmsg("Can't stop the container"));
    }
    curl_easy_cleanup(curl);
  }
  MEMCXT_FINALIZE {}

  PG_RETURN_VOID();
}
