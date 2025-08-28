#include <any>
#include <map>

#include "oink.hpp"

#include <boost/interprocess/sync/interprocess_semaphore.hpp>

extern "C" {
#include <dlfcn.h>
}
#include <boost/container/string.hpp>
#include <boost/function.hpp>

#include <cppgres.hpp>

extern "C" {
PG_MODULE_MAGIC;
#include "commands/dbcommands.h"
#include <omni/omni_v0.h>
OMNI_MAGIC;

OMNI_MODULE_INFO(.name = "omni_worker", .version = EXT_VERSION,
                 .identity = "60d8114c-bce4-4e72-a278-694067a9ffb5");
}

const omni_handle *backend_handle = nullptr;

template <class... Ts> struct overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overload(Ts...) -> overload<Ts...>;

template <typename T>
concept message = oink::message<T> || std::same_as<T, std::any>;

template <message T> struct omni_pool_handler {
  using signature = void(const T *);
  omni_pool_handler(const std::string &library, const std::string &name, bool init, bool leader)
      : dl(dlopen(library.c_str(), RTLD_LAZY), dlclose),
        ptr(reinterpret_cast<void *(*)(const char *, std::size_t *hash, bool, bool)>(
            dlsym(dl.get(), "omni_worker_handler"))(name.c_str(), &hash, init, leader)),
        handler(reinterpret_cast<signature *>(ptr)), library_(library), name_(name) {}

  void operator()(const T &t) { handler(t); }

  const std::string &library() const noexcept { return library_; }
  const std::string &name() const noexcept { return name_; }

private:
  std::shared_ptr<void> dl;

public:
  void *ptr;
  boost::function<signature> handler;
  std::size_t hash;
  std::string library_;
  std::string name_;
};

std::string arena_name() {
  return cppgres::fmt::format("omni_worker_v1_{}_{}", cppgres::ffi_guard{::GetSystemIdentifier}(),
                              MyDatabaseId);
}
std::size_t arena_size() { return 1024 * 1024; }
std::string mq_name() {
  return cppgres::fmt::format("omni_worker_v1_{}_{}_mq",
                              cppgres::ffi_guard{::GetSystemIdentifier}(), MyDatabaseId);
}

namespace bc = boost::container;
namespace bip = boost::interprocess;

struct reload {
  static const char *name() { return "omni_worker:reload"; }
};

static bool reload_upon_commit = false;

postgres_function(reload_handlers, ([]() -> cppgres::value {
                    if (CALLED_AS_TRIGGER(cppgres::current_postgres_function::call_info().value().
                                          operator ::FunctionCallInfo())) {
                      reload_upon_commit = true;

                      return cppgres::value(cppgres::nullable_datum(0),
                                            cppgres::type{.oid = TRIGGEROID});
                    }
                    throw std::runtime_error("must be called as a trigger");
                  }));

static void worker(cppgres::datum main, bool leader) {
  cppgres::current_background_worker bgw;

  bgw.connect(cppgres::from_nullable_datum<cppgres::oid>(cppgres::nullable_datum(main), OIDOID));
  bgw.unblock_signals();

  std::map<std::uint64_t, omni_pool_handler<std::any>> handlers;
  struct handler {
    std::string library;
    std::string name;
  };
  auto do_reload = [&]() {
    cppgres::transaction tx(true);
    cppgres::spi_executor spi;
    decltype(handlers) new_handlers;
    auto have_handler = [&handlers](auto library, auto name) {
      for (auto &[_, h] : handlers) {
        if (h.library() == library && h.name() == name) {
          return true;
        }
      }
      return false;
    };
    for (const auto &h : spi.query<handler>("select library, name from omni_worker.handlers")) {
      bool init = !have_handler(h.library, h.name);
      omni_pool_handler<std::any> hndl(h.library, h.name, init, leader);
      new_handlers.insert({hndl.hash, std::move(hndl)});
    }
    handlers.swap(new_handlers);
  };
  do_reload();

  oink::arena arena(arena_name().c_str(), arena_size());
  oink::receiver rcvr(arena, mq_name().c_str(), 8192);
  cppgres::worker worker;

  std::thread receiver([&]() {
    while (true) {
      rcvr.receive<reload>(
          overload{[&](reload &) {
                     cppgres::report(LOG, "Reloading omni_worker handlers");
                     worker.post(do_reload).wait();
                   },
                   [&](oink::endpoint::msg &msg) {
                     auto it = handlers.find(msg.hash);
                     if (it != handlers.end()) {
                       worker
                           .post([&]() {
                             reinterpret_cast<void (*)(const void *)>(it->second.ptr)(
                                 static_cast<char *>(arena.get_address()) + msg.offset);
                           })
                           .wait();
                     }
                   }});
    }
  });

  worker.run();
}

extern "C" {

void omni_worker_bgw(::Datum main) {
  cppgres::exception_guard{worker}(cppgres::datum(main), false);
}
void omni_worker_bgw_leader(::Datum main) {
  cppgres::exception_guard{worker}(cppgres::datum(main), true);
}

void _Omni_init(const omni_handle *handle) {
  cppgres::exception_guard([&]() {
    backend_handle = handle;
    bool worker_bgw_found;
    handle->allocate_shmem(
        handle, cppgres::fmt::format("omni_pool_worker_{}", MyDatabaseId).c_str(), sizeof(void *),
        [](const omni_handle *handle, void *ptr, void *arg, bool allocated) {
          if (allocated) {
            int default_num_workers = 2;
#ifdef _SC_NPROCESSORS_ONLN
            default_num_workers = sysconf(_SC_NPROCESSORS_ONLN);
#endif
            omni_guc_variable guc_num_workers = {
                .name = "omni_worker.workers",
                .long_desc = "Number of omni_worker workers",
                .type = PGC_INT,
                .typed = {.int_val = {.boot_value = default_num_workers,
                                      .min_value = 0,
                                      .max_value = INT_MAX}},
                .context = PGC_SIGHUP};
            handle->declare_guc_variable(handle, &guc_num_workers);

            for (int i = 0; i < *guc_num_workers.typed.int_val.value; i++) {
              auto bgw =
                  cppgres::background_worker()
                      .name(cppgres::fmt::format(
                          "omni_worker #{} [{}]", i,
                          cppgres::ffi_guard{::get_database_name}(MyDatabaseId)))
                      .type("omni_worker")
                      .library_name(handle->get_library_name(handle))
                      .function_name(i == 0 ? "omni_worker_bgw_leader" : "omni_worker_bgw")
                      .main_arg(cppgres::datum_conversion<cppgres::oid>::into_datum(MyDatabaseId))
                      .flags(BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION)
                      .notify_pid(MyProcPid)
                      .start_time(BgWorkerStart_RecoveryFinished);
              omni_bgworker_handle bgw_handle;
              handle->request_bgworker_start(handle, bgw, &bgw_handle,
                                             {.timing = omni_timing_after_commit});
            }
          }
        },
        nullptr, &worker_bgw_found);
    omni_hook txn_hook = {
        .type = omni_hook_xact_callback,
        .fn = {.xact_callback =
                   [](omni_hook_handle *handle, XactEvent event) {
                     if (event == XACT_EVENT_COMMIT && reload_upon_commit) {
                       oink::arena arena(arena_name().c_str(), arena_size());
                       oink::sender snd(arena, mq_name().c_str(), 8192);
                       snd.send<reload>();
                       reload_upon_commit = false;
                     }
                   }},
        .name = "omni_worker transaction hook",
    };
    handle->register_hook(handle, &txn_hook);
  })();
}
}
