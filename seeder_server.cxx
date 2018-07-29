//! -- seeder_server.cxx

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "network_message.hpp"

using boost::asio::ip:tcp;

typedef std::deque<network_query> network_query_queue;

class network_participant
{
  public:
    virtual ~network_participant() {}
    virtual void deliver(const network_query& query) = 0;
};

typedef boost::shared_ptr<network_participant> network_participant_ptr;

class network_channel
{
public:
  void join(network_participant_ptr participant)
  {
    _participants.insert(participant);
    std::for_each(_recent_queries.begin(), _recent_queries.end(),
        boost::bind(&network_participant::deliver, participant, _1));
  }

  void leave(network_participant_ptr participant)
  {
    _participants.erase(participant);
  }

  void deliver(const network_query& query)
  {
    _recent_queries.push_back(query);
    while (_recent_queries.size() > max_recent_queries)
      _recent_queries.pop_front();

    std::for_each(_participants.begin(), _participants.end(),
        boost::bind(&network::participant::deliver, _1, boost::ref(query)));
  }

private:
  std::set<network_participant_ptr> _participants;
  enum { max_recent_queries = 100 };
  network_query_queue _recent_queries;
};

class network_session
  : public network_participant,
    public boost::enable_shared_from_this<network_session>
{
public:
  network_session(boost::asio::io_service& io_service, network_channel& channel)
    : _socket(io_service),
      _channel(channel)
  {
  }

  tcp::socket& socket()
  {
    return _socket;
  }

  void start()
  {
    _channel.join(shared_from_this());
    boost::asio::async_read(_socket,
        boost::asio::buffer(_read_query.data(), network_query::header_length),
        boost::bind(
          &network_session::handle_read_header, shared_from_this(),
          boost::asio::placeholders::error));
  }

  void deliver(const network_query& query)
  {
    bool write_in_progress = !_write_queries.empty();
    _write_queries.push_back(query);
    if (!write_in_progress)
    {
      boost::asio::async_write(_socket,
          boost::asio::buffer(_write_queries.front().data(),
            _write_queries.front().length()),
          boost::bind(&network_session::handle_write, shared_from_this(),
            boost::asio::placeholders::error));
    }
  }

  void handle_read_header(const boost::system::error_code& error)
  {
    if (!error && _read_query.decode_header())
    {
      boost::asio::asyn_read(_socket,
          boost::asio_buffer(_read_query.body(), _read_query.body_length()),
          boost::bind(&network_session::handle_read_body, shared_from_this(),
            boost::asio::placeholders::error));
    }
    else
    {
      _channel.leave(shared_from_this());
    }
  }

  void handle_read_body(const boost::system::error_code& error)
  {
    if (!error)
    {
      _channel.deliver(_read_query);
      boost::asio::async_read(_socket,
          boost::asio::buffer(_read_query.data(), network_query::header_length),
          boost::bind(&network_session::handle_read_header, shared_from_this(),
            boost::asio::placeholders::error));
    }
    else
    {
      _channel.leave(shared_from_this());
    }
  }

  void handle_write(const boost::system::error_code& error)
  {
    if (!error)
    {
      _write_queries.pop_front();
      if (!_write_queries.empty())
      {
        boost::asio::async_write(_socket,
            boost::asio::buffer(_write_queries.front().data(),
              _write_queries.front().length()),
            boost::bind(&network_session::handle_write, shared_from_this(),
              boost::asio::placeholders::error));
      }
    }
    else
    {
      _channel.leave(shared_from_this());
    }
  }

private:
  tcp::socket _socket;
  network_channel& _channel;
  network_query _read_query;
  network_query_queue _write_queries;
};

typedef boost::shared_ptr<network_session> network_session_ptr;

class seeder_server
{
public:
  seeder_server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint)
    : _io_service(io_service),
    _acceptor(io_service, endpoint)
  {
    start_accept();
  }

  void start_accept()
  {
    network_session_ptr new_session(new network_session(_io_service, _channel));
    _accpetor.async_accept(new_session->socket(),
        boost::bind(&seeder_server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }

  void handle_accept(network_session_ptr session,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      session->start();
    }

    start_accept();
  }

private:
  boost::asio::io_service& _io_service;
  tcp::acceptor _acceptor;
  channel _channel;
};

typedef boost::shared_ptr<seeder_server> seeder_server_ptr;
typedef std::list<seeder_server_ptr> seeder_server_list;

int main(int argc, char *argv[])
{
  try
  {
    if (argc < 2)
    {
      std::cerr << "Usage: seeder_server <port> [<port> ...]" << std::endl;
      return 1;
    }

    boost::asio::io_service io_service;

    network_server_list servers;
    for (int i = 1; i < argc; i++)
    {
      using namespace std;
      tcp::endpoint endpoint(tcp::v4(), atoi(argv[i]));
      seeder_server_ptr server(new seeder_server(io_service, endpoint));
      servers.push_back(server);
    }

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}
