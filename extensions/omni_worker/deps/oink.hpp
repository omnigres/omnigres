
#include <map>
#include <typeindex>

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>

#include <boost/container/vector.hpp>

namespace oink {
namespace bip = boost::interprocess;
namespace bc = boost::container;

template <typename T>
using allocator = bip::allocator<T, bip::managed_shared_memory::segment_manager>;

template <typename Container, typename Mutex> struct shared_container {
  using container_type = Container;
  using mutex_type = Mutex;
  Container container;
  Mutex mutex;

  template <typename... Args>
  explicit shared_container(Args &&...args) : container(std::forward<Args>(args)...) {}

  operator container_type &() { return container; }
  operator mutex_type &() { return mutex; }
};

template <typename T>
concept named_message = requires(T t) {
  { T::name() } -> std::same_as<const char *>;
};

template <typename T>
concept message = named_message<T>;

template <message T> std::size_t message_tag() {
  if constexpr (named_message<T>) {
    return std::hash<std::string>{}(T::name());
  }
  static_assert("message type tag unknown");
}

struct arena {

  friend struct endpoint;
  friend struct sender;
  friend struct receiver;

  struct header {};

  using header_t = shared_container<header, bip::interprocess_upgradable_mutex>;

  arena(const char *segment_name, size_t segment_size)
      : segment(bip::open_or_create, segment_name, segment_size),
        header_(segment.find_or_construct<header_t>("__header")()) {}

  auto get_segment_manager() { return segment.get_segment_manager(); }

  template <typename T> auto get_allocator() {
    return bip::allocator<T, bip::managed_shared_memory::segment_manager>(
        segment.get_segment_manager());
  }

  void *get_address() { return segment.get_address(); }

protected:
  bip::managed_shared_memory segment;

  header_t *header_;
};

struct endpoint {

  struct msg {
    std::size_t hash;
    std::ptrdiff_t offset;
  };

  endpoint(arena &arena, const char *mq_segment_name, size_t mq_max_messages)
      : arena(arena), mq(bip::open_or_create, mq_segment_name, mq_max_messages, sizeof(msg)),
        msgs_(arena.get_segment_manager()->find_or_construct<msg_vec>("__msgs")(
            arena.get_segment_manager())) {}

  template <typename T> auto get_allocator() { return arena.get_allocator<T>(); }

  template <typename T> T &get_msg(std::ptrdiff_t index) {
    void *addr = static_cast<char *>(arena.segment.get_address()) + index;
    return reinterpret_cast<T &>(addr);
  }

protected:
  using msg_allocator_t = allocator<msg>;
  using msg_vec =
      shared_container<bc::vector<msg, msg_allocator_t>, bip::interprocess_recursive_mutex>;

  arena &arena;
  bip::message_queue mq;

  msg_vec *msgs_;
};

struct sender : endpoint {
  using endpoint::endpoint;

  template <message M, typename... Args> M &send(Args &&...args) {
    auto msg_ = get_allocator<M>().allocate(1);
    std::construct_at(msg_.get(), std::forward<Args>(args)...);
    std::ptrdiff_t offset = reinterpret_cast<char *>(msg_.get()) -
                            reinterpret_cast<char *>(arena.segment.get_address());
    auto m = msg{.hash = message_tag<M>(), .offset = offset};
    mq.send(&m, sizeof(decltype(m)), 0);
    return *msg_;
  }
};

struct receiver : endpoint {
  using endpoint::endpoint;

  struct unknown_message : public std::exception {
    explicit unknown_message(std::size_t hash)
        : hash(hash), message(std::string("unknown message ") + std::to_string(hash)) {}

    const char *what() const noexcept override { return message.c_str(); }
    std::size_t message_hash() const { return hash; }

  private:
    std::size_t hash;
    std::string message;
  };

  template <message... Msg> bool receive(auto visitor) {
    bip::message_queue::size_type recvd_size;
    unsigned int priority;

    msg m;
    if (mq.timed_receive(&m, sizeof(m), recvd_size, priority,
                         std::chrono::system_clock::now() + std::chrono::milliseconds(500))) {

      bool matched = false;
      bool accepted = true;

      (try_handle<Msg>(m, matched, accepted, visitor), ...);

      if (!matched) {
        if constexpr (requires(decltype(visitor) v, msg &index) {
                        { v(index) };
                      }) {
          using return_type = std::invoke_result_t<decltype(visitor), msg &>;
          if constexpr (std::same_as<return_type, bool>) {
            accepted = visitor(m);
          } else {
            visitor(m);
          }
          matched = true;
        }
      }

      if (!accepted) {
        mq.send(&m, sizeof(m), 0);
        return false;
      }
      if (!matched) {
        throw unknown_message(m.hash);
      }

      if (mq.get_num_msg() == 0) {
        msgs_->container.clear();
      }

      return true;
    } else {
      return false;
    }
  }

private:
  template <message T> void try_handle(msg &j, bool &matched, bool &accepted, auto visitor) {
    if (j.hash == message_tag<T>()) {
      if constexpr (requires(decltype(visitor) v, T &index) {
                      { v(index) };
                    }) {
        using return_type = std::invoke_result_t<decltype(visitor), T &>;
        auto p = reinterpret_cast<T *>(static_cast<char *>(arena.segment.get_address()) + j.offset);
        if constexpr (std::same_as<return_type, bool>) {
          accepted = visitor(*p);
        } else {
          visitor(*p);
        }
        if (accepted) {
          get_allocator<T>().deallocate(p, 1);
        }
        matched = true;
      }
    }
  }
};

} // namespace oink
