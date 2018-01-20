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

  private:

  // global parameters
  int _time;
  int _rank;

  // vision state
  int* _state;
  int _state_size;

  // place-cell state
  //float *_state_pc;
  int _state_pc_size;
  
  // reward
  std::vector< float > _reward;
  int _reward_size;
  std::vector< float > _rpe;    

  // action
  int _action, _action_prime;
  int _action_size;

  // vision state-action phi_(s,a)
  std::vector< float > _phi_pc;
  std::vector< float >_phi_pc_prime;
  //int _num_phi_pc, _num_phi_pc_prime;

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
  
 public:

  bool VERBOSE;
  
  qbrain(int rank)
  {
    _rank = rank;
    parse_param();

    float gaussian[] = {0.1,0.2,0.4,0.7,0.4,0.2,0.1,0.1};

    /* vision state */ 
    /*_state = new int[_state_size];
    _phi = new int[_state_size]; _num_phi=0;
    _phi_prime = new int [_state_size]; _num_phi_prime=0;

    for(int idx=0;idx<_state_size; idx++)
    {
      _state[idx]=0;
      _phi[idx]=0;
      _phi_prime[idx]=0;
    }
    */
    
    /* place cell state */
    //_state_pc = new float[_state_pc_size];

    //for(int idx=0;idx<_state_pc_size; idx++)
    //_state_pc[idx]=0.0;

    //int _num_phi_pc=0;
    //int _num_pc_prime=0;

    _phi_pc.resize(_state_pc_size);
    _phi_pc_prime.resize(_state_pc_size);
    for (int state_pc_idx=0; state_pc_idx<_state_pc_size; state_pc_idx++)
    {
      _phi_pc[state_pc_idx] = 0.0;
      _phi_pc_prime[state_pc_idx] = 0.0;
    }
    
    /* Q(phi_vision->action) */
    
    /*std::ifstream fs(("./data/qvalue."+std::to_string(_rank)+".log").c_str());
    if(fs.is_open())
    {
      // Read Q matrix from file
      qvalue_infile = fopen(("./data/qvalue."+std::to_string(_rank)+".log").c_str(),"rb");
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
    */
      
    /* Q(phi_placecell->action) */
    
    std::ifstream fs(("./data/qvalue."+std::to_string(_rank)+".log").c_str());
    if(fs.is_open())
    {
      // Read Q matrix from file
      qvalue_infile = fopen(("./data/qvalue."+std::to_string(_rank)+".log").c_str(),"rb");
      fseek(qvalue_infile, -sizeof(float)*_reward_size*_state_pc_size*_action_size, SEEK_END);

      _w.resize(_reward_size);
      for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      {
	_w[reward_idx].resize(_state_pc_size);
	for (int state_pc_idx=0; state_pc_idx<_state_pc_size; state_pc_idx++)
	{
	  _w[reward_idx][state_pc_idx].resize(_action_size);
	  fread(&_w[reward_idx][state_pc_idx][0], sizeof(float), _action_size, qvalue_infile);
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
	_w[reward_idx].resize(_state_pc_size);
	for (int state_pc_idx=0; state_pc_idx<_state_pc_size; state_pc_idx++)
	{
	  _w[reward_idx][state_pc_idx].resize(_action_size,1);
	  for (int action_idx=0; action_idx<_action_size; action_idx++)
	  {
	    _w[reward_idx][state_pc_idx][action_idx] = 0.0; //(float)rand()/RAND_MAX; //gaussian[action_idx]; //
	  }
	}
      }
    }

    // Initialize eligibility matrix
    /*_etrace.resize(_reward_size);
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
    */
    
    _reward.resize(_reward_size);

    _rpe.resize(_reward_size);
    std::fill(_rpe.begin(), _rpe.end(), 1.0);

    qvalue_outfile = fopen(("./data/qvalue."+std::to_string(_rank)+".log").c_str(), "ab");
    fclose(qvalue_outfile);

    state_outfile = fopen(("./data/state."+std::to_string(_rank)+".log").c_str(), "ab");
    fclose(state_outfile);

    reward_outfile = fopen(("./data/reward-punishment."+std::to_string(_rank)+".log").c_str(), "ab");
    fclose(reward_outfile);

    trial_outfile = fopen(("./data/trial."+std::to_string(_rank)+".log").c_str(), "ab");
    fclose(trial_outfile);
  }

  void parse_param()
  {
    //_state_size = 24*4*10; //TODO: Must use a param file     
    _state_pc_size = 50; //TODO: Must use a param file     
    _action_size = 8;  //TODO: Must use a param file     
    _reward_size = 1; //TODO : Must use a param file     
    
    _alpha = 0.01; // learning rate
    _epsilon = 0.9; // epsilon-greedy
    _lambda = 0.0; // eligibility parameter 0.8
    
    _gamma.resize(_reward_size);
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      _gamma[reward_idx] = 0.9; // temporal-decay rate
  }

  void reset()
  {
    //_num_phi_pc=0;
    //_num_phi_pc_prime=0;
    
    for (int state_pc_idx=0; state_pc_idx<_state_pc_size; state_pc_idx++)
    {
      _phi_pc[state_pc_idx] = 0.0;
      _phi_pc_prime[state_pc_idx] = 0.0;
    }

    /*TODOfor(int idx=0;idx<_state_size; idx++)
      _state[idx]=0;
    */

    //for(int idx=0;idx<_state_pc_size; idx++)
    //_state_pc[idx]=0;

    _action = 0;

    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      _reward[reward_idx] = 0.0;
    
    /*
      TODO
      for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      for (int state_idx=0; state_idx<_state_size; state_idx++)
    	for (int action_idx=0; action_idx<_action_size; action_idx++)
    	  _etrace[reward_idx][state_idx][action_idx] = 0.0;
    */
  }

  /*---------- Logger -----------*/
  
  void trial_log(float trial_reward)
  {
    trial_outfile = fopen(("./data/trial."+std::to_string(_rank)+".log").c_str(), "ab"); //ab
    fwrite(&trial_reward, sizeof(float), _reward_size, trial_outfile);
    fclose(trial_outfile);    
  }

  void state_log()
  {
    state_outfile = fopen(("./data/state."+std::to_string(_rank)+".log").c_str(), "ab"); //ab
    //TODO fwrite(&_state[0], sizeof(int)*_state_size, 1, state_outfile);
    fclose(state_outfile);    
  }

  void action_log()
  {
    action_outfile = fopen(("./data/action."+std::to_string(_rank)+".log").c_str(), "ab"); //ab
    fwrite(&_action, sizeof(int), 1, action_outfile);
    fclose(action_outfile);    
  }

  void reward_log()
  {
    reward_outfile = fopen(("./data/reward-punishment."+std::to_string(_rank)+".log").c_str(), "ab"); //ab
    fwrite(&_reward[0], sizeof(float) , _reward_size, reward_outfile);
    fclose(reward_outfile);    
  }

  void qvalue_log()
  {
    //qvalue_outfile = fopen(QVALUE_FILE, "ab");
    qvalue_outfile = fopen(("./data/qvalue."+std::to_string(_rank)+".log").c_str(), "ab");
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      for (int state_pc_idx=0; state_pc_idx<_state_pc_size; state_pc_idx++)
      	fwrite(&_w[reward_idx][state_pc_idx][0], sizeof(float) , _action_size, qvalue_outfile);
    fclose(qvalue_outfile);   
  }
  
  /* ---------- I/O ----------------*/

  void set_test_train(bool is_test_trial)
  {
    _is_test_trial = is_test_trial;
  }
  
  /*void set_state(int num_phi, int* phi)
  {
    _num_phi = num_phi;
    _phi = phi;
    
    for(int idx=0;idx<_state_size; idx++)
      _state[idx]=0;
    for(int idx=0; idx<_num_phi && _phi[idx]<_state_size; idx++)
      _state[_phi[idx]]=1;
      }*/

  void set_state_pc(int num_phi_pc, float* phi_pc, int action)
  {
    // s_i(t) = phi_i(t)
    //for(int idx=0;idx<_state_pc_size; idx++)
    //_state_pc[idx]=phi_pc[idx];

    // phi_i(s,a) = s_i(t) * (a_t==a)
    //_num_phi_pc = num_phi_pc;

    if(num_phi_pc != _state_pc_size)
      printf("\n Bad UDPs!!");

    for(int idx=0; idx<_state_pc_size; idx++)
      _phi_pc[idx] = phi_pc[idx];
        
    //for(int idx=0; idx<_num_phi_pc && _phi_pc[idx]<_state_pc_size; idx++)
    //_state_pc[_phi_pc[idx]]=1;
  }

  void set_action(int action)
  {
    _action = action;
  }
  
  int get_action(int temp_num_phi_pc, float* temp_phi_pc)
  {
    vector< float > temp_vec_phi_pc;
    
    if(temp_num_phi_pc != _state_pc_size)
      printf("Bad UDPs!!");

    temp_vec_phi_pc.resize(_state_pc_size);
    for(int idx=0; idx<_state_pc_size; idx++)
    {
	temp_vec_phi_pc[idx] = temp_phi_pc[idx];
    }

    float* temp_qvalue = new float[_action_size];
    int action;

    // reset condition
    //if(temp_num_phi_pc<0)
    //return rand()%_action_size;

    // forall a: q_cap(s_t,a) = sum_i w(phi_{i,t}(s,a),a) * phi_{i,t}(s,a)
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      temp_qvalue[action_idx] = 0.0;
    
    for(int idx=0; idx<_state_pc_size; idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	if(temp_phi_pc[idx]!=0)
	  temp_qvalue[action_idx] = _w[0][phi_idx][action];//get_q_pc_value(temp_num_phi_pc, temp_vec_phi_pc, action_idx);

    // a_t^greedy = argmax_a q_cap(s_t,a)
    int greedy_action = rand()%_action_size;;

    for(int action_idx=0; action_idx<_action_size; action_idx++)
      if(temp_qvalue[action_idx]>temp_qvalue[greedy_action])
	greedy_action=action_idx;    

    // a_t = a_t^greedy (epsilon greedy)
    if (((float)rand()/RAND_MAX) < _epsilon) // P(exploit) = _epsilon
      action = greedy_action;
    else // explore
      action = rand()%_action_size;

    return action;
  }

  /*------------- Update rules ----------*/
  
  //void update_etrace()
  //{
    //for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
    //for (int state_idx=0; state_idx<_state_size; state_idx++)
    //	for (int action_idx=0; action_idx<_action_size; action_idx++)
    //	  _etrace[reward_idx][state_idx][action_idx] *= (_lambda*_gamma[reward_idx]);
    // TODO
    //for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      //_etrace[reward_idx][_state][_action] = 1.0;
  //}

  //void update_w()
  //{
    /*
    // TODO: add eligibility trace
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
    {
      // releasing dopamine
      // delta_t = r_{t+1} + gamma*q(phi(s_{t+1}),a_{t+1}) - q(phi(s_t),a_t);
      _rpe[reward_idx] = _reward[reward_idx]
	+ _gamma[reward_idx] * get_qvalue(_num_phi_prime, _phi_prime, _action_prime)
      	- get_qvalue(_num_phi, _phi, _action);
    }

    //printf("[delta=%f]",_rpe[0]);
    
    // cortico-striatal learning
    // _w(phi(s_t,a_t)) += alpha*delta*phi(s_t,a_t)
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      for(int idx=0; idx<_num_phi && _phi[idx]<_state_size; idx++)
	_w[reward_idx][_phi[idx]][_action] += _alpha*_rpe[reward_idx]; //_etrace[reward_idx][state_idx][action_idx];    

    if(false) //_VERBOSE_UDP)
    {
      printf("\n{T:%d eps:%0.1f LOG:%d ",_time,_epsilon,_is_test_trial);
      printf("S:(");
      for(int idx=0; idx<_num_phi; idx++)
	printf("%d,",_phi[idx]);
      printf("),A:%d -%0.1f-> ",_action,_reward[0]);
    
      printf("S':(");
      for(int idx=0; idx<_num_phi_prime; idx++)
	printf("%d,",_phi_prime[idx]);
      printf("),A':%d}",_action_prime);
    }
    */
  //}

  void update_w_pc()
  {
    // TODO: add eligibility trace
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
    {
      float max_qvalue_next=0.0;
      float* qvalue_next = new float[_action_size];
      
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	qvalue_next[action_idx] = get_q_pc_value(_num_phi_pc_prime, _phi_pc_prime, action_idx);
      
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	if(qvalue_next[action_idx]>max_qvalue_next)
	  max_qvalue_next = qvalue_next[action_idx]; 
      
      // releasing dopamine
      // delta_t = r_{t+1} + gamma*q(phi(s_{t+1}),a_{t+1}) - q(phi(s_t),a_t);
      // On-policy
      //_rpe[reward_idx] = _reward[reward_idx]
      //	+ _gamma[reward_idx] * get_q_pc_value(_num_phi_pc_prime, _phi_pc_prime, _action_prime)
      //- get_q_pc_value(_num_phi_pc, _phi_pc, _action);

      // Off-policy
      _rpe[reward_idx] = _reward[reward_idx]
      	+ _gamma[reward_idx] * max_qvalue_next
	- get_q_pc_value(_num_phi_pc, _phi_pc, _action);
    }

    //printf("[delta=%f]",_rpe[0]);
    
    // cortico-striatal learning
    // _w(phi(s_t,a_t)) += alpha*delta*phi(s_t,a_t)
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      for(int state_pc_idx=0; state_pc_idx<_state_pc_size; state_pc_idx++)
      {
	_w[reward_idx][state_pc_idx][_action] += _alpha*_rpe[reward_idx]*_phi_pc[state_pc_idx];
	//_etrace[reward_idx][state_idx][action_idx];
      }

    if(false) //_VERBOSE_UDP)
    {
      printf("\n{T:%d eps:%0.1f LOG:%d ",_time,_epsilon,_is_test_trial);
      printf("S:(");
      for(int idx=0; idx<_num_phi_pc; idx++)
	printf("%0.2f,",_phi_pc[idx]);
      printf("),A:%d -%0.1f-> ",_action,_reward[0]);
    
      printf("S':(");
      for(int idx=0; idx<_num_phi_pc_prime; idx++)
	printf("%0.2f,",_phi_pc_prime[idx]);
      printf("),A':%d}",_action_prime);
    }    
  }

  /*float get_q_value(int num_phi, int* phi, int action)
  {
    float _qvalue=0.0;

    // q_cap(s,a) = sum_i w_i*phi_i(s,a)
    for(int idx=0; idx<num_phi && phi[idx]<_state_size; idx++)
      _qvalue += _w[0][phi[idx]][action];

    return _qvalue;
    }*/

  float get_q_pc_value(int num_phi_pc, vector< float > phi_pc, int action)
  {
    float _qvalue=0.0;

    // q_cap(s,a) = sum_i w_i*phi_i(s)
    for(int phi_idx=0; phi_idx<num_phi_pc; phi_idx++)
      _qvalue += _w[0][phi_idx][action]*phi_pc[phi_idx];

    return _qvalue;
  }

  void advance_timestep(int num_phi_pc_prime, float* phi_pc_prime, int action_prime, int reward, int time)    
  {
    if(num_phi_pc_prime != _state_pc_size)
      printf("\n Bad UDPs!!");

    _num_phi_pc_prime = num_phi_pc_prime;

    for(int idx=0; idx<num_phi_pc_prime; idx++)
      _phi_pc_prime[idx] = phi_pc_prime[idx];
    
    _action_prime = action_prime;
    
    _reward[0] = reward;
    _time = time;

    if(_time%1000==0)
      qvalue_log();

    if(_num_phi_pc<0)
      return;
   
    //update_etrace();
    update_w_pc();

    reward_log();
    state_log();
    action_log();
  }  
};
