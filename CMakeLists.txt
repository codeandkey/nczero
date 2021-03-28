cmake_minimum_required (VERSION 3.0)
project (nczero)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Find libtorch
find_package (Torch REQUIRED)
add_compile_options (${TORCH_CXX_FLAGS})

add_subdirectory (src)
