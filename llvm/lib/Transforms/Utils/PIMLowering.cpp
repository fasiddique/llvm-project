//===-- PIMLowering.cpp - Lowers PIM Specific Intrinsics to external function call --------------------------===//
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

CallInst *replaceAddIntrinsic(IntrinsicInst *II, llvm::Module *mod) {
  Function *PAdd = Function::Create(II->getFunctionType(),
                                    Function::ExternalLinkage, "pimAdd", mod);
  std::vector<Value *> PAddArgs;
  for (uint16_t i = 0; i < II->getFunctionType()->getNumParams(); i++) {
    PAddArgs.push_back(II->getArgOperand(i));
  }
  return CallInst::Create(PAdd, PAddArgs, "", II);
}

PreservedAnalyses PIMLoweringPass::run(Function &F,
                                       FunctionAnalysisManager &AM) {
  llvm::Module *mod = F.getParent();
  std::vector<Instruction *> instrToDel;
  for (auto &B : F) {
    for (auto &I : B) {
      IntrinsicInst *II = dyn_cast<IntrinsicInst>(&I);
      if (II) {
        switch (II->getIntrinsicID()) {
        case Intrinsic::PIMIntrinsics::pim_add: {
          I.replaceAllUsesWith(
              dyn_cast<Instruction>(replaceAddIntrinsic(II, mod)));
          instrToDel.push_back(&I);
          break;
        }
        default:
          break;
        }
      }
    }
  }

  for (auto ins : instrToDel) {
    ins->eraseFromParent();
  }
  instrToDel.clear();

  return PreservedAnalyses::all();
}
