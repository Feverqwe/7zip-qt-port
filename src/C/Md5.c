/* Md5.c -- MD5 Hash
: Igor Pavlov : Public domain
This code is based on Colin Plumb's public domain md5.c code */

#include "Precomp.h"
#include <string.h>
#include "Md5.h"
#include "CpuArch.h"
#include "../crypto/opensslhash.h"

#define MD5_UPDATE_BLOCKS(p) sevenzip_md5_blocks
#define Md5_UpdateBlock(p) MD5_UPDATE_BLOCKS(p)(p->state, p->buffer, 1)

void Md5_Init(CMd5 *p)
{
  p->count = 0;
  p->state[0] = 0x67452301;
  p->state[1] = 0xefcdab89;
  p->state[2] = 0x98badcfe;
  p->state[3] = 0x10325476;
}

void Md5_Update(CMd5 *p, const Byte *data, size_t size)
{
  if (size == 0)
    return;
  {
    const unsigned pos = (unsigned)p->count & (MD5_BLOCK_SIZE - 1);
    const unsigned num = MD5_BLOCK_SIZE - pos;
    p->count += size;
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
      Md5_UpdateBlock(p);
    }
  }
  {
    const size_t numBlocks = size >> 6;
    if (numBlocks)
    MD5_UPDATE_BLOCKS(p)(p->state, data, numBlocks);
    size &= MD5_BLOCK_SIZE - 1;
    if (size == 0)
      return;
    data += (numBlocks << 6);
    memcpy(p->buffer, data, size);
  }
}

void Md5_Final(CMd5 *p, Byte *digest)
{
  unsigned pos = (unsigned)p->count & (MD5_BLOCK_SIZE - 1);
  p->buffer[pos++] = 0x80;
  if (pos > (MD5_BLOCK_SIZE - 4 * 2))
  {
    while (pos != MD5_BLOCK_SIZE) { p->buffer[pos++] = 0; }
    // memset(&p->buf.buffer[pos], 0, MD5_BLOCK_SIZE - pos);
    Md5_UpdateBlock(p);
    pos = 0;
  }
  memset(&p->buffer[pos], 0, (MD5_BLOCK_SIZE - 4 * 2) - pos);
  {
    const UInt64 numBits = p->count << 3;
#if defined(MY_CPU_LE_UNALIGN)
    SetUi64 (p->buffer + MD5_BLOCK_SIZE - 4 * 2, numBits)
#else
    SetUi32a(p->buffer + MD5_BLOCK_SIZE - 4 * 2, (UInt32)(numBits))
    SetUi32a(p->buffer + MD5_BLOCK_SIZE - 4 * 1, (UInt32)(numBits >> 32))
#endif
  }
  Md5_UpdateBlock(p);

  SetUi32(digest,      p->state[0])
  SetUi32(digest + 4,  p->state[1])
  SetUi32(digest + 8,  p->state[2])
  SetUi32(digest + 12, p->state[3])
  
  Md5_Init(p);
}
