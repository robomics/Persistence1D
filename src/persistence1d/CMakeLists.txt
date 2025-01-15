add_library(persistence1d INTERFACE)

target_sources(
    persistence1d
    INTERFACE FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(
    persistence1d
    INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>"
)

target_compile_features(persistence1d INTERFACE "cxx_std_${CMAKE_CXX_STANDARD}")

add_executable(persistence1d_driver)
target_sources(
    persistence1d_driver
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/persistence1d_driver.cpp"
)
target_link_libraries(persistence1d_driver PRIVATE persistence1d)
