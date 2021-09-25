#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <argp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>


using std::cout;
using std::endl;
struct server_arguments {
  int port;
  int drop_perc;
};

typedef struct myNode {
  time_t update;
  int seq;
  struct sockaddr_in ip_port;
  struct myNode *next;
}Node;




error_t server_parser(int key, char *arg, struct argp_state *state) {

  struct server_arguments *args = (server_arguments* )(state->input);
  error_t ret = 0;
  switch(key) {
  case 'p':
    /* Validate that port is correct and a number, etc!! */
    args->port = atoi(arg);
    if (args->port == 0 || args->port <= 1024 /* port is invalid */) {
      argp_error(state, "Invalid option for a port");
    }
    break;
  case 'd':
    args->drop_perc = atoi(arg);
		
    break;
  default:
    ret = ARGP_ERR_UNKNOWN;
    break;
  }
    
  return ret;
}

void *server_parseopt(struct server_arguments *args,int argc, char *argv[]) {

  memset(args, 0, sizeof(*args));


  struct argp_option options[] = {
    { "port", 'p', "port", 0, "The port to be used for the server" ,0},
    { "dropped_perc", 'd', "dropped_perc", 0, "The percentage dropped", 0},
    {0}
  };
  struct argp argp_settings = { options, server_parser, 0, 0, 0, 0, 0 };
  if (argp_parse(&argp_settings, argc, argv, 0, NULL, args) != 0) {
    printf("Got an error condition when parsing \n");
  }

  /* What happens if you don't pass in parameters? Check args values
   * for sanity and required parameters being filled in */

  if (!args->port) {
    fputs("A port number must be specified \n", stderr);
    exit(1);
  }

  if (!args->drop_perc) {
    args->drop_perc = 0;
  }

  /* If they don't pass in all required settings, you should detect
   * this and return a non-zero value from main */
  printf("Got port %d and dropped percentage %d\n", args->port, args->drop_perc);

  return args;
}

Node *update_list(Node *head, sockaddr_in ip_port, int new_seq, time_t time)
{
	
  //if head is null, first addition


  Node *curr = head;
  Node *prev = NULL;
  int found = 0;
  if(head == NULL)
    {
      Node *newnode = (Node*)malloc(sizeof(Node));
      memset(newnode,0,sizeof(Node));

      newnode->ip_port = ip_port;
      newnode->seq = new_seq;
      newnode->update = time;
      newnode->next = NULL;

      return newnode;
    }
  else{
	
    //else you have to traverse through the list
    //make a current and previous pointer


    //while end not reached

    while(curr != NULL)
      {   
	//save currentaddr and currentport
	sockaddr_in curr_ip_port = curr->ip_port;
	//if you find the same client , found = 1(found)
        if(curr_ip_port.sin_addr.s_addr == ip_port.sin_addr.s_addr && 
	   curr_ip_port.sin_port == ip_port.sin_port){
	  found = 1;

	  //if you are past the last update of the same addr update it with new one
	  if(difftime(curr->update, time) >= 120){

	    curr->update = time;
	    curr->seq = new_seq;
	  }
	  else
	    {
	      //else check if the current list seq > parameter seq 
	      if(curr->seq > new_seq){
		//print as shown in description
		printf("%s:%s %d %d\n", curr->ip_port.sin_addr, curr->ip_port.sin_port, new_seq, curr->seq);
	      }else{
		// update accordingly

		curr->update = time;
		curr->seq = new_seq;
	      }
            }
        }
	else{

	  //if not the same client but greater than 120 , replace
	  if(difftime(time, curr->update) >= 120){
	    curr->update = time;
	    curr->seq = 0;
	  }
        }

	
	//change current and prev
        prev = curr ;
        curr = curr->next;
      }

  }
  //end while

  if(found==0){

    Node *newnode = (Node*)malloc(sizeof(Node));
    memset(newnode,0,sizeof(Node));

    newnode->ip_port = ip_port;
    newnode->seq = new_seq;
    newnode->update = time;
    newnode->next = NULL;

    prev->next = newnode; 
  }

  return head;

}

void rec_data(int new_socket, int drop)
{
  Node *client_list = NULL;
  struct sockaddr_in client; // Client address
  socklen_t len;
  int random;

  while(true){
    //client = NULL;
    len = sizeof(client);

    // Client sends buffer and recieve here 
    uint8_t data[22]; 

    recvfrom(new_socket, &data, 22, 0, (struct sockaddr *) &client, &len ); 

    //check for dropped Percentage 
    random = ((rand()%(100+1)+0));
        
    if(drop == 0 || random > drop){

      //store current time stamp
      struct timespec curr_t;
      clock_gettime(CLOCK_REALTIME,&curr_t);

      time_t sec = curr_t.tv_sec;
      long nsec = curr_t.tv_nsec;

      //store what the client sent
      uint32_t seq_cli;
      uint64_t sec_cli;
      uint64_t nsec_cli;
        
      memcpy(&seq_cli, data, 4);
      memcpy(&sec_cli, data+6, 8);
      memcpy(&nsec_cli,data+14,  8);
		
      seq_cli = ntohl(seq_cli); 
      sec_cli = be64toh(sec_cli);
      nsec_cli = be64toh(nsec_cli);

      //this function call should change the parameters accordingly 
      client_list = update_list(client_list, client, seq_cli, sec);

      //reconstruct data buffer to send back content
      uint8_t ret_data[38];

      seq_cli = htonl(seq_cli); 
      sec_cli = htobe64(sec_cli);
      nsec_cli = htobe64(nsec_cli);

      uint16_t ver_cli = 7;
      ver_cli = htons(ver_cli);

      uint64_t serv_sec_nb = htobe64(sec);
      uint64_t serv_nanosec_nb = htobe64(nsec);

      //data to be sent seq , version, seconds, nano seconds,
      // server neconds, server nanoseconds(8 + 8 + 2 + 4 + 8 + 8)

      memcpy(ret_data, &seq_cli, 4);
      memcpy(ret_data+4, &ver_cli, 2);
      memcpy(ret_data+6, &sec_cli, 8);
      memcpy(ret_data+14, &nsec_cli, 8);
      memcpy(ret_data+22, &serv_sec_nb, 8);
      memcpy(ret_data+30, &serv_nanosec_nb, 8);

      //sent to client
      printf("while  befote loop check \n");
      sendto(new_socket, ret_data, 38, 0, (struct sockaddr *) &client.sin_addr, sizeof(client));
      printf("while loop check \n");

    }

  }

}


void create(struct server_arguments args, int servSock){

  int new_socket;

  // Construct local address structure
  struct sockaddr_in servAddr; // Local address
  int add_len = sizeof(servAddr);
  memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure

  servAddr.sin_family = AF_INET; // IPv4 address family
  servAddr.sin_addr.s_addr = INADDR_ANY; // Any incoming interface
  servAddr.sin_port = htons(args.port); // Local port

  // Bind to the local address
  if (bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
    perror("bind() failed");
    exit(1);
  }



  // Mark the socket so it will listen for incoming connections
  // if (listen(servSock, 1) < 0) {
  // 	perror("listen() failed");
  // 	exit(1);
  // }
  else{
    cout<<"bollo ja bolbe bolo"<<endl;
  }
	



	
  // 	if ((new_socket = accept(servSock, (struct sockaddr *)&servAddr,  
  //                    (socklen_t*)&add_len))<0) 
  // { 
  //     perror("accept"); 
  //     exit(EXIT_FAILURE); 
  // } 

  rec_data(servSock, args.drop_perc);

	
}




int main(int argc, char *argv[]){
  int servSock;
  if ((servSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket() failed");
    exit(1);
  }
	
  struct server_arguments args;
  server_parseopt(&args, argc, argv); 
  create(args, servSock);

	
  return 1;
}
