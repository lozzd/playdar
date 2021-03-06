#ifndef __RS_HTTP_GATEWAY_SCRIPT_H__
#define __RS_HTTP_GATEWAY_SCRIPT_H__

#include <vector>
#include <iostream>

#include <boost/process.hpp>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include "resolvers/resolver_service.h"
namespace bp = ::boost::process;

class RS_http_gateway_script : public ResolverService
{
    public:
    RS_http_gateway_script(MyApplication * a);
    
    void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    std::string name() { return string("HTTP Gateway script: ")+m_scriptpath; }
    
    private:
        void init_worker();
        void process_output();
        void send_input(string s);
        
        string m_scriptpath;
        bp::child * m_c;
        boost::thread m_t;
        bp::postream * m_os;
};

#endif
