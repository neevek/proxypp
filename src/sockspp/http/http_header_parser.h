/*******************************************************************************
**          File: http_header_parser.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-21 Tue 11:04 AM
**   Description: as the file name suggests
*******************************************************************************/
#ifndef SOCKSPP_HTTP_HEADER_PARSER_H_
#define SOCKSPP_HTTP_HEADER_PARSER_H_
#include <string>
#include <map>

namespace sockspp {
  class HttpHeaderParser final {
    public:
      bool parse(const char *buf, std::size_t len);
      bool getAddrAndPort(std::string &addr, uint16_t &port) const;
      std::string getRequestData() const;
      bool isConnectMethod() const;
    
    private:
      std::string requestData_;
      std::string method_;
      std::string url_;
      std::string httpVersion_;
      std::map<std::string, std::string> headers_;
  };
} /* end of namspace: sockspp */

#endif /* end of include guard: SOCKSPP_HTTP_HEADER_PARSER_H_ */
