project("Logger")

# Fetch all the source and header files and the then add them automatically
file(GLOB_RECURSE SRC_FILES "src/*.cpp")
file(GLOB_RECURSE HDR_FILES "src/*.h")

add_library(logex-static STATIC ${SRC_FILES} ${HDR_FILES} "src/Common.h")

# Set the C++ Standard to 20 for this target.
set_property(TARGET logex-static PROPERTY CXX_STANDARD 20)

add_subdirectory(vendor/fmt ${PROJECT_BINARY_DIR}/fmt EXCLUDE_FROM_ALL)
set(FMT_INCLUDE_DIR vendor/fmt/include CACHE INTERNAL "")
set(FMT_LIBRARIES fmt CACHE INTERNAL "")

target_include_directories(logex-static PUBLIC ${FMT_INCLUDE_DIR})
target_link_libraries(logex-static ${FMT_LIBRARIES})

# Make the base include file public
target_include_directories(logex-static PUBLIC include/)
target_include_directories(logex-static PUBLIC src/)

# Define the debug macro if build type is debug.
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(logex-static PUBLIC LGX_DEBUG)
endif()

# Define a function to add warning flags for GCC/Clang or MSVC
function(add_warning_flags target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall            # All warnings
            -Wextra          # Extra warnings
            #-Werror          # Treat warnings as errors
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_compile_options(${target} PRIVATE
            /W4              # Level 4 warnings
            #/WX              # Treat warnings as errors
        )
    endif()
endfunction()

# Function to add warning and sanitizer flags
function(add_asan_flags target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Enable AddressSanitizer
        target_compile_options(${target} PUBLIC -fsanitize=address -g)
        target_link_options(${target} PUBLIC -fsanitize=address)
        target_compile_definitions(${target} PUBLIC _DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        # Enable AddressSanitizer
        target_compile_options(${target} PRIVATE /fsanitize=address)
        target_link_options(${target} PRIVATE /fsanitize=address)
        target_compile_definitions(${target} PRIVATE _DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
    endif()
endfunction()

install(TARGETS logex-static
	EXPORT logexTargets
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT logexTargets
	NAMESPACE logex::
	DESTINATION lib/cmake/logex
)

# Quality control
add_warning_flags(logex-static)
#add_asan_flags(logex-static)
