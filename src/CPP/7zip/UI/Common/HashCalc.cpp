// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Native CHashBundle/CHasherState and digest formatting only. Hash file reading,
// writing, verification and signing remain mandatory in their hash owner.
#include "HashCalc.h"
#include "../../../../../third_party/7zip/C/CpuArch.h"
#include "Common/IntToString.h"
using namespace NWindows;
static const char *const k_DefaultHashMethod = "CRC32";

HRESULT CHashBundle::SetMethods(DECL_EXTERNAL_CODECS_LOC_VARS const UStringVector &hashMethods) {
    UStringVector names = hashMethods;
    if (names.IsEmpty())
        names.Add(UString(k_DefaultHashMethod));

    CRecordVector<CMethodId> ids;
    CObjectVector<COneMethodInfo> methods;

    unsigned i;
    for (i = 0; i < names.Size(); i++) {
        COneMethodInfo m;
        RINOK(m.ParseMethodFromString(names[i]))

        if (m.MethodName.IsEmpty())
            m.MethodName = k_DefaultHashMethod;

        if (m.MethodName.IsEqualTo("*")) {
            CRecordVector<CMethodId> tempMethods;
            GetHashMethods(EXTERNAL_CODECS_LOC_VARS tempMethods);
            methods.Clear();
            ids.Clear();
            FOR_VECTOR(t, tempMethods) {
                unsigned index = ids.AddToUniqueSorted(tempMethods[t]);
                if (ids.Size() != methods.Size())
                    methods.Insert(index, m);
            }
            break;
        } else {
            // m.MethodName.RemoveChar(L'-');
            CMethodId id;
            if (!FindHashMethod(EXTERNAL_CODECS_LOC_VARS m.MethodName, id))
                return E_NOTIMPL;
            unsigned index = ids.AddToUniqueSorted(id);
            if (ids.Size() != methods.Size())
                methods.Insert(index, m);
        }
    }

    for (i = 0; i < ids.Size(); i++) {
        CMyComPtr<IHasher> hasher;
        AString name;
        RINOK(CreateHasher(EXTERNAL_CODECS_LOC_VARS ids[i], name, hasher))
        if (!hasher)
            throw "Can't create hasher";
        const COneMethodInfo &m = methods[i];
        {
            CMyComPtr<ICompressSetCoderProperties> scp;
            hasher.QueryInterface(IID_ICompressSetCoderProperties, &scp);
            if (scp)
                RINOK(m.SetCoderProps(scp, NULL))
        }
        const UInt32 digestSize = hasher->GetDigestSize();
        if (digestSize > k_HashCalc_DigestSize_Max)
            return E_NOTIMPL;
        CHasherState &h = Hashers.AddNew();
        h.DigestSize = digestSize;
        h.Hasher = hasher;
        h.Name = name;
        for (unsigned k = 0; k < k_HashCalc_NumGroups; k++)
            h.InitDigestGroup(k);
    }

    return S_OK;
}

void CHashBundle::InitForNewFile() {
    CurSize = 0;
    FOR_VECTOR(i, Hashers) {
        CHasherState &h = Hashers[i];
        h.Hasher->Init();
        h.InitDigestGroup(k_HashCalc_Index_Current);
    }
}

void CHashBundle::Update(const void *data, UInt32 size) {
    CurSize += size;
    FOR_VECTOR(i, Hashers)
    Hashers[i].Hasher->Update(data, size);
}

void CHashBundle::SetSize(UInt64 size) { CurSize = size; }

static void AddDigests(Byte *dest, const Byte *src, UInt32 size) {
    unsigned next = 0;
    /*
    // we could use big-endian addition for sha-1 and sha-256
    // but another hashers are little-endian
    if (size > 8)
    {
      for (unsigned i = size; i != 0;)
      {
        i--;
        next += (unsigned)dest[i] + (unsigned)src[i];
        dest[i] = (Byte)next;
        next >>= 8;
      }
    }
    else
    */
    {
        for (unsigned i = 0; i < size; i++) {
            next += (unsigned)dest[i] + (unsigned)src[i];
            dest[i] = (Byte)next;
            next >>= 8;
        }
    }

    // we use little-endian to store extra bytes
    dest += k_HashCalc_DigestSize_Max;
    for (unsigned i = 0; i < k_HashCalc_ExtraSize; i++) {
        next += (unsigned)dest[i];
        dest[i] = (Byte)next;
        next >>= 8;
    }
}

void CHasherState::AddDigest(unsigned groupIndex, const Byte *data) {
    NumSums[groupIndex]++;
    AddDigests(Digests[groupIndex], data, DigestSize);
}

void CHashBundle::Final(bool isDir, bool isAltStream, const UString &path) {
    if (isDir)
        NumDirs++;
    else if (isAltStream) {
        NumAltStreams++;
        AltStreamsSize += CurSize;
    } else {
        NumFiles++;
        FilesSize += CurSize;
    }

    Byte pre[16];
    memset(pre, 0, sizeof(pre));
    if (isDir)
        pre[0] = 1;

    FOR_VECTOR(i, Hashers) {
        CHasherState &h = Hashers[i];
        if (!isDir) {
            h.Hasher->Final(h.Digests[0]); // k_HashCalc_Index_Current
            if (!isAltStream)
                h.AddDigest(k_HashCalc_Index_DataSum, h.Digests[0]);
        }

        h.Hasher->Init();
        h.Hasher->Update(pre, sizeof(pre));
        h.Hasher->Update(h.Digests[0], h.DigestSize);

        for (unsigned k = 0; k < path.Len(); k++) {
            wchar_t c = path[k];

// 21.04: we want same hash for linux and windows paths
#if CHAR_PATH_SEPARATOR != '/'
            if (c == CHAR_PATH_SEPARATOR)
                c = '/';
// if (c == (wchar_t)('\\' + 0xf000)) c = '\\'; // to debug WSL
// if (c > 0xf000 && c < 0xf080) c -= 0xf000; // to debug WSL
#endif

            Byte temp[2] = {(Byte)(c & 0xFF), (Byte)((c >> 8) & 0xFF)};
            h.Hasher->Update(temp, 2);
        }

        Byte tempDigest[k_HashCalc_DigestSize_Max];
        h.Hasher->Final(tempDigest);
        if (!isAltStream)
            h.AddDigest(k_HashCalc_Index_NamesSum, tempDigest);
        h.AddDigest(k_HashCalc_Index_StreamsSum, tempDigest);
    }
}

void HashHexToString(char *dest, const Byte *data, size_t size) {
    if (!data) {
        for (size_t i = 0; i < size; i++) {
            dest[0] = ' ';
            dest[1] = ' ';
            dest += 2;
        }
        *dest = 0;
        return;
    }

    if (size > 8)
        ConvertDataToHex_Lower(dest, data, size);
    else if (size == 0) {
        *dest = 0;
        return;
    } else {
        const char *dest_start = dest;
        dest += size * 2;
        *dest = 0;
        do {
            const size_t b = *data++;
            dest -= 2;
            dest[0] = GET_HEX_CHAR_UPPER(b >> 4);
            dest[1] = GET_HEX_CHAR_UPPER(b & 15);
        } while (dest != dest_start);
    }
}

void CHasherState::WriteToString(unsigned digestIndex, char *s) const {
    HashHexToString(s, Digests[digestIndex], DigestSize);

    if (digestIndex != 0 && NumSums[digestIndex] != 1) {
        unsigned numExtraBytes = GetNumExtraBytes_for_Group(digestIndex);
        if (numExtraBytes > 4)
            numExtraBytes = 8;
        else // if (numExtraBytes >= 0)
            numExtraBytes = 4;
        // if (numExtraBytes != 0)
        {
            s += strlen(s);
            *s++ = '-';
            // *s = 0;
            HashHexToString(s, GetExtraData_for_Group(digestIndex), numExtraBytes);
        }
    }
}
