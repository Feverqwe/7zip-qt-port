#pragma once
#include "7zip/IDecl.h"

// Metadata attached to a child byte stream. Opening that stream as a container
// adds its native handler levels to this snapshot of its containing layer.
inline constexpr GUID IID_IArchiveLayerSource = {
    0xfa92d940, 0x7f27, 0x4e71, {0x8d, 0x38, 0x0b, 0xed, 0xcf, 0x80, 0x6c, 0x1d}};
struct IArchiveLayerSource : public IUnknown {
    // S_FALSE means that the caller must supply the containing context.
    Z7_COM7F_PURE(GetContainingContainerDepth(UInt64 *depth))
};

// Folder navigation carries the same context without depending on the concrete
// Agent or backend resource. Ordinary folders inside an archive share its depth.
inline constexpr GUID IID_IArchiveLayerFolder = {
    0x3e021851, 0x8b87, 0x4848, {0xa8, 0xe2, 0x86, 0xf1, 0x7b, 0xa1, 0xc1, 0x47}};
struct IArchiveLayerFolder : public IUnknown {
    // S_FALSE means this folder has no engine-owned layer context.
    Z7_COM7F_PURE(GetContainerDepth(UInt64 *depth))
    Z7_COM7F_PURE(SetParentContainerDepth(UInt64 depth))
};
