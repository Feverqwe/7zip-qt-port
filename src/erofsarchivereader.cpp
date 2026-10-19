// SPDX-License-Identifier: GPL-2.0-only
// Archive stream, cancellation and decompression services for Linux EROFS.
#include "erofsarchivereader.h"
#include "7zip/Common/StreamObjects.h"
#include "7zip/Common/StreamUtils.h"
#include "7zip/Compress/DeflateDecoder.h"
#include "Common/UTFConvert.h"
#include <cerrno>
#include <cstring>
#include <new>

int ErofsArchiveReader::Read(void *context, unsigned int device, uint64_t offset, void *data,
                             size_t size) {
    auto &self = *static_cast<ErofsArchiveReader *>(context);
    if (self.transport_ != S_OK)
        return -EIO;
    if (device >= self.streams_.size()) {
        self.transport_ = E_FAIL;
        return -EIO;
    }
    try {
        self.transport_ =
            InStream_SeekSet(self.streams_[device], offset + (device ? 0 : self.base_));
        if (self.transport_ == S_OK)
            self.transport_ = ReadStream_FALSE(self.streams_[device], data, size);
    } catch (...) {
        self.transport_ = E_FAIL;
    }
    return self.transport_ == S_OK ? 0 : -EIO;
}

int ErofsArchiveReader::Poll(void *context) {
    auto &self = *static_cast<ErofsArchiveReader *>(context);
    if (self.transport_ != S_OK)
        return -EIO;
    try {
        if (self.openCallback)
            self.transport_ = self.openCallback->SetCompleted(&self.files, nullptr);
        else if (self.extractCallback)
            self.transport_ = self.extractCallback->SetCompleted(&self.completed);
    } catch (...) {
        self.transport_ = E_FAIL;
    }
    return self.transport_ == S_OK ? 0 : -EIO;
}

int ErofsArchiveReader::Decode(void *context, unsigned int algorithm, unsigned int dictionary,
                               const void *input, size_t inputSize, void *output, size_t outputSize,
                               int partial) {
    auto &self = *static_cast<ErofsArchiveReader *>(context);
    if (self.transport_ != S_OK)
        return -EIO;
    if (algorithm != 2)
        return erofs_decode_native(algorithm, dictionary, input, inputSize, output, outputSize,
                                   partial);
    try {
        CMyComPtr2<ISequentialInStream, CBufInStream> source;
        source.Create_if_Empty();
        source->Init(static_cast<const Byte *>(input), inputSize);
        CMyComPtr2<ISequentialOutStream, CBufPtrSeqOutStream> dest;
        dest.Create_if_Empty();
        dest->Init(static_cast<Byte *>(output), outputSize);
        auto *spec = new NCompress::NDeflate::NDecoder::CCOMCoder;
        CMyComPtr<ICompressCoder> coder = spec;
        spec->Set_NeedFinishInput(!partial);
        const UInt64 expected = outputSize;
        const HRESULT result = coder->Code(source, dest, nullptr, &expected, nullptr);
        if (FAILED(result))
            self.transport_ = result;
        return result == S_OK && dest->GetPos() == outputSize ? 0 : -EILSEQ;
    } catch (const std::bad_alloc &) {
        self.transport_ = E_OUTOFMEMORY;
    } catch (...) {
        self.transport_ = E_FAIL;
    }
    return -EIO;
}

int ErofsArchiveReader::OpenDevice(void *context, unsigned int id, const char *tag, uint64_t size) {
    auto &self = *static_cast<ErofsArchiveReader *>(context);
    try {
        const size_t length = strlen(tag);
        const auto unavailable = [&self] {
            self.missingDevice_ = true;
            return -ENOENT;
        };
        if (!length || (length == 1 && tag[0] == '.') || (length == 2 && !strcmp(tag, "..")))
            return unavailable();
        for (size_t i = 0; i < length; ++i)
            if (static_cast<unsigned char>(tag[i]) < 32 || tag[i] == '/' || tag[i] == '\\' ||
                tag[i] == ':')
                return unavailable();
        UString name;
        if (!ConvertUTF8ToUnicode(AString(tag), name))
            return unavailable();
        CMyComPtr<IInStream> stream;
        self.transport_ = self.volumes_->GetStream(name, &stream);
        if (self.transport_ == S_FALSE || (self.transport_ == S_OK && !stream)) {
            self.transport_ = S_OK;
            self.missingDevice_ = true;
            return -ENOENT;
        }
        if (self.transport_ != S_OK)
            return -EIO;
        UInt64 available;
        self.transport_ = InStream_GetSize_SeekToEnd(stream, available);
        if (self.transport_ != S_OK)
            return -EIO;
        if (available < size)
            return -EIO;
        if (self.streams_.size() <= id)
            self.streams_.resize(size_t(id) + 1);
        self.streams_[id] = stream;
        return 0;
    } catch (const std::bad_alloc &) {
        self.transport_ = E_OUTOFMEMORY;
    } catch (...) {
        self.transport_ = E_FAIL;
    }
    return -EIO;
}

HRESULT ErofsArchiveReader::Result(int error) const {
    if (!error)
        return S_OK;
    if (transport_ != S_OK)
        return transport_;
    return error == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
}

HRESULT ErofsArchiveReader::Open(IInStream *stream, IArchiveOpenCallback *callback) {
    streams_.push_back(stream);
    openCallback = callback;
    struct Reset {
        ErofsArchiveReader &reader;
        ~Reset() {
            reader.openCallback = nullptr;
            reader.volumes_.Release();
        }
    } reset{*this};
    RINOK(stream->Seek(0, STREAM_SEEK_CUR, &base_))
    UInt64 size;
    RINOK(InStream_GetSize_SeekToEnd(stream, size))
    if (size < base_)
        return S_FALSE;
    if (callback)
        callback->QueryInterface(IID_IArchiveOpenVolumeCallback,
                                 reinterpret_cast<void **>(&volumes_));
    erofs_source source{this, size - base_, Read, Poll, Decode, OpenDevice, volumes_ ? 1 : 0};
    erofs_volume *volume = nullptr;
    int error = erofs_open_volume(&source, &volume);
    if (error && missingDevice_ && transport_ == S_OK) {
        streams_.resize(1);
        source.external_devices = 0;
        error = erofs_open_volume(&source, &volume);
    }
    volume_.reset(volume);
    RINOK(Result(error))
    erofs_get_volume_info(volume_.get(), &info);
    return S_OK;
}

HRESULT ErofsArchiveReader::OpenFile(uint64_t nid, ErofsFile &file) {
    erofs_file *raw = nullptr;
    const int error = erofs_open_file(volume_.get(), nid, &raw);
    file.reset(raw);
    return Result(error);
}
