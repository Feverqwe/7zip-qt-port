// SPDX-License-Identifier: MIT
module;
#include "dependencies/qemu/qcow.h"
#include "dependencies/qemu/qcow2.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>

export module sevenzip.dependencies.qemu;

export {
#include "dependencies/qemu/qemu.h"
}
