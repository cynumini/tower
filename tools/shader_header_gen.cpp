#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <span>

int main(int argc, char *argv[]) {
    constexpr auto MIN_ARGC = 3; // bin_path, input..., output
    if (argc < MIN_ARGC) return 1;
    auto args = std::span(argv, argc).subspan(1); // skip bin_path
    auto *output = args.back();
    auto inputs = args.first(args.size() - 1);
    std::ofstream output_file(output);
    output_file << "#pragma once\n\n#include <array>\n#include <cstdint>\n";
    for (auto &input : inputs) {
        auto path = std::filesystem::path(input);
        std::ifstream file(path, std::ios::binary);
        auto filename = path.filename().string();
        std::ranges::replace(filename, '.', '_');
        output_file << "\ninline constexpr std::array<std::uint8_t, "
                    << std::filesystem::file_size(path) << "> " << filename << " = {\n   ";
        char c = 0;
        size_t i = 0;
        constexpr size_t bytes_per_line = 15;
        while (file.get(c)) {
            output_file << std::format(" 0x{:02x},", static_cast<uint8_t>(c));
            if ((++i) == bytes_per_line) {
                output_file << "\n   ";
                i = 0;
            }
        }
        output_file << "\n};\n";
    }
    return 0;
}
