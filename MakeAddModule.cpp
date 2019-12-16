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
//{
////     Declare add function
//    ArgumentsTy ArgsTy(2, Type::getInt64Ty(Context));
//    auto addType = FunctionType::get(Type::getInt64Ty(Context), ArgsTy, false);
//    auto AddFunction = Function::Create(addType, Function::ExternalLinkage, "add", M);
//    AddFunction->addFnAttr(Attribute::NoUnwind);
//    // -- args
//    auto args = AddFunction->arg_begin();
//
//    auto args_X = args++;
//    auto args_Y = args++;
//
//    /*
//     *
//     * int add(int x, int y) {
//        Entry:
//            if (y < 0) {
//            y_is_seven:
//                y = 7;
//            }
//        // Entry-point-2
//        return x+y;
//    }
//     */
//
//
//    auto EntryBlock = BasicBlock::Create(Context, "Entry1", AddFunction);
//    auto Y_is_Seven = BasicBlock::Create(Context, "Y_is_Seven", AddFunction);
//    auto EntryBlock2 = BasicBlock::Create(Context, "Entry2", AddFunction);
//
//    // Entry1
//    IRBuilder<> builder(EntryBlock);
//
//    // -- Alloc args to stack
//    auto Stack_X = builder.CreateAlloca(Type::getInt64Ty(Context));
//    auto Stack_Y = builder.CreateAlloca(Type::getInt64Ty(Context));
//    builder.CreateStore(args_X, Stack_X);
//    builder.CreateStore(args_Y, Stack_Y);
//    auto Y = builder.CreateLoad(Type::getInt64Ty(Context), Stack_Y);
//
//    // -- Compare with 0
//    auto Zero = ConstantInt::get(Type::getInt64Ty(Context), 0);
//    auto Y_LT_Zero = builder.CreateICmp(ICmpInst::ICMP_SLT, Y, Zero, "y__lt__0");
//    builder.CreateCondBr(Y_LT_Zero, Y_is_Seven, EntryBlock2);
//
//    // Y = 7 block
//    builder.SetInsertPoint(Y_is_Seven);
//    auto Y2 = ConstantInt::get(Type::getInt64Ty(Context), 7);
//    builder.CreateStore(Y2, Stack_Y);
//    builder.CreateBr(EntryBlock2);
//
//    // Entry 2
//    builder.SetInsertPoint(EntryBlock2);
//    auto X = builder.CreateLoad(Type::getInt64Ty(Context), Stack_X);
//    auto Y3 = builder.CreateLoad(Type::getInt64Ty(Context), Stack_Y);
//    auto Addition = builder.CreateAdd(X, Y3, "addOut");
//    builder.CreateRet(Addition);
//}
{
    auto AddBody = MEFBody::Create(Context, "add", M);
    auto ReturnType = Type::getInt64Ty(Context);

    auto ArgsEntry1Ty = ArgumentsTy {2, Type::getInt64Ty(Context)};
    auto Entry1Type = FunctionType::get(ReturnType, ArgsEntry1Ty, false);
    auto AddEntry1 = MEFEntry::Create(Entry1Type, "add1", M);

    auto ArgsEntry2Ty = ArgumentsTy {1, Type::getInt64Ty(Context)};
    auto Entry2Type = FunctionType::get(ReturnType, ArgsEntry2Ty, false);
    auto AddEntry2 = MEFEntry::Create(Entry2Type, "add2", M);

    auto EntryBlock = BasicBlock::CreateMEF(Context, "add", AddBody);
    auto EntryBlock2 = BasicBlock::CreateMEF(Context, "add2", AddBody);
    auto Y_is_Seven = BasicBlock::CreateMEF(Context, "Y_is_Seven", AddBody);
    auto LastBlock = BasicBlock::CreateMEF(Context, "Last", AddBody);
    auto UnreachableBlock = BasicBlock::CreateMEF(Context, "Im-Unreachable", AddBody);
    auto UnreachableBlock2 = BasicBlock::CreateMEF(Context, "Im-UnreachableAlso", AddBody);
    AddEntry1->Register(EntryBlock);
    AddEntry2->Register(EntryBlock2);

    auto args1 = AddEntry1->arg_begin();
    auto args2 = AddEntry2->arg_begin();

    auto X1 = args1++; auto X2 = args2++;
    auto Y1 = args1++;

    // Entry1 add(x, y)
    auto builder = IRBuilder<>{EntryBlock};

    // -- Alloc args to stack

    auto Stack_X = builder.CreateAlloca(Type::getInt64Ty(Context));
    auto Stack_Y = builder.CreateAlloca(Type::getInt64Ty(Context));
    builder.CreateStore(X1, Stack_X);
    builder.CreateStore(Y1, Stack_Y);
    auto Y = builder.CreateLoad(Type::getInt64Ty(Context), Stack_Y);

    // -- Compare with 0
    auto Zero = ConstantInt::get(Type::getInt64Ty(Context), 0);
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
    auto Y2 = ConstantInt::get(Type::getInt64Ty(Context), 7);
    builder.CreateStore(Y2, Stack_Y);
    builder.CreateBr(LastBlock);

    // Last
    builder.SetInsertPoint(LastBlock);
    auto X = builder.CreateLoad(Type::getInt64Ty(Context), Stack_X);
    auto Y3 = builder.CreateLoad(Type::getInt64Ty(Context), Stack_Y);
    auto Addition = builder.CreateAdd(X, Y3, "addOut");
    builder.CreateRet(Addition);
}
    // --------------------------------------
    // == Multi entry functions
    // --------------------------------------
    //

    /*
     *
     * int add{
        increment (int x):
            x = x; y = 1
        add (int x, int y):
            pass
        return x+y;
    }
     */
//    {
//        //
//        // Increment
//        //
//        ArgumentsTy ArgsTy(1, Type::getInt64Ty(Context));
//        auto ReturnType = Type::getInt64Ty(Context);
//        auto IncType = FunctionType::get(ReturnType, ArgsTy, false);
//        auto Increment = Function::Create(IncType, Function::ExternalLinkage, "inc", M);
//        Increment->addFnAttr(Attribute::NoUnwind);
//
//        auto IncrementEntryBlock = BasicBlock::Create(Context, "IncrementEntryBlock", Increment);
//        auto IncrementRetBlock = BasicBlock::Create(Context, "IncrementRetBlock", Increment);
//
//        auto Increment_Args = Increment->arg_begin();
//        auto X_Increment = Increment_Args++;
//        IRBuilder<> builder(IncrementEntryBlock);
//
//        // Entry1 increment(x)
//        auto One = ConstantInt::get(Type::getInt64Ty(Context), 1);
//        auto Zero1 = ConstantInt::get(Type::getInt64Ty(Context), 0);
//        auto Zero2 = ConstantInt::get(Type::getInt64Ty(Context), 0);
//        auto X1 = builder.CreateAdd(Zero1, X_Increment);
//        auto Y1 = builder.CreateAdd(Zero2, One);
//        builder.CreateBr(IncrementRetBlock);
//
//        builder.SetInsertPoint(IncrementRetBlock);
//        auto X_inc = builder.CreatePHI(Type::getInt64Ty(Context), 1);
//        X_inc->addIncoming(X1, IncrementEntryBlock);
//
//        auto Y_inc = builder.CreatePHI(Type::getInt64Ty(Context), 1);
//        Y_inc->addIncoming(Y1, IncrementEntryBlock);
//        auto Z_inc = builder.CreateAdd(X_inc, Y_inc);
//        builder.CreateRet(Z_inc);
//    //
//    // Plus
//    //
//    ArgumentsTy ArgsTyPlus(2, Type::getInt64Ty(Context));
//    auto ReturnTypePlus = Type::getInt64Ty(Context);
//    auto PlusType = FunctionType::get(ReturnTypePlus, ArgsTyPlus, false);
//    auto Plus = Function::Create(PlusType, Function::ExternalLinkage, "plus", M);
//    Plus->addFnAttr(Attribute::NoUnwind);
//
//    auto PlusEntryBlock = BasicBlock::Create(Context, "PlusEntryBlock", Plus);
//    auto PlusRetBlock = BasicBlock::Create(Context, "PlusReturnBlock", Plus);
//
//    auto Plus_Args = Plus->arg_begin();
//    auto X2_Plus = Plus_Args++;
//    auto Y2_Plus = Plus_Args++;
//    builder.SetInsertPoint(PlusEntryBlock);
//
//    // Entry1 increment(x)
//        auto Zero3 = ConstantInt::get(Type::getInt64Ty(Context), 0);
//        auto Zero4 = ConstantInt::get(Type::getInt64Ty(Context), 0);
//    auto X2 = builder.CreateAdd(Zero3, X2_Plus);
//    auto Y2 = builder.CreateAdd(Zero4, Y2_Plus);
//    builder.CreateBr(PlusRetBlock);
//
//    // Return block: return x+y
//    builder.SetInsertPoint(PlusRetBlock);
//    auto X_plus = builder.CreatePHI(Type::getInt64Ty(Context), 1);
//    X_plus->addIncoming(X2, PlusEntryBlock);
//    auto Y_plus = builder.CreatePHI(Type::getInt64Ty(Context), 1);
//    Y_plus->addIncoming(Y2, PlusEntryBlock);
//
//    auto Z_plus = builder.CreateAdd(X_plus, Y_plus, "Plus_Return");
//    builder.CreateRet(Z_plus);
//
//    //
//    // Main
//    //
//    ArgumentsTy ArgsTyMain(0);
//    auto ReturnTypeMain = Type::getInt64Ty(Context);
//    auto MainTy = FunctionType::get(ReturnTypeMain, ArgsTyMain, false);
//    auto Main = Function::Create(MainTy, Function::ExternalLinkage, "main", M);
//    Main->addFnAttr(Attribute::NoUnwind);
//    auto EntryBlock = BasicBlock::Create(Context, "MainEntry", Main);
//    builder.SetInsertPoint(EntryBlock);
//
//    auto Arg = ConstantInt::get(Type::getInt64Ty(Context), 7);
//    std::vector<Value *> IncArgs = {Arg};
//    auto Ret1 = builder.CreateCall(Increment, IncArgs);
////
////    auto Arg2 = ConstantInt::get(Type::getInt64Ty(Context), 11);
////    auto Arg3 = ConstantInt::get(Type::getInt64Ty(Context), 9);
////    std::vector<Value *> PlusArgs = {Arg2, Arg3};
////    auto Ret2 = builder.CreateCall(Plus, PlusArgs);
//
//    auto Ret = builder.CreateAdd(Ret1, Ret1);
//    builder.CreateRet(Ret);
//}
//{
//    auto AddBody = MEFBody::Create(Context, "addMEF", M);
//    auto ReturnType1 = Type::getInt64Ty(Context);
//    auto ReturnType2 = Type::getInt64Ty(Context);
//
//    auto IncrementArgsTy = ArgumentsTy {1, Type::getInt64Ty(Context)};
//    auto IncrementType = FunctionType::get(ReturnType1, IncrementArgsTy, false);
//    auto Increment = MEFEntry::Create(IncrementType, "incrementMEF", M);
//
//    auto PlusArgsTy = ArgumentsTy {2, Type::getInt64Ty(Context)};
//    auto PlusType = FunctionType::get(ReturnType2, PlusArgsTy, false);
//    auto Plus = MEFEntry::Create(PlusType, "plusMEF", M);
//
//    auto IncrementEntryBlock = BasicBlock::CreateMEF(Context, "IncrementEntry", AddBody);
//    auto PlusEntryBlock = BasicBlock::CreateMEF(Context, "PlusEntry", AddBody);
//    auto AddBlock = BasicBlock::CreateMEF(Context, "AddBlock", AddBody);
//    Increment->Register(IncrementEntryBlock);
//    Plus->Register(PlusEntryBlock);
//
//    auto args1 = Increment->arg_begin();
//    auto args2 = Plus->arg_begin();
//
//    auto X1_arg = args1++;
//    auto X2_arg = args2++; auto Y2_arg = args2++;
//    IRBuilder<> builder(IncrementEntryBlock);
//
//    // Entry1 increment(x)
//    auto One = ConstantInt::get(Type::getInt64Ty(Context), 1);
//    auto Zero1 = ConstantInt::get(Type::getInt64Ty(Context), 0);
//    auto Zero2 = ConstantInt::get(Type::getInt64Ty(Context), 0);
//    auto X1 = builder.CreateAdd(Zero1, X1_arg);
//    auto Y1 = builder.CreateAdd(Zero2, One);
//    builder.CreateBr(AddBlock);
//
//    // Entry2: plus(x, y)
//    //
//    builder.SetInsertPoint(PlusEntryBlock);
//    auto Zero3 = ConstantInt::get(Type::getInt64Ty(Context), 0);
//    auto Zero4 = ConstantInt::get(Type::getInt64Ty(Context), 0);
//    auto X2 = builder.CreateAdd(Zero3, X2_arg);
//    auto Y2 = builder.CreateAdd(Zero4, Y2_arg);
//    builder.CreateBr(AddBlock);
//
//    // Return block
//    builder.SetInsertPoint(AddBlock);
//    auto X = builder.CreatePHI(Type::getInt64Ty(Context), 2);
//    X->addIncoming(X1, IncrementEntryBlock);
//    X->addIncoming(X2, PlusEntryBlock);
//
//    auto Y = builder.CreatePHI(Type::getInt64Ty(Context), 2);
//    Y->addIncoming(Y1, IncrementEntryBlock);
//    Y->addIncoming(Y2, PlusEntryBlock);
//
//    auto Z = builder.CreateAdd(X, Y, "Z");
//    builder.CreateRet(Z);
//}
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
//    PM.run(*M);
    PM.runMEF(*M);
}

