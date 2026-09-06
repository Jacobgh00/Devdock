#pragma once

#include <string>
#include <string_view>

namespace devdock {

    std::string sanitize_terminal_text(
        std::string_view value
    );

}