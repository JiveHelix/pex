
from conans import ConanFile, CMake, tools


class PexConan(ConanFile):
    name = "pex"
    version = "0.3.1"

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

    no_copy_source = True

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_id(self):
        self.info.header_only()

    def build_requirements(self):
        self.test_requires("catch2/2.13.8")

    def requirements(self):
        self.requires("jive/[~1.0]")
        self.requires("fields/[~1.1]")
        self.requires("tau/[~1.1]")
        self.requires("wxwidgets/3.1.5@jivehelix/stable")

