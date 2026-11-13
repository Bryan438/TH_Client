#ifndef _LINE_BUFFER_H_
#define _LINE_BUFFER_H_

#include <string>
#include <vector>

// Splits a TCP byte stream into newline-terminated messages.
// TCP has no message boundaries: one recv() may hold half a message or several,
// so bytes are kept here until a full line is available.
class line_buffer{
  private:
    std::string pending;
    size_t max_line;
  public:
    explicit line_buffer(size_t max_line_bytes = 64 * 1024) : max_line(max_line_bytes) {}

    // Add received bytes and move every complete line into `out`.
    // Returns false if a line grows past the limit (the peer is misbehaving).
    bool feed(const char* data, size_t len, std::vector<std::string>& out){
      pending.append(data, len);
      size_t start = 0;
      size_t newline;
      while((newline = pending.find('\n', start)) != std::string::npos){
        std::string line = pending.substr(start, newline - start);
        if(!line.empty() && line.back() == '\r'){
          line.pop_back();
        }
        if(!line.empty()){
          out.push_back(line);
        }
        start = newline + 1;
      }
      pending.erase(0, start);
      return pending.size() <= max_line;
    }
};

#endif
