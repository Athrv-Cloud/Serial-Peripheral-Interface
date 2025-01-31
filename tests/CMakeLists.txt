# Enable code coverage option
option(COVERAGE "Enable code coverage" ON)

# Enable coverage flags if requested
if(COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -fPIC -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -fPIC -O0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

# Include directories
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/Src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/Inc")
include_directories(${SOURCE_DIR} ${INCLUDE_DIR})

# Collect source and header files
file(GLOB SRC_FILES ${SOURCE_DIR}/*.c)
file(GLOB INC_FILES ${INCLUDE_DIR}/*.h)

# Find CppUTest
find_package(CppUTest REQUIRED)

# Collect test files
file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Create test executable
add_executable(SPI_TESTS ${TEST_FILES} ${SRC_FILES})

# Add compile definition to disable main in source files
target_compile_definitions(SPI_TESTS PRIVATE ENABLE_TESTS)

# Link against the main library and CppUTest
target_link_libraries(SPI_TESTS PRIVATE
    CppUTest
    CppUTestExt
)

# Set output directory for the test executable
set_target_properties(SPI_TESTS PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

# Enable testing
enable_testing()
add_test(NAME SPI_TESTS COMMAND SPI_TESTS)

# Add custom target for generating coverage report
if(COVERAGE)
    add_custom_target(coverage
        # Create coverage directory
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
        
        # Run tests
        COMMAND ${CMAKE_BINARY_DIR}/tests/SPI_TESTS
        
        # Generate initial coverage data
        COMMAND lcov --capture --initial --directory ${CMAKE_BINARY_DIR} --output-file ${CMAKE_BINARY_DIR}/coverage.info
        
        # Run tests again to get coverage data
        COMMAND ${CMAKE_BINARY_DIR}/tests/SPI_TESTS
        
        # Capture coverage data
        COMMAND lcov --capture --directory ${CMAKE_BINARY_DIR} --output-file ${CMAKE_BINARY_DIR}/coverage.info
        
        # Remove system headers from coverage
        COMMAND lcov --remove ${CMAKE_BINARY_DIR}/coverage.info '/usr/*' --output-file ${CMAKE_BINARY_DIR}/coverage.info
        
        # Generate HTML report
        COMMAND genhtml ${CMAKE_BINARY_DIR}/coverage.info --output-directory ${CMAKE_BINARY_DIR}/coverage
        
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS SPI_TESTS
    )
endif()