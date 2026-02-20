
include(FetchContent)

FetchContent_Declare(
   googletest
   URL https://github.com/google/googletest/archive/refs/heads/main.zip
   DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


add_executable(unit_tests test_main.cpp)

# Collega il tuo codice (se necessario) e Google Test
# gtest_main include già il comando main(), quindi non devi scriverlo tu!
target_link_libraries(unit_tests 
PUBLIC
   ${MSHM_LIB_NAME}
   gtest_main
)


# Permette a VS Code o altri IDE di scoprire i test automaticamente
include(GoogleTest)
gtest_discover_tests(unit_tests)



