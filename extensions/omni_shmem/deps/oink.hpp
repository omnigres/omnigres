#pragma once

#ifndef oink_hpp
#define oink_hpp

#include <atomic>
#include <chrono>
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

  template <typename... Args>
  explicit shared_container(Args &&...args) : container(std::forward<Args>(args)...) {}

  std::pair<bip::scoped_lock<mutex_type>, container_type &> scoped_lock() {
    return {bip::scoped_lock<mutex_type>(mutex), container};
  }

private:
  Container container;
  Mutex mutex;
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

template <typename T, typename C, typename... Args>
concept arena_constructor = requires(T t, Args &...args) {
  { t(std::forward<Args>(args)...) } -> std::same_as<C *>;
};

struct arena {

  friend struct endpoint;
  friend struct sender;
  friend struct receiver;
  template <message M> friend struct message_envelope_receipt;

  struct header {
  };

  using header_t = shared_container<header, bip::interprocess_upgradable_mutex>;

  arena(const char *segment_name) : name(segment_name), segment(bip::open_only, segment_name) {
    auto [container, _b] = segment.find<header_t>("__header");
    header_ = container;
  }

  arena(const char *segment_name, size_t segment_size)
      : name(segment_name), segment(bip::open_or_create, segment_name, segment_size),
        header_(segment.find_or_construct<header_t>("__header")()) {}

  auto get_segment_manager() { return segment.get_segment_manager(); }

  template <typename T> auto get_allocator() {
    return bip::allocator<T, bip::managed_shared_memory::segment_manager>(
        segment.get_segment_manager());
  }

  void *get_address() { return segment.get_address(); }

  std::size_t get_free_memory() { return segment.get_free_memory(); }

  template <class T, typename... Args>
  arena_constructor<T, Args...> auto find_or_construct(const char *name) {
    return segment.find_or_construct<T>(name);
  }

  template <class T> std::optional<T *> find(const char *name) {
    auto [ptr, _] = segment.find<T>(name);
    if (ptr == nullptr) {
      return std::nullopt;
    }
    return ptr;
  }

  operator bip::managed_shared_memory &() { return segment; }

  std::size_t get_segment_size() { return segment.get_size(); }

protected:
  std::string name;
  bip::managed_shared_memory segment;

  header_t *header_;
};

struct transient_arena : arena {

  // We use `name.c_str()` to make sure `removal_` has a pointer to a string with a valid
  // lifetime. The one in the argument can go away at any time.
  explicit transient_arena(const char *segment_name)
      : arena(segment_name), removal_(name.c_str()) {}
  transient_arena(const char *segment_name, size_t segment_size)
      : arena(segment_name, segment_size), removal_(name.c_str()) {}

private:
  bip::remove_shared_memory_on_destroy removal_;
};

struct endpoint {

  struct msg {
    std::size_t hash;
    std::ptrdiff_t offset;
  };

  endpoint(arena &arena, const char *mq_segment_name, size_t mq_max_messages)
      : arena_(arena), mq_(bip::open_or_create, mq_segment_name, mq_max_messages, sizeof(msg)),
        msgs_(arena.get_segment_manager()->find_or_construct<msg_vec>("__msgs")(
            arena.get_segment_manager())) {}

  template <typename T> auto get_allocator() { return arena_.get_allocator<T>(); }

  template <typename T> T &get_msg(std::ptrdiff_t index) {
    void *addr = static_cast<char *>(arena_.segment.get_address()) + index;
    return reinterpret_cast<T &>(addr);
  }

protected:
  using msg_allocator_t = allocator<msg>;
  using msg_vec =
      shared_container<bc::vector<msg, msg_allocator_t>, bip::interprocess_recursive_mutex>;

  arena &arena_;
  bip::message_queue mq_;

  msg_vec *msgs_;
};

template <message M> struct message_envelope {
  template <typename... Args>
  message_envelope(Args &&...args) : message(std::forward<Args>(args)...), counter(0) {}

  operator M &() { return message; }

  template <message M_> friend struct message_envelope_receipt;

private:
  M message;
  std::atomic<std::size_t> counter;
};

template <message M> struct message_envelope_receipt {

  M *operator->() { return &envelope->message; }

  operator M &() { return envelope->message; }

  ~message_envelope_receipt() {
    if (envelope != nullptr) {
      std::size_t counter = envelope->counter.fetch_sub(1) - 1;
      if (counter == 0) {
        std::destroy_at(envelope);
        arena_.get().template get_allocator<message_envelope<M>>().deallocate(envelope, 1);
      }
    }
  }

  friend struct sender;
  friend struct receiver;

  message_envelope_receipt(const message_envelope_receipt &other)
      : envelope(other.envelope), arena_(other.arena_) {
    other.envelope->counter.fetch_add(1);
  }

  message_envelope_receipt(message_envelope_receipt &&other) noexcept
      : envelope(other.envelope), arena_(other.arena_) {
    other.envelope = nullptr;
  }

  message_envelope_receipt &operator=(const message_envelope_receipt &other) {
    if (this == &other)
      return *this;
    envelope = other.envelope;
    arena_ = other.arena_;
    other.envelope->counter.fetch_add(1);
    return *this;
  }

  message_envelope_receipt &operator=(message_envelope_receipt &&other) noexcept {
    if (this == &other)
      return *this;
    envelope = other.envelope;
    arena_ = other.arena_;
    other.envelope = nullptr;
    return *this;
  }

private:
  message_envelope_receipt(message_envelope<M> *envelope, arena &arena, bool acquire = true)
      : envelope(envelope), arena_(arena) {
    if (acquire) {
      envelope->counter.fetch_add(2);
    }
  }

  std::ptrdiff_t offset() {
    return reinterpret_cast<char *>(envelope) -
           reinterpret_cast<char *>(arena_.get().get_address());
  }

  void retain() { envelope = nullptr; }

  message_envelope<M> *envelope;
  std::reference_wrapper<arena> arena_;
};

struct sender : endpoint {
  using endpoint::endpoint;

  template <message M, typename... Args> message_envelope_receipt<M> send(Args &&...args) {
    auto msg_ = arena_.get_allocator<message_envelope<M>>().allocate(1);
    std::construct_at(msg_.get(), std::forward<Args>(args)...);
    message_envelope_receipt<M> receipt = message_envelope_receipt(msg_.get(), arena_);
    auto m = msg{.hash = message_tag<M>(), .offset = receipt.offset()};
    mq_.send(&m, sizeof(decltype(m)), 0);
    return receipt;
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
    if (mq_.timed_receive(&m, sizeof(m), recvd_size, priority,
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
        mq_.send(&m, sizeof(m), 0);
        return false;
      }
      if (!matched) {
        throw unknown_message(m.hash);
      }

      if (mq_.get_num_msg() == 0) {
        auto [lock, container] = msgs_->scoped_lock();
        container.clear();
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
        message_envelope_receipt<T> p(
            reinterpret_cast<message_envelope<T> *>(
                static_cast<char *>(arena_.segment.get_address()) + j.offset),
            arena_, false);
        if constexpr (std::same_as<return_type, bool>) {
          accepted = visitor(p.operator T &());
        } else {
          visitor(p.operator T &());
        }
        matched = true;
        if (!accepted) {
          p.retain();
        }
      }
    }
  }
};

} // namespace oink

#endif
