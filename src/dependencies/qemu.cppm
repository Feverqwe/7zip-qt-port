// SPDX-License-Identifier: MIT
module;
#include "../block/qcow.h"
#include "../block/qcow2.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>

export module sevenzip.dependencies.qemu;

export {
#include "qemu.h"
}
