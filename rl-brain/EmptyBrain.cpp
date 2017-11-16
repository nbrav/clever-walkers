#define SERVICE_PORT 7891
#define _VERBOSE false
#define BUFSIZE 2048
#define CLOCK_PRECISION 1E9

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <fstream>
#include <time.h>

using namespace std;

#include "RLBrain.cpp"

int main(int argc, char **argv)
{
        printf("Initialization..");

	/* create a UDP socket */

        struct sockaddr_in myaddr;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	int fd;				/* our socket */
	int msgcnt = 0;			/* count # of messages we received */
	char buf[BUFSIZE];	/* receive buffer */

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(SERVICE_PORT);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}
        printf("Listening..");

	timespec t_send,t_recv;
	bool first_exchange=false;
	
	/* create brain object */
	
	qbrain brain;
	int state=0, action=0, reward=0, timestep=0;
	int text_in=0;
	std::srand (time(NULL));
	
	brain.reset();
	
	for (;;)
	{
	  // receive state,reward[TODO]
	  recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
	  clock_gettime(CLOCK_REALTIME, &t_recv);

	  printf(" unity_latency: %fms",
		 1000*(t_recv.tv_sec-t_send.tv_sec)+1000*(double)(t_recv.tv_nsec-t_send.tv_nsec)/(double)CLOCK_PRECISION);	  

	  if (recvlen > 0)
	  {
	    buf[recvlen] = 0;
	    text_in = atoi(buf);
	    //state = abs(text_in); reward = (text_in>0)?0:-1;
	    state = abs(text_in); reward = (text_in>=0)?0:-1; // TODO: now positive reward
	    printf("\n (\"%d\",\"%d\") -> [BRAIN] ", state, reward);
	  }
	  else
	    printf("uh oh - something went wrong!\n");	  
   	  
	  // simulate the "Brain"
	  msgcnt++;

	  brain.set_state(state);
	  brain.advance_timestep(state, reward, timestep);
	  action = brain.get_action();

	  timestep++;

	  // send action
	  sprintf(buf, "%d", action);
	  
	  //printf("sending response.. \n");
	  printf("-> (\"%s\")", buf);

	  if (sendto(fd, buf, strlen((char*)buf), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
	    perror("sendto");

	  clock_gettime(CLOCK_REALTIME, &t_send);
	  printf(" brain_latency: %fms",
		 1000*(t_send.tv_sec-t_recv.tv_sec)+1000*(double)(t_send.tv_nsec-t_recv.tv_nsec)/(double)CLOCK_PRECISION);

	}
}
