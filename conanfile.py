from conan import ConanFile


class PexConan(ConanFile):
    name = "pex"
    version = "1.1.0"

    python_requires = "boiler/0.1"
    python_requires_extend = "boiler.LibraryConanFile"

    license = "MIT"
    author = "Jive Helix (jivehelix@gmail.com)"
    url = "https://github.com/JiveHelix/pex"
    description = "Improved plumbing for MVC apps."
    topics = ("Publish/Subscribe", "MVC", "Model-View-Controller", "C++")

    def build_requirements(self):
        self.test_requires("catch2/2.13.8")

    def requirements(self):
        self.requires("jive/[~1.4]", transitive_headers=True)
        self.requires("fields/[~1.5]", transitive_headers=True)
        self.requires("fmt/[~10]", transitive_headers=True)
        self.requires("nlohmann_json/[~3.11]")
