#define PORT 7890
#define _VERBOSE_UDP true
#define BUFSIZE 2048
#define CLOCK_PRECISION 1E9

#define NARESH_IP "130.229.176.87" // enter your PC IP here
#define THIS_IP "127.0.0.1" // use if running Brain & Unity in same system

#define NUM_AGENTS 10

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
#include <mpi.h>

using namespace std;

#include "RLBrain.cpp"

int main(int argc, char **argv)
{
	/* setting-up MPI */
	
	int _size, _rank;

	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &_rank);

	if(_size!=NUM_AGENTS)
	  std::
	// Get the name of the processor
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;
	MPI_Get_processor_name(processor_name, &name_len);

	// Print off a hello world message
	if(world_rank==0)
	printf("Hello world from processor %s, rank %d"
	       " out of %d processors\n",
	       processor_name, world_rank, world_size);

	// Finalize the MPI environment.
	MPI_Finalize();

	/* create (multi-)brain objects */
	
	qbrain brain;
	int state=0, action=0, reward=0, timestep=0;
	int text_in=0;
	std::srand (time(NULL));

	/* setting-up socket material*/
	
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen=-1;			/* # bytes received */

	int fd;				/* our socket */ /* music-osc: udpSocket */

	int msgcnt = 0;			/* count # of messages we received */
	char buf[BUFSIZE];	/* receive buffer */
	
        /* creating socket */
	
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

	/* setting-up timer material */
	
	timespec t_send,t_recv;
	bool first_exchange=false;
	
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

	  // receive state,reward [TODO]
	  recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
	  clock_gettime(CLOCK_REALTIME, &t_recv);

	  if (recvlen > 0)
	  {
	    buf[recvlen] = 0;
	    text_in = atoi(buf);
	    state = abs(text_in); 
	    reward = (text_in>=0)?0:-1;
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
