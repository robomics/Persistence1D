cmake_minimum_required(VERSION 3.25)
cmake_policy(VERSION 3.25...3.31)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

cmake_minimum_required(VERSION 2.6)

project(Persistence1D LANGUAGES CXX)

add_subdirectory(src)
