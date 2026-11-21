add_executable(MixwaveCoreTests ipc_test.cpp ${MIXWAVE_CORE_SOURCES})
target_include_directories(MixwaveCoreTests PRIVATE ${MIXWAVE_CORE_DIR})
target_compile_features(MixwaveCoreTests PRIVATE cxx_std_17)
add_test(NAME ipc COMMAND MixwaveCoreTests)
