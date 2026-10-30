// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <expected>
#include <functional>
#include <memory>
// clang-format off
#include <smb2/smb2.h>
#include <smb2/libsmb2.h>
// clang-format on
#include <span>
#include <type_traits>

namespace sevenzip::smb2 {
struct NameDeleter {
    void operator()(const char *name) const noexcept { std::free(const_cast<char *>(name)); }
};
using Name = std::unique_ptr<const char, NameDeleter>;
enum class NameError { conversion_failed };

[[nodiscard]] inline auto decode_name(std::span<const std::uint16_t> name) noexcept
    -> std::expected<Name, NameError> {
    static constexpr std::uint16_t empty = 0;
    Name decoded{smb2_utf16_to_utf8(name.empty() ? &empty : name.data(), name.size())};
    if (!decoded)
        return std::unexpected(NameError::conversion_failed);
    return decoded;
}

// libsmb2 invokes the connection callback synchronously inside serve_port.
// Contexts remain owned by the library; the callback borrows each session.
template <typename Connection>
    requires std::is_nothrow_invocable_r_v<void, Connection &, smb2_context &>
[[nodiscard]] auto serve(smb2_server &server, int backlog, Connection &connection) noexcept
    -> std::expected<void, int> {
    const int result = smb2_serve_port(
        &server, backlog,
        [](smb2_context *context, void *data) noexcept {
            std::invoke(*static_cast<Connection *>(data), *context);
        },
        &connection);
    if (result)
        return std::unexpected(result);
    return {};
}
} // namespace sevenzip::smb2
