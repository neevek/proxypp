/*******************************************************************************
**          File: proxy_session.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-22 Wed 11:33 AM
**   Description: generic connection interface 
*******************************************************************************/
#ifndef SOCKSPP_PROXY_SESSION_H_
#define SOCKSPP_PROXY_SESSION_H_

class ProxySession {
  public:
    virtual ~ProxySession() = default;
    virtual void start() = 0;
    virtual void close() = 0;
};

#endif /* end of include guard: SOCKSPP_PROXY_SESSION_H_ */
