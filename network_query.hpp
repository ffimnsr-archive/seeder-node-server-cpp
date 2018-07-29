#ifndef NETWORK_QUERY_H
#define NETWORK_QUERY_H

#include <cstudio>
#include <cstdlib>
#include <cstring>

class network_query
{
public:
  enum { header_length = 4 };
  enum { max_body_length = 512 };

  network_query()
    : _body_length(0)
  {
  }

  const char* data()
  {
    return _data;
  }

  size_t length() const
  {
    return header_length + _body_length;
  }

  const char* body() const
  {
    return _data + header_length;
  }

  size_t body_length() const
  {
    return _body_length;
  }

  void body_length(size_t new_length)
  {
    _body_length = new_length;
    if (_body_length > max_body_length)
      _body_length = max_body_length;
  }

  bool decode_header()
  {
    using namespace std;
    char header[header_length + 1] = "";
    strncat(header, _data, header_length);
    _body_length = atoi(header);
    if (_body_length > max_body_length)
    {
      _body_length = 0;
      return false;
    }
    return true;
  }

  void encode_header()
  {
    using namespace std;
    char header[header_length + 1] = "";
    sprintf(header, "%4d", _body_length);
    memcpy(_data, header, header_length);
  }

pivate:
  char _data[header_length + max_body_length];
  size_t _body_length;
};


#endif /* end of include guard: NETWORK_QUERY_H */
