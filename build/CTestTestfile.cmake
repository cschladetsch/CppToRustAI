# CMake generated Testfile for 
# Source directory: /home/christian/local/repos/CppToRust
# Build directory: /home/christian/local/repos/CppToRust/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[cpp_to_rust_tests]=] "/home/christian/local/repos/CppToRust/build/cpp_to_rust_tests")
set_tests_properties([=[cpp_to_rust_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/christian/local/repos/CppToRust/CMakeLists.txt;39;add_test;/home/christian/local/repos/CppToRust/CMakeLists.txt;0;")
subdirs("external/CppLmmModelStore")
