#ifndef __TRACK_H__
#define __TRACK_H__

using namespace std;

class Track
{
public:
    Track(int id, string name, artist_ptr ap)
        :m_id(id), m_name(name), m_artist(ap)
    {}
    
    int id() const          { return m_id; }
    string name() const     { return m_name; }
    artist_ptr artist() const { return m_artist; }
    
    
private:
    int m_id;
    string m_name;
    artist_ptr m_artist;
};
#endif
