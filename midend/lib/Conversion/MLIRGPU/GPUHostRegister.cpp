//===- GPUHostRegister.cpp -------------------------------------------------===//
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//===----------------------------------------------------------------------===//
//
// This file implements the GPU host register pass that adds gpu.host_register.
//
//===----------------------------------------------------------------------===//

#include "mlir/Dialect/GPU/IR/GPUDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Visitors.h"
#include "mlir/Support/LLVM.h"
#include "llvm/Support/Casting.h"
#include <mlir/Dialect/Affine/IR/AffineOps.h>
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/Dialect/Linalg/Transforms/Transforms.h>
#include <mlir/IR/Dialect.h>
#include <mlir/IR/Operation.h>
#include <mlir/IR/TypeUtilities.h>
#include <mlir/IR/Value.h>
#include <mlir/Pass/Pass.h>

using namespace mlir;
using namespace vector;

//===----------------------------------------------------------------------===//
// Rewrite Pattern
//===----------------------------------------------------------------------===//

namespace {

class GPUHostRegisterPattern : public ConversionPattern {
public:
  explicit GPUHostRegisterPattern(MLIRContext *context)
      : ConversionPattern(gpu::LaunchFuncOp().getOperationName(), 1, context) {
  }

  LogicalResult
  matchAndRewrite(Operation *op, ArrayRef<Value> /*operands*/,
                  ConversionPatternRewriter &rewriter) const override {
    llvm::errs()<<op->getName().getStringRef()<<"\n";
    return success();
  }

private:
};
} // end anonymous namespace

//===----------------------------------------------------------------------===//
// GPUHostRegisterPass
//===----------------------------------------------------------------------===//

namespace {
class GPUHostRegisterPass
    : public PassWrapper<GPUHostRegisterPass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(GPUHostRegisterPass)
  StringRef getArgument() const final { return "gpu-host-register"; }
  StringRef getDescription() const final { return "Register host memory to legalize gpu access."; }
  GPUHostRegisterPass() = default;
  GPUHostRegisterPass(const GPUHostRegisterPass &
  
  
  ) {}

  void runOnOperation() override;

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<gpu::GPUDialect,memref::MemRefDialect>();
  }
};
} // end anonymous namespace.

void GPUHostRegisterPass::runOnOperation() {
  auto module = getOperation();
  module->walk<WalkOrder::PreOrder>([&](Operation *nestedOp){
    if (auto launchFuncOp = dyn_cast<gpu::LaunchFuncOp>(nestedOp)){
      // OpBuilder barrierBuilder(launchFuncOp->getContext());
      // barrierBuilder.setInsertionPointAfter(launchFuncOp);
      // barrierBuilder.create<gpu::BarrierOp>(launchFuncOp->getLoc());
      for (auto operand : launchFuncOp->getOperands()){
        if (!operand.getType().isa<BaseMemRefType>()) continue;
        if(auto* producerOp = operand.getDefiningOp()){
          // llvm::dbgs()<<producerOp->getName().getStringRef()<<"\n";
          OpBuilder builder(producerOp->getContext());
          builder.setInsertionPointAfter(producerOp);
          auto memrefType = cast<MemRefType>(operand.getType());
          auto elementType = memrefType.getElementType();
          UnrankedMemRefType resType = UnrankedMemRefType::get(elementType, 0);
          Value cast = builder.create<memref::CastOp>(producerOp->getLoc(), resType, operand);
          builder.create<gpu::HostRegisterOp>(producerOp->getLoc(), cast);
        }
      }
      return WalkResult::advance();
    }
    return WalkResult::advance();
  });
  
}

namespace mlir {
namespace buddy {
void registerGPUHostRegisterPass() { PassRegistration<GPUHostRegisterPass>(); }
} // namespace buddy
} // namespace mlir
