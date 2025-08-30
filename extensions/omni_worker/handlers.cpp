#include <boost/container/string.hpp>
#include <oink.hpp>

extern "C" {
#include <omni/omni_v0.h>
}

#include <cppgres.hpp>

#include "omni_worker.hpp"

#include <csignal>

namespace bc = boost::container;
namespace bip = boost::interprocess;

struct sql_message {
  bc::basic_string<char, std::char_traits<char>, oink::allocator<char>> stmt;
  bip::interprocess_semaphore ready;
  volatile int rc;
  static const char *name() { return "sql_message"; }

  sql_message(oink::arena &arena, std::string_view &sql_stmt)
      : stmt(sql_stmt, arena.get_allocator<decltype(stmt)>()), ready(0), rc(0) {}

  sql_message(sql_message &&other) : stmt(std::move(other.stmt)), ready(0) {}
};

void sql_handler(sql_message *msg) {
  cppgres::exception_guard([msg]() {
    cppgres::transaction tx;

    cppgres::spi_executor spi;
    try {
      spi.execute(cppgres::fmt::format("{}", msg->stmt.c_str()));
    } catch (std::exception &e) {
      tx.rollback();
      cppgres::report(WARNING, "%s", e.what());
      msg->rc = -1;
    }
    msg->ready.post();
  })();
}

extern const omni_handle *backend_handle;

static void init_sql(bool leader) {
  if (!leader) {
    return;
  }
  bool found;
  backend_handle->allocate_shmem(
      backend_handle, cppgres::fmt::format("omni_pool<{}>:sql(leader)", MyDatabaseId).c_str(),
      sizeof(bool),
      [](const omni_handle *handle, void *ptr, void *arg, bool allocated) {
        auto done = reinterpret_cast<bool *>(ptr);
        if (allocated) {
          *done = false;
        }
        if (!*done) {
          cppgres::spi_executor spi;
          cppgres::internal_subtransaction tx;
          try {
            auto stmts = spi.query<std::string>(
                "select stmt from omni_worker.sql_autostart_stmt order by position asc");
            for (auto &stmt : stmts) {
              cppgres::report(LOG, "Autostart statement: %s", stmt.c_str());
              try {
                auto res = spi.execute(stmt);
              } catch (std::exception &e) {
                cppgres::report(WARNING, "Statement error %s: %s", stmt.c_str(), e.what());
              }
            }
            *done = true;
          } catch (std::exception &e) {
            cppgres::report(WARNING, "Autostart error: %s", e.what());
          }
        }
      },
      nullptr, &found);
}

extern "C" void *omni_worker_handler(const char *name, std::size_t *hash, bool init, bool leader) {
  if (std::string_view(name) == "sql") {
    if (hash != nullptr) {
      *hash = std::hash<std::string_view>{}(sql_message::name());
    }
    if (init) {
      init_sql(leader);
    }
    return reinterpret_cast<void *>(sql_handler);
  }
  return nullptr;
}

class scoped_interrupt_handler {
public:
  scoped_interrupt_handler(int signal, void (*new_handler)(int))
      : signal_{signal}, previous_handler_{std::signal(signal, new_handler)} {}

  // Deleted copy semantics
  scoped_interrupt_handler(const scoped_interrupt_handler &) = delete;
  scoped_interrupt_handler &operator=(const scoped_interrupt_handler &) = delete;

  // Move semantics
  scoped_interrupt_handler(scoped_interrupt_handler &&other) noexcept
      : signal_{other.signal_}, previous_handler_{other.previous_handler_} {
    other.signal_ = 0;
    other.previous_handler_ = SIG_DFL;
  }

  scoped_interrupt_handler &operator=(scoped_interrupt_handler &&other) noexcept {
    if (this != &other) {
      restore();
      signal_ = other.signal_;
      previous_handler_ = other.previous_handler_;
      other.signal_ = 0;
      other.previous_handler_ = SIG_DFL;
    }
    return *this;
  }

  ~scoped_interrupt_handler() { restore(); }

private:
  void restore() {
    if (signal_ != 0 && previous_handler_ != SIG_ERR) {
      std::signal(signal_, previous_handler_);
    }
  }

  int signal_;
  void (*previous_handler_)(int);
};

postgres_function(sql, ([](std::string_view s,
                           std::optional<std::int32_t> wait_ms) -> std::optional<bool> {
                    oink::arena arena(arena_name().c_str(), arena_size());
                    oink::sender snd(arena, mq_name().c_str(), 8192);
                    auto msg = snd.send<sql_message>(arena, s);
                    if (wait_ms.has_value()) {
                      static bip::interprocess_semaphore *semaphore = nullptr;
// This doesn't really work but basically if we're using a spin semaphore,
// we're safe to use this in a signal handler:
// static_assert(std::same_as<bip::interprocess_semaphore::internal_sem_t,
// bip::ipcdetail::spin_semaphore>);
// so for now we approximate this with defines
#if defined(BOOST_INTERPROCESS_SEMAPHORE_USE_POSIX)
#warning                                                                                           \
    "POSIX semaphores may present a problem, see: https://github.com/boostorg/interprocess/issues/262"
#endif
                      semaphore = &msg->ready;
                      scoped_interrupt_handler ih{SIGINT, [](int) { semaphore->post(); }};
                      if (msg->ready.timed_wait(std::chrono::system_clock::now() +
                                                std::chrono::milliseconds(wait_ms.value()))) {

                        return msg->rc == 0;
                      } else {
                        return std::nullopt;
                      }
                    } else {
                      return std::nullopt;
                    }
                  }));
