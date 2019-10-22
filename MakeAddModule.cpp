//===- examples/ModuleMaker/ModuleMaker.cpp - Example project ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This programs is a simple example that creates an LLVM module "from scratch",
// emitting it as a bitcode file to standard out.  This is just to show how
// LLVM projects work and to demonstrate some of the LLVM APIs.
//
//===----------------------------------------------------------------------===//
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>

#include <llvm/MC/MCContext.h>
#include <llvm/Target/TargetMachine.h>

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>

using namespace llvm;
using ArgumentsTy = std::vector<Type *>;

void printASM(Module *M);

Module* makeAdd(LLVMContext& Context) {
    auto M = new Module("multi_entry.cpp", Context);

    // Declare add function
    ArgumentsTy ArgsTy(2, Type::getInt32Ty(Context));
    auto addType = FunctionType::get(Type::getInt32Ty(Context), ArgsTy, false);
    auto AddFunction = Function::Create(addType, Function::ExternalLinkage, "add", M);

    // -- args
    auto args = AddFunction->arg_begin();

    auto X = args++; X->setName("x");
    auto Y = args++; Y->setName("y");

    /*
     *
     * int add(int y, int x) {
        Entry:
            if (y < 0) {
            y_is_seven:
                y = 7;
            }
        // Entry-point-2
        y = 10;
        return y+x;
    }
     */
    auto EntryBlock = BasicBlock::Create(Context, "Entry1", AddFunction);
    auto Y_is_Seven = BasicBlock::Create(Context, "Y_is_Seven", AddFunction);
    auto EntryBlock2 = BasicBlock::Create(Context, "Entry2", AddFunction);

    // Entry1
    IRBuilder<> builder(EntryBlock);

    // -- compare
    auto Zero = ConstantInt::get(Type::getInt32Ty(Context), 0);
    auto Y_LT_Zero = builder.CreateICmp(ICmpInst::ICMP_SLT, Y, Zero, "y__lt__0");
    builder.CreateCondBr(Y_LT_Zero, Y_is_Seven, EntryBlock2);

    // Y = 7 block
    builder.SetInsertPoint(Y_is_Seven);
    auto Y2 = ConstantInt::get(Type::getInt32Ty(Context), 7);
    builder.CreateBr(EntryBlock2);

    // Entry 2
    builder.SetInsertPoint(EntryBlock2);
    PHINode *Y3 = builder.CreatePHI(Y->getType(), 2);
    Y3->addIncoming(Y, EntryBlock);
    Y3->addIncoming(Y2, Y_is_Seven);

    auto Addition = builder.CreateAdd(X, Y3, "addOut");
    builder.CreateRet(Addition);

    return M;
}

int main() {

    LLVMContext Context;
    Module *M = makeAdd(Context);
    verifyModule(*M, &outs());
    printASM(M);

    return 0;
}

void printASM(Module *M) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    auto TargetTriple = sys::getDefaultTargetTriple();
    M->setTargetTriple(TargetTriple);

    std::string Error;
    const Target *target = TargetRegistry::lookupTarget(TargetTriple, Error);
    auto cpu = sys::getHostCPUName();
    SubtargetFeatures Features;
    StringMap<bool> HostFeatures;
    if (sys::getHostCPUFeatures(HostFeatures))
        for (auto &F : HostFeatures)
            Features.AddFeature(F.first(), F.second);
    auto features = Features.getString();

    TargetOptions Options;
    std::unique_ptr<TargetMachine> TM{
            target->createTargetMachine(
                    TargetTriple, cpu, features, Options,
                    Reloc::PIC_, None, CodeGenOpt::None)
    };

    legacy::PassManager PM;
    M->setDataLayout(TM->createDataLayout());
    TM->addPassesToEmitFile(PM, (raw_pwrite_stream &) outs(), (raw_pwrite_stream *) (&outs()),
                            TargetMachine::CodeGenFileType::CGFT_AssemblyFile, true, nullptr);
    PM.run(*M);
}

