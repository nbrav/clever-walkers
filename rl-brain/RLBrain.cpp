#include <iostream>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <time.h>
#include <algorithm>
#include <math.h>
#include <string>

#define PI 3.14159265

using namespace std;

class qbrain
{
  public:
  
  // meta-parameters
  float _alpha;                // Learning rate
  std::vector< float > _gamma; // Discount factor for Q
  float _epsilon;              // Greedy arbitration exploration
  float _lambda;               // Eligibility decay
  string _tag="def";
  
  int _state_size;
  
  private:

  // global parameters
  int _time;
  int _rank;

  // state
  int *_phi, *_phi_prime;
  int _num_phi, _num_phi_prime;    
  int* _state;
  int _state_prime;

  // reward
  std::vector< float > _reward;
  int _reward_size;
  std::vector< float > _rpe;    

  // action
  int _action, _action_prime;
  int _action_size;

  // weights
  std::vector< std::vector< std::vector<float> > > _w;
  std::vector< std::vector< std::vector<float> > > _etrace;

  // test/train trail variables
  bool _is_test_trial=false;
  
  // i/o files
  FILE *qvalue_outfile;
  FILE *qvalue_infile;   //TODO: do we need two files for reading and writing?  
  FILE *state_outfile;
  FILE *action_outfile;
  FILE *reward_outfile;
  FILE *trial_outfile;
  char const *QVALUE_FILE, *REWARD_FILE, *STATE_FILE, *ACTION_FILE, *TRIAL_FILE;
  
 public:

  bool VERBOSE;
  
  qbrain(int rank, string tag, int state_size=100)
  {
    _tag = tag;
    _rank = rank;
    parse_param();
    _state_size = state_size; // override default state-size

    float gaussian[] = {0.1,0.2,0.4,0.7,0.4,0.2,0.1,0.1};

    _state = new int[_state_size];
    _phi = new int[_state_size]; _num_phi=0;
    _phi_prime = new int [_state_size]; _num_phi_prime=0;

    for(int idx=0;idx<_state_size; idx++)
    {
      _state[idx]=0;
      _phi[idx]=0;
      _phi_prime[idx]=0;
    }
    
    std::ifstream fs(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str());

    if(fs.is_open())
    {
      // Read Q matrix from file
      qvalue_infile = fopen(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str(),"rb");
      fseek(qvalue_infile, -sizeof(float)*_reward_size*_state_size*_action_size, SEEK_END);

      _w.resize(_reward_size);
      for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      {
	_w[reward_idx].resize(_state_size);
	for (int state_idx=0; state_idx<_state_size; state_idx++)
	{
	  _w[reward_idx][state_idx].resize(_action_size);
	  fread(&_w[reward_idx][state_idx][0], sizeof(float), _action_size, qvalue_infile);
	}
      }      
      fclose(qvalue_infile);   
    }
    else
    {
      // Initialize Q matrix
      _w.resize(_reward_size);
      for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      {
	_w[reward_idx].resize(_state_size);
	for (int state_idx=0; state_idx<_state_size; state_idx++)
	{
	  _w[reward_idx][state_idx].resize(_action_size,1);
	  for (int action_idx=0; action_idx<_action_size; action_idx++)
	  {
	    _w[reward_idx][state_idx][action_idx] = 0.0; //(float)rand()/RAND_MAX; //gaussian[action_idx]; //
	  }
	}
      }
    }
           
    // Initialize eligibility matrix
    _etrace.resize(_reward_size);
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
    {
      _etrace[reward_idx].resize(_state_size);
      for (int state_idx=0; state_idx<_state_size; state_idx++)
      {
	_etrace[reward_idx][state_idx].resize(_action_size,1);
	for (int action_idx=0; action_idx<_action_size; action_idx++)
	{
	  _etrace[reward_idx][state_idx][action_idx] = 0.0;
	}
      }
    }   
    
    _reward.resize(_reward_size);
    
    _rpe.resize(_reward_size);
    std::fill(_rpe.begin(), _rpe.end(), 1.0);

    qvalue_outfile = fopen(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    fclose(qvalue_outfile);

    //state_outfile = fopen(("./data/state."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    //fclose(state_outfile);

    //reward_outfile = fopen(("./data/reward-punishment."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    //fclose(reward_outfile);

    trial_outfile = fopen(("./data/trial."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    fclose(trial_outfile);
  }

  void parse_param()
  {
    _state_size = 100; //24*4*10; //TODO: Must use a param file     
    _action_size = 8;  //TODO: Must use a param file     
    _reward_size = 1; //TODO : Must use a param file     
    
    _alpha = 0.01; // learning rate
    _epsilon = 0.9; // epsilon-greedy
    _lambda = 0.0; // eligibility parameter 0.8
    
    _gamma.resize(_reward_size);
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      _gamma[reward_idx] = 0.7; // temporal-decay rate
  }

  void reset()
  {
    _num_phi = -1;
    
    for(int idx=0;idx<_state_size; idx++)
      _state[idx]=0;

    _action = 0;

    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      _reward[reward_idx] = 0.0;
    
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      for (int state_idx=0; state_idx<_state_size; state_idx++)
    	for (int action_idx=0; action_idx<_action_size; action_idx++)
    	  _etrace[reward_idx][state_idx][action_idx] = 0.0;
  }

  /*---------- Logger -----------*/
  
  void trial_log(float trial_reward)
  {    
    trial_outfile = fopen(("./data/trial."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab"); //ab
    fwrite(&trial_reward, sizeof(float), _reward_size, trial_outfile);
    fclose(trial_outfile);    
  }

  void state_log()
  {
    state_outfile = fopen(("./data/state."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab"); //ab
    fwrite(&_state[0], sizeof(int)*_state_size, 1, state_outfile);
    fclose(state_outfile);    
  }

  void action_log()
  {
    action_outfile = fopen(("./data/action."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab"); //ab
    fwrite(&_action, sizeof(int), 1, action_outfile);
    fclose(action_outfile);    
  }

  void reward_log()
  {
    reward_outfile = fopen(("./data/reward-punishment."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab"); //ab
    fwrite(&_reward[0], sizeof(float) , _reward_size, reward_outfile);
    fclose(reward_outfile);    
  }

  void qvalue_log()
  {
    qvalue_outfile = fopen(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      for (int state_idx=0; state_idx<_state_size; state_idx++)
      	fwrite(&_w[reward_idx][state_idx][0], sizeof(float) , _action_size, qvalue_outfile);
    fclose(qvalue_outfile);
  }
  
  /* ---------- I/O ----------------*/

  void set_test_train(bool is_test_trial)
  {
    _is_test_trial = is_test_trial;
  }
  
  void set_state(int num_phi, int* phi)
  {
    _num_phi = num_phi;
    _phi = phi;
    
    for(int idx=0;idx<_state_size; idx++)
      _state[idx]=0;
    for(int idx=0; idx<_num_phi && _phi[idx]<_state_size; idx++)
      _state[_phi[idx]]=1;
  }

  void set_action(int action)
  {
    _action = action;
  }

  float* get_policy(int num_phi, int* phi)
  {
    float* _qvalue = new float[_action_size];
    float* _policy = new float[_action_size];
    float _policy_sum = 0.0;
       
    // forall a: q_cap(s_t,A) = 0
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      _qvalue[action_idx] = 0.0;    

    // forall a: q_cap(s_t,A) = sum_i w_i*phi_i(s_t,A)
    for(int idx=0; idx<num_phi && phi[idx]<_state_size; idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	_qvalue[action_idx] += _w[0][phi[idx]][action_idx];

    for(int action_idx=0; action_idx<_action_size; action_idx++)
    {
      if(_tag=="collide")
	_policy[action_idx] = exp(-_qvalue[action_idx]/1.0);
      else if(_tag=="goal")
	_policy[action_idx] = exp(_qvalue[action_idx]/1.0);
	
      _policy_sum += _policy[action_idx];
    }

    /*if(_tag=="collide")
    {
      printf("\nPI_COLLIDE[");
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	{
	  printf("%0.2f,",_policy[action_idx]);    
	}
      printf("]");
      //printf("%d",action);
    }*/

    if(_policy_sum==0)
      return _policy;

    for(int action_idx=0; action_idx<_action_size; action_idx++)
    {
      _policy[action_idx] /= _policy_sum;
    }
        
    return _policy;
  }
  
  int get_action(int num_phi, int* phi)
  {
    float* _qvalue = new float[_action_size];
    int action;

    // reset condition
    if(num_phi<0)
      return rand()%_action_size;
    
    // forall a: q_cap(s_t,a) = sum_i w_i*phi_i(s_t,a)
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      _qvalue[action_idx] = 0.0;    
    for(int idx=0; idx<num_phi && phi[idx]<_state_size; idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	_qvalue[action_idx] += _w[0][phi[idx]][action_idx];

    // a_t^greedy = argmax_a q_cap(s_t,a)
    int greedy_action = rand()%_action_size;;

    for(int action_idx=0; action_idx<_action_size; action_idx++)
      if(_qvalue[action_idx]>_qvalue[greedy_action])
	greedy_action=action_idx;    

    // a_t = a_t^greedy (epsilon greedy)
    if (((float)rand()/RAND_MAX) < _epsilon) // P(exploit) = _epsilon
      action = greedy_action; 
    else // explore
      action = rand()%_action_size;

   return action;
  }

  /*------------- Update rules ----------*/
  
  void update_etrace()
  {
    //for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
    //for (int state_idx=0; state_idx<_state_size; state_idx++)
    //	for (int action_idx=0; action_idx<_action_size; action_idx++)
    //	  _etrace[reward_idx][state_idx][action_idx] *= (_lambda*_gamma[reward_idx]);
    // TODO
    //for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      //_etrace[reward_idx][_state][_action] = 1.0;
  }

  void update_w()
  {
    // TODO: add eligibility trace

    // get max Q(s',A)
    float max_qprime=-999.0;
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      if(get_qvalue(_num_phi_prime,_phi_prime,action_idx) > max_qprime)
	max_qprime = get_qvalue(_num_phi_prime,_phi_prime,action_idx);
    
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
    {      
      // releasing dopamine
      // delta_t = r_{t+1} + gamma*q(phi(s_{t+1}),a_{t+1}) - q(phi(s_t),a_t);
      _rpe[reward_idx] = _reward[reward_idx]
	+ _gamma[reward_idx] * max_qprime
      	- get_qvalue(_num_phi, _phi, _action);
    }
    //DEBUGGING * get_qvalue(_num_phi_prime, _phi_prime, _action_prime)
    
    // cortico-striatal learning
    // _w(phi(s_t,a_t)) += alpha*delta*phi(s_t,a_t)
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      for(int idx=0; idx<_num_phi && _phi[idx]<_state_size; idx++)
	_w[reward_idx][_phi[idx]][_action] += _alpha*_rpe[reward_idx]; //_etrace[reward_idx][state_idx][action_idx];    */

    if(false) //_VERBOSE_UDP && _tag=="collide")
    {
      printf("\n{AGIDX:%d T:%d eps:%0.1f LOG:%d ",_rank,_time,_epsilon,_is_test_trial);
      printf("S:(");
      for(int idx=0; idx<_num_phi; idx++)
	printf("%d,",_phi[idx]);
      printf("),A:%d -%0.1f-> ",_action,_reward[0]);
    
      printf("S':(");
      for(int idx=0; idx<_num_phi_prime; idx++)
	printf("%d,",_phi_prime[idx]);
      printf("),A':%d}",_action_prime);
    }
    fflush(stdout);
  }

  float get_qvalue(int num_phi, int* phi, int action)
  {
    float _qvalue=0.0;

    // q_cap(s,a) = sum_i w_i*phi_i(s,a)
    for(int idx=0; idx<num_phi && phi[idx]<_state_size; idx++)
      _qvalue += _w[0][phi[idx]][action];

    return _qvalue;
  }
  
  void advance_timestep(int num_phi_prime, int* phi_prime, int action_prime, int reward, int time)    
  {
    _num_phi_prime = num_phi_prime;
    _phi_prime = phi_prime;
    _action_prime = action_prime;
    
    _reward[0] = reward;
    _time = time;

    if(_time%1000==0)
      qvalue_log();

    if(_num_phi<0)
      return;
   
    //update_etrace();
    update_w();

    //reward_log();
    //state_log();
    //action_log();
  }  
};
