// RUN: %clang_cc1 -fsyntax-only -fexperimental-new-constant-interpreter

namespace std {

template<typename T>
struct try_traits;

} // namespace std

struct ValueOrError {
  int Value;
  constexpr ValueOrError(int Value) : Value(Value) {}
};

template<>
struct std::try_traits<ValueOrError> {
  static constexpr bool should_continue(const ValueOrError &VOE) noexcept {
    return VOE.Value >= 0;
  }

  static constexpr int extract_continue(const ValueOrError &VOE) {
    return VOE.Value;
  }

  static constexpr int extract_break(const ValueOrError &VOE) {
    return VOE.Value;
  }

  static constexpr ValueOrError from_break(int Error) {
    return { Error };
  }
};

template<int V1>
constexpr ValueOrError f(int V2) {
  return ValueOrError(V1)!? + ValueOrError(V2)!?;
}

static_assert(f<+1>(+1).Value == +2);
static_assert(f<+1>(-1).Value == -1);
static_assert(f<-1>(+1).Value == -1);
static_assert(f<-1>(-1).Value == -1);
