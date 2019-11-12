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
#include <llvm/IR/MEFBody.h>
#include <llvm/IR/MEFEntry.h>
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
#include <iostream>

#include "llvm/Bitcode/BitcodeWriter.h"

using namespace llvm;
using ArgumentsTy = std::vector<Type *>;

void printASM(Module *M);

Module* makeAdd(LLVMContext& Context) {
    auto M = new Module("multi_entry.cpp", Context);

//     Declare add function
    ArgumentsTy ArgsTy(2, Type::getInt32Ty(Context));
    auto addType = FunctionType::get(Type::getInt32Ty(Context), ArgsTy, false);
    auto AddFunction = Function::Create(addType, Function::ExternalLinkage, "add", M);
    AddFunction->setCallingConv(CallingConv::C);
    // -- args
    auto args = AddFunction->arg_begin();

    auto args_X = args++;
    auto args_Y = args++;

    /*
     *
     * int add(int x, int y) {
        Entry:
            if (y < 0) {
            y_is_seven:
                y = 7;
            }
        // Entry-point-2
        return x+y;
    }
     */


    auto EntryBlock = BasicBlock::Create(Context, "Entry1", AddFunction);
    auto Y_is_Seven = BasicBlock::Create(Context, "Y_is_Seven", AddFunction);
    auto EntryBlock2 = BasicBlock::Create(Context, "Entry2", AddFunction);

    // Entry1
    IRBuilder<> builder(EntryBlock);

    // -- Alloc args to stack
    auto Stack_X = builder.CreateAlloca(Type::getInt32Ty(Context));
    auto Stack_Y = builder.CreateAlloca(Type::getInt32Ty(Context));
    builder.CreateStore(args_X, Stack_X);
    builder.CreateStore(args_Y, Stack_Y);
    auto Y = builder.CreateLoad(Type::getInt32Ty(Context), Stack_Y);

    // -- Compare with 0
    auto Zero = ConstantInt::get(Type::getInt32Ty(Context), 0);
    auto Y_LT_Zero = builder.CreateICmp(ICmpInst::ICMP_SLT, Y, Zero, "y__lt__0");
    builder.CreateCondBr(Y_LT_Zero, Y_is_Seven, EntryBlock2);

    // Y = 7 block
    builder.SetInsertPoint(Y_is_Seven);
    auto Y2 = ConstantInt::get(Type::getInt32Ty(Context), 7);
    builder.CreateStore(Y2, Stack_Y);
    builder.CreateBr(EntryBlock2);

    // Entry 2
    builder.SetInsertPoint(EntryBlock2);
    auto X = builder.CreateLoad(Type::getInt32Ty(Context), Stack_X);
    auto Y3 = builder.CreateLoad(Type::getInt32Ty(Context), Stack_Y);
    auto Addition = builder.CreateAdd(X, Y3, "addOut");
    builder.CreateRet(Addition);

    // --------------------------------------
    // == Multi entry functions
    // --------------------------------------
    //
{    auto AddBody = MEFBody::Create(Context, "add", M);
    auto ReturnType = Type::getInt32Ty(Context);

    auto ArgsEntry1Ty = ArgumentsTy {2, Type::getInt32Ty(Context)};
    auto Entry1Type = FunctionType::get(ReturnType, ArgsEntry1Ty, false);
    auto AddEntry1 = MEFEntry::Create(Entry1Type, "add1", M);

    auto ArgsEntry2Ty = ArgumentsTy {1, Type::getInt32Ty(Context)};
    auto Entry2Type = FunctionType::get(ReturnType, ArgsEntry2Ty, false);
    auto AddEntry2 = MEFEntry::Create(Entry2Type, "add2", M);

    auto EntryBlock = BasicBlock::CreateMEF(Context, "add", AddBody);
    auto EntryBlock2 = BasicBlock::CreateMEF(Context, "add2", AddBody);
    auto Y_is_Seven = BasicBlock::CreateMEF(Context, "Y_is_Seven", AddBody);
    auto LastBlock = BasicBlock::CreateMEF(Context, "Last", AddBody);
    AddEntry1->Register(EntryBlock);
    AddEntry2->Register(EntryBlock2);

    auto args1 = AddEntry1->arg_begin();
    auto args2 = AddEntry2->arg_begin();

    auto X1 = args1++; auto X2 = args2++;
    auto Y1 = args1++;

    // Entry1 add(x, y)
    auto builder = IRBuilder<>{EntryBlock};

    // -- Alloc args to stack
    // debug
    // debug
    std::cout << &(Context) << '\n';
    std::cout << &(X1->getContext()) << '\n';
    std::cout << std::endl;

    auto Stack_X = builder.CreateAlloca(Type::getInt32Ty(Context));
    auto Stack_Y = builder.CreateAlloca(Type::getInt32Ty(Context));
    builder.CreateStore(X1, Stack_X);
    builder.CreateStore(Y1, Stack_Y);
    auto Y = builder.CreateLoad(Type::getInt32Ty(Context), Stack_Y);

    // -- Compare with 0
    auto Zero = ConstantInt::get(Type::getInt32Ty(Context), 0);
    auto Y_LT_Zero = builder.CreateICmp(ICmpInst::ICMP_SLT, Y, Zero, "y__lt__0");
    builder.CreateCondBr(Y_LT_Zero, Y_is_Seven, LastBlock);

    //
    // Block: Entry2 add2(x)
    //
    builder.SetInsertPoint(EntryBlock2);
    builder.CreateStore(X2, Stack_X);
    builder.CreateBr(Y_is_Seven);

    // Y = 7 block
    builder.SetInsertPoint(Y_is_Seven);
    auto Y2 = ConstantInt::get(Type::getInt32Ty(Context), 7);
    builder.CreateStore(Y2, Stack_Y);
    builder.CreateBr(LastBlock);

    // Last
    builder.SetInsertPoint(LastBlock);
    auto X = builder.CreateLoad(Type::getInt32Ty(Context), Stack_X);
    auto Y3 = builder.CreateLoad(Type::getInt32Ty(Context), Stack_Y);
    auto Addition = builder.CreateAdd(X, Y3, "addOut");
    builder.CreateRet(Addition);
}
    return M;
}


int main() {
    LLVMContext Context;
    Context.setDiscardValueNames(true);
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

