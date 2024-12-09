find src -type f -name "*.cpp" | xargs -i clang-format --dry-run -Werror -i {}
find include -type f -name "*.h" | xargs -i clang-format --dry-run -Werror -i {}
