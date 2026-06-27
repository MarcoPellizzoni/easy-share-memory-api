# CTest custom configuration for mShm unit tests

# Default timeout per test (seconds)
set(CTEST_TEST_TIMEOUT 30)

# Run tests in parallel (use number of available CPUs)
set(CTEST_PARALLEL_LEVEL 0)

# Output behaviour: show failed test output automatically
set(CTEST_OUTPUT_ON_FAILURE TRUE)

# Tests to exclude from the run (regex on test name)
# set(CTEST_CUSTOM_TESTS_IGNORE "")

# Extra memcheck options (if using valgrind/DrMemory)
# set(CTEST_MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --error-exitcode=1")
