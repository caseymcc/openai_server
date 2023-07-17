#/bin/bash

build_type="debug"
build_llama=false
cmake_options=""
cmake_custom_options=""

for var in "$@"; do
    if [ "$var" = "release" ]; then
        build_type="release"
    fi
    if [ "$var" = "llama" ]; then
        build_llama=true
    fi
    if [[ $var = -D* ]]; then
        cmake_custom_options+="$var "
    fi
done

echo "**Building for $build_type"

if [ "$build_type" = "debug" ]; then
    cmake_options="$cmake_options \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_DEBUG_POSTFIX=d"
else
    cmake_options="$cmake_options \
        -DCMAKE_BUILD_TYPE=Release"
fi

if [ "$build_llama" = true ]; then
    cmake_options="$cmake_options \
        -DOPENAI_SERVER_LLAMACMD=ON"
fi

build_dir="build_${build_type}"

cmake -S. -B$build_dir $cmake_options $cmake_custom_options \
    -DCMAKE_TOOLCHAIN_FILE=$(pwd)/cmake/clang_rocm_toolchain.cmake

