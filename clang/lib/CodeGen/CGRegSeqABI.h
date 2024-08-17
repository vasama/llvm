#ifndef LLVM_CLANG_LIB_CODEGEN_CGREGSEQABI_H
#define LLVM_CLANG_LIB_CODEGEN_CGREGSEQABI_H

#include "CodeGenModule.h"
#include "llvm/IR/GlobalValue.h"

namespace clang {
namespace CodeGen {

class CGRegSeqABI {
protected:
  CodeGenModule &CGM;

  CGRegSeqABI(CodeGenModule &CGM)
    : CGM(CGM) {}

public:
  virtual ~CGRegSeqABI() = default;

  virtual void emitSequence(const VarDecl &D) = 0;
  virtual void emitSequenceElement(const VarDecl &D,
                                   const VarDecl &ED,
                                   llvm::GlobalVariable *EV) = 0;
};

CGRegSeqABI* createGNURegSeqABI(CodeGenModule &CGM);

} // namespace CodeGen
} // namespace clang

#endif // LLVM_CLANG_LIB_CODEGEN_CGREGSEQABI_H
