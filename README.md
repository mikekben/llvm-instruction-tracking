# llvm-instruction-tracking

Patch for LLVM instruction tracking / pass instrumentation.

## Target commit

This patch applies to LLVM commit `027447c61724` (`llvmorg-23-init-6194-g027447c61724`).

## Running the example

1. Apply `0001-Instruction-tracking.patch` to the LLVM tree and build it
2. Build `touched-diff`:
   ```
   mkdir build && cd build
   cmake -GNinja -DCMAKE_PREFIX_PATH=/path/to/llvm-project/build ..
   ninja
   ```
3. Run the example:
   ```
   ./runner.sh
   ```
