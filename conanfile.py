from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout

class MyLibConan(ConanFile):
    name = "logex"
    version = "1.0.0"

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    exports_sources = "Logger/CMakeLists.txt", "Logger/src/*", "Logger/include/*"
    generators = "CMakeDeps", "CMakeToolchain"

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "logex::Logex")
        self.cpp_info.libs = ["Logex"]

