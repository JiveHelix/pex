from cmake_includes.conan import LibraryConanFile


class PexConan(LibraryConanFile):
    name = "pex"
    version = "1.0.0"

    license = "MIT"
    author = "Jive Helix (jivehelix@gmail.com)"
    url = "https://github.com/JiveHelix/pex"
    description = "Improved plumbing for MVC apps."
    topics = ("Publish/Subscribe", "MVC", "Model-View-Controller", "C++")

    def build_requirements(self):
        self.test_requires("catch2/2.13.8")

    def requirements(self):
        self.requires("jive/[~1.3]", transitive_headers=True)
        self.requires("fields/[~1.4]", transitive_headers=True)
        self.requires("fmt/[~10]", transitive_headers=True)
        self.requires("nlohmann_json/[~3.11]")
