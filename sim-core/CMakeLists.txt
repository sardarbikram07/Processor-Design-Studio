cmake_minimum_required(VERSION 3.16)
project(processor_design_studio CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Compiler flags ───────────────────────────────────────────────────────────
add_compile_options(-O2 -Wall -Wextra)

# ─── OpenMP ───────────────────────────────────────────────────────────────────
find_package(OpenMP REQUIRED)

# ─── Source files ─────────────────────────────────────────────────────────────
set(ISA_SOURCES
    src/isa/decoder.cpp
    src/isa/regfile.cpp
    src/isa/memory.cpp
)

set(PIPELINE_SOURCES
    src/pipeline/hazard.cpp
    src/pipeline/forwarding.cpp
    src/pipelined_simulator.cpp
)

set(CACHE_SOURCES
    src/cache/cache.cpp
)

set(PREDICTOR_SOURCES
    src/predictor/predictor.cpp
)

set(SWEEP_SOURCES
    src/sweep/sweep.cpp
)

set(SIM_SOURCES
    src/assembler.cpp
    src/simulator.cpp
    ${ISA_SOURCES}
    ${PIPELINE_SOURCES}
    ${CACHE_SOURCES}
    ${PREDICTOR_SOURCES}
    ${SWEEP_SOURCES}
)

# ─── Main executable ──────────────────────────────────────────────────────────
add_executable(sim src/main.cpp ${SIM_SOURCES})
target_include_directories(sim PRIVATE include include/pipeline include/cache include/predictor include/sweep)
target_link_libraries(sim PRIVATE OpenMP::OpenMP_CXX)

# ─── Phase 1 test executable ─────────────────────────────────────────────────
add_executable(test_phase1
    tests/test_phase1.cpp
    ${SIM_SOURCES}
)
target_include_directories(test_phase1 PRIVATE include include/pipeline include/cache include/predictor include/sweep)
target_link_libraries(test_phase1 PRIVATE OpenMP::OpenMP_CXX)

# ─── Comprehensive test suite (all phases) ────────────────────────────────────
add_executable(test_all
    tests/test_all.cpp
    ${SIM_SOURCES}
)
target_include_directories(test_all PRIVATE include include/pipeline include/cache include/predictor include/sweep)
target_link_libraries(test_all PRIVATE OpenMP::OpenMP_CXX)

enable_testing()
add_test(NAME phase1_tests COMMAND test_phase1
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/..")
add_test(NAME all_tests COMMAND test_all
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/..")
