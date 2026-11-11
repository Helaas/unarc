#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../Compression.h"
#include "C_4x4.h"

#ifdef __cplusplus

#define _4x4_VERSION  0   /* version of compressed data format */

// Memory-to-memory I/O for decoding one block. Unlike DecompressMem(), it keeps its state
// in the struct rather than in globals, since 4x4 runs in its own MultiDecompress thread.
struct _4x4_MEMIO
{
  BYTE *in;   int inleft;
  BYTE *out;  int outleft;
};

static int _4x4_memio_callback (const char *what, void *buf, int size, void *auxdata)
{
  _4x4_MEMIO *io = (_4x4_MEMIO*) auxdata;
  if (strequ(what,"read")) {
    int n = mymin (size, io->inleft);
    memcpy (buf, io->in, n);
    io->in += n;  io->inleft -= n;
    return n;
  } else if (strequ(what,"write")) {
    if (size > io->outleft)  return FREEARC_ERRCODE_OUTBLOCK_TOO_SMALL;
    memcpy (io->out, buf, size);
    io->out += size;  io->outleft -= size;
    return size;
  }
  return 0;   // "time", "progress" and other notifications
}

// Make sure that buf can hold at least `size` bytes
static bool _4x4_reserve (BYTE* &buf, int &bufsize, int size)
{
  if (size <= bufsize)  return true;
  BYTE *p = (BYTE*) realloc (buf, size);
  if (!p)  return false;
  buf = p;  bufsize = size;
  return true;
}

FOURBYFOUR_METHOD::FOURBYFOUR_METHOD()
{
  strcpy (Method, "tor:3:1mb");
}

int FOURBYFOUR_METHOD::decompress (CALLBACK_FUNC *callback, void *auxdata)
{
  int errcode = FREEARC_OK;
  BYTE *inbuf  = NULL;  int inbufsize  = 0;
  BYTE *outbuf = NULL;  int outbufsize = 0;
  int32 version;

  READ4_OR_EOF (version);
  if (version != _4x4_VERSION)  {errcode = FREEARC_ERRCODE_BAD_COMPRESSED_DATA; goto finished;}

  for (;;)
  {
    int32 origsize, compsize;
    READ4_OR_EOF (origsize);                       // clean EOF here means the last block was processed
    READ4 (compsize);
    if (compsize < 0  ||  origsize < -1)  {errcode = FREEARC_ERRCODE_BAD_COMPRESSED_DATA; goto finished;}

    if (!_4x4_reserve (inbuf, inbufsize, compsize))  {errcode = FREEARC_ERRCODE_NOT_ENOUGH_MEMORY; goto finished;}
    READ (inbuf, compsize);

    if (origsize == -1) {                          // incompressible block, stored as is
      WRITE (inbuf, compsize);
      continue;
    }

    if (!_4x4_reserve (outbuf, outbufsize, origsize))  {errcode = FREEARC_ERRCODE_NOT_ENOUGH_MEMORY; goto finished;}
    _4x4_MEMIO io = {inbuf, compsize, outbuf, origsize};
    int result = Decompress (Method, _4x4_memio_callback, &io);
    if (result < 0)  {errcode = result; goto finished;}
    if (io.outleft != 0)  {errcode = FREEARC_ERRCODE_BAD_COMPRESSED_DATA; goto finished;}
    WRITE (outbuf, origsize);
  }

finished:
  free (inbuf);
  free (outbuf);
  return errcode < 0 ? errcode : FREEARC_OK;
}

// Rough estimate: inner decoder plus one input and one output block buffer
MemSize FOURBYFOUR_METHOD::GetDecompressionMem (void)
{
  MemSize inner = ::GetDecompressionMem (Method);
  return inner * 3;
}

#ifndef FREEARC_DECOMPRESS_ONLY

int FOURBYFOUR_METHOD::compress (CALLBACK_FUNC *callback, void *auxdata)
{
  return FREEARC_ERRCODE_NOT_IMPLEMENTED;
}

void FOURBYFOUR_METHOD::ShowCompressionMethod (char *buf)
{
  sprintf (buf, "4x4:%s", Method);
}

#endif  // !defined (FREEARC_DECOMPRESS_ONLY)

// Parses "4x4[:t2][:i1][:b8mb][:r99]:inner_method[:inner_params...]"
COMPRESSION_METHOD* parse_4x4 (char** parameters)
{
  if (strcmp (parameters[0], "4x4") != 0)  return NULL;

  FOURBYFOUR_METHOD *p = new FOURBYFOUR_METHOD;
  while (*++parameters)
  {
    char *param = *parameters;
    // A parameter that doesn't start with a digit or letter+digit begins the inner method;
    // everything up to the end belongs to it
    if (!(isdigit(param[0]) || isdigit(param[1])))
    {
      p->Method[0] = '\0';
      for (char **q = parameters; *q; q++) {
        if (q != parameters)  strncat (p->Method, ":", sizeof(p->Method) - strlen(p->Method) - 1);
        strncat (p->Method, *q, sizeof(p->Method) - strlen(p->Method) - 1);
      }
      COMPRESSION_METHOD *inner = ParseCompressionMethod (p->Method);
      if (!inner)  {delete p; return NULL;}
      delete inner;
      break;
    }
    // :tN, :iN, :bN, :rN and bare numbers only tune compression threads/buffers/block size
  }
  return p;
}

static int _4x4_x = AddCompressionMethod (parse_4x4);

#endif  // __cplusplus
