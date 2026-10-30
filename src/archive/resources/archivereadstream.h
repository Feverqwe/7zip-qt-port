#pragma once
#include "7zip/IDecl.h"
#include "7zip/IProgress.h"

// A request belongs to one synchronous call, never to the stream lifetime.
inline constexpr GUID IID_IArchiveReadStream = {
    0xce1bdd9b, 0x460e, 0x4fda, {0x9d, 0x80, 0xa4, 0xb0, 0x42, 0x91, 0x9c, 0xb9}};
struct IArchiveReadStream : public IUnknown {
    Z7_COM7F_PURE(ReadWithRequest(void *data, UInt32 size, UInt32 *processed, IProgress *request))
    Z7_COM7F_PURE(SeekWithRequest(Int64 offset, UInt32 origin, UInt64 *position,
                                  IProgress *request))
};

struct ISequentialInStream;
// Native GetStream can read parent metadata. Supply its operation callback
// while acquiring the real stream, before publishing any seek capability.
inline constexpr GUID IID_IArchiveGetStreamWithRequest = {
    0xd63cdb4b, 0x2bb3, 0x4241, {0x97, 0x73, 0x23, 0x5b, 0x87, 0x88, 0xf4, 0x1d}};
struct IArchiveGetStreamWithRequest : public IUnknown {
    Z7_COM7F_PURE(GetStreamWithRequest(UInt32 index, ISequentialInStream **stream,
                                       IProgress *request))
};
