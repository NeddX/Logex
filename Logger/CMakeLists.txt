project("Logger")

# Fetch all the source and header files and the then add them automatically
file(GLOB_RECURSE SRC_FILES "src/*.cpp")
file(GLOB_RECURSE HDR_FILES "src/*.h")

add_library(Logex STATIC ${SRC_FILES} ${HDR_FILES})

# Set the C++ Standard to 20 for this target.
set_property(TARGET Logex PROPERTY CXX_STANDARD 20)

add_subdirectory(vendor/fmt ${PROJECT_BINARY_DIR}/fmt EXCLUDE_FROM_ALL)
set(FMT_INCLUDE_DIRS vendor/fmt/include)
set(FMT_LIBRARIES fmt)

target_include_directories(Logex PUBLIC ${FMT_INCLUDE_DIRS})
target_link_libraries(Logex ${FMT_LIBRARIES})

# Make the base include file public
target_include_directories(Logex PUBLIC include/)
target_include_directories(Logex PUBLIC src/)
