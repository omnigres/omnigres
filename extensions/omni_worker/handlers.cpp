#include <boost/container/string.hpp>
#include <oink.hpp>

#include <cppgres.hpp>

#include "omni_worker.hpp"

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

extern "C" void *omni_worker_handler(const char *name, std::size_t *hash) {
  if (std::string_view(name) == "sql") {
    if (hash != nullptr) {
      *hash = std::hash<std::string_view>{}(sql_message::name());
    }
    return reinterpret_cast<void *>(sql_handler);
  }
  return nullptr;
}

postgres_function(sql, ([](std::string_view s,
                           std::optional<std::int32_t> wait_ms) -> std::optional<bool> {
                    oink::arena arena(arena_name().c_str(), arena_size());
                    oink::sender snd(arena, mq_name().c_str(), 8192);
                    sql_message &msg = snd.send<sql_message>(arena, s);
                    if (wait_ms.has_value()) {
                      if (msg.ready.timed_wait(std::chrono::system_clock::now() +
                                               std::chrono::milliseconds(wait_ms.value()))) {

                        return msg.rc == 0;
                      } else {
                        return std::nullopt;
                      }
                    } else {
                      return std::nullopt;
                    }
                  }));
