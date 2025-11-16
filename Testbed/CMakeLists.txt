project("Testbed")

# Fetch all the source and header files and the then add them automatically
file(GLOB_RECURSE SRC_FILES "src/*.cpp")
file(GLOB_RECURSE HDR_FILES "src/*.h")

add_executable(Testbed ${SRC_FILES} ${HDR_FILES})

# Set the C++ Standard to 20 for this target.
set_property(TARGET Testbed PROPERTY CXX_STANDARD 20)

target_link_libraries(Testbed Logex-static)
