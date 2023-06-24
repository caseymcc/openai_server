set(CMAKE_C_COMPILER "/usr/bin/clang-15")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-15")

#set(CMAKE_C_COMPILER "/projects/clang/llvm-project-llvmorg-16.0.4/build/bin/clang-16")
#set(CMAKE_CXX_COMPILER "/projects/clang/llvm-project-llvmorg-16.0.4/build/bin/clang++")
#set(CMAKE_C_COMPILER "/opt/rocm-4.5.2/bin/amdclang")
#set(CMAKE_CXX_COMPILER "/opt/rocm-4.5.2/bin/amdclang++")
set(GPU_TARGETS "gfx900;gfx801" CACHE STRING "")

set(OPENAI_SERVER_LLAMA ON CACHE BOOL "Use llama.cpp backend")
set(LLAMA_NATIVE ON CACHE BOOL "llama: enable -march=native flag")
set(LLAMA_HIPBLAS ON CACHE BOOL "llama: use HIPBLAS")
set(LLAMA_BUILD_CHATGPT_SERVER ON CACHE BOOL "llama: build server with chatgpt api")

set(LLAMA_CUDA_DMMV_X 64)
set(__HIP_USE_CMPXCHG_FOR_FP_ATOMICS 1 CACHE BOOL "")

#list(APPEND CMAKE_PREFIX_PATH /opt/rocm-4.5.2)