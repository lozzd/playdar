#include "application/application.h"
#include "resolvers/rs_http_gateway_script.h"
#include "library/library.h"
#include <boost/foreach.hpp>


RS_http_gateway_script::RS_http_gateway_script(MyApplication * a) 
    : ResolverService(a)
{
    m_scriptpath = "./etc/demo-resolver.php";
    cout << "HTTP Gateway script starting: "<<m_scriptpath << endl;
    init_worker();
}

void
RS_http_gateway_script::init_worker()
{
        std::vector<std::string> args;
        args.push_back("--playdar-mode");
        bp::context ctx;
        ctx.stdout_behavior   = bp::capture_stream();
        ctx.stdin_behavior    = bp::capture_stream();
//      ctx.stderr_behavior   = bp::capture_stream();

        bp::child c = bp::launch(m_scriptpath, args, ctx);
        m_c = new bp::child(c);
        m_os = & c.get_stdin();
        m_t = boost::thread(&RS_http_gateway_script::process_output, this);
}
    

// runs forever processing output
void 
RS_http_gateway_script::process_output()
{
    using namespace json_spirit;
    bp::pistream &is = m_c->get_stdout();
    std::string line;
    // expects: "QID\tURL"
    while (std::getline(is, line))
    {
        std::cout << "From script: '" << line << "'"<< std::endl;
        try
        {
            Value j;
            if(!read(line, j)) continue;
            Object ro = j.get_obj();
            map<string,Value> rr;
            obj_to_map(ro,rr);    
            query_uid qid = rr["qid"].get_str();
            
            Array resultsA = rr["results"].get_array();
            vector< boost::shared_ptr<PlayableItem> > v;
            BOOST_FOREACH(Value & result, resultsA)
            {
                Object po = result.get_obj();
                boost::shared_ptr<PlayableItem> pip;
                pip = PlayableItem::from_json(po);
                cout << "Parserd pip from script: " << endl;
                write_formatted(  pip->get_json(), cout );
                map<string,Value> po_map;
                obj_to_map(po, po_map);
                string url   = po_map["url"].get_str();  
                cout << "url=" << url << endl;
                boost::shared_ptr<StreamingStrategy> s(new HTTPStreamingStrategy(url));
                pip->set_streaming_strategy(s);
                v.push_back(pip);
            }
            report_results(qid, v);
        }
        catch(...)
        {
            cout << "Invalid response from script, skipping" << endl;
            continue;
        }
    }
    cout << "Script died, fail." << endl;
}

void 
RS_http_gateway_script::send_input(string s)
{
        cout << "Sending to script: '"<<s<<"'" <<endl;
        *m_os << s << endl;
}


void
RS_http_gateway_script::start_resolving(boost::shared_ptr<ResolverQuery> rq)
{
    ostringstream o;
    using namespace json_spirit;
    write_formatted( rq->get_json(), o );
    string s = o.str();
    unsigned int pos;
    while((pos = s.find("\n"))!=string::npos) s.erase(pos,1);
    send_input(s);
}
