#ifdef DEBUG
#undef DEBUG
#define restore_DEBUG
#endif

#include <codon/cir/util/irtools.h>
#include <codon/compiler/compiler.h>
#include <codon/compiler/jit.h>

#include <map>

/* Codon defines these but they clash with Postgres */
#undef LOG
#ifdef restore_DEBUG
#define DEBUG
#endif

#include <cppgres.hpp>

extern "C" {
PG_MODULE_MAGIC;
}

codon::jit::JIT jit = codon::jit::JIT("");
auto resource_tracker = jit.getEngine()->getMainJITDylib().createResourceTracker();

using namespace codon::ir;

struct function_info {
  void *result;
  bool returns_optional;
};

std::map<std::size_t, function_info> function_cache;
std::map<cppgres::oid, std::tuple<std::size_t, cppgres::transaction_id, std::string>>
    function_hash_map;

void codon_init() { llvm::cantFail(jit.init(true)); }

template <typename T> T __codon__arg(int i, cppgres::oid oid) {
  auto fcinfo = *cppgres::current_postgres_function::call_info();
  return cppgres::from_nullable_datum<T>(fcinfo.arg_values()[i].get_nullable_datum(), oid);
}

extern "C" bool __codon__arg_is_null(int i) {
  auto fcinfo = *cppgres::current_postgres_function::call_info();
  auto d = fcinfo.arg_values()[i].get_nullable_datum();
  return d.is_null();
}

extern "C" bool __codon__arg_bool(int i) { return __codon__arg<bool>(i, BOOLOID); }
extern "C" int16_t __codon__arg_int2(int i) { return __codon__arg<int16_t>(i, INT2OID); }
extern "C" int32_t __codon__arg_int4(int i) { return __codon__arg<int32_t>(i, INT4OID); }
extern "C" int64_t __codon__arg_int8(int i) { return __codon__arg<int64_t>(i, INT8OID); }
static_assert(sizeof(float) == 4);
extern "C" float __codon__arg_float32(int i) { return __codon__arg<float>(i, FLOAT4OID); }
static_assert(sizeof(double) == 8);
extern "C" double __codon__arg_float(int i) { return __codon__arg<double>(i, FLOAT8OID); }
extern "C" seq_str_t __codon__arg_text(int i) {
  auto str = __codon__arg<std::string_view>(i, TEXTOID);
  return {static_cast<int64_t>(str.length()), const_cast<char *>(str.data())};
}

const char *preamble = R"preamble(
from C import __codon__arg_int2(int) -> Int[16]
from C import __codon__arg_int4(int) -> Int[32]
from C import __codon__arg_is_null(int) -> bool
from C import __codon__arg_int8(int) -> int
from C import __codon__arg_float(int) -> float
from C import __codon__arg_float32(int) -> float32
from C import __codon__arg_text(int) -> str
from C import __codon__arg_bool(int) -> bool

@__attribute__
@__force__
def pg():
    pass

)preamble";

struct scoped_arena {
  Module *M;
  scoped_arena(Module *M) : M(M) { M->pushArena(); }
  ~scoped_arena() { M->popArena(); }
};

struct jit_state {
  jit_state() : state(jit.getCompiler()->getCache(), true) {}

  ~jit_state() { state.undo(); }

private:
  codon::jit::JITState state;
};

std::hash<std::string_view> hasher;

auto codon_call_handler_impl() -> cppgres::value {
  auto fcinfo = cppgres::current_postgres_function::call_info();
  if (!fcinfo.has_value()) {
    cppgres::report(ERROR, "invalid call");
  }

  ::FunctionCallInfo info = *fcinfo;
  std::size_t hash;
  std::optional<std::string> src = std::nullopt;
  cppgres::syscache<Form_pg_proc, cppgres::oid> proc(info->flinfo->fn_oid);
  cppgres::heap_tuple proc_tup = proc;
check:
  if (function_hash_map.contains(info->flinfo->fn_oid)) {
    auto [_hash, xmin, src_] = function_hash_map[info->flinfo->fn_oid];
    src = src_;
    hash = _hash;
    if (proc_tup.xmin(true) != xmin) {
      function_hash_map.erase(info->flinfo->fn_oid);
      goto check;
    }
  } else {
    src = proc.get_attribute<std::string>(Anum_pg_proc_prosrc);
    hash = hasher(*src);
    function_hash_map.insert(
        {info->flinfo->fn_oid, {hash, proc_tup.xmin(true), std::string(*src)}});
  }
  auto call_info = cppgres::current_postgres_function::call_info();
  if (!call_info.has_value()) {
    throw std::runtime_error("missing fcinfo");
  }

  void *result = nullptr;
  bool returns_optional = false;

  if (function_cache.contains(hash)) {
    auto &fc = function_cache[hash];
    result = fc.result;
    returns_optional = fc.returns_optional;
  } else {
    auto *M = jit.getCompiler()->getModule();

    scoped_arena handler(M);

    auto state_reset = jit_state();
    auto func = jit.compile(cppgres::fmt::format("{} {}", preamble, *src));

    if (auto err = func.takeError()) {
      auto errorInfo = llvm::toString(std::move(err));
      throw std::runtime_error(std::move(errorInfo));
    }
    /*
     This is how it should be done, but for now we do the `import` in the preamble because there
     is an error:

     ```
     JIT session error: Symbols not found: [ ___unnamed_1 ]
     ```
    */
    auto make_arg_func_ = [&M](const char *name, types::Type *type) {
      auto arg_func = M->Nr<ExternalFunc>(name);
      arg_func->realize(M->getFuncType(type, {M->getIntType()}), {"i"});
      llvm::cantFail(jit.compile(arg_func, resource_tracker));
      return arg_func;
    };

    auto make_arg_func = [M = M](const char *name, types::Type *type) {
      return cast<ExternalFunc>(*std::find_if(M->begin(), M->end(), [&name](auto *x) {
        if (auto *efunc = cast<ExternalFunc>(x)) {
          if (efunc->getUnmangledName() == name) {
            return true;
          }
        }
        return false;
      }));
    };

    auto *f__codon__arg_int2 = make_arg_func("__codon__arg_int2", M->getIntNType(16, true));
    auto *f__codon__arg_int4 = make_arg_func("__codon__arg_int4", M->getIntNType(32, true));
    auto *f__codon__arg_int8 = make_arg_func("__codon__arg_int8", M->getIntType());
    auto *f__codon__arg_text = make_arg_func("__codon__arg_text", M->getStringType());
    auto *f__codon__arg_bool = make_arg_func("__codon__arg_bool", M->getBoolType());
    auto *f__codon__arg_float = make_arg_func("__codon__arg_float", M->getFloatType());
    auto *f__codon__arg_float32 = make_arg_func("__codon__arg_float32", M->getFloat32Type());
    auto *f__codon__arg_is_null = make_arg_func("__codon__arg_is_null", M->getBoolType());

    for (auto *var : *M) {
      if (auto *bodied_func = cast<BodiedFunc>(var)) {

        if (util::hasAttribute(bodied_func, codon::ast::getMangledFunc(M->getName(), "pg"))) {
          if (bodied_func->getUnmangledName() != std::string_view(NameStr((*proc).proname))) {
            continue;
          }

          auto argit = bodied_func->arg_begin();

          std::vector<codon::ir::Value *> args;
          int i = 0;
          for (auto val : call_info->arg_values()) {
            auto type = val.get_type().oid;
            auto farg = (*argit)->getType();
            ExternalFunc *arg_extractor = nullptr;
            types::Type *farg_type = nullptr;
            switch (type) {
            case INT2OID:
              arg_extractor = arg_extractor ? arg_extractor : f__codon__arg_int2;
              farg_type = farg_type ? farg_type : M->getIntNType(16, true);
            case INT4OID:
              arg_extractor = arg_extractor ? arg_extractor : f__codon__arg_int4;
              farg_type = farg_type ? farg_type : M->getIntNType(32, true);
            case INT8OID:
              arg_extractor = arg_extractor ? arg_extractor : f__codon__arg_int8;
              farg_type = farg_type ? farg_type : M->getIntType();
            case BOOLOID:
              arg_extractor = arg_extractor ? arg_extractor : f__codon__arg_bool;
              farg_type = farg_type ? farg_type : M->getBoolType();
            case FLOAT4OID:
              arg_extractor = arg_extractor ? arg_extractor : f__codon__arg_float32;
              farg_type = farg_type ? farg_type : M->getFloat32Type();
            case FLOAT8OID:
              arg_extractor = arg_extractor ? arg_extractor : f__codon__arg_float;
              farg_type = farg_type ? farg_type : M->getFloatType();
            case TEXTOID:
              arg_extractor = arg_extractor ? arg_extractor : f__codon__arg_text;
              farg_type = farg_type ? farg_type : M->getStringType();
              // Finally...
              if (farg->is(farg_type)) {
                args.push_back(util::call(arg_extractor, {M->getInt(i)}));
              } else if (farg->is(M->getOptionalType(farg_type))) {
                args.push_back(M->Nr<TernaryInstr>(
                    util::call(f__codon__arg_is_null, {M->getInt(i)}),
                    (*M->getOptionalType(farg_type))(),
                    (*M->getOptionalType(farg_type))(
                        *(codon::ir::Value *)util::call(arg_extractor, {M->getInt(i)}))));
              } else {
                throw std::runtime_error(cppgres::fmt::format(
                    "Mismatched argument {} type (passed {}, function expects {})", i,
                    cppgres::type{.oid = type}.name(true), farg_type->getName()));
              }
              break;
            default:
              throw std::runtime_error("Unsupported arg type");
            }
            argit++;
            i++;
          }
          types::Type *returns = util::getReturnType(bodied_func);
          returns_optional = isA<types::OptionalType>(returns);

          auto *input = M->Nr<BodiedFunc>("_jit_input");
          types::Type *funcType = nullptr;
          if (returns_optional) {
            // types::Type *originalReturns = returns;
            returns = cast<types::OptionalType>(returns)->getBase();
            funcType = M->getFuncType(returns, {M->getPointerType(M->getBoolType())});
            input->realize(funcType, {"hasPtr"});

            auto *body = M->Nr<SeriesFlow>();
            auto *optional = util::makeVar(util::call(bodied_func, args), body, input);
            auto *hasMethod =
                M->getOrRealizeMethod(optional->getType(), "__has__", {optional->getType()});
            auto *valMethod =
                M->getOrRealizeMethod(optional->getType(), "__val__", {optional->getType()});
            auto *hasPtr = input->arg_back();
            auto *setitemMethod =
                M->getOrRealizeMethod(hasPtr->getType(), "__setitem__",
                                      {hasPtr->getType(), M->getIntType(),
                                       cast<types::PointerType>(hasPtr->getType())->getBase()});
            body->push_back(M->Nr<IfFlow>(
                // condition:
                //   optional.__has__()
                util::call(hasMethod, {M->Nr<VarValue>(optional)}),
                // true-branch:
                //   hasPtr[0] = true
                //   return optional.__val__()
                util::series(
                    util::call(setitemMethod,
                               {M->Nr<VarValue>(hasPtr), M->getInt(0), M->getBool(true)}),
                    M->Nr<ReturnInstr>(util::call(valMethod, {M->Nr<VarValue>(optional)}))),
                // false-branch:
                //   hasPtr[0] = false
                util::series(util::call(
                    setitemMethod, {M->Nr<VarValue>(hasPtr), M->getInt(0), M->getBool(false)}))));

            input->setBody(body);
          } else {
            funcType = M->getFuncType(returns, {});
            input->realize(funcType, {});
            input->setBody(util::series(M->Nr<ReturnInstr>(util::call(bodied_func, args))));
          }
          input->setJIT();

          auto compiled = jit.compile(input, resource_tracker);
          if (!compiled) {
            auto address = jit.address(input, resource_tracker);
            if (address) {
              result = address.get();
              function_cache.insert({hash, {result, returns_optional}});
            } else {
              throw std::runtime_error(llvm::toString(address.takeError()));
            }
          } else {
            throw std::runtime_error(llvm::toString(std::move(compiled)));
          }
          break;
        }
      }
    }
  }
  if (result == nullptr) {
    cppgres::report(ERROR, "can't find matching function");
  }
  if (returns_optional) {
    bool is_present = false;
    switch (call_info->return_type().oid) {
    case BOOLOID: {
      auto val = cppgres::value(
          cppgres::into_nullable_datum(reinterpret_cast<bool (*)(bool *)>(result)(&is_present)),
          call_info->return_type());
      auto res =
          cppgres::value(is_present ? cppgres::into_nullable_datum(val) : cppgres::nullable_datum(),
                         call_info->return_type());
      return res;
    }
    case INT2OID: {
      auto val = cppgres::value(
          cppgres::into_nullable_datum(reinterpret_cast<int16_t (*)(bool *)>(result)(&is_present)),
          call_info->return_type());
      auto res =
          cppgres::value(is_present ? cppgres::into_nullable_datum(val) : cppgres::nullable_datum(),
                         call_info->return_type());
      return res;
    }
    case INT4OID: {
      auto val = cppgres::value(
          cppgres::into_nullable_datum(reinterpret_cast<int32_t (*)(bool *)>(result)(&is_present)),
          call_info->return_type());
      auto res =
          cppgres::value(is_present ? cppgres::into_nullable_datum(val) : cppgres::nullable_datum(),
                         call_info->return_type());
      return res;
    }
    case INT8OID: {
      auto val = cppgres::value(
          cppgres::into_nullable_datum(reinterpret_cast<int64_t (*)(bool *)>(result)(&is_present)),
          call_info->return_type());
      auto res =
          cppgres::value(is_present ? cppgres::into_nullable_datum(val) : cppgres::nullable_datum(),
                         call_info->return_type());
      return res;
    }
    case FLOAT4OID: {
      auto val = cppgres::value(
          cppgres::into_nullable_datum(reinterpret_cast<float (*)(bool *)>(result)(&is_present)),
          call_info->return_type());
      auto res =
          cppgres::value(is_present ? cppgres::into_nullable_datum(val) : cppgres::nullable_datum(),
                         call_info->return_type());
      return res;
    }
    case FLOAT8OID: {
      auto val = cppgres::value(
          cppgres::into_nullable_datum(reinterpret_cast<double (*)(bool *)>(result)(&is_present)),
          call_info->return_type());
      auto res =
          cppgres::value(is_present ? cppgres::into_nullable_datum(val) : cppgres::nullable_datum(),
                         call_info->return_type());
      return res;
    }
    case TEXTOID: {
      struct s {
        int64_t len;
        char *str;
      };
      auto s_ = reinterpret_cast<s (*)(bool *)>(result)(&is_present);
      if (is_present) {
        return cppgres::value(cppgres::into_nullable_datum(std::string_view(s_.str, s_.len)),
                              call_info->return_type());
      }
      return cppgres::value(cppgres::nullable_datum(), call_info->return_type());
    }
    default:
      throw std::runtime_error("Unsupported return type");
    }
  }
  switch (call_info->return_type().oid) {
  case BOOLOID:
    return cppgres::value(cppgres::into_nullable_datum(reinterpret_cast<bool (*)()>(result)()),
                          call_info->return_type());
  case INT2OID:
    return cppgres::value(cppgres::into_nullable_datum(reinterpret_cast<int16_t (*)()>(result)()),
                          call_info->return_type());
  case INT4OID: {
    auto res =
        cppgres::value(cppgres::into_nullable_datum(reinterpret_cast<int32_t (*)()>(result)()),
                       call_info->return_type());
    return res;
  }
  case INT8OID: {
    auto res =
        cppgres::value(cppgres::into_nullable_datum(reinterpret_cast<int64_t (*)()>(result)()),
                       call_info->return_type());
    return res;
  }
  case FLOAT4OID: {
    auto res = cppgres::value(cppgres::into_nullable_datum(reinterpret_cast<float (*)()>(result)()),
                              call_info->return_type());
    return res;
  }
  case FLOAT8OID: {
    auto res =
        cppgres::value(cppgres::into_nullable_datum(reinterpret_cast<double (*)()>(result)()),
                       call_info->return_type());
    return res;
  }
  case TEXTOID: {
    struct s {
      int64_t len;
      char *str;
    };
    auto s_ = reinterpret_cast<s (*)()>(result)();
    return cppgres::value(cppgres::into_nullable_datum(std::string_view(s_.str, s_.len)),
                          call_info->return_type());
  }
  default:
    throw std::runtime_error("Unsupported return type");
  }
}

postgres_function(codon_call_handler, codon_call_handler_impl);

void codon_init();

extern "C" void _PG_init(void) { codon_init(); }
