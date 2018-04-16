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
  float _reward, _rpe;    

  // action
  int _action, _action_prime, _action_size;

  // actor traces/weights
  std::vector<float>  _actor_e;
  std::vector<float>  _actor_w;

  // critic traces/weights
  std::vector<float> _critic_e;
  std::vector<float> _critic_w;

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
    _actor_e.resize(_state_size*_action_size);
    for (int idx=0; idx<_state_size*_action_size; idx++)
      _actor_e[idx] = 0.0;
    
    std::ifstream fs(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str());
    if(fs.is_open())
    {
      qvalue_infile = fopen(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str(),"rb");
      fseek(qvalue_infile, -sizeof(float)*_state_size*_action_size, SEEK_END);

      _actor_w.resize(_state_size*_action_size);
      for (int idx=0; idx<_state_size*_action_size; idx++)
	fread(&_actor_w[0], sizeof(float), _state_size*_action_size, qvalue_infile);
      fclose(qvalue_infile);   
    }
    else
    {
      _actor_w.resize(_state_size*_action_size);
      for (int idx=0; idx<_state_size*_action_size; idx++)
	_actor_w[idx] = 0.0; 
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
    _rpe = 1.0;
    
    qvalue_outfile = fopen(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    fclose(qvalue_outfile);

    trial_outfile = fopen(("./data/trial."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    fclose(trial_outfile);
  }

  void parse_param()
  {
    _action_size = 8*3;  // Must use a param file     
    
    _alpha = 0.01;       // learning rate
    _rho = 1.0;          // importance-sampling 
    _lambda = 0.8;       // eligibility parameter 0.8
    _epsilon = 0.8;      // epsilon-greedy
    _omega = 0.0;        // softmax-temp    
    _gamma = 0.9;        // temporal-decay rate
  }

  void reset()
  {
    _num_phi = -1;
    
    for(int idx=0;idx<_state_size; idx++)
      _phi[idx]=0;

    _action = rand()%_action_size;

    _reward = 0.0;

    // reset actor_e
    for (int idx=0; idx<_state_size*_action_size; idx++)
	_actor_e[idx] = 0.0;

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
    fwrite(&_actor_w[0], sizeof(float) , _state_size*_action_size, qvalue_outfile);
    fclose(qvalue_outfile);
  }

  void critic_log()
  {
    critic_outfile = fopen(("./data/critic."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    fwrite(&_critic_w[0], sizeof(float) , _state_size, critic_outfile);
    fclose(critic_outfile);
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

      //cout<<"\n"<<idx<<":"<<phi_idx[idx]<<","<<phi_val[phi_idx[idx]];
    }
  }

  void set_action(int action)
  {
    _action = action;
  }

  double* get_policy(int num_phi, int* phi_idx, float* phi_val)
  {
    float* h = new float[_action_size]; float hmax = std::numeric_limits<float>::min();    
    double* policy = new double[_action_size]; double policy_sum = 0.0;

    // check if phi(x) is nan?
    for(int idx=0; idx<num_phi && phi_idx[idx]<_state_size; idx++)
      if(phi_val[idx]!=phi_val[idx])
	cerr<<"\n===NANs in PHI!==="<<idx<<":"<<phi_idx[idx]<<","<<phi_val[idx];
    
    // forall a: h(s_t,A) = 0
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      h[action_idx] = 0.0;    

    // forall a: h(s_t,A) = sum_i w_i*phi_i(s_t,A)
    for(int idx=0; idx<num_phi && phi_idx[idx]<_state_size; idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
      {
	int sa_idx = action_idx + _phi_idx[idx]*_action_size;
	h[action_idx] += _actor_w[sa_idx]*phi_val[idx];	
      }
    
    for(int action_idx=0; action_idx<_action_size; action_idx++)
    {
      hmax = (h[action_idx]>hmax)?h[action_idx]:hmax;
      if(h[action_idx]!=h[action_idx]) cerr<<"\n===NANs in Q!==="<<_tag;    
    }
    
    for(int action_idx=0; action_idx<_action_size; action_idx++)
    {
      policy[action_idx] = exp((h[action_idx]-hmax)*_omega); //DEBUG for AC /_omega);
      policy_sum += policy[action_idx];

      if(policy[action_idx]!=policy[action_idx]) cerr<<"\n===NANs in PI!==="<<_tag;
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

  float get_value(int num_phi, int* phi_idx, float* phi_val)
  {
    float _value=0.0;

    // q_cap(s,a) = sum_i w_i*phi_i(s,a)
    for(int idx=0; idx<num_phi; idx++)
      _value += _critic_w[phi_idx[idx]]*phi_val[idx];
    
    return _value;
  }

  float get_rpe()
  {
    return _reward
      + _gamma * get_value(_num_phi_prime, _phi_prime_idx, _phi_prime_val)
      - get_value(_num_phi, _phi_idx, _phi_val);
  }
  
  /*------------- Update rules ----------*/

  void update_importance_samples(double* policy_our, double* policy_behaviour, int action_behaviour)
  {
    _rho = policy_our[action_behaviour]/policy_behaviour[action_behaviour];
    _rho = min(_rho,1.0);    
  }
  
  void update_actor()
  {
    // delta = r + \gamma * v(s') - v(s)
    double delta = get_rpe();

    double* policy = get_policy(_num_phi,_phi_idx,_phi_val);    
    double* grad = new double[_state_size*_action_size];

    for(int idx=0; idx<_state_size*_action_size; idx++)
      grad[idx] = 0.0; 
    
    //compute grad_theta ln pi(a_t|s_t;theta)
    for(int state_idx=0; state_idx<_state_size; state_idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
      {
	int idx = action_idx+state_idx*_action_size;
	grad[idx] = _phi[state_idx] * ((action_idx==_action)-policy[action_idx]);      
      }
    
    for(int idx=0; idx<_state_size*_action_size; idx++)
      _actor_e[idx] = _lambda*_actor_e[idx] + grad[idx];
    
    for(int idx=0; idx<_state_size*_action_size; idx++)
      _actor_w[idx] += _alpha * delta * _actor_e[idx]; //* _rho
    
    delete[] policy;
    delete[] grad;
  }

  void update_qvalue()
  {
    // delta = r + \gamma * v(s') - v(s)
    double delta = get_rpe();

    //compute grad_theta ln pi(a_t|s_t;theta)
    for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
    {
      int sa_idx = _action + _phi_idx[idx]*_action_size;
      _actor_w[sa_idx] += _alpha * delta * _phi_val[idx];
    }
           
  }

  void update_critic()
  {
    // delta = r + \gamma * v(s') - v(s)
    double delta = get_rpe();

    for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
      _critic_e[_phi_idx[idx]] = _lambda*_critic_e[_phi_idx[idx]] + _phi_val[idx];
    
    for (int state_idx=0; state_idx<_state_size; state_idx++)
      _critic_w[state_idx] += _alpha * _rho * delta * _critic_e[state_idx];
  }
  
  void advance_timestep(int num_phi_prime, int* phi_prime_idx, float* phi_prime_val, int action_prime, float reward, int time)    
  {
    _time = time;
    _num_phi_prime = num_phi_prime; _phi_prime_idx = phi_prime_idx;_phi_prime_val = phi_prime_val;
    _action_prime = action_prime;
    _reward = reward;

    if(_time%1000==0)
    {
      actor_log();
      critic_log();
    }
    
    if(_num_phi<0)
      return;
    
    //update_actor();
    update_critic();
    update_qvalue();
  }  
};
