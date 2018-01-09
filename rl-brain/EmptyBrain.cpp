#define PORT 7890
#define _VERBOSE_UDP false
#define BUFSIZE 2048
#define CLOCK_PRECISION 1E9

#define UNITY_IP "0.0.0.0" // enter your PC IP here
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

        int NUM_AGENTS = _size;

	if(_size!=NUM_AGENTS)
	  perror("agents!=cores");

	char processor_name[MPI_MAX_PROCESSOR_NAME]; int name_len;
	MPI_Get_processor_name(processor_name, &name_len);
	_master = (_rank==0);

	/* create (multi-)brain objects */
	
	qbrain brain(_rank);
	int state=0, state_previous=-1, action=0, reward=0, timestep=0;
	int num_phi=0;
	int* phi;
	int sr_local[100];

	/* trial variables */
	int reward_trial=0;
	int test_period=10;
	int trial_idx=0;

	bool is_test_trial=false;

	int32_t action_global[NUM_AGENTS], sr_global[100*NUM_AGENTS];
	
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

	brain.reset();

	// print message
	if(_master)
	  printf("\nGearing up \"%s\" system for %d rl-brain", processor_name, _size);

	// ----------------------------------- //
	// -------------master loop ---------- //
	// ----------------------------------- //
	for (;;)
	{	  
	  //if(!_rank && msgcnt%1000==0)
	  //printf("\n%d updates and going strong..", msgcnt); 

          // send (action)
	  MPI_Gather(&action, 1, MPI_INT, action_global, 1, MPI_INT, 0, MPI_COMM_WORLD);
	  
	  if(_master)
	  {
	    if (sendto(fd, action_global, sizeof(int)*NUM_AGENTS, 0, (struct sockaddr *)&remaddr, addrlen) < 0)
	      perror("sendto");
	    
	    if (_VERBOSE_UDP)
	    {
	      printf("\nT:%d (A:",msgcnt);
	      for(int _r=0; _r<NUM_AGENTS; _r++) printf("%d,", action_global[_r]);
	      printf(")");
	    }
	    fflush(stdout);
	  }

	  // receive (K+1,phi_0,..,phi_K,reward)
	  for(;_master;)
	  {
	    recvlen = recvfrom(fd, sr_global, sizeof(int)*100, 0, (struct sockaddr *)&remaddr, &addrlen);
	    
	    if (recvlen<0) {printf("Uh oh! Something horrible happened with the simulator\n");break;}
	    
	    // reset condition
	    if(recvlen==sizeof(int) && sr_global[0]==255)
	    {
	      brain.reset();

	      if(_VERBOSE_UDP) printf("\n\n");
	      
	      trial_idx++;

	      if(trial_idx%10==0)
	      {
		brain.trial_log(float(reward_trial));
		brain._epsilon=1.0;
		brain.set_test_train(true);
	      }
	      else
	      {
		brain._epsilon=1.0;
		brain.set_test_train(false);
	      }
	      reward_trial=0;		
	    }	    
	    else
	      break;
	  }

	  MPI_Scatter(&sr_global, 100, MPI_INT, sr_local, 100, MPI_INT, 0, MPI_COMM_WORLD);

	  num_phi = sr_local[0]-1;
	  phi = new int[num_phi];
	  
	  if(_master)
	  {
	    if(_VERBOSE_UDP) printf(" S'(%d:",num_phi);
	    for(int phi_idx=1; phi_idx<=num_phi; phi_idx++)
	    {
	      phi[phi_idx-1] = sr_local[phi_idx];	      
	      if(_VERBOSE_UDP) printf("%d,", sr_local[phi_idx]);
	    }
	  }
	  reward = sr_local[sr_local[0]];
	  reward_trial += reward;
	      
	  if(_VERBOSE_UDP) printf(") R':(%d)->[brain] ",reward);	    
	  fflush(stdout);

	  // simulate the "Brain"
	  msgcnt++;

	  //if(state_previous != state) // filter Markovian state
	  {	    
	    // a' ~ Q(.|s',A)
	    action = brain.get_action(num_phi, phi); 

	    // w += alpha*(r'+gamma*q(s',a')-q(s,a))
	    brain.advance_timestep(num_phi, phi, action, reward, timestep);

	    // s = s'
	    brain.set_state(num_phi, phi);

	    // a = a'
	    brain.set_action(action);
	    
	    timestep++;	    
	  }	  
	}

	MPI_Finalize();
}
