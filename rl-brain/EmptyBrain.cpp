#define _VERBOSE_UDP false
#define _VERBOSE_AS true
#define LEARNING true
#define SOFTMAX true

#define PORT 7890
#define BUFSIZE 2048
#define CLOCK_PRECISION 1E9
#define HALTING_ACTION 4

#define UNITY_IP "0.0.0.0"  // enter your PC IP here
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

	/* create variables for master thread */
	
	int* phi_goal_idx; float* phi_goal_val; int num_phi_goal=0; // Allocentric state 
	int* phi_goal_idx_prev; float* phi_goal_val_prev; int num_phi_goal_prev=0; // Allocentric state 
	
	int* phi_collide_idx; float* phi_collide_val; int num_phi_collide=0; // Egocentric state
	int* phi_collide_idx_prev; float* phi_collide_val_prev; int num_phi_collide_prev=0; // Egocentric state

	int direction_size=36, speed_size=5;
	float *action = new float[direction_size*speed_size], *action_previous = new float[direction_size*speed_size], *action_collide = new float[direction_size*speed_size];	
	for(int action_idx=0; action_idx<direction_size*speed_size; action_idx++)
	  action[action_idx]=0.0;
	
	double *policy_goal, *policy_collide; double* policy_behaviour = new double[direction_size*speed_size];
	
	int timestep=0; float reward_goal=0.0, reward_collide=0.0;		
	float sensor_msg[1000];
	float motor_msg[2];  // (direction,speed)
	
	float heading_direction = 0.0;

	phi_goal_idx = new int[100]; phi_goal_val = new float[100];
	phi_goal_idx_prev = new int[100]; phi_goal_val_prev = new float[100];
	phi_collide_idx = new int[1000]; phi_collide_val = new float[1000];
	phi_collide_idx_prev = new int[1000]; phi_collide_val_prev = new float[1000];
	
	/* create (multi-)brain objects */
	
	qbrain brain_goal(_rank,"goal",225,direction_size*speed_size); // allocentric
	qbrain brain_collide(_rank,"collide",1000,direction_size*speed_size); // egocentric

	/* trial variables */
	
	float reward_goal_trial=0, reward_collide_trial=0;
	float temperature = 1.0f;
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

	if(_master) printf("\nGearing up \"%s\" system for %d rl-brain..\n", processor_name, _size);

	// learning and trial meta-paremeters
	float Tr = 50*1000, Te = 100*1000;

	// >---------------------------------------------------> //
	// ^                    master loop                    v //
	// <---------------------------------------------------< //
	for (;timestep<Tr;) //timestep<Tr+Te
	{
	  // -------------------------------------------------------------
	  // ---------------------send action-----------------------------
	  if (sendto(fd, &motor_msg[0], sizeof(float)*2, 0, (struct sockaddr *)&remaddr, addrlen) < 0) perror("sendto");
	    
	  // -------------------------------------------------------------
	  // ------receive observations (K+2,s,phi_0,..,phi_K,reward)-----
	  for(;;)
	  {
	    recvlen = recvfrom(fd, sensor_msg, sizeof(float)*1000, 0, (struct sockaddr *)&remaddr, &addrlen);
	    if (recvlen<0) {printf("Uh oh! Something horrible happened with the simulator\n");break;}
  	    
	    // if global reset for all agents
	    if(recvlen==2*sizeof(float) && sensor_msg[0]==std::numeric_limits<float>::max()) 
	    {
	      //for(int action_idx=0; action_idx<direction_size*speed_size; action_idx++)
	      //action_previous[action_idx] = -1;

	      brain_goal.reset(); brain_collide.reset();
	      
	      brain_goal.trial_log(float(reward_goal_trial));
	      brain_collide.trial_log(float(reward_collide_trial));
	      
	      reward_goal_trial=0;
	      reward_collide_trial=0;

	      HALTING=false;
	    }	    
	    else if(recvlen==sizeof(float) && sensor_msg[0]==std::numeric_limits<float>::max()) // if reset agent (individual)
	      HALTING=true;
	    else
	      break;
	  }

	  if(HALTING)
	  {
	    if(_VERBOSE_UDP && _master)
	      printf("\n\n\nRESET! [AgIdx:%d T:%d] ",_rank,timestep); fflush(stdout);
	    continue;
	  }

	  // debug printing
	  if(_VERBOSE_UDP && _master) 
	  {
	    printf("\n\n\nRAW_UDP [AgIdx:%d T:%d] ",_rank,timestep);
	    
	    printf("[Out:%f,%f] ->",motor_msg[0],motor_msg[1]);
	    printf(" [In:");
	    for(int idx=0;idx<recvlen/sizeof(float);idx++)
	      printf("%0.1f,",sensor_msg[idx]); printf(" ]");	    
	    fflush(stdout);
	  }

	  // -------------------------------------------------------------
	  // ----------------extract from observations--------------------
	  // {K+4, #phi_goal, phi_goal_idx0, phi_goal_val0, phi_goal_idx1, phi_goal_val1,.., phi_collide_0,..,phi_collide_{K-1},reward_goal,reward_collide,heading_dir}
	  int sensor_msg_idx = 1;

	  if(_VERBOSE_UDP && _master) cout<<"\n\n";
	  
	  // Extract allocentric state
	  num_phi_goal = int(sensor_msg[sensor_msg_idx++]);	  
	  if(_VERBOSE_UDP && _master) cout<<"#S="<<num_phi_goal<<" (";
	  for(int phi_idx=0; phi_idx<num_phi_goal; phi_idx++)
	  {
	    phi_goal_idx[phi_idx] = int(sensor_msg[sensor_msg_idx++]);
	    phi_goal_val[phi_idx] = sensor_msg[sensor_msg_idx++];
	    if(_VERBOSE_UDP && _master) cout<<phi_goal_idx[phi_idx]<<"->"<<phi_goal_val[phi_idx]<<",";
	  }
	  if(_VERBOSE_UDP && _master) cout<<")";
	  	  
	  //Extract egocentric state vector
	  num_phi_collide = sensor_msg[sensor_msg_idx++]; 
	  if(_VERBOSE_UDP && _master) cout<<"\n#X="<<num_phi_collide<<" (";
	  for(int phi_idx=0; phi_idx<num_phi_collide; phi_idx++)
	  {
	    phi_collide_idx[phi_idx] = int(sensor_msg[sensor_msg_idx++]);
	    phi_collide_val[phi_idx] = sensor_msg[sensor_msg_idx++];
	    if(_VERBOSE_UDP && _master) cout<<phi_collide_idx[phi_idx]<<"->"<<phi_collide_val[phi_idx]<<",";
	  }
	  if(_VERBOSE_UDP && _master) cout<<")";

	  // extract rewards and heading direction
	  reward_goal = float(sensor_msg[int(sensor_msg_idx++)]);
	  reward_collide = float(sensor_msg[int(sensor_msg_idx++)]);	  
	  reward_goal_trial += reward_goal;
	  reward_collide_trial += reward_collide;
	  heading_direction = sensor_msg[int(sensor_msg_idx++)];

	  if(_VERBOSE_UDP && _master)
	    cout<<" R1:"<<reward_goal<<" R2:"<<reward_collide<<" HD:"<<heading_direction;

	  // -------------------------------------------------------------
	  // ----------------simulate the "Brain"-------------------------	  
	  msgcnt++;

	  // get policies from all modules
	  policy_goal = brain_goal.get_policy(num_phi_goal,phi_goal_idx,phi_goal_val);
	  policy_collide = brain_collide.get_policy(num_phi_collide,phi_collide_idx,phi_collide_val);
	  geocentricate(policy_collide, direction_size, speed_size, heading_direction);

	  // combine to behavioural policy
	  action_selection(policy_behaviour, policy_goal, policy_collide, direction_size*speed_size, heading_direction, _master*_VERBOSE_AS);

	  // GAIN POLICY BEHAVE
	  //temperature = 10.0*(1-float(timestep)/Tr);
	  //gain_policy(policy_behaviour,direction_size*speed_size, temperature);

	  // convert to action distribtions: find motor command (direction,speed) from action distributions
	  brain_goal.policy_to_motor(policy_behaviour,action,motor_msg);
	  action_collide = egocentricate(action,direction_size,speed_size,heading_direction); 
  
	  //for(int action_idx=0; action_idx<direction_size*speed_size; action_idx++)
	  //action_previous[action_idx] = action[action_idx];

	  // debugging
	  if(_master*_VERBOSE_AS*(timestep%1000>0 && timestep%1000<10))
	  {
	    cout<<"\n";
	    print_policy(policy_goal,direction_size*speed_size,"goal");     //KL(policy_goal,policy_behaviour,direction_size*speed_size);
	    print_policy(policy_collide,direction_size*speed_size,"coll");  //KL(policy_collide,policy_behaviour,direction_size*speed_size);
	    print_policy(policy_behaviour,direction_size*speed_size,"beha"); 
	  }
	  
	  // w += alpha*(r'+gamma*q(s',a')-q(s,a))
	  if(LEARNING)
	  {
	    //brain_goal.update_importance_samples(policy_goal, policy_behaviour, action);
	    //brain_collide.update_importance_samples(policy_collide, policy_behaviour, action);
		  
	    brain_goal.advance_timestep(num_phi_goal,phi_goal_idx,phi_goal_val,action,reward_goal,timestep);
	    brain_collide.advance_timestep(num_phi_collide,phi_collide_idx,phi_collide_val,action_collide,reward_collide,timestep);
	  }

	  // s = s' 
	  brain_goal.set_state(num_phi_goal,phi_goal_idx,phi_goal_val);
	  brain_collide.set_state(num_phi_collide, phi_collide_idx, phi_collide_val);

	  // a = a'
	  brain_goal.set_action(action);
	  brain_collide.set_action(action_collide);
	  
	  timestep++;

	  if(_master && msgcnt%1000==1)
	    printf("\n*[t:%d,temp=%f]",timestep,temperature); 
	}
	MPI_Finalize();
}
