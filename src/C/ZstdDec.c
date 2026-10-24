/* ZstdDec.c -- Zstd Decoder
2024-06-18 : the code was developed by Igor Pavlov, using Zstandard format
             specification and original zstd decoder code as reference code.
original zstd decoder code: Copyright (c) Facebook, Inc. All rights reserved.
This source code is licensed under BSD 3-Clause License.
*/

/* Adaptation: keep the upstream frame parser, result/metadata API and window
   ownership. Official libzstd supplies compressed-block entropy/LZ decoding. */
#include "Precomp.h"
#include <string.h>
#include "Alloc.h"
#include "Xxh64.h"
#include "ZstdDec.h"
#include "CpuArch.h"
#define ZSTD_STATIC_LINKING_ONLY
/* The pinned block API preserves the upstream parser's recovery boundaries. */
#define ZSTD_DISABLE_DEPRECATE_WARNINGS
#include "../zstd/lib/zstd.h"
#include "../zstd/lib/zstd_errors.h"

#define MAX_WINDOW_SIZE_LOG 31
#define kBlockSizeMax (1u << 17)
#define COPY_CHUNK_SIZE 16
#define MAX_BACKWARD_DEPTH 0
#define STAT_INC(v)
#define STAT_UPDATE(v)

typedef struct
{
  Byte *win;
  SizeT cycSize;
  SizeT winPos;
  size_t winSize;
  ZSTD_DCtx *context;
} CZstdDec1;

#define ZstdDec1_GET_BLOCK_SIZE_LIMIT(p) \
  ((p)->winSize < kBlockSizeMax ? (UInt32)(p)->winSize : kBlockSizeMax)
#define UPDATE_TOTAL_OUT(p, size) ((void)0);

static void *ZstdDec_Alloc(void *opaque, size_t size)
{
  return ISzAlloc_Alloc((ISzAllocPtr)opaque, size);
}

static void ZstdDec_Free(void *opaque, void *address)
{
  ISzAlloc_Free((ISzAllocPtr)opaque, address);
}

static void ZstdDec1_Construct(CZstdDec1 *p, ISzAllocPtr alloc)
{
  ZSTD_customMem memory;
  memory.customAlloc = ZstdDec_Alloc;
  memory.customFree = ZstdDec_Free;
  memory.opaque = (void *)alloc;
  p->win = NULL;
  p->cycSize = 0;
  p->context = ZSTD_createDCtx_advanced(memory);
}

static void ZstdDec1_Init(CZstdDec1 *p)
{
  ZSTD_decompressBegin(p->context);
}

static SRes ZstdDec1_DecodeBlock(CZstdDec1 *p, const Byte *src,
    SizeT inSize, SizeT afterAvail, size_t outLimit)
{
  const size_t size = ZSTD_decompressBlock(p->context,
      p->win + p->winPos, outLimit, src, inSize);
  UNUSED_VAR(afterAvail)
  if (ZSTD_isError(size))
    return ZSTD_getErrorCode(size) == ZSTD_error_memory_allocation ?
        SZ_ERROR_MEM : SZ_ERROR_DATA;
  p->winPos += size;
  return SZ_OK;
}

static size_t ZstdDec1_NeedTempBufferForInput(
    SizeT beforeSize, const Byte *src, SizeT inSize)
{
  UNUSED_VAR(beforeSize)
  UNUSED_VAR(src)
  UNUSED_VAR(inSize)
  return 0;
}

// ---------- ZSTD FRAME ----------

#define kBlockType_Raw          0
#define kBlockType_RLE          1
#define kBlockType_Compressed   2
#define kBlockType_Reserved     3

typedef enum
{
  // begin: states that require 4 bytes:
  ZSTD2_STATE_SIGNATURE,
  ZSTD2_STATE_HASH,
  ZSTD2_STATE_SKIP_HEADER,
  // end of states that require 4 bytes

  ZSTD2_STATE_SKIP_DATA,
  ZSTD2_STATE_FRAME_HEADER,
  ZSTD2_STATE_AFTER_HEADER,
  ZSTD2_STATE_BLOCK,
  ZSTD2_STATE_DATA,
  ZSTD2_STATE_FINISHED
} EZstd2State;


struct CZstdDec
{
  EZstd2State frameState;
  unsigned tempSize;

  Byte temp[14]; // 14 is required

  Byte descriptor;
  Byte windowDescriptor;
  Byte isLastBlock;
  Byte blockType;
  Byte isErrorState;
  Byte hashError;
  Byte disableHash;
  Byte isCyclicMode;

  UInt32 blockSize;
  UInt32 dictionaryId;
  UInt32 curBlockUnpackRem; // for compressed blocks only
  UInt32 inTempPos;

  UInt64 contentSize;
  UInt64 contentProcessed;
  CXxh64State xxh64;

  Byte *inTemp;
  SizeT winBufSize_Allocated;
  Byte *win_Base;

  ISzAllocPtr alloc_Small;
  ISzAllocPtr alloc_Big;

  CZstdDec1 decoder;
};

#define ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p) \
  ((unsigned)(p)->contentProcessed & (Z7_XXH64_BLOCK_SIZE - 1))

#define ZSTD_DEC_IS_LAST_BLOCK(p) ((p)->isLastBlock)


static void ZstdDec_FreeWindow(CZstdDec * const p)
{
  if (p->win_Base)
  {
    ISzAlloc_Free(p->alloc_Big, p->win_Base);
    p->win_Base = NULL;
    // p->decoder.win = NULL;
    p->winBufSize_Allocated = 0;
  }
}


CZstdDecHandle ZstdDec_Create(ISzAllocPtr alloc_Small, ISzAllocPtr alloc_Big)
{
  CZstdDec *p = (CZstdDec *)ISzAlloc_Alloc(alloc_Small, sizeof(CZstdDec));
  if (!p)
    return NULL;
  p->alloc_Small = alloc_Small;
  p->alloc_Big = alloc_Big;
  // ZstdDec_CONSTRUCT(p)
  p->inTemp = NULL;
  p->win_Base = NULL;
  p->winBufSize_Allocated = 0;
  p->disableHash = False;
  ZstdDec1_Construct(&p->decoder, alloc_Small);
  if (!p->decoder.context)
  {
    ISzAlloc_Free(alloc_Small, p);
    return NULL;
  }
  return p;
}

void ZstdDec_Destroy(CZstdDecHandle p)
{

  ZSTD_freeDCtx(p->decoder.context);
  // p->->decoder.literalsBase = NULL;
  ISzAlloc_Free(p->alloc_Small, p->inTemp);
  // p->inTemp = NULL;
  ZstdDec_FreeWindow(p);
  ISzAlloc_Free(p->alloc_Small, p);
}



#define kTempBuffer_PreSize  (1u << 6)
#if kTempBuffer_PreSize < MAX_BACKWARD_DEPTH
  #error Stop_Compiling_Bad_kTempBuffer_PreSize
#endif

static SRes ZstdDec_AllocateMisc(CZstdDec *p)
{
  #define k_Lit_AfterAvail  (1u << 6)
  #if k_Lit_AfterAvail < (COPY_CHUNK_SIZE - 1)
    #error Stop_Compiling_Bad_k_Lit_AfterAvail
  #endif
  // return ZstdDec1_Allocate(&p->decoder, p->alloc_Small);
  if (!p->inTemp)
  {
    // we need k_Lit_AfterAvail here for owerread from raw literals stream
    p->inTemp = (Byte *)ISzAlloc_Alloc(p->alloc_Small,
        kBlockSizeMax + kTempBuffer_PreSize + k_Lit_AfterAvail);
    if (!p->inTemp)
      return SZ_ERROR_MEM;
  }
  return SZ_OK;
}


static void ZstdDec_Init_ForNewFrame(CZstdDec *p)
{
  p->frameState = ZSTD2_STATE_SIGNATURE;
  p->tempSize = 0;

  p->isErrorState = False;
  p->hashError = False;
  p->isCyclicMode = False;
  p->contentProcessed = 0;
  Xxh64State_Init(&p->xxh64);
  ZstdDec1_Init(&p->decoder);
}


void ZstdDec_Init(CZstdDec *p)
{
  ZstdDec_Init_ForNewFrame(p);
  p->decoder.winPos = 0;
  memset(p->temp, 0, sizeof(p->temp));
}


#define DESCRIPTOR_Get_DictionaryId_Flag(d)   ((d) & 3)
#define DESCRIPTOR_FLAG_CHECKSUM              (1 << 2)
#define DESCRIPTOR_FLAG_RESERVED              (1 << 3)
// #define DESCRIPTOR_FLAG_UNUSED                (1 << 4)
#define DESCRIPTOR_FLAG_SINGLE                (1 << 5)
#define DESCRIPTOR_Get_ContentSize_Flag3(d)   ((d) >> 5)
#define DESCRIPTOR_Is_ContentSize_Defined(d)  (((d) & 0xe0) != 0)


static EZstd2State ZstdDec_UpdateState(CZstdDec * const p, const Byte b, CZstdDecInfo * const info)
{
  unsigned tempSize = p->tempSize;
  p->temp[tempSize++] = b;
  p->tempSize = tempSize;

  if (p->frameState == ZSTD2_STATE_BLOCK)
  {
    if (tempSize < 3)
      return ZSTD2_STATE_BLOCK;
    {
      UInt32 b0 = GetUi32(p->temp);
      const unsigned type = ((unsigned)b0 >> 1) & 3;
      if (type == kBlockType_RLE && tempSize == 3)
        return ZSTD2_STATE_BLOCK;
      // info->num_Blocks_forType[type]++;
      info->num_Blocks++;
      if (type == kBlockType_Reserved)
      {
        p->isErrorState = True; // SZ_ERROR_UNSUPPORTED
        return ZSTD2_STATE_BLOCK;
      }
      p->blockType = (Byte)type;
      p->isLastBlock = (Byte)(b0 & 1);
      p->inTempPos = 0;
      p->tempSize = 0;
      b0 >>= 3;
      b0 &= 0x1fffff;
      // info->num_BlockBytes_forType[type] += b0;
      if (b0 == 0)
      {
        // empty RAW/RLE blocks are allowed in original-zstd decoder
        if (type == kBlockType_Compressed)
        {
          p->isErrorState = True;
          return ZSTD2_STATE_BLOCK;
        }
        if (!ZSTD_DEC_IS_LAST_BLOCK(p))
          return ZSTD2_STATE_BLOCK;
        if (p->descriptor & DESCRIPTOR_FLAG_CHECKSUM)
          return ZSTD2_STATE_HASH;
        return ZSTD2_STATE_FINISHED;
      }
      p->blockSize = b0;
      {
        UInt32 blockLim = ZstdDec1_GET_BLOCK_SIZE_LIMIT(&p->decoder);
        // compressed and uncompressed block sizes cannot be larger than min(kBlockSizeMax, window_size)
        if (b0 > blockLim)
        {
          p->isErrorState = True; // SZ_ERROR_UNSUPPORTED;
          return ZSTD2_STATE_BLOCK;
        }
        if (DESCRIPTOR_Is_ContentSize_Defined(p->descriptor))
        {
          const UInt64 rem = p->contentSize - p->contentProcessed;
          if (blockLim > rem)
              blockLim = (UInt32)rem;
        }
        p->curBlockUnpackRem = blockLim;
        // uncompressed block size cannot be larger than remain data size:
        if (type != kBlockType_Compressed)
        {
          if (b0 > blockLim)
          {
            p->isErrorState = True; // SZ_ERROR_UNSUPPORTED;
            return ZSTD2_STATE_BLOCK;
          }
        }
      }
    }
    return ZSTD2_STATE_DATA;
  }

  if ((unsigned)p->frameState < ZSTD2_STATE_SKIP_DATA)
  {
    UInt32 v;
    if (tempSize != 4)
      return p->frameState;
    v = GetUi32(p->temp);
    if ((unsigned)p->frameState < ZSTD2_STATE_HASH) // == ZSTD2_STATE_SIGNATURE
    {
      if (v == 0xfd2fb528)
      {
        p->tempSize = 0;
        info->num_DataFrames++;
        return ZSTD2_STATE_FRAME_HEADER;
      }
      if ((v & 0xfffffff0) == 0x184d2a50)
      {
        p->tempSize = 0;
        info->num_SkipFrames++;
        return ZSTD2_STATE_SKIP_HEADER;
      }
      p->isErrorState = True;
      return ZSTD2_STATE_SIGNATURE;
      // return ZSTD2_STATE_ERROR; // is not ZSTD stream
    }
    if (p->frameState == ZSTD2_STATE_HASH)
    {
      info->checksum_Defined = True;
      info->checksum = v;
      // #ifndef DISABLE_XXH_CHECK
      if (!p->disableHash)
      {
        if (p->decoder.winPos < ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p))
        {
          // unexpected code failure
          p->isErrorState = True;
          // SZ_ERROR_FAIL;
        }
        else
        if ((UInt32)Xxh64State_Digest(&p->xxh64,
            p->decoder.win + (p->decoder.winPos - ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p)),
            p->contentProcessed) != v)
        {
          p->hashError = True;
          // return ZSTD2_STATE_ERROR; // hash error
        }
      }
      // #endif
      return ZSTD2_STATE_FINISHED;
    }
    // (p->frameState == ZSTD2_STATE_SKIP_HEADER)
    {
      p->blockSize = v;
      info->skipFrames_Size += v;
      p->tempSize = 0;
      /* we want the caller could know that there was finished frame
         finished frame. So we allow the case where
         we have ZSTD2_STATE_SKIP_DATA state with (blockSize == 0).
      */
      // if (v == 0) return ZSTD2_STATE_SIGNATURE;
      return ZSTD2_STATE_SKIP_DATA;
    }
  }

  // if (p->frameState == ZSTD2_STATE_FRAME_HEADER)
  {
    unsigned descriptor;
    const Byte *h;
    descriptor = p->temp[0];
    p->descriptor = (Byte)descriptor;
    if (descriptor & DESCRIPTOR_FLAG_RESERVED) // reserved bit
    {
      p->isErrorState = True;
      return ZSTD2_STATE_FRAME_HEADER;
      // return ZSTD2_STATE_ERROR;
    }
    {
      const unsigned n = DESCRIPTOR_Get_ContentSize_Flag3(descriptor);
      // tempSize -= 1 + ((1u << (n >> 1)) | ((n + 1) & 1));
      tempSize -= (0x9a563422u >> (n * 4)) & 0xf;
    }
    if (tempSize != (4u >> (3 - DESCRIPTOR_Get_DictionaryId_Flag(descriptor))))
      return ZSTD2_STATE_FRAME_HEADER;

    info->descriptor_OR     = (Byte)(info->descriptor_OR     |  descriptor);
    info->descriptor_NOT_OR = (Byte)(info->descriptor_NOT_OR | ~descriptor);

    h = &p->temp[1];
    {
      Byte w = 0;
      if ((descriptor & DESCRIPTOR_FLAG_SINGLE) == 0)
      {
        w = *h++;
        if (info->windowDescriptor_MAX < w)
            info->windowDescriptor_MAX = w;
        // info->are_WindowDescriptors = True;
        // info->num_WindowDescriptors++;
      }
      else
      {
        // info->are_SingleSegments = True;
        // info->num_SingleSegments++;
      }
      p->windowDescriptor = w;
    }
    {
      unsigned n = DESCRIPTOR_Get_DictionaryId_Flag(descriptor);
      UInt32 d = 0;
      if (n)
      {
        n = 1u << (n - 1);
        d = GetUi32(h) & ((UInt32)(Int32)-1 >> (32 - 8u * n));
        h += n;
      }
      p->dictionaryId = d;
      // info->dictionaryId_Cur = d;
      if (d != 0)
      {
        if (info->dictionaryId == 0)
          info->dictionaryId = d;
        else if (info->dictionaryId != d)
          info->are_DictionaryId_Different = True;
      }
    }
    {
      unsigned n = DESCRIPTOR_Get_ContentSize_Flag3(descriptor);
      UInt64 v = 0;
      if (n)
      {
        n >>= 1;
        if (n == 1)
          v = 256;
        v += GetUi64(h) & ((UInt64)(Int64)-1 >> (64 - (8u << n)));
        // info->are_ContentSize_Known = True;
        // info->num_Frames_with_ContentSize++;
        if (info->contentSize_MAX < v)
            info->contentSize_MAX = v;
        info->contentSize_Total += v;
      }
      else
      {
        info->are_ContentSize_Unknown = True;
        // info->num_Frames_without_ContentSize++;
      }
      p->contentSize = v;
    }
    // if ((size_t)(h - p->temp) != headerSize) return ZSTD2_STATE_ERROR; // it's unexpected internal code failure
    p->tempSize = 0;

    info->checksum_Defined = False;
    /*
    if (descriptor & DESCRIPTOR_FLAG_CHECKSUM)
      info->are_Checksums = True;
    else
      info->are_Non_Checksums = True;
    */

    return ZSTD2_STATE_AFTER_HEADER; // ZSTD2_STATE_BLOCK;
  }
}


static void ZstdDec_Update_XXH(CZstdDec * const p, size_t xxh64_winPos)
{
 /*
 #ifdef DISABLE_XXH_CHECK
  UNUSED_VAR(data)
 #else
 */
  if (!p->disableHash && (p->descriptor & DESCRIPTOR_FLAG_CHECKSUM))
  {
    // const size_t pos = p->xxh64_winPos;
    const size_t size = (p->decoder.winPos - xxh64_winPos) & ~(size_t)31;
    if (size)
    {
      // p->xxh64_winPos = pos + size;
      Xxh64State_UpdateBlocks(&p->xxh64,
          p->decoder.win + xxh64_winPos,
          p->decoder.win + xxh64_winPos + size);
    }
  }
}


/*
in:
  (winLimit) : is relaxed limit, where this function is allowed to stop writing of decoded data (if possible).
    - this function uses (winLimit) for RAW/RLE blocks only,
        because this function can decode single RAW/RLE block in several different calls.
    - this function DOESN'T use (winLimit) for Compressed blocks,
        because this function decodes full compressed block in single call.
  (CZstdDec1::winPos <= winLimit)
  (winLimit <= CZstdDec1::cycSize).
  Note: if (ds->outBuf_fromCaller) mode is used, then
  {
    (strong_limit) is stored in CZstdDec1::cycSize.
    So (winLimit) is more strong than (strong_limit).
  }

exit:
  Note: (CZstdDecState::winPos) will be set by caller after exit of this function.

  This function can exit for any of these conditions:
    - (frameState == ZSTD2_STATE_AFTER_HEADER)
    - (frameState == ZSTD2_STATE_FINISHED) : frame was finished : (status == ZSTD_STATUS_FINISHED_FRAME) is set
    - finished non-empty non-last block. So (CZstdDec1::winPos_atExit != winPos_atFuncStart).
    - ZSTD_STATUS_NEEDS_MORE_INPUT in src
    - (CZstdDec1::winPos) have reached (winLimit) in non-finished RAW/RLE block

  This function decodes no more than one non-empty block.
  So it fulfills the condition at exit:
    (CZstdDec1::winPos_atExit - winPos_atFuncStart <= block_size_max)
  Note: (winPos_atExit > winLimit) is possible in some cases after compressed block decoding.

  if (ds->outBuf_fromCaller) mode (useAdditionalWinLimit medo)
  {
    then this function uses additional strong limit from (CZstdDec1::cycSize).
    So this function will not write any data after (CZstdDec1::cycSize)
    And it fulfills the condition at exit:
      (CZstdDec1::winPos_atExit <= CZstdDec1::cycSize)
  }
*/
static SRes ZstdDec_DecodeBlock(CZstdDec * const p, CZstdDecState * const ds,
    SizeT winLimitAdd)
{
  const Byte *src = ds->inBuf;
  SizeT * const srcLen = &ds->inPos;
  const SizeT inSize = ds->inLim;
  // const int useAdditionalWinLimit = ds->outBuf_fromCaller ? 1 : 0;
  enum_ZstdStatus * const status = &ds->status;
  CZstdDecInfo * const info = &ds->info;
  SizeT winLimit;

  const SizeT winPos_atFuncStart = p->decoder.winPos;
  src += *srcLen;
  *status = ZSTD_STATUS_NOT_SPECIFIED;

  // finishMode = ZSTD_FINISH_ANY;
  if (ds->outSize_Defined)
  {
    if (ds->outSize < ds->outProcessed)
    {
      // p->isAfterSizeMode = 2; // we have extra bytes already
      *status = ZSTD_STATUS_OUT_REACHED;
      return SZ_OK;
      // size = 0;
    }
    else
    {
      // p->outSize >= p->outProcessed
      const UInt64 rem = ds->outSize - ds->outProcessed;
      /*
      if (rem == 0)
      p->isAfterSizeMode = 1; // we have reached exact required size
      */
      if (winLimitAdd >= rem)
      {
        winLimitAdd = (SizeT)rem;
        // if (p->finishMode) finishMode = ZSTD_FINISH_END;
      }
    }
  }

  winLimit = p->decoder.winPos + winLimitAdd;
  // (p->decoder.winPos <= winLimit)

  // while (p->frameState != ZSTD2_STATE_ERROR)
  while (!p->isErrorState)
  {
    SizeT inCur = inSize - *srcLen;

    if (p->frameState == ZSTD2_STATE_DATA)
    {
      /* (p->decoder.winPos == winPos_atFuncStart) is expected,
         because this function doesn't start new block.
         if it have finished some non-empty block in this call. */
      if (p->decoder.winPos != winPos_atFuncStart)
        return SZ_ERROR_FAIL; // it's unexpected

      /*
      if (p->decoder.winPos > winLimit)
      {
        // we can be here, if in this function call
        //      - we have extracted non-empty compressed block, and (winPos > winLimit) after that.
        //      - we have started new block decoding after that.
        // It's unexpected case, because we exit after non-empty non-last block.
        *status = (inSize == *srcLen) ?
            ZSTD_STATUS_NEEDS_MORE_INPUT :
            ZSTD_STATUS_NOT_FINISHED;
        return SZ_OK;
      }
      */
      // p->decoder.winPos <= winLimit

      if (p->blockType != kBlockType_Compressed)
      {
        // it's RLE or RAW block.
        // p->BlockSize != 0_
        // winLimit <= p->decoder.cycSize
        /* So here we use more strong (winLimit), even for
           (ds->outBuf_fromCaller) mode. */
        SizeT outCur = winLimit - p->decoder.winPos;
        {
          const UInt32 rem = p->blockSize;
          if (outCur > rem)
              outCur = rem;
        }
        if (p->blockType == kBlockType_Raw)
        {
          if (outCur > inCur)
              outCur = inCur;
          /* output buffer is better aligned for XXH code.
             So we use hash for output buffer data */
          // ZstdDec_Update_XXH(p, src, outCur); // for debug:
          memcpy(p->decoder.win + p->decoder.winPos, src, outCur);
          src += outCur;
          *srcLen += outCur;
        }
        else // kBlockType_RLE
        {
          #define RLE_BYTE_INDEX_IN_temp  3
          memset(p->decoder.win + p->decoder.winPos,
              p->temp[RLE_BYTE_INDEX_IN_temp], outCur);
        }
        {
          const SizeT xxh64_winPos = p->decoder.winPos - ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p);
          if (outCur)
            ZSTD_insertBlock(p->decoder.context, p->decoder.win + p->decoder.winPos, outCur);
          p->decoder.winPos += outCur;
          UPDATE_TOTAL_OUT(&p->decoder, outCur)
          p->contentProcessed += outCur;
          ZstdDec_Update_XXH(p, xxh64_winPos);
        }
        // ds->winPos = p->decoder.winPos;  // the caller does it instead. for debug:
        ds->outProcessed += outCur;
        if (p->blockSize -= (UInt32)outCur)
        {
          /*
          if (ds->outSize_Defined)
          {
            if (ds->outSize <= ds->outProcessed) ds->isAfterSizeMode = (enum_ZstdStatus)
               (ds->outSize == ds->outProcessed ? 1u: 2u);
          }
          */
          *status = (enum_ZstdStatus)
              (ds->outSize_Defined && ds->outSize <= ds->outProcessed ?
                ZSTD_STATUS_OUT_REACHED : (p->blockType == kBlockType_Raw && inSize == *srcLen) ?
                ZSTD_STATUS_NEEDS_MORE_INPUT :
                ZSTD_STATUS_NOT_FINISHED);
          return SZ_OK;
        }
      }
      else // kBlockType_Compressed
      {
        // p->blockSize != 0
        // (uncompressed_size_of_block == 0) is allowed
        // (p->curBlockUnpackRem == 0) is allowed
        /*
        if (p->decoder.winPos >= winLimit)
        {
          if (p->decoder.winPos != winPos_atFuncStart)
          {
            // it's unexpected case
            // We already have some data in finished blocks in this function call.
            //   So we don't decompress new block after (>=winLimit),
            //   even if it's empty block.
            *status = (inSize == *srcLen) ?
                ZSTD_STATUS_NEEDS_MORE_INPUT :
                ZSTD_STATUS_NOT_FINISHED;
            return SZ_OK;
          }
          // (p->decoder.winPos == winLimit == winPos_atFuncStart)
          // we will decode current block, because that current
          //   block can be empty block and we want to make some visible
          //   change of (src) stream after function start.
        }
        */
        /*
        if (ds->outSize_Defined && ds->outSize < ds->outProcessed)
        {
          // we don't want to start new block, if we have more extra decoded bytes already
          *status = ZSTD_STATUS_OUT_REACHED;
          return SZ_OK;
        }
        */
        {
          const Byte *comprStream;
          size_t afterAvail;
          UInt32 inTempPos = p->inTempPos;
          const UInt32 rem = p->blockSize - inTempPos;
          // rem != 0
          if (inTempPos != 0  // (inTemp) buffer already contains some input data
              || inCur < rem  // available input data size is smaller than compressed block size
              || ZstdDec1_NeedTempBufferForInput(*srcLen, src, rem))
          {
            if (inCur > rem)
                inCur = rem;
            if (inCur)
            {
              STAT_INC(g_Num_Blocks_memcpy)
              // we clear data for backward lookahead reading
              if (inTempPos == 0)
                memset(p->inTemp + kTempBuffer_PreSize - MAX_BACKWARD_DEPTH, 0, MAX_BACKWARD_DEPTH);
              // { unsigned y = 0; for(;y < 1000; y++)
              memcpy(p->inTemp + inTempPos + kTempBuffer_PreSize, src, inCur);
              // }
              src += inCur;
              *srcLen += inCur;
              inTempPos += (UInt32)inCur;
              p->inTempPos = inTempPos;
            }
            if (inTempPos != p->blockSize)
            {
              *status = ZSTD_STATUS_NEEDS_MORE_INPUT;
              return SZ_OK;
            }
            #if COPY_CHUNK_SIZE > 1
              memset(p->inTemp + kTempBuffer_PreSize + inTempPos, 0, COPY_CHUNK_SIZE);
            #endif
            comprStream = p->inTemp + kTempBuffer_PreSize;
            afterAvail = k_Lit_AfterAvail;
            // we don't want to read non-initialized data or junk in CopyMatch():
          }
          else
          {
            // inCur >= rem
            // we use direct decoding from (src) buffer:
            afterAvail = inCur - rem;
            comprStream = src;
            src += rem;
            *srcLen += rem;
          }

          #ifdef Z7_ZSTD_DEC_USE_CHECK_OF_NEED_TEMP
            ZstdDec1_NeedTempBufferForInput(*srcLen, comprStream, p->blockSize);
          #endif
          // printf("\nblockSize=%u", p->blockSize);
          // printf("%x\n", (unsigned)p->contentProcessed);
          STAT_INC(g_Num_Blocks_Compressed)
          {
            SRes sres;
            const size_t winPos = p->decoder.winPos;
            /*
               if ( useAdditionalWinLimit), we use strong unpack limit: smallest from
                  - limit from stream : (curBlockUnpackRem)
                  - limit from caller : (cycSize - winPos)
               if (!useAdditionalWinLimit), we use only relaxed limit:
                  - limit from stream : (curBlockUnpackRem)
            */
            SizeT outLimit = p->curBlockUnpackRem;
            if (ds->outBuf_fromCaller)
            // if (useAdditionalWinLimit)
            {
              const size_t limit = p->decoder.cycSize - winPos;
              if (outLimit > limit)
                  outLimit = limit;
            }
            sres = ZstdDec1_DecodeBlock(&p->decoder,
                comprStream, p->blockSize, afterAvail, outLimit);
            // ds->winPos = p->decoder.winPos;  // the caller does it instead. for debug:
            if (sres)
            {
              p->isErrorState = True;
              return sres;
            }
            {
              const SizeT xxh64_winPos = winPos - ZstdDec_GET_UNPROCESSED_XXH64_SIZE(p);
              const size_t num = p->decoder.winPos - winPos;
              ds->outProcessed += num;
              p->contentProcessed += num;
              ZstdDec_Update_XXH(p, xxh64_winPos);
            }
          }
          // printf("\nwinPos=%x", (int)(unsigned)p->decoder.winPos);
        }
      }

      /*
      if (ds->outSize_Defined)
      {
        if (ds->outSize <= ds->outProcessed) ds->isAfterSizeMode = (enum_ZstdStatus)
           (ds->outSize == ds->outProcessed ? 1u: 2u);
      }
      */

      if (!ZSTD_DEC_IS_LAST_BLOCK(p))
      {
        p->frameState = ZSTD2_STATE_BLOCK;
        if (ds->outSize_Defined && ds->outSize < ds->outProcessed)
        {
          *status = ZSTD_STATUS_OUT_REACHED;
          return SZ_OK;
        }
        // we exit only if (winPos) was changed in this function call:
        if (p->decoder.winPos != winPos_atFuncStart)
        {
          // decoded block was not empty. So we exit:
          *status = (enum_ZstdStatus)(
              (inSize == *srcLen) ?
                ZSTD_STATUS_NEEDS_MORE_INPUT :
                ZSTD_STATUS_NOT_FINISHED);
          return SZ_OK;
        }
        // (p->decoder.winPos == winPos_atFuncStart)
        // so current decoded block was empty.
        // we will try to decode more blocks in this function.
        continue;
      }

      // decoded block was last in frame
      if (p->descriptor & DESCRIPTOR_FLAG_CHECKSUM)
      {
        p->frameState = ZSTD2_STATE_HASH;
        if (ds->outSize_Defined && ds->outSize < ds->outProcessed)
        {
          *status = ZSTD_STATUS_OUT_REACHED;
          return SZ_OK; // disable if want to
          /* We want to get same return codes for any input buffer sizes.
             We want to get faster ZSTD_STATUS_OUT_REACHED status.
             So we exit with ZSTD_STATUS_OUT_REACHED here,
             instead of ZSTD2_STATE_HASH and ZSTD2_STATE_FINISHED processing.
             that depends from input buffer size and that can set
             ZSTD_STATUS_NEEDS_MORE_INPUT or return SZ_ERROR_DATA or SZ_ERROR_CRC.
          */
        }
      }
      else
      {
        /* ZSTD2_STATE_FINISHED proccesing doesn't depend from input buffer */
        p->frameState = ZSTD2_STATE_FINISHED;
      }
      /*
      p->frameState = (p->descriptor & DESCRIPTOR_FLAG_CHECKSUM) ?
          ZSTD2_STATE_HASH :
          ZSTD2_STATE_FINISHED;
      */
      /* it's required to process ZSTD2_STATE_FINISHED state in this function call,
         because we must check contentSize and hashError in ZSTD2_STATE_FINISHED code,
         while the caller can reinit full state for ZSTD2_STATE_FINISHED
         So we can't exit from function here. */
      continue;
    }

    if (p->frameState == ZSTD2_STATE_FINISHED)
    {
      *status = ZSTD_STATUS_FINISHED_FRAME;
      if (DESCRIPTOR_Is_ContentSize_Defined(p->descriptor)
          && p->contentSize != p->contentProcessed)
        return SZ_ERROR_DATA;
      if (p->hashError) // for debug
        return SZ_ERROR_CRC;
      return SZ_OK;
      // p->frameState = ZSTD2_STATE_SIGNATURE;
      // continue;
    }

    if (p->frameState == ZSTD2_STATE_AFTER_HEADER)
      return SZ_OK; // we need memory allocation for that state

    if (p->frameState == ZSTD2_STATE_SKIP_DATA)
    {
      UInt32 blockSize = p->blockSize;
      // (blockSize == 0) is possible
      if (inCur > blockSize)
          inCur = blockSize;
      src += inCur;
      *srcLen += inCur;
      blockSize -= (UInt32)inCur;
      p->blockSize = blockSize;
      if (blockSize == 0)
      {
        p->frameState = ZSTD2_STATE_SIGNATURE;
        // continue; // for debug: we can continue without return to caller.
        // we notify the caller that skip frame was finished:
        *status = ZSTD_STATUS_FINISHED_FRAME;
        return SZ_OK;
      }
      // blockSize != 0
      // (inCur) was smaller than previous value of p->blockSize.
      // (inSize == *srcLen) now
      *status = ZSTD_STATUS_NEEDS_MORE_INPUT;
      return SZ_OK;
    }

    if (inCur == 0)
    {
      *status = ZSTD_STATUS_NEEDS_MORE_INPUT;
      return SZ_OK;
    }

    {
      (*srcLen)++;
      p->frameState = ZstdDec_UpdateState(p, *src++, info);
    }
  }

  *status = ZSTD_STATUS_NOT_SPECIFIED;
  p->isErrorState = True;
  // p->frameState = ZSTD2_STATE_ERROR;
  // if (p->frameState = ZSTD2_STATE_SIGNATURE)  return SZ_ERROR_NO_ARCHIVE
  return SZ_ERROR_DATA;
}




SRes ZstdDec_Decode(CZstdDecHandle dec, CZstdDecState *p)
{
  p->needWrite_Size = 0;
  p->status = ZSTD_STATUS_NOT_SPECIFIED;
  dec->disableHash = p->disableHash;

  if (p->outBuf_fromCaller)
  {
    dec->decoder.win = p->outBuf_fromCaller;
    dec->decoder.cycSize = p->outBufSize_fromCaller;
  }

  // p->winPos = dec->decoder.winPos;

  for (;;)
  {
    SizeT winPos, size;
    // SizeT outProcessed;
    SRes res;

    if (p->wrPos > dec->decoder.winPos)
      return SZ_ERROR_FAIL;

    if (dec->frameState == ZSTD2_STATE_FINISHED)
    {
      if (!p->outBuf_fromCaller)
      {
        // we need to set positions to zero for new frame.
        if (p->wrPos != dec->decoder.winPos)
        {
          /* We have already asked the caller to flush all data
             with (p->needWrite_Size) and (ZSTD_STATUS_FINISHED_FRAME) status.
             So it's unexpected case */
          // p->winPos = dec->decoder.winPos;
          // p->needWrite_Size = dec->decoder.winPos - p->wrPos; // flush size asking
          // return SZ_OK; // ask to flush again
          return SZ_ERROR_FAIL;
        }
        // (p->wrPos == dec->decoder.winPos), and we wrap to zero:
        dec->decoder.winPos = 0;
        p->winPos = 0;
        p->wrPos = 0;
      }
      ZstdDec_Init_ForNewFrame(dec);
      // continue;
    }

    winPos = dec->decoder.winPos;
    {
      SizeT next = dec->decoder.cycSize;
      /* cycSize == 0, if no buffer was allocated still,
         or, if (outBuf_fromCaller) mode and (outBufSize_fromCaller == 0) */
      if (!p->outBuf_fromCaller
          && next
          && next <= winPos
          && dec->isCyclicMode)
      {
        // (0 < decoder.cycSize <= winPos) in isCyclicMode.
        // so we need to wrap (winPos) and (wrPos) over (cycSize).
        const size_t delta = next;
        // (delta) is how many bytes we remove from buffer.
        /*
        // we don't need data older than last (cycSize) bytes.
        size_t delta = winPos - next; // num bytes after (cycSize)
        if (delta <= next) // it's expected case
          delta = next;
        // delta == Max(cycSize, winPos - cycSize)
        */
        if (p->wrPos < delta)
        {
          // (wrPos < decoder.cycSize)
          // We have asked already the caller to flush required data
          // p->status = ZSTD_STATUS_NOT_SPECIFIED;
          // p->winPos = winPos;
          // p->needWrite_Size = delta - p->wrPos; // flush size asking
          // return SZ_OK; // ask to flush again
          return SZ_ERROR_FAIL;
        }
        // p->wrPos >= decoder.cycSize
        // we move extra data after (decoder.cycSize) to start of cyclic buffer:
        winPos -= delta;
        if (winPos)
        {
          if (winPos >= delta)
            return SZ_ERROR_FAIL;
          memmove(dec->decoder.win, dec->decoder.win + delta, winPos);
          // printf("\nmemmove processed=%8x winPos=%8x\n", (unsigned)p->outProcessed, (unsigned)dec->decoder.winPos);
          STAT_INC(g_Num_Wrap_memmove_Num)
          STAT_UPDATE(g_Num_Wrap_memmove_Bytes += (unsigned)winPos;)
        }
        dec->decoder.winPos = winPos;
        p->winPos = winPos;
        p->wrPos -= delta;
        // dec->xxh64_winPos -= delta;

        // (winPos < delta)
        #ifdef Z7_STD_DEC_USE_AFTER_CYC_BUF
          /* we set the data after cycSize, because
             we don't want to read non-initialized data or junk in CopyMatch(). */
          memset(dec->decoder.win + next, 0, COPY_CHUNK_SIZE);
        #endif

        /*
        if (winPos == next)
        {
          if (winPos != p->wrPos)
          {
            // we already requested before to flush full data for that case.
            //   but we give the caller a second chance to flush data:
            p->needWrite_Size = winPos - p->wrPos;
            return SZ_OK;
          }
          // (decoder.cycSize == winPos == p->wrPos)
          // so we do second wrapping to zero:
          winPos = 0;
          dec->decoder.winPos = 0;
          p->winPos = 0;
          p->wrPos = 0;
        }
        */
        // (winPos < next)
      }

      if (winPos > next)
        return SZ_ERROR_FAIL; // it's unexpected case
      /*
        if (!outBuf_fromCaller && isCyclicMode && cycSize != 0)
          then (winPos <  cycSize)
          else (winPos <= cycSize)
      */
      if (!p->outBuf_fromCaller)
      {
        // that code is optional. We try to optimize write chunk sizes.
        /* (next2) is expected next write position in the caller,
           if the caller writes by kBlockSizeMax chunks.
        */
        /*
        const size_t next2 = (winPos + kBlockSizeMax) & (kBlockSizeMax - 1);
        if (winPos < next2 && next2 < next)
          next = next2;
        */
      }
      size = next - winPos;
    }

    // note: ZstdDec_DecodeBlock() uses (winLimit = winPos + size) only for RLE and RAW blocks
    res = ZstdDec_DecodeBlock(dec, p, size);
    /*
      after one block decoding:
      if (!outBuf_fromCaller && isCyclicMode && cycSize != 0)
        then (winPos <  cycSize + max_block_size)
        else (winPos <= cycSize)
    */

    if (!p->outBuf_fromCaller)
      p->win = dec->decoder.win;
    p->winPos = dec->decoder.winPos;

    // outProcessed = dec->decoder.winPos - winPos;
    // p->outProcessed += outProcessed;

    if (res != SZ_OK)
      return res;

    if (dec->frameState != ZSTD2_STATE_AFTER_HEADER)
    {
      if (p->outBuf_fromCaller)
        return SZ_OK;
      {
        // !p->outBuf_fromCaller
        /*
          if (ZSTD_STATUS_FINISHED_FRAME), we request full flushing here because
            1) it's simpler to work with allocation and extracting of next frame,
            2) it's better to start writing to next new frame with aligned memory
               for faster xxh 64-bit reads.
        */
        size_t end = dec->decoder.winPos;  // end pos for all data flushing
        if (p->status != ZSTD_STATUS_FINISHED_FRAME)
        {
          // we will request flush here only for cases when wrap in cyclic buffer can be required in next call.
          if (!dec->isCyclicMode)
            return SZ_OK;
          // isCyclicMode
          {
            const size_t delta = dec->decoder.cycSize;
            if (end < delta)
              return SZ_OK; // (winPos < cycSize). no need for flush
            // cycSize <= winPos
            // So we ask the caller to flush of (cycSize - wrPos) bytes,
            // and then we will wrap cylicBuffer in next call
            end = delta;
          }
        }
        p->needWrite_Size = end - p->wrPos;
      }
      return SZ_OK;
    }

    // ZSTD2_STATE_AFTER_HEADER
    {
      BoolInt useCyclic = False;
      size_t cycSize;

      // p->status = ZSTD_STATUS_NOT_FINISHED;
      if (dec->dictionaryId != 0)
      {
        /* actually we can try to decode some data,
           because it's possible that some data doesn't use dictionary */
        // p->status = ZSTD_STATUS_NOT_SPECIFIED;
        return SZ_ERROR_UNSUPPORTED;
      }

      {
        UInt64 winSize = dec->contentSize;
        UInt64 winSize_Allocate = winSize;
        const unsigned descriptor = dec->descriptor;

        if ((descriptor & DESCRIPTOR_FLAG_SINGLE) == 0)
        {
          const Byte wd = dec->windowDescriptor;
          winSize = (UInt64)(8 + (wd & 7)) << ((wd >> 3) + 10 - 3);
          if (!DESCRIPTOR_Is_ContentSize_Defined(descriptor)
              || winSize_Allocate > winSize)
          {
            winSize_Allocate = winSize;
            useCyclic = True;
          }
        }
        /*
        else
        {
          if (p->info.singleSegment_ContentSize_MAX < winSize)
              p->info.singleSegment_ContentSize_MAX = winSize;
          // p->info.num_SingleSegments++;
        }
        */
        if (p->info.windowSize_MAX < winSize)
            p->info.windowSize_MAX = winSize;
        if (p->info.windowSize_Allocate_MAX < winSize_Allocate)
            p->info.windowSize_Allocate_MAX = winSize_Allocate;
        /*
           winSize_Allocate is MIN(content_size, window_size_from_descriptor).
           Wven if (content_size < (window_size_from_descriptor))
             original-zstd still uses (window_size_from_descriptor) to check that decoding is allowed.
           We try to follow original-zstd, and here we check (winSize) instead of (winSize_Allocate))
        */
        if (
              // winSize_Allocate   // it's relaxed check
              winSize               // it's more strict check to be compatible with original-zstd
            > ((UInt64)1 << MAX_WINDOW_SIZE_LOG))
          return SZ_ERROR_UNSUPPORTED; // SZ_ERROR_MEM
        cycSize = (size_t)winSize_Allocate;
        if (cycSize != winSize_Allocate)
          return SZ_ERROR_MEM;
        // cycSize <= winSize
        /* later we will use (CZstdDec1::winSize) to check match offsets and check block sizes.
           if (there is window descriptor)
           {
             We will check block size with (window_size_from_descriptor) instead of (winSize_Allocate).
             Does original-zstd do it that way also?
           }
           Here we must reduce full real 64-bit (winSize) to size_t for (CZstdDec1::winSize).
           Also we don't want too big values for (CZstdDec1::winSize).
           our (CZstdDec1::winSize) will meet the condition:
             (CZstdDec1::winSize < kBlockSizeMax || CZstdDec1::winSize <= cycSize).
        */
        dec->decoder.winSize = (winSize < kBlockSizeMax) ? (size_t)winSize: cycSize;
        // note: (CZstdDec1::winSize > cycSize) is possible, if (!useCyclic)
      }

      RINOK(ZstdDec_AllocateMisc(dec))

      if (p->outBuf_fromCaller)
        dec->isCyclicMode = False;
      else
      {
        size_t d = cycSize;

        if (dec->decoder.winPos != p->wrPos)
          return SZ_ERROR_FAIL;

        dec->decoder.winPos = 0;
        p->wrPos = 0;
        p->winPos = dec->decoder.winPos;

        /*
        const size_t needWrite = dec->decoder.winPos - p->wrPos;
        if (!needWrite)
        {
          dec->decoder.winPos = 0;
          p->wrPos = 0;
          p->winPos = dec->decoder.winPos;
        }
        */
        /* if (!useCyclic) we allocate only cycSize = ContentSize.
           But if we want to support the case where new frame starts with winPos != 0,
           then we will wrap over zero, and we still need
           to set (useCyclic) and allocate additional buffer spaces.
           Now we don't allow new frame starting with (winPos != 0).
           so (dec->decoder->winPos == 0)
           can use (!useCyclic) with reduced buffer sizes.
        */
        /*
        if (dec->decoder->winPos != 0)
          useCyclic = True;
        */

        if (useCyclic)
        {
          /* cyclyc buffer size must be at least (COPY_CHUNK_SIZE - 1) bytes
             larger than window size, because CopyMatch() can write additional
             (COPY_CHUNK_SIZE - 1) bytes and overwrite oldests data in cyclyc buffer.
             But for performance reasons we align (cycSize) for (kBlockSizeMax).
             also we must provide (cycSize >= max_decoded_data_after_cycSize),
             because after data move wrapping over zero we must provide (winPos < cycSize).
          */
          const size_t alignSize = kBlockSizeMax;
          /* here we add (1 << 7) instead of (COPY_CHUNK_SIZE - 1), because
             we want to get same (cycSize) for different COPY_CHUNK_SIZE values. */
          // cycSize += (COPY_CHUNK_SIZE - 1) + (alignSize - 1); // for debug : we can get smallest (cycSize)
          cycSize += (1 << 7) + alignSize;
          cycSize &= ~(size_t)(alignSize - 1);
          // cycSize must be aligned for 32, because xxh requires 32-bytes blocks.
          // cycSize += 12345; // for debug
          // cycSize += 1 << 10; // for debug
          // cycSize += 32; // for debug
          // cycSize += kBlockSizeMax; // for debug
          if (cycSize < d)
            return SZ_ERROR_MEM;
          /*
             in cyclic buffer mode we allow to decode one additional block
             that exceeds (cycSize).
             So we must allocate additional (kBlockSizeMax) bytes after (cycSize).
             if defined(Z7_STD_DEC_USE_AFTER_CYC_BUF)
             {
               we can read (COPY_CHUNK_SIZE - 1) bytes after (cycSize)
               but we aready allocate additional kBlockSizeMax that
               is larger than COPY_CHUNK_SIZE.
               So we don't need additional space of COPY_CHUNK_SIZE after (cycSize).
             }
          */
          /*
          #ifdef Z7_STD_DEC_USE_AFTER_CYC_BUF
          d = cycSize + (1 << 7); // we must add at least (COPY_CHUNK_SIZE - 1)
          #endif
          */
          d = cycSize + kBlockSizeMax;
          if (d < cycSize)
            return SZ_ERROR_MEM;
        }

        {
          const size_t kMinWinAllocSize = 1 << 12;
          if (d < kMinWinAllocSize)
              d = kMinWinAllocSize;
        }

        if (d > dec->winBufSize_Allocated)
        {
          /*
          if (needWrite)
          {
            p->needWrite_Size = needWrite;
            return SZ_OK;
            // return SZ_ERROR_FAIL;
          }
          */

          if (dec->winBufSize_Allocated != 0)
          {
            const size_t k_extra = (useCyclic || d >= (1u << 20)) ?
                2 * kBlockSizeMax : 0;
            unsigned i = useCyclic ? 17 : 12;
            for (; i < sizeof(size_t) * 8; i++)
            {
              const size_t d2 = ((size_t)1 << i) + k_extra;
              if (d2 >= d)
              {
                d = d2;
                break;
              }
            }
          }
          // RINOK(ZstdDec_AllocateWindow(dec, d))
          ZstdDec_FreeWindow(dec);
          dec->win_Base = (Byte *)ISzAlloc_Alloc(dec->alloc_Big, d);
          if (!dec->win_Base)
            return SZ_ERROR_MEM;
          dec->decoder.win = dec->win_Base;
          dec->winBufSize_Allocated = d;
        }
        /*
        else
        {
          // for non-cyclycMode we want flush data, and set winPos = 0
          if (needWrite)
          {
            if (!useCyclic || dec->decoder.winPos >= cycSize)
            {
              p->needWrite_Size = needWrite;
              return SZ_OK;
              // return SZ_ERROR_FAIL;
            }
          }
        }
        */

        dec->decoder.cycSize = cycSize;
        p->win = dec->decoder.win;
        // p->cycSize = dec->decoder.cycSize;
        dec->isCyclicMode = (Byte)useCyclic;
      } // (!p->outBuf_fromCaller) end

      // p->winPos = dec->decoder.winPos;
      dec->frameState = ZSTD2_STATE_BLOCK;
      // continue;
    } // ZSTD2_STATE_AFTER_HEADER end
  }
}


void ZstdDec_GetResInfo(const CZstdDec *dec,
    const CZstdDecState *p,
    SRes res,
    CZstdDecResInfo *stat)
{
  // ZstdDecInfo_CLEAR(stat);
  stat->extraSize = 0;
  stat->is_NonFinishedFrame = False;
  if (dec->frameState != ZSTD2_STATE_FINISHED)
  {
    if (dec->frameState == ZSTD2_STATE_SIGNATURE)
    {
      stat->extraSize = (Byte)dec->tempSize;
      if (ZstdDecInfo_GET_NUM_FRAMES(&p->info) == 0)
        res = SZ_ERROR_NO_ARCHIVE;
    }
    else
    {
      stat->is_NonFinishedFrame = True;
      if (res == SZ_OK && p->status == ZSTD_STATUS_NEEDS_MORE_INPUT)
        res = SZ_ERROR_INPUT_EOF;
    }
  }
  stat->decode_SRes = res;
}


size_t ZstdDec_ReadUnusedFromInBuf(
    CZstdDecHandle dec,
    size_t afterDecoding_tempPos,
    void *data, size_t size)
{
  size_t processed = 0;
  if (dec->frameState == ZSTD2_STATE_SIGNATURE)
  {
    Byte *dest = (Byte *)data;
    const size_t tempSize = dec->tempSize;
    while (afterDecoding_tempPos < tempSize)
    {
      if (size == 0)
        break;
      size--;
      *dest++ = dec->temp[afterDecoding_tempPos++];
      processed++;
    }
  }
  return processed;
}


void ZstdDecState_Clear(CZstdDecState *p)
{
  memset(p, 0 , sizeof(*p));
}
