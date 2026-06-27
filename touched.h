#pragma once

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <optional>
#include <string>
#include <unordered_set>

namespace instr_tracking {

/// Returns the !mymd index of an instruction, or nullopt if absent.
inline std::optional<uint64_t> getMymIdx(llvm::Instruction *inst) {
  auto *MD = inst->getMetadata("mymd");
  if (!MD)
    return std::nullopt;
  if (auto *CI = llvm::mdconst::dyn_extract<llvm::ConstantInt>(MD->getOperand(0)))
    return CI->getZExtValue();
  if (auto *MS = llvm::dyn_cast<llvm::MDString>(MD->getOperand(0))) {
    uint64_t idx;
    if (!MS->getString().getAsInteger(10, idx))
      return idx;
  }
  return std::nullopt;
}

inline std::map<uint64_t, llvm::Instruction *> buildMymdMap(llvm::Function &F) {
  std::map<uint64_t, llvm::Instruction *> m;
  for (auto &BB : F)
    for (auto &Inst : BB)
      if (auto idx = getMymIdx(const_cast<llvm::Instruction *>(&Inst)))
        m[*idx] = const_cast<llvm::Instruction *>(&Inst);
  return m;
}

inline std::string stripMetadata(std::string s) {
  auto pos = s.find(", !");
  if (pos != std::string::npos)
    s.erase(pos);
  return s;
}

/// Indices of instructions present in pre_fn but absent in post_fn.
inline std::unordered_set<uint64_t>
removedSet(llvm::Function &pre_fn, llvm::Function &post_fn) {
  auto pre  = buildMymdMap(pre_fn);
  auto post = buildMymdMap(post_fn);
  std::unordered_set<uint64_t> result;
  for (auto &[idx, _] : pre)
    if (!post.count(idx))
      result.insert(idx);
  return result;
}

/// Indices of instructions absent in pre_fn but present in post_fn.
inline std::unordered_set<uint64_t>
addedSet(llvm::Function &pre_fn, llvm::Function &post_fn) {
  auto pre  = buildMymdMap(pre_fn);
  auto post = buildMymdMap(post_fn);
  std::unordered_set<uint64_t> result;
  for (auto &[idx, _] : post)
    if (!pre.count(idx))
      result.insert(idx);
  return result;
}

/// Indices of instructions present in both but whose text differs.
inline std::unordered_set<uint64_t>
modifiedSet(llvm::Function &pre_fn, llvm::Function &post_fn) {
  auto pre  = buildMymdMap(pre_fn);
  auto post = buildMymdMap(post_fn);
  std::unordered_set<uint64_t> result;
  for (auto &[idx, pre_inst] : pre) {
    auto it = post.find(idx);
    if (it == post.end())
      continue;
    std::string pre_s, post_s;
    llvm::raw_string_ostream(pre_s) << *pre_inst;
    llvm::raw_string_ostream(post_s) << *it->second;
    if (stripMetadata(pre_s) != stripMetadata(post_s))
      result.insert(idx);
  }
  return result;
}

/// Indices of instructions in pre_fn that were removed or changed (removedSet u modifiedSet).
inline std::unordered_set<uint64_t>
touchedPre(llvm::Function &pre_fn, llvm::Function &post_fn) {
  auto pre  = buildMymdMap(pre_fn);
  auto post = buildMymdMap(post_fn);

  std::unordered_set<uint64_t> result;
  for (auto &[idx, pre_inst] : pre) {
    auto it = post.find(idx);
    if (it == post.end()) {
      result.insert(idx);
    } else {
      std::string pre_s, post_s;
      llvm::raw_string_ostream(pre_s) << *pre_inst;
      llvm::raw_string_ostream(post_s) << *it->second;
      if (stripMetadata(pre_s) != stripMetadata(post_s))
        result.insert(idx);
    }
  }
  return result;
}

/// Indices of instructions in post_fn that were inserted or changed (addedSet u modifiedSet).
inline std::unordered_set<uint64_t>
touchedPost(llvm::Function &pre_fn, llvm::Function &post_fn) {
  auto pre  = buildMymdMap(pre_fn);
  auto post = buildMymdMap(post_fn);

  std::unordered_set<uint64_t> result;
  for (auto &[idx, post_inst] : post) {
    auto it = pre.find(idx);
    if (it == pre.end()) {
      result.insert(idx);
    } else {
      std::string pre_s, post_s;
      llvm::raw_string_ostream(pre_s) << *it->second;
      llvm::raw_string_ostream(post_s) << *post_inst;
      if (stripMetadata(pre_s) != stripMetadata(post_s))
        result.insert(idx);
    }
  }
  return result;
}

} // namespace instr_tracking
