config="Debug"
build_dir="build-debug"

script_path="$(realpath $0)"
repo_dir="$(dirname $script_path)"

cmake \
    -B $build_dir \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=$config \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
    -DCMAKE_LINKER_TYPE=LLD \
    -DLLVM_ROOT=/opt/llvm-debug && \
    ln -sf $repo_dir/$build_dir/compile_commands.json $repo_dir/compile_commands.json && \
    cmake --build $build_dir --parallel $(nproc)
