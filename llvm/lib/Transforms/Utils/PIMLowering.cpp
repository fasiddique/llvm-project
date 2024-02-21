//===-- PIMLowering.cpp - Lowers PIM Specific Intrinsics to external function
// call --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/PIMLowering.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IntrinsicsPIM.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

std::vector<Instruction *> InstructionToDelete;

void replaceAddIntrinsic(IntrinsicInst *II, llvm::Module *module) {

  std::vector<Value *> PAddArgs;
  std::vector<Type *> PAddArgTypes;
  for (uint16_t i = 0; i < II->getFunctionType()->getNumParams(); i++) {
    auto *val = II->getArgOperand(i);
    if (isa<LoadInst>(val)) {
      LoadInst *LI = dyn_cast<LoadInst>(val);
      PAddArgTypes.push_back(LI->getPointerOperandType());
      PAddArgs.push_back(LI->getPointerOperand());
    }
  }
  Instruction *InsertBeforeInstruction = II;
  for (auto U : II->users()) {
    if (isa<StoreInst>(U)) {
      StoreInst *SI = dyn_cast<StoreInst>(U);
      PAddArgTypes.push_back(SI->getPointerOperandType());
      PAddArgs.push_back(SI->getPointerOperand());
      InstructionToDelete.push_back(SI);
      InsertBeforeInstruction = SI;
    }
  }
  FunctionType *PAddTy =
      FunctionType::get(Type::getVoidTy(II->getContext()), PAddArgTypes, false);
  Function *PAdd =
      Function::Create(PAddTy, Function::ExternalLinkage, "pimAdd", module);
  CallInst::Create(PAdd, PAddArgs, "", InsertBeforeInstruction);
  return;
}

PreservedAnalyses PIMLoweringPass::run(Function &F,
                                       FunctionAnalysisManager &AM) {
  llvm::Module *module = F.getParent();
  for (auto &B : F) {
    for (auto &I : B) {
      IntrinsicInst *II = dyn_cast<IntrinsicInst>(&I);
      if (II) {
        switch (II->getIntrinsicID()) {
        case Intrinsic::PIMIntrinsics::pim_add: {
          replaceAddIntrinsic(II, module);
          InstructionToDelete.push_back(&I);
          break;
        }
        default:
          break;
        }
      }
    }
  }

  for (auto I : InstructionToDelete) {
    I->eraseFromParent();
  }
  InstructionToDelete.clear();

  return PreservedAnalyses::all();
}
