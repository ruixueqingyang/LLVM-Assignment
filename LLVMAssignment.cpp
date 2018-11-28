//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ToolOutputFile.h>

#include <llvm/Transforms/Scalar.h>

#include <llvm/IR/Function.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#if LLVM_VERSION_MAJOR >= 4
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>

#else
#include <llvm/Bitcode/ReaderWriter.h>
#endif
using namespace llvm;

/*********add**********/
#include <iostream>
#include <list>
#include <llvm/IR/BasicBlock.h>   //  BasicBlock
#include <llvm/IR/Function.h>     //  Function
#include <llvm/IR/Instruction.h>  //  Instruction
#include <llvm/IR/Instructions.h> //  CallInst  PHINode
#include <llvm/IR/Module.h>       //  Moudle
#include <llvm/IR/User.h>         //  User
using namespace std;
/*********add**********/

#if LLVM_VERSION_MAJOR >= 4
static ManagedStatic<LLVMContext> GlobalContext;
static LLVMContext &getGlobalContext() { return *GlobalContext; }
#endif
/* In LLVM 5.0, when  -O0 passed to clang , the functions generated with clang
 * will have optnone attribute which would lead to some transform passes
 * disabled, like mem2reg.
 */
#if LLVM_VERSION_MAJOR == 5
struct EnableFunctionOptPass : public FunctionPass {
  static char ID;
  EnableFunctionOptPass() : FunctionPass(ID) {}
  bool runOnFunction(Function &F) override {
    if (F.hasFnAttribute(Attribute::OptimizeNone)) {
      F.removeFnAttr(Attribute::OptimizeNone);
    }
    return true;
  }
};

char EnableFunctionOptPass::ID = 0;
#endif

///!TODO TO BE COMPLETED BY YOU FOR ASSIGNMENT 2
/// Updated 11/10/2017 by fargo: make all functions
/// processed by mem2reg before this pass.
struct FuncPtrPass : public ModulePass {
  static char ID; // Pass identification, replacement for typeid
  FuncPtrPass() : ModulePass(ID) {}

  list<StringRef> funcList;
  list<Value *> valueList;

  void getPHINode(PHINode *pHINode) {
    // incoming_values()的返回值类型是op_range，user类型
    for (Value *inComingVal : pHINode->incoming_values()) {

      if (pHINode = dyn_cast<PHINode>(inComingVal)) {
        errs() << "PHINode / getPHINode"
               << "\n";
        getPHINode(pHINode);
      } else if (Argument *argument = dyn_cast<Argument>(inComingVal)) {
        errs() << "Argument / getPHINode"
               << "\n";
      } else if (Function *func = dyn_cast<Function>(inComingVal)) {
        errs() << "Function / getPHINode"
               << "\n";
        funcList.push_back(func->getName());
        errs() << func->getName() << "\n";
      }
    }
  }
  /*
   * User: 所有llvm节点的基类,
   *对User类的操作都直接作用于其所指向的llvm的Value类型
   *
   **/
  void getArgument(Argument *arg) {
    // Return the index of this formal argument
    int index = arg->getArgNo();
    errs() << "index " << index << "\n";
    Function *parentFunc = arg->getParent();
    // iterator_range< use_iterator >   uses (),
    for (User *user : parentFunc->users()) {
      if (CallInst *callInst = dyn_cast<CallInst>(user)) {
        // 获得参数操作数
        Value *value = callInst->getArgOperand(index);
        if (Function *func = dyn_cast<Function>(value)) {
          errs() << "Function / CallInst / getArgument"
                 << "\n";
          funcList.push_back(func->getName());
        } else if (PHINode *pHINode = dyn_cast<PHINode>(value)) {
          errs() << "PHINode / CallInst / getArgument"
                 << "\n";
          getPHINode(pHINode);
        } else if (LoadInst *loadInst = dyn_cast<LoadInst>(value)) {
          errs() << "LoadInst / CallInst / getArgument"
                 << "\n";
        } else if (Argument *argument = dyn_cast<Argument>(value)) {
          errs() << "Argument / CallInst / getArgument"
                 << "\n";
          getArgument(argument);
        }
      }
      // else if (PHINode *pHINode = dyn_cast<PHINode>(user)) {
      //   for (User *user : pHINode->users()) {
      //     if (CallInst *callInst = dyn_cast<CallInst>(user)) {
      //       Value *value = callInst->getOperand(index);
      //       if (Function *func = dyn_cast<Function>(value)) {
      //         errs() << "Function / PHINode / getArgument"
      //                << "\n";
      //         funcList.push_back(func->getName());
      //         errs() << func->getName() << "\n";
      //       }
      //     }
      //   }
      // }
    }
  }

  // show the result
  void showResult(unsigned line) {
    errs() << line << " : ";
    auto it = funcList.begin();
    auto ie = funcList.end();
    if (it != ie) {
      errs() << *it;
      ++it;
    }
    for (; it != ie; ++it) {
      errs() << ", " << *it;
    }
    errs() << "\n";
  }
  /** Value: the base class of all values
   * Argument: an incoming formal argument to a Function
   * PHINode:
   * CallInst: function call
   */
  void getFunction(CallInst *callInst) {
    // Value　is the base class of all values
    Value *value = callInst->getCalledValue();
    if (LoadInst *loadInst = dyn_cast<LoadInst>(value)) {
      errs() << "LoadInst / getFunction"
             << "\n";
    } else if (PHINode *pHINode = dyn_cast<PHINode>(value)) {
      errs() << "PHINode / getFunction"
             << "\n";
      getPHINode(pHINode);
    } else if (Argument *argument = dyn_cast<Argument>(value)) {
      errs() << "Argument / getFunction"
             << "\n";
      getArgument(argument);
    } else if (CallInst *callInst = dyn_cast<CallInst>(value)) {
      errs() << "CallInst / getFunction"
             << "\n";
    }
  }

  bool runOnModule(Module &M) override {
    // errs() << "Hello: ";
    // errs().write_escaped(M.getName()) << '\n';
    // M.dump();
    // errs() << "------------------------------\n";
    // Moudle迭代
    for (Module::iterator md_i = M.begin(), md_e = M.end(); md_i != md_e;
         ++md_i) {
      // Function迭代
      for (Function::iterator func_i = md_i->begin(), func_e = md_i->end();
           func_i != func_e; ++func_i) {
        // BasicBlock迭代
        for (BasicBlock::iterator bb_i = func_i->begin(), bb_e = func_i->end();
             bb_i != bb_e; ++bb_i) {
          // 指令类型
          Instruction *inst = dyn_cast<Instruction>(bb_i);
          // Isa函数的作用是测试给定的变量是否具有指定的类型
          if (isa<CallInst>(bb_i)) {
            CallInst *callInst = dyn_cast<CallInst>(inst);
            // 返回调用的函数，如果这是间接函数调用，则返回null
            Function *func = callInst->getCalledFunction();
            // 调用函数的行号
            unsigned callLine = callInst->getDebugLoc().getLine();
            // 如果是间接引用
            if (func == NULL) {
              errs() << "************indirect function invocation************"
                     << "\n";
              getFunction(callInst);
              showResult(callLine);
            } else {
              // Returns true if the function's name starts with "llvm."
              if (!func->isIntrinsic()) {
                errs() << "************direct function invocation************"
                       << "\n";
                errs() << callLine << " : " << func->getName() << "\n";
              }
            }
          }
        }
      }
    }

    return false;
  }
};

char FuncPtrPass::ID = 0;
static RegisterPass<FuncPtrPass> X("funcptrpass",
                                   "Print function call instruction");

static cl::opt<std::string>
    InputFilename(cl::Positional, cl::desc("<filename>.bc"), cl::init(""));

int main(int argc, char **argv) {
  LLVMContext &Context = getGlobalContext();
  SMDiagnostic Err;
  // Parse the command line to read the Inputfilename
  cl::ParseCommandLineOptions(
      argc, argv, "FuncPtrPass \n My first LLVM too which does not do much.\n");

  // Load the input module
  std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
  if (!M) {
    Err.print(argv[0], errs());
    return 1;
  }

  llvm::legacy::PassManager Passes;

/// Remove functions' optnone attribute in LLVM5.0
#if LLVM_VERSION_MAJOR == 5
  Passes.add(new EnableFunctionOptPass());
#endif
  /// Transform it to SSA
  Passes.add(llvm::createPromoteMemoryToRegisterPass());

  /// Your pass to print Function and Call Instructions
  Passes.add(new FuncPtrPass());
  Passes.run(*M.get());
}
