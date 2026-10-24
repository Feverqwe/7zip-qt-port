/* Sha1.c -- SHA-1 Hash
: Igor Pavlov : Public domain
This code is based on public domain code of Steve Reid from Wei Dai's Crypto++ library. */

#include "Precomp.h"
#include <string.h>
#include "Sha1.h"
#include "CpuArch.h"
#include "../crypto/opensslhash.h"

static void Z7_FASTCALL Sha1_UpdateBlocks(UInt32 state[5], const Byte *data, size_t numBlocks)
{ sevenzip_sha1_blocks(state, data, numBlocks); }

#define SHA1_UPDATE_BLOCKS(p) (p)->v.vars.func_UpdateBlocks
#define Sha1_UpdateBlock(p) SHA1_UPDATE_BLOCKS(p)(p->state, p->buffer, 1)

BoolInt Sha1_SetFunction(CSha1 *p, unsigned algo)
{
  if (algo != SHA1_ALGO_DEFAULT) return False;
  p->v.vars.func_UpdateBlocks = Sha1_UpdateBlocks;
  return True;
}

void Sha1_InitState(CSha1 *p)
{
  p->v.vars.count = 0;
  p->state[0] = 0x67452301;
  p->state[1] = 0xEFCDAB89;
  p->state[2] = 0x98BADCFE;
  p->state[3] = 0x10325476;
  p->state[4] = 0xC3D2E1F0;
}

void Sha1_Init(CSha1 *p)
{ p->v.vars.func_UpdateBlocks = Sha1_UpdateBlocks; Sha1_InitState(p); }

void Sha1_Update(CSha1 *p, const Byte *data, size_t size)
{
  if (size == 0)
    return;
  {
    const unsigned pos = (unsigned)p->v.vars.count & (SHA1_BLOCK_SIZE - 1);
    const unsigned num = SHA1_BLOCK_SIZE - pos;
    p->v.vars.count += size;
    if (num > size)
    {
      memcpy(p->buffer + pos, data, size);
      return;
    }
    if (pos != 0)
    {
      size -= num;
      memcpy(p->buffer + pos, data, num);
      data += num;
      Sha1_UpdateBlock(p);
    }
  }
  {
    const size_t numBlocks = size >> 6;
    // if (numBlocks)
    SHA1_UPDATE_BLOCKS(p)(p->state, data, numBlocks);
    size &= SHA1_BLOCK_SIZE - 1;
    if (size == 0)
      return;
    data += (numBlocks << 6);
    memcpy(p->buffer, data, size);
  }
}

void Sha1_Final(CSha1 *p, Byte *digest)
{
  unsigned pos = (unsigned)p->v.vars.count & (SHA1_BLOCK_SIZE - 1);
  p->buffer[pos++] = 0x80;
  if (pos > (SHA1_BLOCK_SIZE - 4 * 2))
  {
    while (pos != SHA1_BLOCK_SIZE) { p->buffer[pos++] = 0; }
    // memset(&p->buf.buffer[pos], 0, SHA1_BLOCK_SIZE - pos);
    Sha1_UpdateBlock(p);
    pos = 0;
  }
  memset(&p->buffer[pos], 0, (SHA1_BLOCK_SIZE - 4 * 2) - pos);
  {
    const UInt64 numBits = p->v.vars.count << 3;
    SetBe32(p->buffer + SHA1_BLOCK_SIZE - 4 * 2, (UInt32)(numBits >> 32))
    SetBe32(p->buffer + SHA1_BLOCK_SIZE - 4 * 1, (UInt32)(numBits))
  }
  Sha1_UpdateBlock(p);

  SetBe32(digest,      p->state[0])
  SetBe32(digest + 4,  p->state[1])
  SetBe32(digest + 8,  p->state[2])
  SetBe32(digest + 12, p->state[3])
  SetBe32(digest + 16, p->state[4])
  
  Sha1_InitState(p);
}

void Sha1_PrepareBlock(const CSha1 *p, Byte *block, unsigned size)
{
  const UInt64 numBits = (p->v.vars.count + size) << 3;
  SetBe32(&((UInt32 *)(void *)block)[SHA1_NUM_BLOCK_WORDS - 2], (UInt32)(numBits >> 32))
  SetBe32(&((UInt32 *)(void *)block)[SHA1_NUM_BLOCK_WORDS - 1], (UInt32)(numBits))
  // SetBe32((UInt32 *)(block + size), 0x80000000);
  SetUi32((UInt32 *)(void *)(block + size), 0x80)
  size += 4;
  while (size != (SHA1_NUM_BLOCK_WORDS - 2) * 4)
  {
    *((UInt32 *)(void *)(block + size)) = 0;
    size += 4;
  }
}

void Sha1_GetBlockDigest(const CSha1 *p, const Byte *data, Byte *destDigest)
{
  MY_ALIGN (16)
  UInt32 st[SHA1_NUM_DIGEST_WORDS];

  st[0] = p->state[0];
  st[1] = p->state[1];
  st[2] = p->state[2];
  st[3] = p->state[3];
  st[4] = p->state[4];
  
  SHA1_UPDATE_BLOCKS(p)(st, data, 1);

  SetBe32(destDigest + 0    , st[0])
  SetBe32(destDigest + 1 * 4, st[1])
  SetBe32(destDigest + 2 * 4, st[2])
  SetBe32(destDigest + 3 * 4, st[3])
  SetBe32(destDigest + 4 * 4, st[4])
}

void Sha1Prepare(void) {}
