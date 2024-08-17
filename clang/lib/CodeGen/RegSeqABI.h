#ifndef LLVM_CLANG_LIB_CODEGEN_REGSEQABI_H
#define LLVM_CLANG_LIB_CODEGEN_REGSEQABI_H

#include "CodeGenModule.h"

namespace clang {
namespace CodeGen {

class RegSeqABI {
  CodeGenModule &CGM;

public:
  explicit RegSeqABI(CodeGenModule &CGM) :
    CGM(CGM) {}

  
};

} // namespace CodeGen
} // namespace clang

#endif // LLVM_CLANG_LIB_CODEGEN_REGSEQABI_H
