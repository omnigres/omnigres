/**
 * @file apex.h
 * @brief APEX – APIs for Postgres Extensions
 *
 * APEX provides a standardized framework for Postgres extensions to expose and consume
 * C APIs across extension boundaries using a publisher-consumer pattern with automatic
 * discovery and lifecycle management.
 *
 * @section overview Overview
 *
 * APEX enables extensions to:
 * - Share APIs across extension boundaries
 * - Handle API lifecycle events (available/unavailable) through callbacks
 *
 * @section usage Usage Pattern
 *
 * **API Provider (Owner) Extension:**
 * 1. Define your API structure type
 * 2. Define `apex_api` macro with a unique string identifier
 * 3. Define `apex_api_type` macro with your API structure type
 * 4. Define `apex_api_owner` macro to enable provider functionality
 * 5. Include this header to generate provider code
 * 6. Call the generated initializer function with your API instance
 *
 * **API Consumer Extension:**
 * 1. Define `apex_api` and `apex_api_type` macros (matching the provider)
 * 2. Include this header to generate consumer code
 * 3. Register a consumer with callbacks using the generated finder function
 *
 * @section implementation Implementation Details
 *
 * The framework uses Postgres rendezvous variables as a shared registry where:
 * - API providers register their implementations
 * - API consumers register for notifications
 * - Automatic callback invocation occurs when APIs become available/unavailable
 *
 * All generated types and functions are prefixed with the API type name to avoid
 * symbol collisions when multiple APIs are used within the same extension.
 *
 * @section requirements Requirements
 *
 * - API types must be trivial (C-compatible, no constructors/destructors in C++)
 * - API types must have non-zero size
 * - Each API must have a unique string identifier across the system
 *
 * @section memory_management Memory Management
 *
 * - API wrappers are allocated in TopMemoryContext for cross-extension persistence
 * - Consumer callback management is handled automatically
 * - No explicit cleanup is required due to Postgres process lifecycle
 *
 * @version 0.1.0
 */
// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <utils/memutils.h>

#ifndef apex_api
#error "Define `apex_api` to name the API point"
#endif

#ifndef apex_api_type
#error "Define `apex_api_type` to name the API type"
#endif

#ifdef __cplusplus
#include <type_traits>
static_assert(std::is_trivial_v<apex_api_type>);
static_assert(sizeof(apex_api_type) > 0);
#endif

#define _apex__cat__(a, ...) _apex__primitive__cat__(a, __VA_ARGS__)
#define _apex__primitive__cat__(a, ...) a##__VA_ARGS__

#define apex_api_wrapper_type _apex__cat__(apex_api_type, _apex_t)
#define apex_api_wrapper_consumer_type _apex__cat__(apex_api_type, _apex_consumer_t)
#define apex_api_wrapper_consumer_callback_type                                                    \
  _apex__cat__(apex_api_type, _apex_consumer_callback_t)
#define apex_api_finder _apex__cat__(apex_find_, apex_api_type)
#define apex_api_initializer _apex__cat__(apex_initialize_, apex_api_type)

#define _apex_api_var "APEX0" apex_api

#ifdef __cplusplus
extern "C" {
#endif

struct apex_api_wrapper_type;

struct apex_api_wrapper_consumer_type;
typedef void(apex_api_wrapper_consumer_callback_type)(apex_api_wrapper_type *,
                                                      apex_api_wrapper_consumer_type *consumer);

typedef struct apex_api_wrapper_consumer_type {
  apex_api_wrapper_consumer_callback_type *on_available;
  apex_api_wrapper_consumer_callback_type *on_unavailable;
  apex_api_wrapper_consumer_type *next;
  void *ctx;
} apex_api_wrapper_consumer_type;

typedef struct apex_api_wrapper_type {
  const char *name;
  apex_api_type *api;
  apex_api_wrapper_consumer_type *pending_consumers;
} apex_api_wrapper_type;

static void apex_api_finder(apex_api_wrapper_consumer_type *new_consumer) {
  if (new_consumer == NULL) {
    return;
  }
  apex_api_wrapper_type **var = (apex_api_wrapper_type **)find_rendezvous_variable(_apex_api_var);
  if (*var == NULL) {
    *var = (apex_api_wrapper_type *)MemoryContextAllocZero(TopMemoryContext,
                                                           sizeof(apex_api_wrapper_type));
  }
  apex_api_wrapper_consumer_type **consumer = &(*var)->pending_consumers;
  while (*consumer != NULL) {
    consumer = &((*consumer)->next);
  }
  *consumer = new_consumer;
  if ((*var)->api != NULL) {
    if (new_consumer->on_available != NULL) {
      new_consumer->on_available(*var, new_consumer);
    }
  }
}

#ifdef apex_api_owner
static void apex_api_initializer(apex_api_type *api) {
  apex_api_wrapper_type **var = (apex_api_wrapper_type **)find_rendezvous_variable(_apex_api_var);
  if (*var == NULL) {
    *var = (apex_api_wrapper_type *)MemoryContextAllocZero(TopMemoryContext,
                                                           sizeof(apex_api_wrapper_type));
  }
  (*var)->api = api;
  apex_api_wrapper_consumer_type *consumer = (*var)->pending_consumers;
  while (consumer != NULL) {
    consumer->on_available(*var, consumer);
    consumer = consumer->next;
  }
}
#endif

#ifdef __cplusplus
}
#endif

#undef apex_api_initializer
#undef apex_api_finder
#undef apex_api_wrapper_consumer_type
#undef apex_api_wrapper_consumer_callback_type
#undef apex_api_wrapper_type
#undef apex_api

#undef apex_api_owner

#undef _apex__cat__
#undef _apex__primitive__cat
