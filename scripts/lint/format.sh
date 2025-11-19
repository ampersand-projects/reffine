find benchmark/include benchmark/main.cpp include/ src/ test/ python/ -type f -name "*.cpp" -o -name "*.h" | xargs -i clang-format -i {}
