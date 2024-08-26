#ifndef _MESSAGE_CONTENT_H_
#define _MESSAGE_CONTENT_H_
class message_content{
  private:
    int length;
    char* charmessage;
    int intmessage;
    int command;
  public:
    message_content();
    ~message_content();

    void set_command(int command);
    void set_charmessage(char* msg, int length);
    void set_intmessage(int msg);

    int get_length();
    char* get_charmessage();
    int get_intmessage();
    int get_command();
}; 
#endif

