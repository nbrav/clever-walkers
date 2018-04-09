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
  float _omega;                // Boltzmann constant + overloaded preference
  double _rho;                 // Importance-Sampling factor
  string _tag="def";
  
  int _state_size;
  
  private:

  // global parameters
  int _time;
  int _rank;

  // state
  int *_phi_idx, *_phi_prime_idx;
  float *_phi_val, *_phi_prime_val;
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

  // actor traces/weights
  std::vector< std::vector< std::vector<float> > > _etrace;
  std::vector< std::vector< std::vector<float> > > _actor;

  // critic traces/weights
  std::vector< std::vector<float> > _critic_e;
  std::vector< std::vector<float> > _critic_z;

  // test/train trail variables
  bool _is_test_trial=false;
  
  // i/o files
  FILE *qvalue_outfile, *qvalue_infile;   //TODO: do we need two files for reading and writing?
  FILE *critic_outfile, *critic_infile;  
  FILE *state_outfile;
  FILE *action_outfile;
  FILE *reward_outfile;
  FILE *trial_outfile;
  char const *QVALUE_FILE, *CRITIC_FILE, *REWARD_FILE, *STATE_FILE, *ACTION_FILE, *TRIAL_FILE;
  
 public:

  bool VERBOSE=false;
  
  qbrain(int rank, string tag, int state_size=100)
  {
    _tag = tag;
    _rank = rank;
    parse_param();
    _state_size = state_size; // override default state-size

    _state = new int[_state_size];
    
    _num_phi=0; _phi_idx = new int[_state_size]; _phi_val = new float[_state_size];    
    _num_phi_prime=0; _phi_prime_idx = new int [_state_size]; _phi_prime_val = new float[_state_size];

    for(int idx=0;idx<_state_size; idx++)
    {
      _state[idx]=0;
      _phi_idx[idx]=0; _phi_val[idx] = 0.0f;
      _phi_prime_idx[idx]=0; _phi_prime_val[idx]=0.0f;
    }
    
    std::ifstream fs(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str());
    if(fs.is_open())
    {
      // Read Q matrix from file
      qvalue_infile = fopen(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str(),"rb");
      fseek(qvalue_infile, -sizeof(float)*_reward_size*_state_size*_action_size, SEEK_END);

      _actor.resize(_reward_size);
      for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      {
	_actor[reward_idx].resize(_state_size);
	for (int state_idx=0; state_idx<_state_size; state_idx++)
	{
	  _actor[reward_idx][state_idx].resize(_action_size);
	  fread(&_actor[reward_idx][state_idx][0], sizeof(float), _action_size, qvalue_infile);
	}
      }      
      fclose(qvalue_infile);   
    }
    else
    {
      // Initialize Q matrix
      _actor.resize(_reward_size);
      for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      {
	_actor[reward_idx].resize(_state_size);
	for (int state_idx=0; state_idx<_state_size; state_idx++)
	{
	  _actor[reward_idx][state_idx].resize(_action_size,1);
	  for (int action_idx=0; action_idx<_action_size; action_idx++)
	  {
	    _actor[reward_idx][state_idx][action_idx] = 0.0; //(float)rand()/RAND_MAX; //gaussian[action_idx]; //
	  }
	}
      }
    }

    // initialize critic

    _critic_e.resize(_reward_size);
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
    {
      _critic_e[reward_idx].resize(_state_size);
      for (int state_idx=0; state_idx<_state_size; state_idx++)
      {
	_critic_e[reward_idx][state_idx] = 0.0;
      }
    }
    
    std::ifstream fs_critic_z(("./data/critic."+_tag+"."+std::to_string(_rank)+".log").c_str());
    if(fs_critic_z.is_open())
    {
      critic_infile = fopen(("./data/critic."+_tag+"."+std::to_string(_rank)+".log").c_str(),"rb");
      fseek(critic_infile, -sizeof(float)*_reward_size*_state_size, SEEK_END);

      _critic_z.resize(_reward_size);
      for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      {
	_critic_z[reward_idx].resize(_state_size);
	fread(&_critic_z[reward_idx][0], sizeof(float), _state_size, critic_infile);
      }      
      fclose(critic_infile);   
    }
    else
    {
      _critic_z.resize(_reward_size);
      for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      {
	_critic_z[reward_idx].resize(_state_size);
	for (int state_idx=0; state_idx<_state_size; state_idx++)
	{
	  _critic_z[reward_idx][state_idx] = 0.0;
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
    //_state_size = 100; // 24*4*10; //TODO: Must use a param file     
    _action_size = 8*3;  // Must use a param file     
    _reward_size = 1; // Must use a param file     
    
    _alpha = 0.01; // learning rate
    _rho = 1.0;  // importance-sampling 
    _lambda = 0.0; // eligibility parameter 0.8

    _epsilon = 0.8; // epsilon-greedy
    _omega = 0.0; // softmax-temp
    
    _gamma.resize(_reward_size);
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      _gamma[reward_idx] = 0.9; // temporal-decay rate
  }

  void reset()
  {
    _num_phi = -1;
    
    for(int idx=0;idx<_state_size; idx++)
      _state[idx]=0;

    _action = rand()%_action_size;

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
      	fwrite(&_actor[reward_idx][state_idx][0], sizeof(float) , _action_size, qvalue_outfile);
    fclose(qvalue_outfile);
  }

  void critic_log()
  {
    critic_outfile = fopen(("./data/critic."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      fwrite(&_critic_z[reward_idx][0], sizeof(float) , _state_size, critic_outfile);
    fclose(critic_outfile);
  }

  /* ---------- I/O ----------------*/

  void set_state(int num_phi, int* phi_idx, float* phi_val)
  {
    _num_phi = num_phi;

    for(int idx=0; idx<num_phi; idx++)
    {
      _phi_idx[idx] = phi_idx[idx];
      _phi_val[idx] = phi_val[idx];
    }
    
    for(int idx=0;idx<_state_size; idx++)
      _state[idx]=0;
    for(int idx=0; idx<_num_phi && phi_idx[idx]<_state_size; idx++)
      _state[phi_idx[idx]]=phi_val[idx];
  }

  void set_action(int action)
  {
    _action = action;
  }

  double* get_policy(int num_phi, int* phi_idx, float* phi_val)
  {
    float* _qvalue = new float[_action_size];
    float _qmax = std::numeric_limits<float>::min();
    double* _policy = new double[_action_size]; double _policy_sum = 0.0;

    for(int idx=0; idx<num_phi; idx++)
    {
      if(_phi_val[idx]!=_phi_val[idx])
	cerr<<"\n===NANs in PHI==="<<_tag<<" "<<idx<<"&"<<num_phi<<","<<_phi_val[idx]<<",";
    }

    // forall a: q_cap(s_t,A) = 0
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      _qvalue[action_idx] = 0.0;    

    // forall a: q_cap(s_t,A) = sum_i w_i*phi_i(s_t,A)
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      for(int idx=0; idx<num_phi && phi_idx[idx]<_state_size; idx++)
      {
	if(_phi_val[idx]!=_phi_val[idx]) {cout<<"\nNANs![Agent:"<<_rank<<"]";continue;}
	
	_qvalue[action_idx] += _actor[0][phi_idx[idx]][action_idx]*_phi_val[idx];
	_qmax = (_qvalue[action_idx]>_qmax)?_qvalue[action_idx]:_qmax;

	if(_qvalue[action_idx]!=_qvalue[action_idx])cerr<<"\nNANs in Q_"<<_tag<<"!"<<" W["<<phi_idx[idx]<<","<<action_idx<<"]="<<_actor[0][phi_idx[idx]][action_idx]<<" PHI="<<_phi_val[idx];
      }

    for(int action_idx=0; action_idx<_action_size; action_idx++)
    {
      _policy[action_idx] = exp((_qvalue[action_idx]-_qmax)/_omega);
      _policy_sum += _policy[action_idx];

      if(_policy[action_idx]!=_policy[action_idx]) cerr<<"\n===NANs in PI_i!==="<<_tag;
    }

    if(_policy_sum==0)
    {
      cerr<<"Warning! Zero-sum policy in "<<_tag<<"! \nPHI:[";

      for(int action_idx=0; action_idx<_action_size; action_idx++)
	for(int idx=0; idx<num_phi && phi_idx[idx]<_state_size; idx++)
	  cout<<idx<<","<<phi_idx[idx]<<","<<_actor[0][phi_idx[idx]][action_idx]<<","<<_phi_val[idx]<<" ";
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	cerr<<_qvalue[action_idx]<<",";
      cerr<<"]\nPI:[";
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	cerr<<_policy[action_idx]<<",";
      cerr<<"]";
      return _policy;
    }
    
    if(VERBOSE && _tag=="goal") cout<<"\n"<<_tag<<" ";
    for(int action_idx=0; action_idx<_action_size; action_idx++)
    {
      _policy[action_idx] /= _policy_sum;
      if(VERBOSE && _tag=="goal") cout<<" "<<_policy[action_idx]<<",";
    }

    delete[] _qvalue;
    return _policy;
  }

  double* get_locked_policy(int num_phi, int* phi_idx, float* phi_val)
  {
    double* _policy = new double[_action_size]; double _policy_sum = 0.0;
    double sigma=2.5;

    for(int idx=0; idx<num_phi; idx++)
    {
      if(_phi_val[idx]!=_phi_val[idx]) cerr<<"\n===NANs in PHI==="<<_tag<<" "<<idx<<"&"<<num_phi<<","<<_phi_val[idx]<<",";
    }

    for(int action_idx=0; action_idx<_action_size; action_idx++)
    {
      if(_rank%2)
	_policy[action_idx] = exp(-(action_idx%8-0)*(action_idx%8-0)*2.0/sigma/sigma);
      else
	_policy[action_idx] = exp(-(action_idx%8-4)*(action_idx%8-4)*2.0/sigma/sigma);
      
      _policy_sum += _policy[action_idx];

      if(_policy[action_idx]!=_policy[action_idx]) cerr<<"\n===NANs in PI_i!==="<<_tag;
    }

    if(_policy_sum==0)
    {
      cerr<<"Warning! Zero-sum policy in "<<_tag<<"!";
      return _policy;
    }
    
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      _policy[action_idx] /= _policy_sum;

    return _policy;
  }

  float get_rpe()
  {
    return _reward[0]
      + _gamma[0] * get_value(_num_phi_prime, _phi_prime_idx, _phi_prime_val)
      - get_value(_num_phi, _phi_idx, _phi_val);
  }
  
  /*------------- Update rules ----------*/

  void update_importance_samples(double* policy_our, double* policy_behaviour, int action_behaviour)
  {
    _rho = policy_our[action_behaviour]/policy_behaviour[action_behaviour];
    _rho = min(_rho,1.0);    
  }
  
  void update_omega(double prob_behave, double prob_policy)
  {    
  }
  
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

  void update_actor()
  {
    // TODO: add eligibility trace

    // releasing dopamine
    // delta_t = r_{t+1} + gamma*q(phi(s_{t+1}),a_{t+1}) - q(phi(s_t),a_t);
    
    // delta = r + \gamma * v(s') - v(s)
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
    {
      _rpe[reward_idx] = get_rpe();
    }      
    
    // cortico-striatal learning
    // _actor(phi(s_t,a_t)) += alpha*delta*phi(s_t,a_t)
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
	_actor[reward_idx][_phi_idx[idx]][_action]+=_alpha*_rho*_rpe[reward_idx]*_phi_val[idx];//_etrace[reward_idx][state_idx][action_idx];
    
    if(VERBOSE && !_rank && _tag=="goal")
    {
      printf("\n{AGIDX:%d T:%d eps:%0.1f LOG:%d ",_rank,_time,_epsilon,_is_test_trial);
      printf("S:(");
      for(int idx=0; idx<_num_phi; idx++)
	printf("[%d,%0.1f],",_phi_idx[idx],_phi_val[idx]);
      printf("),A:%d -{%0.1f,%0.3f}-> ",_action,_reward[0],_rpe[0]);
    
      printf("S':(");
      for(int idx=0; idx<_num_phi_prime; idx++)
	printf("[%d,%0.1f],",_phi_prime_idx[idx],_phi_prime_val[idx]);
      printf("),A':%d}",_action_prime);
    }
    
    fflush(stdout);    
  }

  void update_critic()
  {
    
    
    double delta = _reward[0] + _gamma[0]*get_value(_num_phi_prime, _phi_prime_idx, _phi_prime_val)
      - get_value(_num_phi, _phi_idx, _phi_val);
    
    for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
      _critic_z[0][_phi_idx[idx]] += _alpha*_rho*delta*_phi_val[idx];
  }
  
  float get_qvalue(int num_phi, int* phi_idx, float* phi_val, int action)
  {
    float _qvalue=0.0;

    // q_cap(s,a) = sum_i w_i*phi_i(s,a)
    for(int idx=0; idx<num_phi; idx++)
      _qvalue += _actor[0][phi_idx[idx]][action] * phi_val[idx];
    
    return _qvalue;
  }

  float get_value(int num_phi, int* phi_idx, float* phi_val)
  {
    float _value=0.0;

    // q_cap(s,a) = sum_i w_i*phi_i(s,a)
    for(int idx=0; idx<num_phi; idx++)
      _value += _critic_z[0][phi_idx[idx]]*phi_val[idx];
    
    return _value;
  }

  void advance_timestep(int num_phi_prime, int* phi_prime_idx, float* phi_prime_val, int action_prime, float reward, int time)    
  {
    _num_phi_prime = num_phi_prime;
    _phi_prime_idx = phi_prime_idx;
    _phi_prime_val = phi_prime_val;
    _action_prime = action_prime;

    _reward[0] = reward;
    _time = time;

    if(_time%1000==0)
    {
      qvalue_log();
      critic_log();
    }
    
    if(_num_phi<0)
      return;
   
    //update_etrace();
    update_actor();
    update_critic();
  }  
};
