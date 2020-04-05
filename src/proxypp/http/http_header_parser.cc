/*******************************************************************************
**          File: http_header_parser.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-21 Tue 11:34 AM
**   Description: see the header file 
*******************************************************************************/
#include "proxypp/http/http_header_parser.h"
#include "nul/log.h"
#include "nul/util.hpp"
#include "nul/uri.hpp"
#include "proxypp/util.h"

#include <algorithm>

namespace proxypp {
  using su = nul::StringUtil;

  bool HttpHeaderParser::parse(
    const std::string &data, std::string::size_type headerEndPos) {
    if (data.size() < 10) {
      LOG_E("Invalid http header");
      return false;
    }

    std::string header;
    if (headerEndPos == std::string::npos) {
      headerEndPos = findHeaderEndPos(data);
    }
    if (headerEndPos == std::string::npos) {
      const_cast<char *>(data.data())[data.size() - 1] = '\0';
      LOG_E("Invalid http header: %s", data.c_str());
      return false;
    }

    header = data.substr(0, headerEndPos);

    return su::split(header, "\r\n", [this](auto index, const auto &part) {
      if (index == 0) {
        return nul::StringUtil::split(
          part, " ", [this](auto index, const auto &part) {
          switch(index) {
            case 0:
              method_ = part; break;
            case 1:
              url_ = part; break;
            case 2:
              httpVersion_ = part; break;
            default:
              LOG_E("Invalid http header");
              return false;
          }

          return true;
        });

      } else {
        auto colonIndex = part.find(":");
        if (colonIndex == std::string::npos) {
          LOG_E("Invalid http header: %s", part.c_str());
          return false;
        }

        auto key = su::trim(part.substr(0, colonIndex));
        su::tolower(key);
        headers_[key] = su::trim(part.substr(colonIndex + 1));
        return true;
      }
    });
  }

  bool HttpHeaderParser::getAddrAndPort(std::string &addr, uint16_t &port) const {
    auto hostIt = headers_.find("host");
    if (hostIt != headers_.end()) {
      auto &host = hostIt->second;
      auto addrPortSepIndex = host.rfind(":");
      if (addrPortSepIndex == 0 || addrPortSepIndex == host.length() - 1) {
        LOG_W("Invalid Host header: %s", host.c_str());
        return false;
      }

      if (addrPortSepIndex == std::string::npos ||
          // two colons, treat it as IPv6 with default port number
          (host[addrPortSepIndex - 1] == ':')) {
        port = 80;
        addr = host;

      } else {
        port = std::stoi(host.substr(addrPortSepIndex + 1).c_str());
        addr = host.substr(0, addrPortSepIndex);
      }
      return true;

    } else if (!url_.empty()) {
      nul::URI uri;
      uri.parse(url_);
      addr = uri.getHost();
      port = uri.getPort() != 0 ? uri.getPort() : 80;
      return true;
    }

    LOG_E("URL is empty");
    return false;
  }

  bool HttpHeaderParser::isConnectMethod() const {
    return "CONNECT" == method_;
  }

  std::string::size_type HttpHeaderParser::findHeaderEndPos(
    const std::string &data) {
    auto size = data.size();
    if (size < 4) {
      return std::string::npos;
    }

    if ((data[size - 4] == '\r') &&
        (data[size - 3] == '\n') &&
        (data[size - 2] == '\r') &&
        (data[size - 1] == '\n')) {
      return size - 4;
    }
    return data.find("\r\n\r\n");
  }

  bool HttpHeaderParser::startsWithValidHttpMethod(
    const std::string &data) {
    if (data.size() < 7) {
      // no enough data, assume that it starts with valid http method
      return true;
    }

    return
      Util::strStartsWith(data, "CONNECT", 0) ||
      Util::strStartsWith(data, "GET", 0) ||
      Util::strStartsWith(data, "HEAD", 0) ||
      Util::strStartsWith(data, "POST", 0) ||
      Util::strStartsWith(data, "PUT", 0) ||
      Util::strStartsWith(data, "DELETE", 0) ||
      Util::strStartsWith(data, "OPTIONS", 0) ||
      Util::strStartsWith(data, "PATCH", 0);
  }

} /* end of namspace: proxypp */
