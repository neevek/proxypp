/*******************************************************************************
**          File: buffer_pool.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-26 Thu 08:08 PM
**   Description: the buffer pool 
*******************************************************************************/
#ifndef BUFFER_POOL_H_
#define BUFFER_POOL_H_
#include "uvcpp.h"
#include <deque>
#include <memory>

namespace sockspp {
  class BufferPool {
    public:
      BufferPool(int maxBufferSize, int maxBufferCount);

      std::unique_ptr<uvcpp::Buffer> requestBuffer(std::size_t size);
      void returnBuffer(std::unique_ptr<uvcpp::Buffer> &&data);
      std::unique_ptr<uvcpp::Buffer> assembleDataBuffer(
        const char *data, std::size_t dataLen);
    
    private:
      std::deque<std::unique_ptr<uvcpp::Buffer>> freeBuffers_;
      int maxBufferSize_;
      int maxBufferCount_;
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: BUFFER_POOL_H_ */
