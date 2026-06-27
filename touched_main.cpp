// Loads two LLVM IR files (pre-pass and post-pass snapshots of the same
// function) and prints the added, removed, and modified instruction index sets
// computed from their !mymd metadata tags.
//
// Usage: touched-diff <pre.ll> <post.ll> <function-name>

#include "touched.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

int main(int argc, char **argv) {
  if (argc != 4) {
    llvm::errs() << "usage: touched-diff <pre.ll> <post.ll> <function-name>\n";
    return 1;
  }

  llvm::LLVMContext ctx;
  llvm::SMDiagnostic diag;

  auto pre_module = llvm::parseIRFile(argv[1], diag, ctx);
  if (!pre_module) { diag.print(argv[0], llvm::errs()); return 1; }

  auto post_module = llvm::parseIRFile(argv[2], diag, ctx);
  if (!post_module) { diag.print(argv[0], llvm::errs()); return 1; }

  auto *pre_fn  = pre_module->getFunction(argv[3]);
  auto *post_fn = post_module->getFunction(argv[3]);

  if (!pre_fn)  { llvm::errs() << "function '" << argv[3] << "' not found in pre\n";  return 1; }
  if (!post_fn) { llvm::errs() << "function '" << argv[3] << "' not found in post\n"; return 1; }

  auto print = [](const char *label, const std::unordered_set<uint64_t> &s) {
    llvm::outs() << label << ": {";
    bool first = true;
    for (auto idx : s) { if (!first) llvm::outs() << ", "; llvm::outs() << idx; first = false; }
    llvm::outs() << "}\n";
  };

  print("Removed",  instr_tracking::removedSet(*pre_fn,  *post_fn));
  print("Added",    instr_tracking::addedSet(*pre_fn,    *post_fn));
  print("Modified", instr_tracking::modifiedSet(*pre_fn, *post_fn));

  return 0;
}
