
from conans import ConanFile, CMake, tools


class PexConan(ConanFile):
    name = "pex"
    version = "0.5.3"

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
        self.cpp_info.includes = ["include"]
        self.cpp_info.libs = ["pex"]

    def build_requirements(self):
        self.test_requires("catch2/2.13.8")

    def requirements(self):
        self.requires("jive/[~1.0]")
        self.requires("fields/[~1]")
        self.requires("tau/[~1.3]")
        self.requires("wxwidgets/3.1.7@jivehelix/stable")

