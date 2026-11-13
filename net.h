#ifndef _NET_H_
#define _NET_H_

#include <functional>
#include <string>

// TCP connection to the server. One JSON message per line in both directions.
class connection{
  private:
    int sock = -1;
  public:
    ~connection();

    // host may be an IP address or a host name. Returns false with a reason on failure.
    bool open(const std::string& host, int port, std::string& error);

    // Read lines on a background thread. on_close runs once when the server goes away.
    void start_reader(std::function<void(const std::string&)> on_line, std::function<void()> on_close);

    bool send_line(const std::string& line);
};

#endif
