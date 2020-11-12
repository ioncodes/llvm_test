#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/GenericValue.h>

#include <iostream>
#include <vector>
#include <memory>
#include <fstream>

int main(int argc, char* argv[])
{
    llvm::LLVMContext context;
    llvm::IRBuilder builder(context);
    auto module = std::make_unique<llvm::Module>("test", context);

    auto funcType = llvm::FunctionType::get(builder.getVoidTy(), false);
    auto mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", module.get());

    auto entry = llvm::BasicBlock::Create(context, "entrypoint", mainFunc);
    builder.SetInsertPoint(entry);

    std::vector<llvm::Type*> printfArgs = { builder.getInt8Ty()->getPointerTo() };
    llvm::ArrayRef<llvm::Type*> printfArgsRef(printfArgs);

    auto printfType = llvm::FunctionType::get(builder.getInt32Ty(), printfArgsRef, false);
    auto printfFunc = module->getOrInsertFunction("printf", printfType);

    auto helloWorldStr = builder.CreateGlobalStringPtr("Hello World!\n", "helloWorldStr");

    builder.CreateCall(printfFunc, helloWorldStr);
    builder.CreateRetVoid();

    auto success = !llvm::verifyFunction(*mainFunc, &llvm::outs());
    success = success && !llvm::verifyModule(*module, &llvm::outs());

    module->dump();

    if(!success) return -1;

    std::string ir;
    llvm::raw_string_ostream stream(ir);
    module->print(stream, nullptr);

    std::ofstream output("test.ll");
    output << ir;
    output.close();

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    std::string error;
    auto vm = llvm::EngineBuilder(std::move(module))
        .setErrorStr(&error)
        .setEngineKind(llvm::EngineKind::Interpreter)
        .create();

    if(!error.empty()) std::cout << "Error: " << error << std::endl;
    
    vm->finalizeObject();

    llvm::ArrayRef<llvm::GenericValue> mainArgsRef({ });
    vm->runFunction(mainFunc, mainArgsRef);

    return 0;
}