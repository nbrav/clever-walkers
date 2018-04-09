#define _VERBOSE_UDP false
#define _VERBOSE_AS false

#define LEARNING true
#define SOFTMAX true

#define OMEGA_LEARNING false

#define PORT 7890
#define BUFSIZE 2048
#define CLOCK_PRECISION 1E9
#define HALTING_ACTION 12

#define UNITY_IP "0.0.0.0" // enter your PC IP here
#define THIS_IP "127.0.0.1" // use if running Brain & Unity in same system

#include <iostream>
#include <iomanip>
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
#include "action_selection.cpp"

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
	srand((unsigned)time(NULL)+_rank);

	/* create (multi-)brain objects */
	
	qbrain brain_goal(_rank,"goal",225); // allocentric
	qbrain brain_collide(_rank,"collide",1000); // egocentric
	
	int* phi_goal_idx; float* phi_goal_val; int num_phi_goal=0; // Allocentric state (scalar)
	int* phi_goal_idx_prev; float* phi_goal_val_prev; int num_phi_goal_prev=0; // Allocentric state (scalar)
	
	int* phi_collide_idx; float* phi_collide_val; int num_phi_collide=0; // Egocentric state (vector)
	int* phi_collide_idx_prev; float* phi_collide_val_prev; int num_phi_collide_prev=0; // Egocentric state (vector)

	double *policy_goal, *policy_collide; double* policy_behaviour = new double[8*3];
	int action=rand()%(8*3), timestep=0; float reward_goal=0.0, reward_collide=0.0;
	int action_previous = rand()%(8*3);
	float sr_local[100];

	int action_collide = 0;
	
	float heading_direction = 0.0;

	phi_goal_idx = new int[100]; phi_goal_val = new float[100];
	phi_goal_idx_prev = new int[100]; phi_goal_val_prev = new float[100];
	phi_collide_idx = new int[100]; phi_collide_val = new float[100];
	phi_collide_idx_prev = new int[100]; phi_collide_val_prev = new float[100];
	
	/* trial variables */
	
	float reward_goal_trial=0, reward_collide_trial=0;
	int test_period=10;
	int trial_idx=0;
	std::srand (time(NULL)+_rank);
	int HALTING=false;

	/* setting-up socket material*/

	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen=-1;			/* # bytes received */	
	int fd;				/* our socket */ /* music-osc: udpSocket */       
	int msgcnt = 0;			/* count # of messages we received */
	char buf[BUFSIZE];	/* receive buffer */
	
        /* creating socket for each rank */

	if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) 
	{ 
	  perror("cannot create socket\n");
	  return 0;
	}
	memset((char *)&remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(PORT+_rank); 	
	if (inet_aton(THIS_IP, &remaddr.sin_addr)==0) 
	{
	  fprintf(stderr, "inet_aton() failed\n");
	  exit(1);
	}

	// gearing up brains
	
	brain_goal.reset(); 
	brain_collide.reset();

	if(_master)
	  printf("\nGearing up \"%s\" system for %d rl-brain..\n", processor_name, _size);

	// Learning and trial meta-paremeters
	float Tr = 100*1000, Te = 10000;

	if(LEARNING)
	{
	  brain_goal._omega = min(1.0f, max(0.001f, float(Tr)/Tr));
	  brain_collide._omega = min(1.0f, max(0.1f, float(Tr)/Tr));
	}
	else
	{
	  brain_goal._omega = 0.001;
	  brain_collide._omega = 0.1; // WORKING WITH 0.5!
	}
	
	// >---------------------------------------------------> //
	// ^                    master loop                    v //
	// <---------------------------------------------------< //
	for (;;) //timestep<Tr+Te
	{
	  // -------------------------------------------------------------
	  // ---------------------send action (A)-------------------------
	  if (sendto(fd, &action, sizeof(int), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
	    perror("sendto");
	    
	  // -------------------------------------------------------------
	  // ------receive observations (K+2,s,phi_0,..,phi_K,reward)-----
	  for(;;)
	  {
	    recvlen = recvfrom(fd, sr_local, sizeof(float)*100, 0, (struct sockaddr *)&remaddr, &addrlen);
	    if (recvlen<0) {printf("Uh oh! Something horrible happened with the simulator\n");break;}
  	    
	    // if global reset for all agents
	    if(recvlen==2*sizeof(float) && sr_local[0]==std::numeric_limits<float>::max()) 
	    {
	      action_previous = -1; brain_goal.reset(); brain_collide.reset();
	      trial_idx++;
	      
	      brain_goal.trial_log(float(reward_goal_trial)); brain_collide.trial_log(float(reward_collide_trial));
	      reward_goal_trial=0; reward_collide_trial=0;

	      if(LEARNING && SOFTMAX)
	      {
		brain_goal._omega = min(1.0f, max(0.001f, float(Tr-timestep)/Tr));
		brain_collide._omega = min(1.0f, max(0.01f, float(Tr-timestep)/Tr));
	      }
	
	      HALTING=false;
	    }	    
	    else if(recvlen==sizeof(float) && sr_local[0]==std::numeric_limits<float>::max()) // if reset agent (individual)
	      HALTING=true;
	    else
	      break;
	  }

	  if(HALTING)
	  {
	    if(_VERBOSE_UDP && _master)
	    {
	       printf("\n\n\nRESET! [AgIdx:%d T:%d] ",_rank,timestep); fflush(stdout);
	    }
	    action=HALTING_ACTION;
	    continue;
	  }

	  // debug printing
	  if(_VERBOSE_UDP && _master) 
	  {
	    //printf("\n\n\nRAW_UDP [AgIdx:%d T:%d] ",_rank,timestep); printf("[Out:%d] ->",action);
	    //printf(" [In:");
	    //for(int idx=0;idx<recvlen/sizeof(float);idx++) printf("%0.1f,",sr_local[idx]);
	    //printf(" ]");	    
	  }
	  //fflush(stdout);

	  // -------------------------------------------------------------
	  // ----------------extract from observations--------------------
	  // {K+4,
	  //  #phi_goal, phi_goal_idx0, phi_goal_val0, phi_goal_idx1, phi_goal_val1,..,
	  //  phi_collide_0,..,phi_collide_{K-1},
	  //  reward_goal,reward_collide,heading_dir}
	  int sr_local_idx = 1;
	  
	  // Extract allocentric state
	  num_phi_goal = int(sr_local[sr_local_idx++]);	  
	  //if(_VERBOSE_UDP && _master) cout<<"\n#S="<<num_phi_goal<<" (";
	  for(int phi_idx=0; phi_idx<num_phi_goal; phi_idx++)
	  {
	    phi_goal_idx[phi_idx] = int(sr_local[sr_local_idx++]);
	    phi_goal_val[phi_idx] = sr_local[sr_local_idx++];
	    //if(_VERBOSE_UDP && _master) cout<<phi_goal_idx[phi_idx]<<"->"<<phi_goal_val[phi_idx]<<",";
	  }
	  //if(_VERBOSE_UDP && _master) cout<<")";
	  	  
	  //Extract egocentric state vector
	  num_phi_collide = sr_local[sr_local_idx++]; 
	  if(_VERBOSE_UDP && _master) cout<<"\n#X="<<num_phi_collide<<" (";
	  for(int phi_idx=0; phi_idx<num_phi_collide; phi_idx++)
	  {
	    phi_collide_idx[phi_idx] = int(sr_local[sr_local_idx++]);
	    phi_collide_val[phi_idx] = 1;
	    if(_VERBOSE_UDP && _master) cout<<phi_collide_idx[phi_idx]<<",";
	  }
	  if(_VERBOSE_UDP && _master) cout<<")";

	  // extract rewards and heading direction
	  reward_goal = float(sr_local[int(sr_local_idx++)]);
	  reward_collide = float(sr_local[int(sr_local_idx++)]);	  
	  reward_goal_trial += reward_goal;
	  reward_collide_trial += reward_collide;
	  heading_direction = sr_local[int(sr_local_idx++)];

	  if(_VERBOSE_UDP && _master)
	    cout<<" R1:"<<reward_goal<<" R2:"<<reward_collide<<" HD:"<<heading_direction;

	  // -------------------------------------------------------------
	  // ----------------simulate the "Brain"-------------------------	  
	  msgcnt++;

	  // omega-learning
	  if(msgcnt!=1)
	  {
	    //brain_goal.update_omega(prob_behave,policy_goal[action]);
	    //brain_collide.update_omega(prob_behave,policy_collide[action_collide]);
	  }
	  
	  // a' ~ Q(.|s',A)
	  policy_goal = brain_goal.get_policy(num_phi_goal,phi_goal_idx,phi_goal_val);
	  policy_collide = brain_collide.get_policy(num_phi_collide,phi_collide_idx,phi_collide_val);
	  geocentricate(policy_collide, 8, 3, heading_direction); // important!  

	  // b = \prod_i pi_i
	  action_selection(policy_behaviour, policy_goal, action_previous, policy_collide, heading_direction, _master*_VERBOSE_AS*(msgcnt%1000==0));

	  action = get_softmax_action(policy_behaviour,8*3);
	  action_collide = egocentricate(action,8,3,heading_direction);

	  action_previous = action;

	  // w += alpha*(r'+gamma*q(s',a')-q(s,a))
	  if(LEARNING)
	  {
	    //brain_goal.update_importance_samples(policy_goal, policy_behaviour, action);
	    //brain_collide.update_importance_samples(policy_collide, policy_behaviour, action);
	    
	    brain_goal.advance_timestep(num_phi_goal,phi_goal_idx,phi_goal_val,action,reward_goal,timestep);
	    //brain_collide.advance_timestep(num_phi_collide,phi_collide_idx,phi_collide_val,action_collide,reward_collide,timestep);
	  }
	  
	  // s = s'
	  brain_goal.set_state(num_phi_goal,phi_goal_idx,phi_goal_val);
	  brain_collide.set_state(num_phi_collide, phi_collide_idx, phi_collide_val);
	  
	  // a = a'
	  brain_goal.set_action(action);
	  brain_collide.set_action(action_collide);
	  
	  timestep++;

	  if(_master && msgcnt%1000==1)
	    printf("\n*[t:%d](omega_g=%f,omega_c=%f)",timestep, brain_goal._omega,brain_collide._omega); 
	}
	MPI_Finalize();
}
