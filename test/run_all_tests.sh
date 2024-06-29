# Shell script to run CppUTest and generate coverage report
#
# Assumes you have installed a c++ compiler, cpputest, gcov, etc
#
# Also needs the Python package junit2html
#
# Run from the top level serial9 folder, this script will create
# a new folder for the output, which is NOT stored in the .git
# repository
#
mkdir -p build

g++ -D GCOV --coverage test/main.c test/test.c test/mock.cpp arduino/serial9/serial9.cpp -I test -I arduino/serial9  -lCppUTest -lCppUTestExt -o build/test_serial9

build/test_serial9 -ojunit 

junit2html cpputest_Serial9.xml html/cpputest_Serial9.html
rm cpputest_Serial9.xml

lcov -c --rc lcov_branch_coverage=1 -d build -o build/test_serial9.info
lcov -e --rc lcov_branch_coverage=1 build/test_serial9.info "*/arduino/*" -o build/test_serial9.info # Extract the arduino results

genhtml --branch-coverage -o html build/test_serial9.info
