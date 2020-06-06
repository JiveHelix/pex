#!/usr/bin/env python3

##
# @file run.py
#
# @brief Automatically finds and runs all test cases in this folder.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

import unittest
import os

scriptDirectory = os.path.dirname(os.path.realpath(__file__))
loader = unittest.TestLoader()
tests = loader.discover(scriptDirectory)
testRunner = unittest.runner.TextTestRunner(verbosity=2)
testRunner.run(tests)
