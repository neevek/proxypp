/*******************************************************************************
**          File: buffer_pool.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-26 Thu 08:10 PM
**   Description: see the header file 
*******************************************************************************/
#include "buffer_pool.h"

namespace sockspp {

  BufferPool::BufferPool(int maxBufferSize, int maxBufferCount) :
    maxBufferSize_(maxBufferSize), maxBufferCount_(maxBufferCount) {
  }

  std::unique_ptr<uvcpp::Buffer> BufferPool::requestBuffer(std::size_t size) {
    if (!freeBuffers_.empty()) {
      auto freeBufIt = std::find_if(
        freeBuffers_.begin(), freeBuffers_.end(), [size](auto &buf) {
          return buf->getCapacity() >= size;
      });
      if (freeBufIt != freeBuffers_.end()) {
        auto freeBuf = std::move(*freeBufIt);
        freeBuffers_.erase(freeBufIt);
        return freeBuf;
      }
    }
    return std::make_unique<uvcpp::Buffer>(size);
  }

  void BufferPool::returnBuffer(std::unique_ptr<uvcpp::Buffer> &&data) {
    if (data->getCapacity() > maxBufferSize_ &&
        freeBuffers_.size() < maxBufferCount_) {
      freeBuffers_.push_back(std::move(data));
    }
  }

  std::unique_ptr<uvcpp::Buffer> BufferPool::assembleDataBuffer(
    const char *data, std::size_t dataLen) {
    auto dataBuf = requestBuffer(dataLen);
    dataBuf->assign(data, dataLen);
    return dataBuf;
  }
  
} /* end of namspace: sockspp */
