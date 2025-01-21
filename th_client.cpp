#include "controller.h"
#include "cli_transport.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char * argv []){
  
  printf("%s\n", argv[1]);
  printf("%d\n", atoi(argv[2]));
  cli_transport::get_instance()->set_ip_addr(argv[1]);
  cli_transport::get_instance()->set_port(atoi(argv[2]));

  controller* ob = new controller();
  ob->start();
}
