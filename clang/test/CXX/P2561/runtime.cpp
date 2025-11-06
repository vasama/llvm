// RUN: %clang_cc1 %s -triple=x86_64-linux-gnu -emit-llvm -o- | FileCheck %s

// CHECK: @_Z1fv()
// CHECK: [[COND:%[\._a-zA-Z0-9]+]] = call
// CHECK-SAME: @_ZNSt10try_traitsI4typeE15should_continueERKS0_
// CHECK: br i1 [[COND]]
// CHECK-SAME: label %[[CONTINUE:[\._a-zA-Z0-9]+]]
// CHECK-SAME: label %[[BREAK:[\._a-zA-Z0-9]+]]

// CHECK: [[BREAK]]:
// CHECK: call
// CHECK-SAME: @_ZNSt10try_traitsI4typeE13extract_breakERKS0_
// CHECK: call
// CHECK-SAME: @_ZNSt10try_traitsI4typeE10from_breakEi
// CHECK: br
// CHECK-SAME: label %[[RETURN:[\._a-zA-Z0-9]+]]

// CHECK: [[CONTINUE]]:
// CHECK: call
// CHECK-SAME: @_ZNSt10try_traitsI4typeE16extract_continueERKS0_
// CHECK: call
// CHECK-SAME: @_Z1ei
// CHECK: br
// CHECK-SAME: label %[[RETURN]]

// CHECK: [[RETURN]]:
// CHECK: ret void

namespace std {

template<typename T>
struct try_traits;

} // namespace std

class type {};

template<>
struct std::try_traits<type> {
  static bool should_continue(const type&) noexcept;
  static int extract_continue(const type&);
  static int extract_break(const type&);
  static type from_break(int);
};

type e(int);

type f() {
  return e(type()!?);
}
