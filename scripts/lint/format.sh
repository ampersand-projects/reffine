find include/ src/ -type f -name "*.cpp" -o -name "*.h" | xargs -i clang-format -i {}
