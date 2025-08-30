#define BOOST_INTERPROCESS_ENABLE_TIMEOUT_WHEN_LOCKING
#include <oink.hpp>

#include <ranges>

#include <cppgres.hpp>
extern "C" {
PG_MODULE_MAGIC;

#include <omni/omni_v0.h>
OMNI_MAGIC;

OMNI_MODULE_INFO(.name = "omni_shmem", .version = EXT_VERSION,
                 .identity = "53fead74-c3de-4095-8c3c-3c01e928b5fd");
}

struct managed_arenas {

  oink::arena &find_or_create(std::string name, std::size_t size, bool transient = false) {
    if (transient) {
      auto [it, inserted] =
          arenas_.try_emplace(name, std::in_place_type<oink::transient_arena>, name.c_str(), size);

      return std::visit([](auto &arena) -> oink::arena & { return arena; }, it->second);
    } else {
      auto [it, inserted] =
          arenas_.try_emplace(name, std::in_place_type<oink::arena>, name.c_str(), size);

      return std::visit([](auto &arena) -> oink::arena & { return arena; }, it->second);
    }
  }

  oink::arena &find(std::string name, bool transient = false) {
    auto it = arenas_.find(name);
    if (it != arenas_.end()) {
      return std::visit([](auto &arena) -> oink::arena & { return arena; }, it->second);
    }
    if (transient) {
      auto [it_, inserted] =
          arenas_.try_emplace(name, std::in_place_type<oink::transient_arena>, name.c_str());

      return std::visit([](auto &arena) -> oink::arena & { return arena; }, it_->second);
    } else {
      auto [it_, inserted] =
          arenas_.try_emplace(name, std::in_place_type<oink::arena>, name.c_str());

      return std::visit([](auto &arena) -> oink::arena & { return arena; }, it_->second);
    }
  }

  void delete_by_name(std::string name) {
    arenas_.erase(name);
    oink::bip::shared_memory_object::remove(name.c_str());
  }

  void clear() { arenas_.clear(); }

  std::map<std::string, std::variant<oink::arena, oink::transient_arena>> &arenas() {
    return arenas_;
  }

  template <typename T> friend class allocation;
  friend class omni_arena_super;

private:
  std::map<std::string, std::variant<oink::arena, oink::transient_arena>> arenas_;
};

static managed_arenas arenas;

std::string shmem_arena_impl(std::string name, std::int64_t size, std::optional<bool> transient) {
  auto &arena = arenas.find_or_create(name, size, transient.value_or(true));
  if (arena.get_segment_size() != size) {
    cppgres::report(WARNING, "Mismatched arena size. Requested %d, got %d", size,
                    arena.get_segment_size());
  }
  return name;
}
postgres_function(shmem_arena, shmem_arena_impl);

void shmem_delete_arena_impl(std::string name) { arenas.delete_by_name(name); }
postgres_function(shmem_delete_arena, shmem_delete_arena_impl);

std::int64_t shmem_arena_free_memory_impl(std::string name) {
  return arenas.find(name).get_free_memory();
}
postgres_function(shmem_arena_free_memory, shmem_arena_free_memory_impl);

template <typename T = void> struct allocation {

  allocation(oink::arena &arena, std::size_t size, bool &constructed,
             void(destructor)(void *, void *) = nullptr, void *destructor_ctx = nullptr)
      : size_(size), ptr(reinterpret_cast<T *>(
                         arena.operator oink::bip::managed_shared_memory &().allocate(size_))),
        destructor_(destructor), destructor_ctx_(destructor_ctx) {
    constructed = true;
  }

  T *operator*() { return ptr.get(); }
  size_t size() const { return size_; }

  ~allocation() {
    if (destructor_) {
      destructor_(ptr.get(), destructor_ctx_);
    }
    for (auto &[name, arena] : arenas.arenas_) {
      std::visit(
          [this](auto &arena) {
            oink::bip::managed_shared_memory &mem = arena;
            if (mem.belongs_to_segment(ptr.get())) {
              mem.deallocate(ptr.get());
            }
          },
          arena);
    }
  }

private:
  std::size_t size_;
  oink::bip::offset_ptr<T> ptr;
  void (*destructor_)(void *, void *) = nullptr;
  void *destructor_ctx_ = nullptr;
};

struct find_or_construct_report {
  bool constructed;
  std::int64_t size;
  static cppgres::type composite_type() {
    auto schema = cppgres::ffi_guard{::GetConfigOption}("omni_shmem.schema", false, true);
    return cppgres::named_type(schema ? schema : "omni_shmem", "find_or_construct_report");
  }
};

find_or_construct_report shmem_find_or_construct_impl(std::string arena_name, std::string name,
                                                      std::int64_t size) {
  auto &arena = arenas.find(std::move(arena_name));
  bool constructed = false;
  oink::bip::managed_shared_memory &mem = arena;
  auto alloc =
      arena.find_or_construct<allocation<std::byte>>(name.c_str())(arena, size, constructed);
  return {constructed, static_cast<std::int64_t>(alloc->size())};
}
postgres_function(shmem_find_or_construct, shmem_find_or_construct_impl);

find_or_construct_report shmem_find_or_construct_by_value_impl(std::string arena_name,
                                                               std::string name,
                                                               cppgres::byte_array ba) {
  auto &arena = arenas.find(std::move(arena_name));
  bool constructed = false;
  oink::bip::managed_shared_memory &mem = arena;
  auto *alloc = arena.find_or_construct<allocation<std::byte>>(name.c_str())(arena, ba.size_bytes(),
                                                                             constructed);
  if (constructed) {
    std::memcpy(**alloc, ba.data(), ba.size_bytes());
  }
  return {constructed, static_cast<std::int64_t>(alloc->size())};
}
postgres_function(shmem_find_or_construct_by_value, shmem_find_or_construct_by_value_impl);

cppgres::byte_array shmem_read_impl(std::string arena_name, const std::string name,
                                    std::optional<std::int32_t> start,
                                    std::optional<std::int32_t> size) {
  auto &arena = arenas.find(std::move(arena_name));
  auto alloc = arena.find<allocation<std::byte>>(name.c_str());
  if (!alloc.has_value()) {
    throw std::runtime_error("allocation not found");
  }
  auto allocation = *alloc;
  std::size_t start_ = start.value_or(0);
  if (start_ > allocation->size() - 1) {
    throw std::runtime_error("start can only be within the allocation");
  }
  std::byte *ptr = allocation->operator*() + start_;
  std::size_t size_ = size.value_or(allocation->size());
  return {ptr, std::min(allocation->size() - start_, size_ - start_)};
}
postgres_function(shmem_read, shmem_read_impl);

bool shmem_destroy_impl(std::string arena_name, const std::string name) {
  auto &arena = arenas.find(std::move(arena_name));
  oink::bip::managed_shared_memory &mem = arena;
  return mem.destroy<allocation<std::byte>>(name.c_str());
}
postgres_function(shmem_destroy, shmem_destroy_impl);

auto shmem_arenas_impl() {
  std::vector<std::tuple<std::string, std::int64_t>> vec;
  for (auto &arena : arenas.arenas()) {
    vec.push_back({std::string(arena.first),
                   static_cast<int64_t>(
                       std::visit([](auto &a) { return a.get_segment_size(); }, arena.second))});
  }
  return vec;
}

postgres_function(shmem_arenas, shmem_arenas_impl);

/// APEX API

#define apex_api_owner
#include "apex_omni_shmem.h"

#include <memory>

struct omni_arena_super;

template <typename T> struct function_traits;

template <typename R, typename C, typename... Args> struct function_traits<R (*)(C *, Args...)> {
  using args_tuple = std::tuple<Args...>;
};

template <typename T, typename F, auto Func> auto apex_call_bridge() {
  using traits = function_traits<decltype(Func)>;
  return []<typename... Args>(std::tuple<Args...> *) {
    return +[](T *ptr, Args... args) -> decltype(auto) {
      auto *obj = reinterpret_cast<F *>(ptr);
      return Func(obj, args...);
    };
  }(static_cast<typename traits::args_tuple *>(nullptr));
}

struct omni_arena_super {
  template <auto Func> static auto bridge() {
    return apex_call_bridge<omni_arena, omni_arena_super, Func>();
  }

  static void *alloc(omni_arena_super *obj, size_t sz) {
    oink::bip::managed_shared_memory &mem = obj->arena_.get();
    return mem.allocate(sz);
  }

  static void *find(omni_arena_super *obj, const char *name) {
    auto &arena = obj->arena_.get();
    auto alloc = arena.find<allocation<std::byte>>(name);
    return alloc.has_value() ? reinterpret_cast<void *>(alloc.value()->operator*()) : nullptr;
  }

  static omni_arena_allocation find_or_construct(omni_arena_super *obj, const char *name,
                                                 size_t size, void(destructor)(void *, void *),
                                                 void *destructor_ctx) {
    auto &arena = obj->arena_.get();
    bool constructed;
    auto alloc = arena.find_or_construct<allocation<std::byte>>(name)(arena, size, constructed,
                                                                      destructor, destructor_ctx);
    auto ptr = reinterpret_cast<void *>(alloc->operator*());
    omni_arena_allocation result = {.ptr = ptr, .found = !constructed};
    return result;
  }

  static bool destroy(omni_arena_super *obj, const char *name) {
    auto &arena = obj->arena_.get();
    return arena.get_segment_manager()->destroy<allocation<std::byte>>(name);
  }

  static void free(omni_arena_super *obj, void *ptr) {
    oink::bip::managed_shared_memory &mem = obj->arena_.get();
    mem.deallocate(ptr);
  }

  static void release(omni_arena_super *obj) {
    arenas.arenas_.erase(obj->name_);
    std::destroy_at(obj);
    cppgres::top_memory_context().free(obj);
  }

  explicit omni_arena_super(oink::arena &arena, std::string name) : arena_(arena), name_(name) {
    inner.alloc = bridge<alloc>();
    inner.find = bridge<find>();
    inner.find_or_construct = bridge<find_or_construct>();
    inner.destroy = bridge<destroy>();
    inner.free = bridge<free>();
    inner.release = bridge<release>();
  }

  omni_arena inner;
  std::reference_wrapper<oink::arena> arena_;
  std::string name_;
};
static_assert(offsetof(omni_arena_super, inner) == 0);

omni_arena *open(const char *name, bool transient) {
  auto *shmem = cppgres::top_memory_context().alloc<omni_arena_super>();
  std::construct_at(shmem, arenas.find(name, transient), name);
  return reinterpret_cast<omni_arena *>(shmem);
}
omni_arena *open_or_create(const char *name, size_t size, bool transient) {
  auto *shmem = cppgres::top_memory_context().alloc<omni_arena_super>();
  std::construct_at(shmem, arenas.find_or_create(name, size, transient), name);
  return reinterpret_cast<omni_arena *>(shmem);
}

omni_shmem_api api = {.open = open, .open_or_create = open_or_create};
void _Omni_init(const omni_handle *handle) {
  apex_initialize_omni_shmem_api(&api);
  cppgres::backend::atexit([](auto code) { arenas.clear(); });
}
