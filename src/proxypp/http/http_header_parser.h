/*******************************************************************************
**          File: http_header_parser.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-21 Tue 11:04 AM
**   Description: as the file name suggests
*******************************************************************************/
#ifndef PROXYPP_HTTP_HEADER_PARSER_H_
#define PROXYPP_HTTP_HEADER_PARSER_H_
#include <string>
#include <map>

namespace proxypp {
  class HttpHeaderParser final {
    public:
      bool parse(const std::string &data, std::string::size_type headerEndPos);
      bool getAddrAndPort(std::string &addr, uint16_t &port) const;
      bool isConnectMethod() const;

      static std::string::size_type findHeaderEndPos(const std::string &data);
      static bool startsWithValidHttpMethod(const std::string &data);
    
    private:
      std::string method_;
      std::string url_;
      std::string httpVersion_;
      std::map<std::string, std::string> headers_;
  };
} /* end of namspace: proxypp */

#endif /* end of include guard: PROXYPP_HTTP_HEADER_PARSER_H_ */
