source_files=$(find . -type f -name "*.cpp")

output_dir="build"
if [[ ! -d $output_dir ]]; then
    mkdir -p $output_dir
fi

linker_ops=""
compiler_flags="-Wall -Wextra"
assembly_name="Server"
clang $source_files $linker_ops $compiler_flags -o "$output_dir/$assembly_name"
