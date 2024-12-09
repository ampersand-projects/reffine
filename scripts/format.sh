find src -type f -name "*.cpp" | xargs -i clang-format -i {}
find include -type f -name "*.h" | xargs -i clang-format -i {}
