/*******************************************************************************
**          File: conn.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-22 Wed 11:33 AM
**   Description: generic connection interface 
*******************************************************************************/
#ifndef SOCKSPP_CONN_H_
#define SOCKSPP_CONN_H_

class Conn {
  public:
    virtual ~Conn() = default;
    virtual void start() = 0;
    virtual void close() = 0;
};

#endif /* end of include guard: SOCKSPP_CONN_H_ */
