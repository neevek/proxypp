/*******************************************************************************
**          File: http_header_parser.cc
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-21 Tue 11:34 AM
**   Description: see the header file 
*******************************************************************************/
#include "proxypp/http/http_header_parser.h"
#include "nul/log.hpp"
#include "nul/util.hpp"
#include "nul/uri.hpp"
#include <algorithm>

namespace proxypp {
  using su = nul::StringUtil;

  bool HttpHeaderParser::parse(const char *buf, std::size_t len) {
    if (len < 10) {
      LOG_E("Invalid http header");
      return false;
    }

    requestData_.assign(buf, len);
    std::string header;
    if ((buf[len - 4] != '\r') ||
        (buf[len - 3] != '\n') ||
        (buf[len - 2] != '\r') ||
        (buf[len - 1] != '\n')) {
      auto pos = requestData_.find("\r\n\r\n");
      if (pos == std::string::npos) {
        const_cast<char *>(buf)[len - 1] = '\0';
        LOG_E("Invalid http header: %s", buf);
        return false;
      }
      header = requestData_.substr(0, pos);

    } else {
      header = requestData_;
    }

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

    } else if (url_.empty()) {
      LOG_E("URL is empty");
      return false;
    }

    nul::URI uri;
    uri.parse(url_);
    addr = uri.getHost();
    port = uri.getPort() != 0 ? uri.getPort() : 80;
    return true;
  }

  std::string HttpHeaderParser::getRequestData() const {
    return requestData_;
  }

  bool HttpHeaderParser::isConnectMethod() const {
    return "CONNECT" == method_;
  }

} /* end of namspace: proxypp */
