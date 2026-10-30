// SPDX-License-Identifier: MIT
#pragma once
#include "7zip/Archive/IArchive.h"
#include "Common/MyBuffer.h"
#include "crypto/fscrypt/fscryptreader.h"
#include <memory>
#include <vector>

using CExtCryptInode = std::shared_ptr<sevenzip_fscrypt_inode>;

// Explicit raw-key input over the existing SDK password callback. Input text is
// fscrypt:v2:<hex>[;v1:<descriptor hex>:<hex>...], never a protector password.
// fscrypt:none explicitly selects the native no-key view.
class CExtCryptKeys {
    using Key = std::unique_ptr<sevenzip_fscrypt_key, decltype(&sevenzip_fscrypt_key_free)>;
    std::vector<Key> keys_;

  public:
    HRESULT Read(IArchiveOpenCallback *callback);
    HRESULT Prepare(sevenzip_fscrypt_inode_options options, CExtCryptInode &result) const;
};

// Changes only acquisition of initialized allocated bytes in the upstream
// mapping streams. Holes remain their original zero-fill path.
class CExtCryptData {
    CExtCryptInode inode_;
    CByteBuffer bytes_;
    UInt64 cachedIndex_ = 0;
    bool cached_ = false;

  public:
    ~CExtCryptData();
    void Init(const CExtCryptInode &inode);
    bool Defined() const { return bool(inode_); }
    HRESULT Read(IInStream *, UInt64 logical, UInt64 physical, void *, UInt32, UInt32 *);
};

HRESULT ExtCryptPresentSymlink(const CExtCryptInode &, CByteBuffer &payload);
HRESULT ExtCryptRequireKey(const CExtCryptInode &);
