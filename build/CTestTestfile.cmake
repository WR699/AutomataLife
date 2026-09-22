# CMake generated Testfile for 
# Source directory: F:/AutomataLife
# Build directory: F:/AutomataLife/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(pruebas_automatas "F:/AutomataLife/build/Debug/Pruebas.exe")
  set_tests_properties(pruebas_automatas PROPERTIES  _BACKTRACE_TRIPLES "F:/AutomataLife/CMakeLists.txt;22;add_test;F:/AutomataLife/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(pruebas_automatas "F:/AutomataLife/build/Release/Pruebas.exe")
  set_tests_properties(pruebas_automatas PROPERTIES  _BACKTRACE_TRIPLES "F:/AutomataLife/CMakeLists.txt;22;add_test;F:/AutomataLife/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(pruebas_automatas "F:/AutomataLife/build/MinSizeRel/Pruebas.exe")
  set_tests_properties(pruebas_automatas PROPERTIES  _BACKTRACE_TRIPLES "F:/AutomataLife/CMakeLists.txt;22;add_test;F:/AutomataLife/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(pruebas_automatas "F:/AutomataLife/build/RelWithDebInfo/Pruebas.exe")
  set_tests_properties(pruebas_automatas PROPERTIES  _BACKTRACE_TRIPLES "F:/AutomataLife/CMakeLists.txt;22;add_test;F:/AutomataLife/CMakeLists.txt;0;")
else()
  add_test(pruebas_automatas NOT_AVAILABLE)
endif()
