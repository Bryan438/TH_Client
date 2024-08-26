#include "observer.h"
#include "message_content.h"

class cli_transport{
  private:
    cli_transport();
    int client_socket;
    observer* p_listener = NULL;
    static cli_transport* instance;
  public:
    ~cli_transport(){};

    static cli_transport* get_instance();

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
