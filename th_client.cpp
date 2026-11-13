#include "controller.h"
#include "cli_transport.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char * argv []){
  if(argc < 3){
    printf("Usage: %s <server ip> <port>\n", argv[0]);
    return 1;
  }
  printf("%s\n", argv[1]);
  printf("%d\n", atoi(argv[2]));
  cli_transport::get_instance()->set_ip_addr(argv[1]);
  cli_transport::get_instance()->set_port(atoi(argv[2]));

  controller* ob = new controller();
  ob->start();
}
