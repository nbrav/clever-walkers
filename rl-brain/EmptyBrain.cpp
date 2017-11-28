#define PORT 7890
#define _VERBOSE_UDP false
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
	
        int _size, _rank, _master;

	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &_rank);

	if(_size!=NUM_AGENTS)
	  perror("agents!=cores");

	char processor_name[MPI_MAX_PROCESSOR_NAME]; int name_len;
	MPI_Get_processor_name(processor_name, &name_len);
	_master = (_rank==0);

	/* create (multi-)brain objects */
	
	qbrain brain(_rank);
	int state=0, action=0, reward=0, timestep=0;
	int sr_local[2];
	
	int32_t action_global[NUM_AGENTS], sr_global[NUM_AGENTS*2];
	
	int text_in=0;
	std::srand (time(NULL)+_rank);

	/* setting-up socket material*/

	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen=-1;			/* # bytes received */
	
	int fd;				/* our socket */ /* music-osc: udpSocket */
	
	int msgcnt = 0;			/* count # of messages we received */
	char buf[BUFSIZE];	/* receive buffer */
	
        /* creating socket in master */

	if(_master)
	{
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
	}

	/* setting-up timer material */
	
	timespec t_send,t_recv;
	bool first_exchange=false;
	
	brain.reset();

	// print message
	if(_master)
	  printf("\nGearing up \"%s\" system for %d rl-brain", processor_name, _size);

	for (;;)
	{
	  if(!_rank && msgcnt%1000==0)
	    printf("\n%d updates and going strong..", msgcnt); 

          // send action                              
	  MPI_Gather(&action, 1, MPI_INT, action_global, 1, MPI_INT, 0, MPI_COMM_WORLD);
	  
	  if(_master)
	  {
	    if (sendto(fd, action_global, sizeof(int)*NUM_AGENTS, 0, (struct sockaddr *)&remaddr, addrlen) < 0)
	      perror("sendto");

	    if(_VERBOSE_UDP)
	    {
	      printf("->(");
	      for(int _r=0; _r<NUM_AGENTS; _r++)
		printf("%d,", action_global[_r]);
	      printf(")");
	    }
	    fflush(stdout);
	  }

	  // receive state,reward [TODO]
	  if(_master)
	  {
	    recvlen = recvfrom(fd, sr_global, sizeof(int)*NUM_AGENTS*2, 0, (struct sockaddr *)&remaddr, &addrlen);
	    clock_gettime(CLOCK_REALTIME, &t_recv);
	    
	    if (recvlen<0)	    
	      printf("Uh oh! Something horrible happened with the simulator\n");	  
	  }

	  MPI_Scatter(&sr_global, 2, MPI_INT, sr_local, 2, MPI_INT, 0, MPI_COMM_WORLD);

	  state = (sr_local[0]);
	  reward = (sr_local[1]);
	  
	  if(_master && _VERBOSE_UDP)
	  {
	    printf("\n(");
	    for(int _r=0; _r<NUM_AGENTS; _r++)
	      printf("[%d,%d]", sr_global[_r*2],sr_global[_r*2+1]);
	    printf(")->[brain]");
	  }
	  fflush(stdout);

	  // simulate the "Brain"
	  msgcnt++;

	  brain.advance_timestep(state, reward, timestep);

	  brain.set_state(state);
	  action = brain.get_action();

	  timestep++;
	  
	  clock_gettime(CLOCK_REALTIME, &t_send);	  
	}

	MPI_Finalize();
}
