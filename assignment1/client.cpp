#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <argp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream> 
#include <math.h>
#include <unistd.h>


using std::cout;
using std::endl;

#define MAX_PAYLOAD 16777216
struct client_arguments {
  struct sockaddr_in ip_port;

  int treq;
  int timeout;
};

typedef struct message {
  uint32_t seq;
  float theta;
  float delta;
  message(){
    seq = 0;
    theta = 0;
    delta = 0;
  }
}message;





error_t client_parser(int key, char *arg, struct argp_state *state) {
  struct client_arguments *args = (client_arguments *) state->input;
  error_t ret = 0;
  switch(key) {
  case 'a':
    inet_pton(AF_INET, arg, &(args->ip_port.sin_addr));
    break;	
  case 'p':
    args->ip_port.sin_port = atoi(arg);
    break;
  case 'n':
    /* validate argument makes sense */
    args->treq = atoi(arg);
    break;
  case 't':
    args->timeout = atoi(arg);
    break;
  default:
    ret = ARGP_ERR_UNKNOWN;
    break;
  }
  return ret;
}


client_arguments client_parseopt(int argc, char *argv[]) {

  // memset(args, 0, sizeof(*args));

  struct argp_option options[] = {
    { "addr", 'a', "addr", 0, "The IP address the server is listening at", 0},
    { "port", 'p', "port", 0, "The port that is being used at the server", 0},
    { "treq", 'n', "treq", 0, "The number of time requests to send to the server", 0},
    { "timeout", 't', "timeout", 0, "The timeout percentage", 0},
    {0}
  };

  struct argp argp_settings = { options, client_parser, 0, 0, 0, 0, 0 };
  struct client_arguments args;

  bzero(&args, sizeof(args));

  if (argp_parse(&argp_settings, argc, argv, 0, NULL, &args) != 0) {
    printf("Got error in parse\n");
  }
  /* If they don't pass in all required settings, you should detect
   * this and return a non-zero value from main */

  printf("Got port %d with n=%d timeout=%d \n", args.ip_port.sin_port, args.treq, args.timeout);

  return args;
		   
	

}



int main(int argc, char *argv[])
{

  client_arguments client = client_parseopt(argc, argv);
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr = client.ip_port;
  if (sock < 0)
    {
      perror("socket() failed");
    }else{
    printf("Socket success");
  }

  memset(&serv_addr, 0, sizeof(serv_addr)); // Zero out structure
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(client.ip_port.sin_port);

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("\nConnection Failed \n");
		
    }

    
  uint32_t timeout = htonl(client.timeout);
  uint32_t treq = client.treq;

  int i=0;
	
  while(i < (int)treq)
    {	
      //get sequence to be sent
      uint32_t seq = i +1;
      seq = htonl(seq);
		
      //version already initialized, doesnt change but needs to be convertion
      uint16_t ver = 7;
      ver = htonl(ver);

      //get the time stamp to be sent
      struct timespec curr_t;
      clock_gettime(CLOCK_REALTIME,&curr_t);

      time_t sec = curr_t.tv_sec;
      long nsec = curr_t.tv_nsec;

      uint8_t data[22]; //data to be sent seq , version, seconds, nano seconds,(8 + 8 + 2 + 4)

      memcpy(data,&seq, 4);
      memcpy(data,&seq, 2);
      memcpy(data,&seq, 8);
      memcpy(data,&seq, 8);

      sendto(sock, data,22, 0, (const struct sockaddr *) &serv_addr, sizeof(serv_addr));
      i++;
    }




  //recieve from server

  //create poll
  struct pollfd check[1];
  check[0].fd = sock;
  check[0].events = POLLIN;

  struct sockaddr server_addr; // Source address of server
  socklen_t len = sizeof(server_addr);
  i = 0;
  int t_out, c_tout = client.timeout;

  message finish[client.treq];
  //memset(finish, 0, client.treq*sizeof(finish));

  while(i < treq)
    {   
      // printf("Enters the while loop: i: %d treq: %d\n", i, (int)treq);
      if(c_tout == 0)
        {
	  printf("time = 0");
	  t_out = -1;
        }
      else
        {
	  t_out = c_tout*1000;
        }


      // if(poll(check ,1 ,t_out) == -1 || poll(check ,1 ,t_out) == -0){
      int poller = poll(check, 1, t_out);
      if(poller == 0){
	printf("BREAK\n");
	break;
      }

      uint8_t ret_data[38]; 
      printf("trying to recieve\n");
      int ret = recvfrom(sock, &ret_data, 38, 0, (struct sockaddr *)&server_addr, &len);
      printf("recieved\n");

      if (ret>= 0) {

        struct timespec curr_t;
        clock_gettime(CLOCK_REALTIME,&curr_t);
        time_t sec_rn = curr_t.tv_sec;
        long nsec_rn = curr_t.tv_nsec;

        uint32_t seq;
        uint16_t ver;
        uint64_t c_sec;
        uint64_t c_nsec;
        uint64_t s_sec;
        uint64_t s_nsec;

        memcpy(&seq, ret_data,4);
        memcpy(&ver, ret_data+4,2);
        memcpy(&c_sec, ret_data+6,8);
        memcpy(&c_nsec, ret_data+14,8);
        memcpy(&s_nsec, ret_data+22,8);
        memcpy(&s_nsec, ret_data+30,8);

        seq = ntohl(seq);
        ver = ntohs(ver);
        c_sec = be64toh(c_sec);
        c_nsec = be64toh(c_nsec);
        s_sec = be64toh(s_sec);
        s_nsec = be64toh(s_nsec);

        if(finish[seq-1].seq != 0){
	  i--;
        }
        else
	  {

	    long double temp = c_nsec/1000000000;
	    long double t0 = c_sec + temp;
	    temp = s_nsec/1000000000;
	    long double t1 = s_sec + temp;
	    temp = nsec_rn/1000000000;
	    long double t2 = sec_rn + temp;
        
	    float theta = (t1-t0 + t1 - t2)/2;
	    float delta = t2 - t0;
	    finish[seq-1].seq = seq;
	    finish[seq-1].delta = delta;
	    finish[seq-1].theta = theta;

	  }
      }
      else {
	break;
      }
      i++;
    }

  close(sock);

  for(int i =0;i < (int)client.treq; i++){
    if(finish[i].seq == 0)
      printf("%d: Dropped\n", i+1);
    else
      printf("%d: %.4f %.4f\n", i+1, finish[i].theta, finish[i].delta);
  }


	
}
