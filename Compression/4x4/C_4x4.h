#include "../Compression.h"

#ifdef __cplusplus

// 4x4: FreeArc 0.666+ multithreaded block compression wrapper, e.g. "4x4:tor:16mb:c3".
// Compressed stream (all integers are little-endian int32):
//   version (always 0)
//   for every block: origsize (-1 = block is stored as is), compsize, compsize bytes of data
// Every block is compressed independently with the inner method, so it is decoded from memory.
// This is a single-threaded decoder of that format; options (:t/:i/:b/:r) only matter for compression.
class FOURBYFOUR_METHOD : public COMPRESSION_METHOD
{
public:
  char Method[MAX_METHOD_STRLEN];           // Inner compression method used for every block

  FOURBYFOUR_METHOD();
  virtual int decompress (CALLBACK_FUNC *callback, void *auxdata);
#ifndef FREEARC_DECOMPRESS_ONLY
  virtual int compress   (CALLBACK_FUNC *callback, void *auxdata);
  virtual void ShowCompressionMethod (char *buf);
  virtual MemSize GetCompressionMem     (void)         {return 0;}
  virtual MemSize GetDictionary         (void)         {return 0;}
  virtual MemSize GetBlockSize          (void)         {return 0;}
  virtual void    SetCompressionMem     (MemSize mem)  {}
  virtual void    SetDecompressionMem   (MemSize mem)  {}
  virtual void    SetDictionary         (MemSize dict) {}
  virtual void    SetBlockSize          (MemSize bs)   {}
#endif
  virtual MemSize GetDecompressionMem   (void);
};

COMPRESSION_METHOD* parse_4x4 (char** parameters);

#endif  // __cplusplus
