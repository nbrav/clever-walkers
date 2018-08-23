#include <iostream>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <time.h>
#include <algorithm>
#include <math.h>
#include <string>
#include <chrono>

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

  unsigned seed;

  // state
  int _state_size;

  int* _phi_s; // full state
  int *_phi_s_idx, *_phi_s_prime_idx; // compressed phi_s indices
  float *_phi_s_val, *_phi_s_prime_val; // compressed phi_s values
  int _num_phi_s, _num_phi_s_prime; // compressed phi_s size

  // reward
  float _reward; double delta;    

  // action
  int _action_size, _direction_size, _speed_size;

  float *_action_code_speed, *_action_code_direction;
  float *_action, *_action_prime;

  float _action_t = 0.0;
  float _actor_mean = 0.0, _actor_std = 1.0;
    
  // actor traces/weights
  std::vector< std::vector<float> > _actor_e;
  std::vector< std::vector<float> > _actor_w; // {theta_mean, theta_std}

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
  
  qbrain(int rank, string tag, int state_size=100, int action_size=36*5, int direction_size=36, int speed_size=5)
  {
    _tag = tag;
    _rank = rank;
    parse_param();
    
    // initialize state-space
    _state_size = state_size;
 
    _phi_s = new int[_state_size];
    _num_phi_s=0; _phi_s_idx = new int[_state_size]; _phi_s_val = new float[_state_size];    
    _num_phi_s_prime=0; _phi_s_prime_idx = new int [_state_size]; _phi_s_prime_val = new float[_state_size];

    for(int idx=0;idx<_state_size; idx++)
    {
      _phi_s[idx]=0;
      _phi_s_idx[idx]=0; _phi_s_val[idx] = 0.0f;
      _phi_s_prime_idx[idx]=0; _phi_s_prime_val[idx]=0.0f;
    }

    // initialize action-space
    _action_size = action_size;
    _action = new float[_action_size];
    for (int action_idx=0; action_idx<_action_size; action_idx++)
      _action[action_idx] = 0.0;

    // population coding in action-space
    _direction_size = direction_size;
    _speed_size = speed_size;
    
    _action_code_direction = new float[direction_size];
    _action_code_speed = new float[speed_size];
    for (int direction_idx=0; direction_idx<direction_size; direction_idx++)
      _action_code_direction[direction_idx] = 2.0*PI*float(direction_idx)/float(direction_size);
    
    //for (int speed_idx=0; speed_idx<speed_size; speed_idx++)
    //  _action_code_speed[speed_idx] = float(speed_idx)/float(speed_size);
    _action_code_speed[0] = 1.0f; //0.5f;
    
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
	  _actor_w[state_idx][action_idx] = (float)rand()/RAND_MAX; //((float)rand()/RAND_MAX-0.5)/5.0;
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
	_critic_w[state_idx] = ((float)rand()/RAND_MAX-0.5)/_state_size;
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
    _alpha   = 0.05;  // learning rate
    _rho     = 1.00;  // importance-sampling 
    _lambda  = 0.50;  // eligibility parameter
    _epsilon = 0.80;  // epsilon-greedy    
    _gamma   = 0.80;  // temporal-decay rate
  }

  void reset()
  {
    _num_phi_s = -1;
    
    for(int idx=0;idx<_state_size; idx++)
      _phi_s[idx]=0;

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
    fwrite(&_phi_s[0], sizeof(int)*_state_size, 1, state_outfile);
    fclose(state_outfile);    
  }

  void action_log()
  {
    action_outfile = fopen(("./data/action."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab"); //ab
    fwrite(&_action[0], sizeof(float), _action_size, action_outfile);
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
    /*double mean_actor_update = 0.0, mean_critic_update = 0.0;    
    for(int state_idx=0; state_idx<_state_size; state_idx++)
    {
      mean_critic_update += abs(_critic_w[state_idx]-_retro_critic_w[state_idx]);
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	mean_actor_update += abs(_actor_w[state_idx][action_idx]-_retro_actor_w[state_idx][action_idx]);
    }*/
  }

  /* ---------- I/O ----------------*/

  void set_state(int num_phi_s, int* phi_s_idx, float* phi_s_val)
  {
    _num_phi_s = num_phi_s;

    float phi_s_sum=0;
    
    for(int idx=0; idx<num_phi_s; idx++)
    {
      _phi_s_idx[idx] = phi_s_idx[idx];
      _phi_s_val[idx] = phi_s_val[idx];
      phi_s_sum += _phi_s_val[idx];
    }
    
    for(int idx=0;idx<_state_size; idx++)
      _phi_s[idx]=0;

    for(int idx=0; idx<num_phi_s && phi_s_idx[idx]<_state_size; idx++)
    {
      _phi_s[phi_s_idx[idx]]=phi_s_val[idx]/phi_s_sum;

      if(_phi_s_idx[idx]!=_phi_s_idx[idx]) {cerr<<"\n===NANs in PHI_S_IDX!==="<<_tag; exit(0);  }
      if(_phi_s_val[idx]!=_phi_s_val[idx]) {cerr<<"\n===NANs in PHI_S_VAL!==="<<_tag; exit(0);  }
    }
  }

  void set_action(float* action, float action_t)
  {
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      _action[action_idx] = action[action_idx];

    _action_t = action_t;
  }

  double* get_policy(int num_phi_s, int* phi_s_idx, float* phi_s_val)
  {
    float* h = new float[_action_size];  float hmax = std::numeric_limits<float>::min();
    double* policy = new double[_action_size]; double policy_sum = 0.0;

    // h(A)=0
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      h[action_idx] = 0.0;
    
    // forall a: h(s_t,A) = sum_i w_i*phi_s_i(s_t,A)
    for(int idx=0; idx<num_phi_s && phi_s_idx[idx]<_state_size; idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
      {
	if(isnan(_phi_s_val[idx]) || isinf(_phi_s_val[idx]))
	  cerr<<"\n===NANs in PHI_S===";
	
	h[action_idx] += _actor_w[phi_s_idx[idx]][action_idx] * _phi_s_val[idx];
	hmax = (h[action_idx]>hmax)?h[action_idx]:hmax;      
      }

    for(int action_idx=0; action_idx<_action_size; action_idx++)
    {
      policy[action_idx] = exp((h[action_idx]-hmax));
      policy_sum += policy[action_idx];

      if(isnan(policy[action_idx]) || isinf(policy[action_idx]))
	cerr<<"\n===NANs in PI_i!==="<<_tag;
    }

    if(policy_sum==0)
    {
      if(_num_phi_s != 0) cout<<"ZERO-SUM '"<<_tag<<"'POLICY!";
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	policy[action_idx] = 1.0/float(_action_size);
    }
    else
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	policy[action_idx] /= policy_sum;

    // DEBUGGER //
    if(!_rank && false)
    {
      cout<<"\n\n=== S:";
      for(int idx=0; idx<num_phi_s && phi_s_idx[idx]<_state_size; idx++)
	cout<<"{"<<phi_s_idx[idx]<<","<<phi_s_val[idx]<<"}, ";
      cout<<"\n    A:";
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	cout<< std::fixed << std::setprecision(2) << policy[action_idx]<<",";
      cout<<"";
    }

    delete[] h;
    return policy;
  }

  // -----------------------------------
  // action ~ Gaussian(mean=W1.phi(s),var=W2.phi(s))
  // -----------------------------------
  void get_motor_msg_contGauss(float* motor_msg)
  {
    _actor_mean = 0.0;
    for(int idx=0; idx<_num_phi_s && _phi_s_idx[idx]<_state_size; idx++)
    {
      _actor_mean += _actor_w[_phi_s_idx[idx]] * _phi_s_val[idx];

      if(isinf(_phi_s_idx[idx]) || isnan(_phi_s_idx[idx])) cerr<<"\nBad phi(s)";
      if(isinf(_phi_s_val[idx]) || isnan(_phi_s_val[idx])) cerr<<"\nBad phi(s)";
      if(isinf(_actor_mean) || isnan(_actor_mean)) cerr<<"\nBad mu(s)";
    }
    
    _actor_std = 0.0;
    for(int idx=0; idx<_num_phi_s && _phi_s_idx[idx]<_state_size; idx++)
    {
      _actor_std += _actor_w[_state_size+_phi_s_idx[idx]] * _phi_s_val[idx];
      if(isinf(_actor_std) || isnan(_actor_std)) cerr<<"\nBad std(s)";
    }

    seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::normal_distribution<double> distribution(_actor_mean,_actor_std);
    std::default_random_engine generator(seed);

    motor_msg[0] = fmod(distribution(generator)+PI,PI);
    motor_msg[1] = 1.0;

    if(!_rank)
      cout<<"\nA: ["<<motor_msg[0]<<","<<motor_msg[1]<<"] ~ N("<<_actor_mean<<","<<_actor_std<<")";   
  }

  // soft-max action-selection from policy pi
  void get_motor_msg(double* pi, float* action_t, float* motor_msg)
  {
    float rand_prob = (float)rand()/RAND_MAX, pi_bucket = 0.0;

    for(int action_idx=0; action_idx<_action_size; action_idx++)
      action_t[action_idx] = 0.0;

    for(int action_idx=0; action_idx<_action_size; action_idx++)
    {
      pi_bucket += pi[action_idx];
      
      if(rand_prob <= pi_bucket)
      {
	motor_msg[0] = _action_code_direction[action_idx%_direction_size];
	motor_msg[1] = _action_code_speed[action_idx/_direction_size];
	action_t[action_idx] = 1.0;
	break;
      }
    }
  }

  float get_rpe()
  {
    float delta =  _reward
      + get_value(_num_phi_s_prime, _phi_s_prime_idx, _phi_s_prime_val)*_gamma
      - get_value(_num_phi_s, _phi_s_idx, _phi_s_val);

    if(isinf(delta) || isnan(delta))
    {
      cerr<<"R IS INF/NAN!";
      delta = 0.0;
    }      
    
    return delta;
  }
  
  /*-------experimental update rules ----------*/

  void update_importance_samples(double* policy_our, double* policy_behaviour, float* a_t)
  {
    float _rho_t = 0;
    
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      if(!policy_behaviour[action_idx]==0.0)
	_rho = a_t[action_idx]*policy_our[action_idx]/policy_behaviour[action_idx];
    
    _rho = _rho<1?_rho_t:1.0;
  }

  /*double grad_entropy(double* policy)
  {
    double grad_H = 0.0;

    for(int idx=0; idx<_num_phi_s && _phi_s_idx[idx]<_state_size; idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	grad_H += _phi_s_val[idx]*(1.0/_action_size-policy[action_idx]);

    return grad_H;
   }*/

  /*------------- Update rules ----------*/
  
  /*void update_actor_contGauss(double delta)
  {
    double* policy = get_policy(_num_phi_s,_phi_s_idx,_phi_s_val);
    
    float temp = fmod(_action_t-_actor_mean, PI);

    // weight consolidation
    for(int idx=0; idx<_num_phi_s && _phi_s_idx[idx]<_state_size; idx++)
    {
      if(_actor_std)
      {
	_actor_w[_phi_s_idx[idx]] = _alpha * delta * _phi_s_val[idx] * temp / _actor_std / _actor_std;      
	_actor_w[_state_size + _phi_s_idx[idx]] = _alpha * delta * _phi_s_val[idx] * (temp * temp / _actor_std / _actor_std - 1.0);
      }
      
      if(isinf(_actor_w[_phi_s_idx[idx]]) || isnan(_actor_w[_phi_s_idx[idx]]))
	cerr<<"\nBad w_mean";
      if(isinf(_actor_w[_state_size + _phi_s_idx[idx]]) || isnan(_actor_w[_state_size + _phi_s_idx[idx]]))
	cerr<<"\nBad w_std";
    }

    if(!_rank)
      cout<<"\n a_t="<<_action_t<<" mu="<<_actor_mean<<" std="<<_actor_std;

    delete[] policy;
    }*/

  void update_actor(double delta)
  {
    double* policy = get_policy(_num_phi_s,_phi_s_idx,_phi_s_val);

    // e-trace decay
    for(int state_idx=0; state_idx<_state_size; state_idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	_actor_e[state_idx][action_idx] *= _lambda;

    // efference-update of e-trace
    for(int idx=0; idx<_num_phi_s && _phi_s_idx[idx]<_state_size; idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	_actor_e[_phi_s_idx[idx]][action_idx] += _gamma*_phi_s_val[idx]*(_action[action_idx]-policy[action_idx]);
    
    // weight consolidation
    for(int state_idx=0; state_idx<_state_size; state_idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
      {
	_actor_w[state_idx][action_idx] += _alpha*delta*_actor_e[state_idx][action_idx]*_rho;

	if(isnan(_actor_w[state_idx][action_idx]) && isinf(_actor_w[state_idx][action_idx]))
	  cerr<<"NANs in w";
      }
  
    delete[] policy;
  }

  void update_critic(double delta)
  {
    for(int state_idx=0; state_idx<_state_size; state_idx++)
      _critic_e[state_idx] = _lambda*_critic_e[state_idx];

    for(int idx=0; idx<_num_phi_s && _phi_s_idx[idx]<_state_size; idx++)
      _critic_e[_phi_s_idx[idx]] += _gamma * _phi_s_val[idx];
    
    for (int state_idx=0; state_idx<_state_size; state_idx++)
    {
      _critic_w[state_idx] += _alpha * delta * _critic_e[state_idx] * _rho;

      if(_critic_w[state_idx]!=_critic_w[state_idx]) cerr<<"NANs in v";
    }
  }

  float get_value(int num_phi_s, int* phi_s_idx, float* phi_s_val)
  {
    float _value=0.0;

    // q_cap(s,a) = sum_i w_i*phi_s_i(s,a)
    for(int idx=0; idx<num_phi_s; idx++)
    {
      if(_critic_w[phi_s_idx[idx]]!=_critic_w[phi_s_idx[idx]]) cerr<<"NANs in RPE";

      _value += _critic_w[phi_s_idx[idx]]*phi_s_val[idx];
    }
    
    return _value;
  }

  void advance_timestep(int num_phi_s_prime, int* phi_s_prime_idx, float* phi_s_prime_val, float* action_prime, float reward, int time)    
  {
    _num_phi_s_prime = num_phi_s_prime;
    _phi_s_prime_idx = phi_s_prime_idx;
    _phi_s_prime_val = phi_s_prime_val;
        
    _action_prime = action_prime;

    _reward = reward;
    _time = time;

    action_log();
    if(_time%1000==0)
    {
      actor_log();
      critic_log();
      if(!_rank) check_convergence();
    }
    
    if(_num_phi_s<0)
      return;
    
    delta = get_rpe();
    update_actor(delta);
    update_critic(delta);
  }  
};
