
from conans import ConanFile, CMake, tools


class PexConan(ConanFile):
    name = "pex"
    version = "0.9.6"

    scm = {
        "type": "git",
        "url": "https://github.com/JiveHelix/pex.git",
        "revision": "auto",
        "submodule": "recursive"}

    license = "MIT"
    author = "Jive Helix (jivehelix@gmail.com)"
    url = "https://github.com/JiveHelix/pex"
    description = "Improved plumbing for MVC apps."
    topics = ("Publish/Subscribe", "MVC", "Model-View-Controller", "C++")

    settings = "os", "compiler", "build_type", "arch"

    generators = "cmake"

    options = {"ENABLE_PEX_LOG": [True, False]}

    default_options = {"ENABLE_PEX_LOG": False}

    no_copy_source = True

    def build(self):
        cmake = CMake(self)
        cmake.definitions["ENABLE_PEX_LOG"] = self.options.ENABLE_PEX_LOG
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["pex"]

        if self.settings.os == "Linux":
            self.cpp_info.cxxflags.append("-fconcepts")

    def build_requirements(self):
        self.test_requires("catch2/2.13.8")
        self.test_requires("nlohmann_json/[~3.11]")

    def requirements(self):
        self.requires("jive/[>=1.1.3 < 1.2]")
        self.requires("fields/[~1.3]")
