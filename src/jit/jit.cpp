/* -*- C++ -*-
 * Serene Programming Language
 *
 * Copyright (c) 2019-2023 Sameer Rahmani <lxsameer@gnu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "jit/jit.h"

#include "options.h" // for Options

#include <__type_traits/remove_reference.h> // for remov...
#include <__utility/move.h>                 // for move
#include <system_error>                     // for error_code

#include <llvm/ADT/StringMapEntry.h>                 // for StringMapEntry
#include <llvm/ADT/iterator.h>                       // for iterator_facade_base
#include <llvm/ExecutionEngine/JITEventListener.h>   // for JITEventListener
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>   // for TMOwn...
#include <llvm/ExecutionEngine/Orc/Core.h>           // for JITDy...
#include <llvm/ExecutionEngine/Orc/DebugUtils.h>     // for opera...
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h> // for Dynam...
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h> // for IRCom...
#include <llvm/ExecutionEngine/Orc/LLJIT.h>          // IWYU pragma: keep
#include <llvm/ExecutionEngine/Orc/Layer.h>          // for Objec...
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h> // for Threa...
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/DataLayout.h>          // for DataL...
#include <llvm/IR/LLVMContext.h>         // for LLVMC...
#include <llvm/IR/Module.h>              // for Module
#include <llvm/Support/FileSystem.h>     // for OpenFlags
#include <llvm/Support/ToolOutputFile.h> // for ToolOutputFile
#include <llvm/TargetParser/Triple.h>    // for Triple

#include <assert.h> // for assert
#include <string>   // for operator+, char_t...

namespace serene::jit {

// ----------------------------------------------------------------------------
// ObjectCache Implementation
// ----------------------------------------------------------------------------
void ObjectCache::notifyObjectCompiled(const llvm::Module *m,
                                       llvm::MemoryBufferRef objBuffer) {
  cachedObjects[m->getModuleIdentifier()] =
      llvm::MemoryBuffer::getMemBufferCopy(objBuffer.getBuffer(),
                                           objBuffer.getBufferIdentifier());
}

std::unique_ptr<llvm::MemoryBuffer>
ObjectCache::getObject(const llvm::Module *m) {
  auto i = cachedObjects.find(m->getModuleIdentifier());

  if (i == cachedObjects.end()) {
    JIT_LOG("No object for " + m->getModuleIdentifier() +
            " in cache. Compiling.");
    return nullptr;
  }

  JIT_LOG("Object for " + m->getModuleIdentifier() + " loaded from cache.");
  return llvm::MemoryBuffer::getMemBuffer(i->second->getMemBufferRef());
}

void ObjectCache::dumpToObjectFile(llvm::StringRef outputFilename) {
  // Set up the output file.
  std::error_code error;

  auto file = std::make_unique<llvm::ToolOutputFile>(outputFilename, error,
                                                     llvm::sys::fs::OF_None);
  if (error) {

    llvm::errs() << "cannot open output file '" + outputFilename.str() +
                        "': " + error.message()
                 << "\n";
    return;
  }
  // Dump the object generated for a single module to the output file.
  // TODO: Replace this with a runtime check
  assert(cachedObjects.size() == 1 && "Expected only one object entry.");

  auto &cachedObject = cachedObjects.begin()->second;
  file->os() << cachedObject->getBuffer();
  file->keep();
}

// ----------------------------------------------------------------------------
// JIT Implementation
// ----------------------------------------------------------------------------
orc::JITDylib *JIT::getLatestJITDylib(const llvm::StringRef &nsName) {
  if (jitDylibs.count(nsName) == 0) {
    return nullptr;
  }

  auto vec = jitDylibs[nsName];
  // TODO: Make sure that the returning Dylib still exists in the JIT
  //       by calling jit->engine->getJITDylibByName(dylib_name);
  return vec.empty() ? nullptr : vec.back();
};

void JIT::pushJITDylib(const llvm::StringRef &nsName, llvm::orc::JITDylib *l) {
  if (jitDylibs.count(nsName) == 0) {
    llvm::SmallVector<llvm::orc::JITDylib *, 1> vec{l};
    jitDylibs[nsName] = vec;
    return;
  }
  auto vec = jitDylibs[nsName];
  vec.push_back(l);
  jitDylibs[nsName] = vec;
}

size_t JIT::getNumberOfJITDylibs(const llvm::StringRef &nsName) {
  if (jitDylibs.count(nsName) == 0) {
    return 0;
  }

  return jitDylibs[nsName].size();
};

JIT::JIT(llvm::orc::JITTargetMachineBuilder &&jtmb,
         std::unique_ptr<Options> opts)
    :

      options(std::move(opts)),
      cache(options->JITenableObjectCache ? new ObjectCache() : nullptr),
      gdbListener(options->JITenableGDBNotificationListener
                      ? llvm::JITEventListener::createGDBRegistrationListener()
                      : nullptr),
      perfListener(options->JITenablePerfNotificationListener
                       ? llvm::JITEventListener::createPerfJITEventListener()
                       : nullptr),
      jtmb(jtmb){};

void JIT::dumpToObjectFile(const llvm::StringRef &filename) {
  cache->dumpToObjectFile(filename);
};

int JIT::getOptimizatioLevel() const {
  if (options->compilationPhase <= CompilationPhase::NoOptimization) {
    return 0;
  }

  if (options->compilationPhase == CompilationPhase::O1) {
    return 1;
  }
  if (options->compilationPhase == CompilationPhase::O2) {
    return 2;
  }
  return 3;
}

llvm::Error JIT::createCurrentProcessJD() {

  auto &es           = engine->getExecutionSession();
  auto *processJDPtr = es.getJITDylibByName(MAIN_PROCESS_JD_NAME);

  if (processJDPtr != nullptr) {
    // We already created the JITDylib for the current process
    return llvm::Error::success();
  }

  auto processJD = es.createJITDylib(MAIN_PROCESS_JD_NAME);

  if (!processJD) {
    return processJD.takeError();
  }

  auto generator =
      llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
          engine->getDataLayout().getGlobalPrefix());

  if (!generator) {
    return generator.takeError();
  }

  processJD->addGenerator(std::move(*generator));
  return llvm::Error::success();
};

MaybeJIT JIT::make(llvm::orc::JITTargetMachineBuilder &&jtmb,
                   std::unique_ptr<Options> opts) {
  auto dl = jtmb.getDefaultDataLayoutForTarget();
  if (!dl) {
    return dl.takeError();
  }

  auto jitEngine = std::make_unique<JIT>(std::move(jtmb), std::move(opts));
  //
  // What might go wrong?
  // in a repl env when we have to create new modules on top of each other
  // having two different contex might be a problem, but i think since we
  // use the first context to generate the IR and the second one to just
  // run it.
  std::unique_ptr<llvm::LLVMContext> ctx(new llvm::LLVMContext);

  // Callback to create the object layer with symbol resolution to current
  // process and dynamically linked libraries.
  auto objectLinkingLayerCreator = [&](llvm::orc::ExecutionSession &session,
                                       const llvm::Triple &tt) {
    (void)tt;

    auto objectLayer =
        std::make_unique<llvm::orc::RTDyldObjectLinkingLayer>(session, []() {
          return std::make_unique<llvm::SectionMemoryManager>();
        });

    // Register JIT event listeners if they are enabled.
    if (jitEngine->gdbListener != nullptr) {
      objectLayer->registerJITEventListener(*jitEngine->gdbListener);
    }
    if (jitEngine->perfListener != nullptr) {
      objectLayer->registerJITEventListener(*jitEngine->perfListener);
    }

    // COFF format binaries (Windows) need special handling to deal with
    // exported symbol visibility.
    // cf llvm/lib/ExecutionEngine/Orc/LLJIT.cpp
    // LLJIT::createObjectLinkingLayer
    if (jitEngine->options->hostTriple.isOSBinFormatCOFF()) {
      objectLayer->setOverrideObjectFlagsWithResponsibilityFlags(true);
      objectLayer->setAutoClaimResponsibilityForObjectSymbols(true);
    }

    return objectLayer;
  };

  // Callback to inspect the cache and recompile on demand.
  auto compileFunctionCreator = [&](llvm::orc::JITTargetMachineBuilder JTMB)
      -> llvm::Expected<
          std::unique_ptr<llvm::orc::IRCompileLayer::IRCompiler>> {
    llvm::CodeGenOpt::Level jitCodeGenOptLevel =
        static_cast<llvm::CodeGenOpt::Level>(jitEngine->getOptimizatioLevel());

    JTMB.setCodeGenOptLevel(jitCodeGenOptLevel);

    auto targetMachine = JTMB.createTargetMachine();
    if (!targetMachine) {
      return targetMachine.takeError();
    }

    return std::make_unique<llvm::orc::TMOwningSimpleCompiler>(
        std::move(*targetMachine), jitEngine->cache.get());
  };

  auto compileNotifier = [&](llvm::orc::MaterializationResponsibility &r,
                             llvm::orc::ThreadSafeModule tsm) {
    auto syms = r.getRequestedSymbols();
    tsm.withModuleDo([&](llvm::Module &m) {
      JIT_LOG("Compiled " << syms
                          << " for the module: " << m.getModuleIdentifier());
    });
  };

  if (jitEngine->options->JITLazy) {
    // Setup a LLLazyJIT instance to the times that latency is important
    // for example in a REPL. This way
    auto jit =
        cantFail(llvm::orc::LLLazyJITBuilder()
                     .setCompileFunctionCreator(compileFunctionCreator)
                     .setObjectLinkingLayerCreator(objectLinkingLayerCreator)
                     .create());
    jit->getIRCompileLayer().setNotifyCompiled(compileNotifier);
    jitEngine->engine = std::move(jit);

  } else {
    // Setup a LLJIT instance for the times that performance is important
    // and we want to compile everything as soon as possible. For instance
    // when we run the JIT in the compiler
    auto jit =
        cantFail(llvm::orc::LLJITBuilder()
                     .setCompileFunctionCreator(compileFunctionCreator)
                     .setObjectLinkingLayerCreator(objectLinkingLayerCreator)
                     .create());
    jit->getIRCompileLayer().setNotifyCompiled(compileNotifier);
    jitEngine->engine = std::move(jit);
  }

  if (auto err = jitEngine->createCurrentProcessJD()) {
    return err;
  }

  return MaybeJIT(std::move(jitEngine));
};

MaybeJIT makeJIT(std::unique_ptr<Options> opts) {
  llvm::orc::JITTargetMachineBuilder jtmb(opts->hostTriple);
  auto maybeJIT = JIT::make(std::move(jtmb), std::move(opts));

  if (!maybeJIT) {
    return maybeJIT.takeError();
  }

  return maybeJIT;
};
} // namespace serene::jit
