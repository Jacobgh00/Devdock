#include "cli/terminal_text.hpp"

#include <cstdint>
#include <format>

namespace devdock {

    namespace {

        bool is_continuation_byte(
            unsigned char byte
        ) {
            return (
                       byte & 0xC0U
                   ) == 0x80;
        }

        std::string hex_escape(
            unsigned char byte
        ) {
            return std::format(
                "\\x{:02X}",
                byte
            );
        }

        std::string unicode_escape(
            std::uint32_t code_point
        ) {
            return std::format(
                "\\u{:04X}",
                code_point
            );
        }

        bool is_control_code_point(
            std::uint32_t code_point
        ) {
            return code_point < 0x20 || code_point == 0x7F || (code_point >= 0x80 && code_point <= 0x9F);
        }

        void append_control_escape(
            std::string& output,
            std::uint32_t code_point
        ) {
            switch (code_point) {
            case '\n':
                output += "\\n";
                return;

            case '\r':
                output += "\\r";
                return;

            case '\t':
                output += "\\t";
                return;

            default:
                break;
            }

            if (code_point <= 0x7F) {
                output += hex_escape(
                    static_cast<unsigned char>(
                        code_point
                    )
                );

                return;
            }

            output += unicode_escape(
                code_point
            );
        }

        struct DecodedCodePoint {
            std::uint32_t value;
            std::size_t byte_count;
        };

        DecodedCodePoint decode_utf8(
            std::string_view value,
            std::size_t offset
        ) {
            const auto first =
                static_cast<unsigned char>(
                    value.at(offset)
                );

            if (first < 0x80) {
                return {
                    .value = first,
                    .byte_count = 1,
                };
            }

            std::size_t byte_count = 0;
            std::uint32_t code_point = 0;

            if (
                (first & 0xE0U) == 0xC0
            ) {
                byte_count = 2;
                code_point =
                    first & 0x1FU;
            } else if (
                (first & 0xF0U) == 0xE0
            ) {
                byte_count = 3;
                code_point =
                    first & 0x0FU;
            } else if (
                (first & 0xF8U) == 0xF0
            ) {
                byte_count = 4;
                code_point =
                    first & 0x07U;
            } else {
                return {
                    .value = first,
                    .byte_count = 0,
                };
            }

            if (
                offset + byte_count > value.size()
            ) {
                return {
                    .value = first,
                    .byte_count = 0,
                };
            }

            for (
                std::size_t index = 1;
                index < byte_count;
                ++index) {
                const auto byte =
                    static_cast<unsigned char>(
                        value.at(offset + index)
                    );

                if (
                    !is_continuation_byte(
                        byte
                    )
                ) {
                    return {
                        .value = first,
                        .byte_count = 0,
                    };
                }

                code_point =
                    (code_point << 6U) | (byte & 0x3FU);
            }

            const bool overlong =
                (byte_count == 2 && code_point < 0x80) || (byte_count == 3 && code_point < 0x800) || (byte_count == 4 && code_point < 0x10000);

            const bool invalid_code_point =
                code_point > 0x10FFFF || (code_point >= 0xD800 && code_point <= 0xDFFF);

            if (
                overlong || invalid_code_point
            ) {
                return {
                    .value = first,
                    .byte_count = 0,
                };
            }

            return {
                .value = code_point,
                .byte_count = byte_count,
            };
        }

    } // namespace

    std::string sanitize_terminal_text(
        std::string_view value
    ) {
        std::string output;

        output.reserve(
            value.size()
        );

        for (
            std::size_t offset = 0;
            offset < value.size();) {
            const auto decoded =
                decode_utf8(
                    value,
                    offset
                );

            if (
                decoded.byte_count == 0
            ) {
                output +=
                    hex_escape(
                        static_cast<
                            unsigned char>(value.at(offset))
                    );

                ++offset;
                continue;
            }

            if (
                is_control_code_point(
                    decoded.value
                )
            ) {
                append_control_escape(
                    output,
                    decoded.value
                );
            } else {
                output.append(
                    value.substr(
                        offset,
                        decoded.byte_count
                    )
                );
            }

            offset +=
                decoded.byte_count;
        }

        return output;
    }

} // namespace devdock