#!/bin/bash 
find ./examples ./GLwrapper ./utils  -regex '.*\.\(cpp\|hpp\|cc\|cxx\|hh\|h\)' -exec clang-format.exe -i {} \;
