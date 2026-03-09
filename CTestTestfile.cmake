# CMake generated Testfile for 
# Source directory: /Users/amil21/Documents/emulator/LLD_gbemu/emulator
# Build directory: /Users/amil21/Documents/emulator/LLD_gbemu/emulator
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(check_gbe "/Users/amil21/Documents/emulator/LLD_gbemu/emulator/tests/check_gbe")
set_tests_properties(check_gbe PROPERTIES  _BACKTRACE_TRIPLES "/Users/amil21/Documents/emulator/LLD_gbemu/emulator/CMakeLists.txt;81;add_test;/Users/amil21/Documents/emulator/LLD_gbemu/emulator/CMakeLists.txt;0;")
subdirs("lib")
subdirs("gbemu")
subdirs("tests")
