#include "CGCXXABI.h"
#include "CGRegSeqABI.h"
#include "CodeGenFunction.h"

using namespace clang;
using namespace CodeGen;

namespace {

using ShortString = llvm::SmallString<256>;

struct GNURegSeqABI : CGRegSeqABI {
  llvm::DenseSet<const VarDecl *> Sequences;
  llvm::StructType *RootType = nullptr;
  llvm::StructType *NodeType = nullptr;
  llvm::FunctionCallee LinkFunction = nullptr;

  explicit GNURegSeqABI(CodeGenModule &CGM) :
    CGRegSeqABI(CGM) {}

  bool isCXX() const {
    return CGM.getLangOpts().CPlusPlus;
  }

  MangleContext &getMangleContext() const {
    assert(isCXX());
    return CGM.getCXXABI().getMangleContext();
  }

  llvm::StructType *getRootType();
  llvm::StructType *getNodeType();
  llvm::FunctionCallee getLinkFunction();

  llvm::GlobalVariable *getRoot(const VarDecl &D);

  llvm::GlobalVariable *emitNode(const VarDecl &D,
                                 llvm::StringRef NodeName,
                                 llvm::Constant *Begin,
                                 llvm::Constant *End);

  ShortString emitModuleNode(const VarDecl &D);

  void emitSequence(const VarDecl &D) override;
  void emitSequenceElement(const VarDecl &D,
                           const VarDecl &ED,
                           llvm::GlobalVariable *EV) override;
};

llvm::StructType *GNURegSeqABI::getRootType() {
  if (!RootType) {
    llvm::StructType *T = llvm::StructType::create(CGM.getLLVMContext());
    llvm::Type *P = llvm::PointerType::getUnqual(T);
    T->setBody(P, P);
    RootType = T;
  }
  return RootType;
}

llvm::StructType *GNURegSeqABI::getNodeType() {
  if (!NodeType) {
    llvm::Type *RT = getRootType();
    llvm::Type *PRT = llvm::PointerType::get(RT, 0);
    llvm::Type *PV = llvm::PointerType::getUnqual(CGM.getLLVMContext());
    NodeType = llvm::StructType::get(PRT, PV, PV);
  }
  return NodeType;
}

llvm::FunctionCallee GNURegSeqABI::getLinkFunction() {
  if (!LinkFunction) {
    llvm::Type *Args[] = {
      llvm::PointerType::get(getRootType(), 0),
      llvm::PointerType::get(getNodeType(), 0),
    };
    LinkFunction = CGM.CreateRuntimeFunction(
      llvm::FunctionType::get(CGM.VoidTy, Args), "__regseq_link");
  }
  return LinkFunction;
}

llvm::GlobalVariable *GNURegSeqABI::getRoot(const VarDecl &D) {
  return llvm::cast<llvm::GlobalVariable>(
    CGM.GetAddrOfGlobalVar(&D, getRootType(), ForDefinition_t(false)));
}

llvm::GlobalVariable *GNURegSeqABI::emitNode(const VarDecl &D,
                                                 const llvm::StringRef NodeName,
                                                 llvm::Constant *const Begin,
                                                 llvm::Constant *const End) {
  llvm::Constant *const Root = getRoot(D);

  llvm::Constant *const Init = llvm::ConstantStruct::get(
      getNodeType(), llvm::ConstantPointerNull::get(llvm::PointerType::get(getRootType(), 0)),
      Begin, End);

  llvm::GlobalVariable *const Node = new llvm::GlobalVariable(
      CGM.getModule(), getNodeType(),
      /*isConstant=*/false, llvm::GlobalValue::LinkOnceAnyLinkage, Init,
      NodeName);

  ShortString FnName;
  if (isCXX()) {
    llvm::raw_svector_ostream Out(FnName);
    getMangleContext().mangleRegisteredSequenceInit(D, Out);
  } else {
    FnName += Root->getName();
    FnName += ".regseq_init";
  }

  llvm::FunctionType *const FTy = llvm::FunctionType::get(CGM.VoidTy, false);
  const CGFunctionInfo &FI = CGM.getTypes().arrangeNullaryFunction();

  llvm::Function *const Fn = CGM.CreateGlobalInitOrCleanUpFunction(
    FTy, FnName, FI, D.getLocation());

  // Emit function
  {
    CodeGenFunction CGF(CGM);
    CGF.StartFunction(GlobalDecl(&D, DynamicInitKind::Initializer),
                      CGM.getContext().VoidTy, Fn, FI, FunctionArgList());
    CGF.EmitNounwindRuntimeCall(getLinkFunction(), { Root, Node });
    CGF.FinishFunction();
  }

  //TODO: P2889: Look deeper into the global ctor comdat.
  CGM.AddGlobalCtor(Fn, 90, ~0U, CGM.supportsCOMDAT() ? Root : nullptr);

  return Node;
}

ShortString GNURegSeqABI::emitModuleNode(const VarDecl &D) {
  ShortString SectionName;
  if (isCXX()) {
    llvm::raw_svector_ostream Out(SectionName);
    getMangleContext().mangleRegisteredSequenceSection(D, Out);
  } else {
    SectionName += getRoot(D)->getName();
    SectionName += ".regseq_section";
  }

  if (Sequences.insert(&D).second) {
    llvm::Type *const ArrayType = llvm::ArrayType::get(CGM.CharTy, 0);

    llvm::GlobalVariable *const Begin = new llvm::GlobalVariable(
        CGM.getModule(),
        ArrayType,
        /*isConstant=*/false,
        llvm::GlobalValue::ExternalLinkage,
        nullptr,
        llvm::Twine("__start_") + SectionName);

    llvm::GlobalVariable *const End = new llvm::GlobalVariable(
        CGM.getModule(),
        ArrayType,
        /*isConstant=*/false,
        llvm::GlobalValue::ExternalLinkage,
        nullptr,
        llvm::Twine("__stop_") + SectionName);

    ShortString NodeName;
    if (isCXX()) {
      llvm::raw_svector_ostream Out(NodeName);
      getMangleContext().mangleRegisteredSequenceNode(D, Out);
    } else {
      NodeName += getRoot(D)->getName();
      NodeName += ".regseq_module_node";
    }

    llvm::GlobalVariable *const Node = emitNode(D, NodeName, Begin, End);
    Node->setVisibility(llvm::GlobalValue::HiddenVisibility);
  }

  return SectionName;
}

void GNURegSeqABI::emitSequence(const VarDecl &D) {}

void GNURegSeqABI::emitSequenceElement(const VarDecl &D,
                                       const VarDecl &ED,
                                       llvm::GlobalVariable *const EV) {
  auto requiresSeparateNode = [&]() -> bool {
    if (EV->hasSection())
      return true;

    if (EV->hasImplicitSection())
      return true;

    if (EV->hasLinkOnceLinkage())
      return getRoot(D)->hasDefaultVisibility();

    return false;
  };

  if (!requiresSeparateNode()) {
    EV->setSection(emitModuleNode(D));
    return;
  }

  ShortString NodeName;
  if (isCXX()) {
    llvm::raw_svector_ostream Out(NodeName);
    getMangleContext().mangleRegisteredSequenceNode(ED, Out);
  } else {
    NodeName += EV->getName();
    NodeName += ".regseq_node";
  }

  llvm::Constant *OnePastEV = llvm::ConstantExpr::getInBoundsGetElementPtr(
    EV->getValueType(), EV, llvm::ConstantInt::get(CGM.IntTy, 1));

  emitNode(D, NodeName, EV, OnePastEV);
}

} // namespace

CGRegSeqABI* CodeGen::createGNURegSeqABI(CodeGenModule &CGM) {
  return new GNURegSeqABI(CGM);
}
