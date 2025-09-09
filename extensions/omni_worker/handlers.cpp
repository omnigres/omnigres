#include <boost/asio.hpp>
#include <boost/container/string.hpp>
#include <oink.hpp>
#include <type_traits>

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

bool sql_handler(sql_message *msg) {
  return cppgres::exception_guard([msg]() {
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
    return true;
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

struct timer_message {

  struct after {
    std::int64_t milliseconds;
    cppgres::oid function_oid;
    cppgres::role role;
    volatile std::int64_t id;
    after(std::int64_t ms, cppgres::oid oid, cppgres::role role_)
        : milliseconds(ms), function_oid(oid), role(role_), id(0) {}
    after(std::int64_t ms, cppgres::oid oid, cppgres::role role_, std::int64_t id_val)
        : milliseconds(ms), function_oid(oid), role(role_), id(id_val) {}
  };

  struct execute {
    std::int64_t id;
    cppgres::oid function_oid;
    cppgres::role role;
    execute(std::int64_t id_, cppgres::oid oid, cppgres::role role_)
        : id(id_), function_oid(oid), role(role_) {}
  };

  struct cancel {
    std::int64_t id;
    cppgres::role requester;
    volatile bool cancelled;
    cancel(std::int64_t id_, cppgres::role requester_)
        : id(id_), requester(requester_), cancelled(false) {}
  };

  bip::interprocess_semaphore ready;
  std::variant<after, execute, cancel> msg;

  static const char *name() { return "timer_message"; }

  template <typename T>
  timer_message(std::in_place_type_t<T> ip, auto &&...args)
      : ready(0), msg(ip, std::forward<decltype(args)>(args)...) {}

  timer_message(timer_message &&other) : ready(0), msg(std::move(other.msg)) {}
};

struct timer_runner {

  timer_runner()
      : work_guard_(boost::asio::make_work_guard(io_context_)),
        arena(arena_name().c_str(), arena_size()), snd(arena, mq_name().c_str(), 8192) {
    worker_thread_ = std::thread([this]() { io_context_.run(); });
  }

  ~timer_runner() {
    work_guard_.reset(); // Allow io_context to finish
    post_task([](boost::asio::io_context &ctx) { ctx.stop(); });
    if (worker_thread_.joinable()) {
      worker_thread_.join();
    }
  }

  template <typename Task, typename... Args>
  std::future<std::invoke_result_t<Task, timer_runner &, Args...>> post_task(Task &&task,
                                                                             Args &&...args) {
    using ReturnType = std::invoke_result_t<Task, timer_runner &, Args...>;
    auto ftask = std::make_shared<std::packaged_task<ReturnType(timer_runner &, Args...)>>(
        std::forward<Task>(task));
    std::future<ReturnType> result = ftask->get_future();

    boost::asio::post(io_context_,
                      [ftask = std::move(ftask), this, ... args = std::move(args)]() mutable {
                        return (*ftask)(*this, args...);
                      });

    return result;
  }

  operator boost::asio::io_context &() { return io_context_; }

  void register_timer(std::int64_t id, std::shared_ptr<boost::asio::steady_timer> timer,
                      cppgres::role role) {
    timers[id] = {std::move(timer), role};
  }

  bool cancel_timer(std::int64_t id, cppgres::role requester) {
    auto it = timers.find(id);
    if (it == timers.end()) {
      return false;
    }
    auto [timer, owner] = it->second;
    if (owner == requester) {
      timer->cancel();
      timers.erase(it);
      return true;
    } else {
      return false;
    }
  }

  void stop() { work_guard_.reset(); }

  operator oink::sender &() { return snd; }

private:
  boost::asio::io_context io_context_;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
  std::thread worker_thread_;

  std::map<std::int64_t, std::tuple<std::shared_ptr<boost::asio::steady_timer>, cppgres::role>>
      timers;

  oink::arena arena;
  oink::sender snd;
};

static std::optional<timer_runner> timer;
static std::atomic<std::uint64_t> *timer_id = nullptr;

static void init_timer() {
  if (!timer.has_value()) {
    timer.emplace();
  }
  bool found;
  backend_handle->allocate_shmem(
      backend_handle, cppgres::fmt::format("omni_pool<{}>:timer", MyDatabaseId).c_str(),
      sizeof(bool),
      [](const omni_handle *handle, void *ptr, void *arg, bool allocated) {
        timer_id = reinterpret_cast<std::atomic<uint64_t> *>(ptr);
        if (allocated) {
          std::construct_at(timer_id, 0);
        }
      },
      nullptr, &found);
}

struct timer_service {
  timer_service(timer_message *msg) : top_msg(msg) {}
  bool operator()(timer_message::after &msg) {
    if (timer.has_value()) {
      timer
          ->post_task([&msg, this](timer_runner &runner) mutable {
            boost::asio::io_context &io_context = runner;
            auto t = std::make_shared<boost::asio::steady_timer>(
                io_context, std::chrono::milliseconds(msg.milliseconds));
            msg.id = static_cast<std::int64_t>(timer_id->fetch_add(1));
            std::uint64_t id = msg.id;
            cppgres::role role = msg.role;
            cppgres::oid function_oid = msg.function_oid;
            top_msg->ready.post();
            runner.register_timer(msg.id, t, msg.role);
            t->async_wait([&msg, t = std::move(t), id, function_oid, role, this,
                           runner = &runner](const boost::system::error_code &ec) {
              if (!ec) {
                auto msg = runner->operator oink::sender &().send<timer_message>(
                    std::in_place_type<timer_message::execute>, id, function_oid, role);
              } else {
                main_worker->post(
                    [=]() { cppgres::report(WARNING, "Timer error: %s", ec.message().c_str()); });
              }
            });
          })
          .wait();
      return true;
    } else {
      return false;
    }
  }

  bool operator()(timer_message::execute &msg) {
    main_worker->post([&msg]() {
      if (timer.has_value()) {
        timer->operator oink::sender &().send<timer_message>(
            std::in_place_type<timer_message::cancel>, msg.id, msg.role);
      }
      cppgres::transaction tx;
      try {
        cppgres::function<cppgres::value> f(msg.function_oid);
        cppgres::security_context ctx(msg.role, cppgres::security_local_user_id_change);
        f();
      } catch (std::exception &e) {
        tx.rollback();
        cppgres::report(WARNING, "%s", e.what());
      }
    });
    return true;
  }

  bool operator()(timer_message::cancel &msg) {
    if (timer.has_value()) {
      bool removed = timer
                         ->post_task([&msg, this](timer_runner &runner) {
                           return runner.cancel_timer(msg.id, msg.requester);
                         })
                         .get();
      if (removed) {
        msg.cancelled = removed;
        top_msg->ready.post();
      }
      return removed;
    } else {
      return false;
    }
  }

private:
  timer_message *top_msg;
};

bool timer_handler(timer_message *msg) {
  return cppgres::exception_guard([=]() { return std::visit(timer_service(msg), msg->msg); })();
}

extern "C" void *omni_worker_handler(const char *name, std::size_t *hash, bool init, bool leader) {
  std::string_view handler_name(name);
  if (handler_name == "sql") {
    if (hash != nullptr) {
      *hash = std::hash<std::string_view>{}(sql_message::name());
    }
    if (init) {
      init_sql(leader);
    }
    return reinterpret_cast<void *>(sql_handler);
  } else if (handler_name == "timer") {
    if (hash != nullptr) {
      *hash = std::hash<std::string_view>{}(timer_message::name());
      if (init) {
        init_timer();
      }
    }
    return reinterpret_cast<void *>(timer_handler);
  }
  return nullptr;
}

std::optional<std::int64_t> timer_after_impl(std::int64_t milliseconds,
                                             cppgres::function<cppgres::value> f,
                                             std::optional<cppgres::role> run_as) {
  cppgres::role role = run_as.value_or(cppgres::role());
  if (!cppgres::role().is_member(role)) {
    throw std::runtime_error(
        cppgres::fmt::format("Role {} is not a member of {}", cppgres::role().name(), role.name()));
  }
  oink::arena arena(arena_name().c_str(), arena_size());
  oink::sender snd(arena, mq_name().c_str(), 8192);
  auto msg = snd.send<timer_message>(std::in_place_type<timer_message::after>, milliseconds,
                                     f.function_oid(), role);

  static bip::interprocess_semaphore *semaphore = nullptr;
  semaphore = &msg->ready;
  scoped_interrupt_handler ih{SIGINT, [](int) { semaphore->post(); }};
  if (msg->ready.timed_wait(std::chrono::system_clock::now() + std::chrono::milliseconds(1000))) {
    return std::get<timer_message::after>(msg->msg).id;
  }
  cppgres::report(WARNING, "Timer service timeout");
  return std::nullopt;
}

postgres_function(timer_after, timer_after_impl);

std::optional<bool> timer_cancel_impl(std::int64_t id) {
  oink::arena arena(arena_name().c_str(), arena_size());
  oink::sender snd(arena, mq_name().c_str(), 8192);
  auto msg =
      snd.send<timer_message>(std::in_place_type<timer_message::cancel>, id, cppgres::role());

  static bip::interprocess_semaphore *semaphore = nullptr;
  semaphore = &msg->ready;
  scoped_interrupt_handler ih{SIGINT, [](int) { semaphore->post(); }};
  if (msg->ready.timed_wait(std::chrono::system_clock::now() + std::chrono::milliseconds(1000))) {
    return std::get<timer_message::cancel>(msg->msg).cancelled;
  }
  cppgres::report(WARNING, "Timer service timeout");
  return std::nullopt;
}

postgres_function(timer_cancel, timer_cancel_impl);
