add_executable(persistence1d_tests)
target_sources(
    persistence1d_tests
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests.cpp"
)
target_link_libraries(persistence1d_tests PRIVATE persistence1d)
