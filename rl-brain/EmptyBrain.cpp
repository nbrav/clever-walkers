#define PORT 7890
#define _VERBOSE_UDP false
#define BUFSIZE 2048
#define CLOCK_PRECISION 1E9

//#define NARESH_IP "130.229.176.87"
#define THIS_IP "127.0.0.1" // use if running Brain & Unity in same system

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
	/* create a UDP socket */
	
        //struct sockaddr_in myaddr;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen=-1;			/* # bytes received */

	int fd;				/* our socket */ /* music-osc: udpSocket */

	int msgcnt = 0;			/* count # of messages we received */
	char buf[BUFSIZE];	/* receive buffer */
	
        // creating socket
	if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) 
	{
	  perror("cannot create socket\n");
	  return 0;
	}

	memset((char *)&remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(PORT); 

	if (inet_aton(THIS_IP, &remaddr.sin_addr)==0) 
	{
	  fprintf(stderr, "inet_aton() failed\n");
	  exit(1);
	}
	
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
	  if(msgcnt%1000==0)
	    printf("\n%d updates and going strong..", msgcnt); 

          // send action                              
          sprintf(buf, "%d ", action);          

          if (sendto(fd, buf, strlen((char*)buf), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
            perror("sendto");

          if(_VERBOSE_UDP) printf("-> (\"%s\")", buf); fflush(stdout);

	  // receive state,reward[TODO]
	  recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
	  clock_gettime(CLOCK_REALTIME, &t_recv);

	  if (recvlen > 0)
	  {
	    buf[recvlen] = 0;
	    text_in = atoi(buf);
	    state = abs(text_in); reward = (text_in>=0)?0:-1;
	    if(_VERBOSE_UDP) printf("\n (\"%d\",\"%d\") -> [BRAIN] ", state, reward);
	  }
	  else
	    printf("Uh oh! Something horrible happened with the simulator\n");	  
   	  
	  // simulate the "Brain"
	  msgcnt++;

	  brain.advance_timestep(state, reward, timestep);

	  brain.set_state(state);
	  action = brain.get_action();

	  timestep++;
	  
	  clock_gettime(CLOCK_REALTIME, &t_send);	  
	}
}
