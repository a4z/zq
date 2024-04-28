//  Hello World server
#include <stdio.h>
#include <string.h>
#include <zmq.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <assert.h>

int main(void) {
  //  Socket to talk to clients
  void *context = zmq_ctx_new();

  // create zmq message with hello world
  zmq_msg_t msg;
  zmq_msg_init_size(&msg, 11);
  memcpy(zmq_msg_data(&msg), "Hello World", 11);
  // create an other message, and move msg into it
  zmq_msg_t msg2;
  zmq_msg_init(&msg2);
  zmq_msg_move(&msg2, &msg);
  // close both messages
  zmq_msg_close(&msg);
  zmq_msg_close(&msg2);

  void *responder = zmq_socket(context, ZMQ_REP);
  int rc = zmq_bind(responder, "tcp://*:5555");
  assert(rc == 0);
  printf("start loop\n");
  int i = 0;
  while (i++ < 3) {

    char buffer[10];
    zmq_recv(responder, buffer, 10, 0);
    printf("Received Hello\n");
#ifdef _WIN32
    Sleep(1000);
#else
    sleep(1); /* sleep for 100 milliSeconds */
#endif
    zmq_send(responder, "World", 5, 0);
  }
  printf("Done with 3 replies\n");
  return 0;
}
