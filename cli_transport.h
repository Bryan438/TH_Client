#include "observer.h"
#include "message_content.h"
#include <cstddef>

class cli_transport{
  private:
    cli_transport();
    int client_socket;
    observer* p_listener = NULL;
    static cli_transport* instance;
    char ip_addr[16]; 
    int port;
  public:
    ~cli_transport(){};

    static cli_transport* get_instance();

    void set_ip_addr(char* ip);
    void set_port(int p);

    int start_client();
    int send_msg(char* message, int length);
    void read(int socket_id);
    void read_with_thread(int socket_id);
    void serialize(int command, int length, char* buf);
    message_content* deserialize(const char* buf);
    void disconnect();
    void set_listener(observer* ob);
    char* convert_buffer_to_base64(char* message, int length);
};
