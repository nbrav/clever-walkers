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
  float _gamma;                // Discount factor for Q
  float _epsilon;              // Greedy arbitration exploration
  float _lambda;               // Eligibility decay
  float _omega;                // Boltzmann constant + overloaded preference
  double _rho;                 // Importance-Sampling factor
  string _tag="def";
    
  private:

  // global parameters
  int _time;
  int _rank;

  // state
  int _state_size;

  int* _phi; // full state
  int *_phi_idx, *_phi_prime_idx; // compressed phi indices
  float *_phi_val, *_phi_prime_val; // compressed phi values
  int _num_phi, _num_phi_prime; // compressed phi size

  // reward
  float _reward; double delta;    

  // action
  int _action, _action_prime, _action_size;
  
  // actor traces/weights
  std::vector< std::vector<float> > _actor_e;
  std::vector< std::vector<float> > _actor_w;

  // critic traces/weights
  std::vector<float> _critic_e;
  std::vector<float> _critic_w;

  // convergence checker
  std::vector< std::vector<float> > _retro_actor_w;
  std::vector<float> _retro_critic_w;  

  // i/o files
  FILE *qvalue_outfile, *qvalue_infile;   //TODO: do we need two files for reading and writing?
  FILE *critic_outfile, *critic_infile;  
  FILE *state_outfile, *action_outfile, *reward_outfile, *trial_outfile;
  char const *QVALUE_FILE, *CRITIC_FILE, *REWARD_FILE, *STATE_FILE, *ACTION_FILE, *TRIAL_FILE;
  
 public:

  bool VERBOSE=false;
  
  qbrain(int rank, string tag, int state_size=100)
  {
    _tag = tag;
    _rank = rank;
    parse_param();
    _state_size = state_size; // override default state-size

    _phi = new int[_state_size];
    
    _num_phi=0; _phi_idx = new int[_state_size]; _phi_val = new float[_state_size];    
    _num_phi_prime=0; _phi_prime_idx = new int [_state_size]; _phi_prime_val = new float[_state_size];

    // initialize state
    for(int idx=0;idx<_state_size; idx++)
    {
      _phi[idx]=0;
      _phi_idx[idx]=0; _phi_val[idx] = 0.0f;
      _phi_prime_idx[idx]=0; _phi_prime_val[idx]=0.0f;
    }

    // initialize actor
    _actor_e.resize(_state_size);
    for (int state_idx=0; state_idx<_state_size; state_idx++)
    {
      _actor_e[state_idx].resize(_action_size,1);
      for (int action_idx=0; action_idx<_action_size; action_idx++)
      {
	_actor_e[state_idx][action_idx] = 0.0;
      }
    }

    std::ifstream fs(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str());
    if(fs.is_open())
    {
      qvalue_infile = fopen(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str(),"rb");
      fseek(qvalue_infile, -sizeof(float)*_state_size*_action_size, SEEK_END);

      _actor_w.resize(_state_size);
      for (int state_idx=0; state_idx<_state_size; state_idx++)
      {
	_actor_w[state_idx].resize(_action_size);
	fread(&_actor_w[state_idx][0], sizeof(float), _action_size, qvalue_infile);
      }
      fclose(qvalue_infile);   
    }
    else
    {
      _actor_w.resize(_state_size);
      for (int state_idx=0; state_idx<_state_size; state_idx++)
      {
	_actor_w[state_idx].resize(_action_size,1);
	for (int action_idx=0; action_idx<_action_size; action_idx++)
	{
	  _actor_w[state_idx][action_idx] = 0.0; //(float)rand()/RAND_MAX; //gaussian[action_idx]; //
	}
      }
    }
    
    // initialize critic
    _critic_e.resize(_state_size);
    for (int state_idx=0; state_idx<_state_size; state_idx++)
      _critic_e[state_idx] = 0.0;
    
    std::ifstream fs_critic_w(("./data/critic."+_tag+"."+std::to_string(_rank)+".log").c_str());
    if(fs_critic_w.is_open())
    {
      critic_infile = fopen(("./data/critic."+_tag+"."+std::to_string(_rank)+".log").c_str(),"rb");
      fseek(critic_infile, -sizeof(float)*_state_size, SEEK_END);

      _critic_w.resize(_state_size);
      fread(&_critic_w[0], sizeof(float), _state_size, critic_infile);
      fclose(critic_infile);   
    }
    else
    {
      _critic_w.resize(_state_size);
      for (int state_idx=0; state_idx<_state_size; state_idx++)
	_critic_w[state_idx] = 0.0;
    }
    
    _reward = 0.0;

    _retro_actor_w = _actor_w;
    _retro_critic_w = _critic_w;

    qvalue_outfile = fopen(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    fclose(qvalue_outfile);

    trial_outfile = fopen(("./data/trial."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    fclose(trial_outfile);
  }

  void parse_param()
  {
    _action_size = 8*3;  // Must use a param file     
    
    _alpha   = 0.01; // learning rate
    _rho     = 1.0; // importance-sampling 
    _lambda  = 0.6; // eligibility parameter 0.8

    _epsilon = 0.8; // epsilon-greedy
    _omega   = 0.1; // softmax-temp
    
    _gamma   = 0.95; // temporal-decay rate
  }

  void reset()
  {
    _num_phi = -1;
    
    for(int idx=0;idx<_state_size; idx++)
      _phi[idx]=0;

    _action = rand()%_action_size;

    _reward = 0.0;

    _rho = 1.0;

    // reset actor_e
    for (int state_idx=0; state_idx<_state_size; state_idx++)
      for (int action_idx=0; action_idx<_action_size; action_idx++)
	_actor_e[state_idx][action_idx] = 0.0;

    // reset critic_e
    for (int state_idx=0; state_idx<_state_size; state_idx++)
      _critic_e[state_idx] = 0.0;
  }

  /*---------- Logger -----------*/
  
  void trial_log(float trial_reward)
  {    
    trial_outfile = fopen(("./data/trial."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab"); //ab
    fwrite(&trial_reward, sizeof(float), 1, trial_outfile);
    fclose(trial_outfile);    
  }

  void state_log()
  {
    state_outfile = fopen(("./data/state."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab"); //ab
    fwrite(&_phi[0], sizeof(int)*_state_size, 1, state_outfile);
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
    fwrite(&_reward, sizeof(float) , 1, reward_outfile);
    fclose(reward_outfile);    
  }

  void actor_log()
  {    
    qvalue_outfile = fopen(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    for (int state_idx=0; state_idx<_state_size; state_idx++)
      fwrite(&_actor_w[state_idx][0], sizeof(float), _action_size, qvalue_outfile);
    fclose(qvalue_outfile);
  }

  void critic_log()
  {
    critic_outfile = fopen(("./data/critic."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    fwrite(&_critic_w[0], sizeof(float), _state_size, critic_outfile);
    fclose(critic_outfile);
  }

  void check_convergence()
  {
    double mean_actor_update = 0.0, mean_critic_update = 0.0;    
    for(int state_idx=0; state_idx<_state_size; state_idx++)
    {
      mean_critic_update += abs(_critic_w[state_idx]-_retro_critic_w[state_idx]);
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	mean_actor_update += abs(_actor_w[state_idx][action_idx]-_retro_actor_w[state_idx][action_idx]);
    }
  }

  /* ---------- I/O ----------------*/

  void set_state(int num_phi, int* phi_idx, float* phi_val)
  {
    _num_phi = num_phi;

    float phi_sum=0;
    
    for(int idx=0; idx<num_phi; idx++)
    {
      _phi_idx[idx] = phi_idx[idx];
      _phi_val[idx] = phi_val[idx];
      phi_sum += _phi_val[idx];
    }
    
    for(int idx=0;idx<_state_size; idx++)
      _phi[idx]=0;

    for(int idx=0; idx<num_phi && phi_idx[idx]<_state_size; idx++)
    {
      _phi[phi_idx[idx]]=phi_val[idx]/phi_sum;

      if(_phi_idx[idx]!=_phi_idx[idx]) cerr<<"\n===NANs in PHI_IDX!==="<<_tag;    
      if(_phi_val[idx]!=_phi_val[idx]) cerr<<"\n===NANs in PHI_VAL!==="<<_tag;
    }
  }

  void set_action(int action)
  {
    _action = action;
  }

  double* get_policy(int num_phi, int* phi_idx, float* phi_val)
  {
    float* h = new float[_action_size];  float hmax = std::numeric_limits<float>::min();
    double* policy = new double[_action_size]; double policy_sum = 0.0;

    for(int idx=0; idx<num_phi; idx++)
      if(_phi_val[idx]!=_phi_val[idx])
	cerr<<"\n===NANs in PHI===";

    // h(A)=0
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      h[action_idx] = 0.0;    

    // forall a: h(s_t,A) = sum_i w_i*phi_i(s_t,A)
    for(int idx=0; idx<num_phi && phi_idx[idx]<_state_size; idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
      {
	h[action_idx] += _actor_w[phi_idx[idx]][action_idx] * _phi_val[idx];
	hmax = (h[action_idx]>hmax)?h[action_idx]:hmax;

	if(h[action_idx]!=h[action_idx]) cerr<<"\nNANs in Q_"<<_tag;
      }
    
    for(int action_idx=0; action_idx<_action_size; action_idx++)
    {
      policy[action_idx] = exp((h[action_idx]-hmax)); // ACTOR-CRITIC?? /_omega
      policy_sum += policy[action_idx];

      if(policy[action_idx]!=policy[action_idx]) cerr<<"\n===NANs in PI_i!==="<<_tag;
    }

    if(policy_sum==0)
      policy_sum=1.0;

    for(int action_idx=0; action_idx<_action_size; action_idx++)
      policy[action_idx] /= policy_sum;

    delete[] h;
    return policy;
  }

  double* get_locked_policy(int num_phi, int* phi_idx, float* phi_val)
  {
    double* _policy = new double[_action_size]; double _policy_sum = 0.0;
    double sigma=2.5;

    for(int idx=0; idx<num_phi; idx++)
      if(_phi_val[idx]!=_phi_val[idx])
	cerr<<"\n===NANs in PHI===";

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
    return _reward
      + get_value(_num_phi_prime, _phi_prime_idx, _phi_prime_val)*_gamma
      - get_value(_num_phi, _phi_idx, _phi_val);
  }
  
  /*------------- Update rules ----------*/

  void update_importance_samples(double* policy_our, double* policy_behaviour, int a_t)
  {
    float _rho_t = 0;
    
    if(a_t<0 || a_t>=_action_size)
      cerr<<"Invalided action for importance sampling!";
    
    if(!policy_behaviour[a_t]==0.0)
      _rho_t = policy_our[a_t]/policy_behaviour[a_t];
    
    _rho = _rho_t<1?_rho_t:1.0;
    //if(!_rank) cout<<"\nIS"<<_tag<<"="<<_rho;
  }

  double grad_entropy(double* policy)
  {
    double grad_H = 0.0;
    for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	grad_H += _phi_val[idx]*(1.0/_action_size-policy[action_idx]);
    return grad_H;
  }
  
  void update_actor(double delta)
  {
    double* policy = get_policy(_num_phi,_phi_idx,_phi_val);

    // e-trace decay
    for(int state_idx=0; state_idx<_state_size; state_idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	_actor_e[state_idx][action_idx] *= _lambda;

    // efference-update of e-trace
    for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	_actor_e[_phi_idx[idx]][action_idx] += _gamma * _phi_val[idx] * ((action_idx==_action)-policy[action_idx]);      

    // weight consolitation
    for(int state_idx=0; state_idx<_state_size; state_idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	_actor_w[state_idx][action_idx] += _alpha * (delta*_actor_e[state_idx][action_idx]*_rho); //+ grad_entropy(policy)

    delete[] policy;   
  }

  void update_critic(double delta)
  {
    for(int state_idx=0; state_idx<_state_size; state_idx++)
      _critic_e[state_idx] = _lambda*_critic_e[state_idx];

    for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
      _critic_e[_phi_idx[idx]] += _gamma * _phi_val[idx];
    
    for (int state_idx=0; state_idx<_state_size; state_idx++)
      _critic_w[state_idx] += _alpha * delta * _critic_e[state_idx] * _rho;
  }

  float get_value(int num_phi, int* phi_idx, float* phi_val)
  {
    float _value=0.0;

    // q_cap(s,a) = sum_i w_i*phi_i(s,a)
    for(int idx=0; idx<num_phi; idx++)
      _value += _critic_w[phi_idx[idx]]*phi_val[idx];
    
    return _value;
  }

  void advance_timestep(int num_phi_prime, int* phi_prime_idx, float* phi_prime_val, int action_prime, float reward, int time)    
  {
    _num_phi_prime = num_phi_prime;
    _phi_prime_idx = phi_prime_idx;
    _phi_prime_val = phi_prime_val;
    _action_prime = action_prime;

    _reward = reward;
    _time = time;
    _num_phi_prime = num_phi_prime; _phi_prime_idx = phi_prime_idx;_phi_prime_val = phi_prime_val;
    _action_prime = action_prime;
    _reward = reward;

    if(_time%1000==0)
    {
      actor_log();
      critic_log();
      if(!_rank) check_convergence();
    }
    
    if(_num_phi<0)
      return;
    
    delta = get_rpe();
    update_actor(delta);
    update_critic(delta);
  }  
};
