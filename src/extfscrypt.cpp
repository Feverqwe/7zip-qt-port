// SPDX-License-Identifier: MIT
#include "extfscrypt.h"
#include "7zip/Common/StreamUtils.h"
#include "7zip/IPassword.h"
#include "Common/MyCom.h"
#include "Common/MyString.h"
#include "Common/StringToInt.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <cwchar>

namespace {
constexpr HRESULT kNoKey = HRESULT(0x80070005L); // E_ACCESSDENIED on the SDK ABI.
HRESULT CryptoError(int error) {
    if (!error)
        return S_OK;
    if (error == -ENOMEM)
        return E_OUTOFMEMORY;
    // ENOKEY is absent from the MSVC errno namespace.
    if (error == -126)
        return kNoKey;
    return S_FALSE;
}

bool ReadHex(const wchar_t *begin, size_t size, Byte *output, size_t capacity) {
    if (size > capacity * 2 || size % 2)
        return false;
    AString_Wipe hex;
    for (size_t i = 0; i < size; ++i) {
        if (begin[i] > 127 || begin[i] == 0)
            return false;
        hex += (char)begin[i];
    }
    return ParseHexString(hex, output) == output + size / 2;
}
} // namespace

HRESULT CExtCryptKeys::Read(IArchiveOpenCallback *callback) {
    keys_.clear();
    if (!callback)
        return S_OK;
    CMyComPtr<ICryptoGetTextPassword> crypto;
    callback->QueryInterface(IID_ICryptoGetTextPassword, (void **)&crypto);
    if (!crypto)
        return S_OK;
    CMyComBSTR_Wipe text;
    RINOK(crypto->CryptoGetTextPassword(&text))
    const wchar_t *input = text;
    const size_t length = SysStringLen(const_cast<BSTR>(input));
    if (!length)
        return S_OK;
    const wchar_t prefix[] = L"fscrypt:";
    if (length < 8 || std::wmemcmp(input, prefix, 8))
        return E_INVALIDARG;
    if (length == 12 && !std::wmemcmp(input + 8, L"none", 4))
        return S_OK;
    std::vector<Key> prepared;
    std::vector<std::array<Byte, 8>> descriptors;
    size_t pos = 8;
    while (pos < length) {
        size_t end = pos;
        while (end < length && input[end] != L';')
            ++end;
        if (end - pos < 3 || input[pos] != L'v' || input[pos + 2] != L':')
            return E_INVALIDARG;
        const unsigned type = input[pos + 1] == L'1'   ? SEVENZIP_FSCRYPT_V1_DESCRIPTOR
                              : input[pos + 1] == L'2' ? SEVENZIP_FSCRYPT_V2_IDENTIFIER
                                                       : 0;
        if (!type)
            return E_INVALIDARG;
        pos += 3;
        Byte descriptor[8] = {};
        if (type == SEVENZIP_FSCRYPT_V1_DESCRIPTOR) {
            if (end - pos < 17 || input[pos + 16] != L':' ||
                !ReadHex(input + pos, 16, descriptor, sizeof(descriptor)))
                return E_INVALIDARG;
            pos += 17;
            std::array<Byte, 8> value;
            std::copy(std::begin(descriptor), std::end(descriptor), value.begin());
            if (std::find(descriptors.begin(), descriptors.end(), value) != descriptors.end())
                return E_INVALIDARG;
            descriptors.push_back(value);
        }
        const size_t digits = end - pos;
        CByteBuffer_Wipe raw(64);
        if (digits < 32 || !ReadHex(input + pos, digits, raw, raw.Size()))
            return E_INVALIDARG;
        sevenzip_fscrypt_key *key = nullptr;
        const int status = sevenzip_fscrypt_key_create(type, raw, digits / 2, descriptor, &key);
        if (status)
            return status == -ENOMEM ? E_OUTOFMEMORY : E_INVALIDARG;
        Key owner(key, sevenzip_fscrypt_key_free);
        prepared.push_back(std::move(owner));
        if (end + 1 == length)
            return E_INVALIDARG;
        pos = end + 1;
    }
    if (prepared.empty())
        return E_INVALIDARG;
    keys_ = std::move(prepared);
    return S_OK;
}

HRESULT CExtCryptKeys::Prepare(sevenzip_fscrypt_inode_options options,
                               CExtCryptInode &result) const {
    std::vector<const sevenzip_fscrypt_key *> keys;
    for (const auto &key : keys_)
        keys.push_back(key.get());
    options.keys = keys.data();
    options.key_count = keys.size();
    sevenzip_fscrypt_inode *inode = nullptr;
    const int status = sevenzip_fscrypt_inode_create(&options, &inode);
    if (status)
        return CryptoError(status);
    result = CExtCryptInode(inode, sevenzip_fscrypt_inode_free);
    return S_OK;
}

CExtCryptData::~CExtCryptData() { bytes_.Wipe(); }

void CExtCryptData::Init(const CExtCryptInode &inode) {
    inode_ = inode;
    cached_ = false;
    bytes_.Wipe();
    bytes_.Alloc(sevenzip_fscrypt_data_unit_size(inode.get()));
}

HRESULT CExtCryptData::Read(IInStream *stream, UInt64 logical, UInt64 physical, void *out,
                            UInt32 size, UInt32 *processed) {
    if (processed)
        *processed = 0;
    const size_t unit = bytes_.Size();
    if (!unit)
        return kNoKey;
    const UInt64 index = logical / unit;
    const size_t offset = (size_t)(logical % unit);
    if (physical < offset)
        return S_FALSE;
    if (!cached_ || index != cachedIndex_) {
        cached_ = false;
        RINOK(InStream_SeekSet(stream, physical - offset))
        RINOK(ReadStream_FALSE(stream, bytes_, unit))
        const int status = sevenzip_fscrypt_decrypt_unit(inode_.get(), index, bytes_, bytes_, unit);
        if (status)
            return CryptoError(status);
        cachedIndex_ = index;
        cached_ = true;
    }
    const UInt32 amount = (UInt32)std::min<size_t>(size, unit - offset);
    memcpy(out, (const Byte *)bytes_ + offset, amount);
    if (processed)
        *processed = amount;
    return S_OK;
}

HRESULT ExtCryptPresentSymlink(const CExtCryptInode &inode, CByteBuffer &payload) {
    CByteBuffer output(std::max<size_t>(payload.Size(), 252) + 1);
    size_t size = 0;
    const int status = sevenzip_fscrypt_present_symlink(inode.get(), payload, payload.Size(),
                                                        output, output.Size(), &size);
    if (status)
        return CryptoError(status);
    payload.CopyFrom(output, size);
    return S_OK;
}

HRESULT ExtCryptRequireKey(const CExtCryptInode &inode) {
    return CryptoError(sevenzip_fscrypt_require_key(inode.get()));
}
