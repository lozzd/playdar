#ifndef __HTTP_STRAT_H__
#define __HTTP_STRAT_H__
#include <boost/asio.hpp>

using namespace boost::asio::ip;
/*
    Consider this a nasty hack until I find a decent c++ http library

*/
class HTTPStreamingStrategy : public StreamingStrategy
{
public:

    HTTPStreamingStrategy(string url)
        : m_connected(false)
    {
        
        boost::regex re("http://([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+):([0-9]+)(.*)");
        boost::cmatch matches;
        if(boost::regex_match(url.c_str(), matches, re))
        {
            m_host = matches[1];
            m_port = boost::lexical_cast<int>(matches[2]);
            m_url  = matches[3];
        }
        else
        {
            cerr << "Invalid URL passed to HTTPStreamingStrategy" << endl;
            throw;
        }
    }

    HTTPStreamingStrategy(string host, unsigned short port, string url)
        : m_host(host), m_url(url), m_port(port)
    {
        m_connected = false;
    }
    
    ~HTTPStreamingStrategy(){  }
    

    int read_bytes(char * buf, int size)
    {
        if(!m_connected) do_connect();
        if(!m_connected)
        {
            cout << "Failed to fetch over http" << endl;
            reset();
            return 0;
        }
        int p = m_partial.length();
        if(p)
        {
            if(p <= size)
            {
                memcpy(buf, m_partial.c_str(), p);
                m_partial="";
                return p;
            }else{
                memcpy(buf, m_partial.c_str(), size);
                m_partial = m_partial.substr(size-1, string::npos);
                return size;
            }
        }
        boost::system::error_code error;
        size_t len = m_socket->read_some(boost::asio::buffer(buf, size), error);

        if (error == boost::asio::error::eof)
        {
            reset();
            return len; // Connection closed cleanly by peer.
        }
        else if (error)
        {
            reset();
            throw boost::system::system_error(error); // Some other error.
        }   
        return len;
    }

    
    string debug()
    { 
        ostringstream s;
        s<< "HTTPStreamingStrategy( host='"<<m_host<<"' port='"<<m_port<<"' url='"<<m_url<<"')";
        return s.str();
    }
    
    void reset()
    {
        m_socket.reset();
        m_partial="";
        m_connected=false;
    }
    
private:



    void do_connect()
    {
        //cout << debug() << endl; //"HTTPStreamingStrategy: http://" << m_host << ":" << m_port << m_url << endl;
        boost::asio::ip::address_v4 ip = boost::asio::ip::address_v4::from_string(m_host);

        tcp::resolver resolver(m_io_service);
        tcp::endpoint ep(ip, m_port);

        m_socket = boost::shared_ptr<boost::asio::ip::tcp::socket>(new tcp::socket(m_io_service));

        boost::system::error_code error = boost::asio::error::host_not_found;
        m_socket->connect(ep, error);
        if (error) throw boost::system::system_error(error);

        boost::system::error_code werror;
        ostringstream rs;
        rs  << "GET " << m_url << " HTTP/1.0\r\n"
            << "Host: " <<m_host<<":"<<m_port<<"\r\n"
            << "Accept: */*\r\n"
            << "Connection: close\r\n"
            << "\r\n";
        boost::asio::write(*m_socket, boost::asio::buffer(rs.str()), boost::asio::transfer_all(), werror);
        if(werror)
        {
            cerr << "Error making request" << endl;
            throw boost::system::system_error(werror);
            return;
        }
        boost::asio::streambuf response;
       
        boost::asio::read_until(*m_socket, response, "\r\n");

        // Check that response is OK.
        std::istream response_stream(&response);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
        {
          std::cout << "Invalid response\n";
          return;
        }
        if (status_code != 200)
        {
          std::cout << "Response returned with status code " << status_code << "\n";
          return;
        }

        // Read the response headers, which are terminated by a blank line.
        boost::asio::read_until(*m_socket, response, "\r\n\r\n");

        // Process the response headers.
        std::string header;
        while (std::getline(response_stream, header) && header != "\r")
          std::cout << header << "\n";
        //std::cout << "\n";

        // save whatever content we already have.
        if (response.size() > 0)
        {
            ostringstream part;
            part << &response;
            m_partial = string(part.str());
            //cout << "partial size: " << m_partial.length() << " partial='"<< part.str() <<"'" << endl;
        }
        m_connected = true;
    }
    
    boost::asio::io_service m_io_service;
    boost::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
    bool m_connected;
    string m_partial;
    string m_host;
    string m_url;
    unsigned short m_port;
};

#endif
