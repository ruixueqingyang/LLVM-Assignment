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
// #include <iostream>
#include <list>
#include <llvm/IR/BasicBlock.h>   //  BasicBlock
#include <llvm/IR/Constants.h>    //  ConstantInt
#include <llvm/IR/Function.h>     //  Function
#include <llvm/IR/Instruction.h>  //  Instruction
#include <llvm/IR/Instructions.h> //  CallInst  PHINode
#include <llvm/IR/Instructions.h> //  BranchInst
#include <llvm/IR/Module.h>       //  Moudle
#include <llvm/IR/User.h>         //  User
#include <llvm/Support/FileSystem.h>
// #include <llvm/Support/raw_ostream.h>
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
  using nameList = list<StringRef>;
  nameList funcList;
  map<int, nameList> funcMap;
  list<Value *> valueList;
  StringRef funcName;

  void getCallInst(CallInst *callInst) {
    // 返回调用的函数，如果这是间接函数调用，则返回null
    Function *func = callInst->getCalledFunction();
    // 如果是直接引用
    if (func) {
      // BasicBlock迭代
      for (Function::iterator bb_i = func->begin(), bb_e = func->end();
           bb_i != bb_e; ++bb_i) {
        // Instruction迭代
        for (BasicBlock::iterator inst_i = bb_i->begin(), inst_e = bb_i->end();
             inst_i != inst_e; ++inst_i) {
          Instruction *inst = dyn_cast<Instruction>(inst_i);
          if (ReturnInst *ret = dyn_cast<ReturnInst>(inst)) {
            Value *value = ret->getReturnValue();
            if (Argument *argument = dyn_cast<Argument>(value)) {
              getArgument(argument);
            } else if (PHINode *pHINode = dyn_cast<PHINode>(value)) {
              getPHINode(pHINode);
            } else if (CallInst *callInst = dyn_cast<CallInst>(value)) {
              getCallInst(callInst);
            }
          }
        }
      }
    } else {
      Value *value = callInst->getCalledValue();
      if (PHINode *pHINode = dyn_cast<PHINode>(value)) {
        for (Value *inComingValue : pHINode->incoming_values()) {
          if (Function *func = dyn_cast<Function>(inComingValue)) {
            // BasicBlock迭代
            for (Function::iterator bb_i = func->begin(), bb_e = func->end();
                 bb_i != bb_e; ++bb_i) {
              // Instruction迭代
              for (BasicBlock::iterator inst_i = bb_i->begin(),
                                        inst_e = bb_i->end();
                   inst_i != inst_e; ++inst_i) {
                Instruction *inst = dyn_cast<Instruction>(inst_i);
                if (ReturnInst *ret = dyn_cast<ReturnInst>(inst)) {
                  Value *value = ret->getReturnValue();
                  if (Argument *argument = dyn_cast<Argument>(value)) {
                    getArgument(argument);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  // Φ节点
  void getPHINode(PHINode *pHINode) {
    // incoming_values()的返回值类型是op_range
    for (Value *inComingVal : pHINode->incoming_values()) {
      if (pHINode = dyn_cast<PHINode>(inComingVal)) {
        getPHINode(pHINode);
      } else if (Argument *argument = dyn_cast<Argument>(inComingVal)) {
        getArgument(argument);
      } else if (Function *func = dyn_cast<Function>(inComingVal)) {
        pushBackFunc(func->getName());
      }
    }
  }

  /*
   * User: 所有llvm节点的基类,
   * 对User类的操作都直接作用于其所指向的llvm的Value类型
   **/
  void getArgument(Argument *arg) {
    // Return the index of this formal argument
    int index = arg->getArgNo();
    Function *parentFunc = arg->getParent();
    // iterator_range< use_iterator >   uses (),
    for (User *user : parentFunc->users()) {
      if (CallInst *callInst = dyn_cast<CallInst>(user)) {
        // 获得参数操作数
        Value *value = callInst->getArgOperand(index);
        if (Function *func = dyn_cast<Function>(value)) {
          pushBackFunc(func->getName());
        } else if (PHINode *pHINode = dyn_cast<PHINode>(value)) {
          getPHINode(pHINode);
        } else if (Argument *argument = dyn_cast<Argument>(value)) {
          getArgument(argument);
        }
      } else if (PHINode *pHINode = dyn_cast<PHINode>(user)) {
        for (User *user : pHINode->users()) {
          if (CallInst *callInst = dyn_cast<CallInst>(user)) {
            Value *value = callInst->getOperand(index);
            if (Function *func = dyn_cast<Function>(value)) {
              pushBackFunc(func->getName());
            } else if (PHINode *pHINode = dyn_cast<PHINode>(value)) {
              getPHINode(pHINode);
            } else if (Argument *argument = dyn_cast<Argument>(value)) {
              getArgument(argument);
            }
          }
        }
      }
    }
  }

  // push back function
  void pushBackFunc(StringRef name) {
    if (find(funcList.begin(), funcList.end(), name) == funcList.end()) {
      funcList.push_back(name);
      // errs() << name << "\n";
    }
  }

  /** Value: the base class of all values
   * Argument: an incoming formal argument to a Function
   * PHINode:
   * CallInst: function call
   */
  void getFunction(CallInst *callInst) {
    // Value　is the base class of all values
    Value *value = callInst->getCalledValue();
    if (PHINode *pHINode = dyn_cast<PHINode>(value)) {
      getPHINode(pHINode);
    } else if (Argument *argument = dyn_cast<Argument>(value)) {
      getArgument(argument);
    } else if (CallInst *callInst = dyn_cast<CallInst>(value)) {
      ;
      getCallInst(callInst);
    }
  }

  bool runOnModule(Module &M) override {
    // Function迭代
    for (Module::iterator func_i = M.begin(), func_e = M.end();
         func_i != func_e; ++func_i) {
      // BasicBlock迭代
      for (Function::iterator bb_i = func_i->begin(), bb_e = func_i->end();
           bb_i != bb_e; ++bb_i) {
        // Instruction迭代
        for (BasicBlock::iterator inst_i = bb_i->begin(), inst_e = bb_i->end();
             inst_i != inst_e; ++inst_i) {
          // 指令类型
          Instruction *inst = dyn_cast<Instruction>(inst_i);
          // Isa函数的作用是测试给定的变量是否具有指定的类型
          if (isa<CallInst>(inst_i)) {
            CallInst *callInst = dyn_cast<CallInst>(inst);
            // 返回调用的函数，如果这是间接函数调用，则返回null
            Function *func = callInst->getCalledFunction();
            // 调用函数的行号
            unsigned callLine = callInst->getDebugLoc().getLine();
            // 如果是间接引用
            if (!func) {
              funcList.clear();
              getFunction(callInst);
              funcMap.insert(pair<int, nameList>(callLine, funcList));
            } else {
              // Returns true if the function's name starts with "llvm."
              if (!func->isIntrinsic()) {
                funcList.clear();
                map<int, nameList>::iterator it = funcMap.begin(),
                                             ie = funcMap.end();
                StringRef name = func->getName();
                if (it == ie) {
                  funcList.push_back(name);
                  funcMap.insert(pair<int, nameList>(callLine, funcList));
                }
                for (; it != ie; ++it) {
                  if (it->first == callLine) {
                    it->second.push_back(func->getName());
                  } else { // 先插入一个空list,再次遍历的时候插入值
                    funcMap.insert(pair<int, nameList>(callLine, funcList));
                  }
                }
              }
            }
          }
        }
      }
    }
    // 输出结果
    for (map<int, nameList>::iterator map_it = funcMap.begin(),
                                      map_ie = funcMap.end();
         map_it != map_ie; ++map_it) {
      errs() << map_it->first << " : ";
      auto it = map_it->second.begin();
      auto ie = map_it->second.end();

      if (it != ie) {
        errs() << *it;
        ++it;
      }
      for (; it != ie; ++it) {
        errs() << ", " << *it;
      }
      errs() << "\n";
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
  bool isModified = Passes.run(*M.get());
  if (isModified) {
    errs()<<"isModified"<<"\n";
    error_code EC;
    raw_fd_ostream OS(InputFilename, EC, llvm::sys::fs::F_None);
    WriteBitcodeToFile(&(*M), OS);
    OS.flush();
  }
}
