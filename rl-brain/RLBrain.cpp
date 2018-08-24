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
#define USE_CONT_ACTION true

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
  int _action_size, _direction_size, _speed_size;

  float *_action_code_speed, *_action_code_direction;
  float *_action, *_action_prime;

  float _action_xt = 0.0,_action_yt = 0.0;
  float _actor_xmean, _actor_ymean, _actor_xstd, _actor_ystd;
    
  // actor traces/weights
#if USE_CONT_ACTION
  std::vector<float> _actor_e;
  std::vector<float> _actor_w; // {theta_mean, theta_std}
#else
  std::vector< std::vector<float> > _actor_e; 
  std::vector< std::vector<float> > _actor_w; // {theta_mean, theta_std}
#endif
  
  // critic traces/weights
  std::vector<float> _critic_e;
  std::vector<float> _critic_w;

  // convergence checker
#if USE_CONT_ACTION
  std::vector<float> _retro_actor_w;
#else
  std::vector< std::vector<float> > _retro_actor_w;  
#endif
  
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
 
    _phi = new int[_state_size];
    _num_phi=0; _phi_idx = new int[_state_size]; _phi_val = new float[_state_size];    
    _num_phi_prime=0; _phi_prime_idx = new int [_state_size]; _phi_prime_val = new float[_state_size];

    for(int idx=0;idx<_state_size; idx++)
    {
      _phi[idx]=0;
      _phi_idx[idx]=0; _phi_val[idx] = 0.0f;
      _phi_prime_idx[idx]=0; _phi_prime_val[idx]=0.0f;
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
#if USE_CONT_ACTION
    _actor_e.resize(_state_size*4);
    for (int state_idx=0; state_idx<_state_size*4; state_idx++)
      _actor_e[state_idx] = 0.0;
    
    std::ifstream fs(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str());
    if(fs.is_open())
    {
      qvalue_infile = fopen(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str(),"rb");
      fseek(qvalue_infile, -sizeof(float)*_state_size*4, SEEK_END);
      
      _actor_w.resize(_state_size*4);
      fread(&_actor_w[0], sizeof(float), _state_size*4, qvalue_infile);
      fclose(qvalue_infile);   
    }
    else
    {
      _actor_w.resize(_state_size*4);
      for (int state_idx=0; state_idx<_state_size; state_idx++)
	_actor_w[state_idx] = 0.0; //(float)rand()/RAND_MAX/_state_size/10.0;
      for (int state_idx=_state_size; state_idx<_state_size*2; state_idx++)
	_actor_w[state_idx] = 0.0; //((float)rand()/RAND_MAX)/_state_size;
      for (int state_idx=_state_size*2; state_idx<_state_size*3; state_idx++)
	_actor_w[state_idx] = ((float)rand()/RAND_MAX-0.5)/10.0;
      for (int state_idx=_state_size*3; state_idx<_state_size*4; state_idx++)
	_actor_w[state_idx] = 0.0; //((float)rand()/RAND_MAX)/_state_size;
    }
#else
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
	    _actor_w[state_idx][action_idx] = ((float)rand()/RAND_MAX-0.5f);
	  }
	}
      }
#endif    
        
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
    _num_phi = -1;
    
    for(int idx=0;idx<_state_size; idx++)
      _phi[idx]=0;

    _reward = 0.0;

    _rho = 1.0;

    // reset actor_e
#if USE_CONT_ACTION
    for (int state_idx=0; state_idx<_state_size*4; state_idx++)
	_actor_e[state_idx] = 0.0;      
#else
    for (int state_idx=0; state_idx<_state_size; state_idx++)
	for (int action_idx=0; action_idx<_action_size; action_idx++)
	  _actor_e[state_idx][action_idx] = 0.0;
#endif
    
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
#if USE_CONT_ACTION
    qvalue_outfile = fopen(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    fwrite(&_actor_w[0], sizeof(float), _state_size*4, qvalue_outfile);
    fclose(qvalue_outfile);
#else    
    qvalue_outfile = fopen(("./data/qvalue."+_tag+"."+std::to_string(_rank)+".log").c_str(), "ab");
    for (int state_idx=0; state_idx<_state_size; state_idx++)
      fwrite(&_actor_w[state_idx][0], sizeof(float), _action_size, qvalue_outfile);
    fclose(qvalue_outfile);
#endif    
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

      if(_phi_idx[idx]!=_phi_idx[idx]) {cerr<<"\n===NANs in PHI_IDX!==="<<_tag; exit(0);  }
      if(_phi_val[idx]!=_phi_val[idx]) {cerr<<"\n===NANs in PHI_VAL!==="<<_tag; exit(0);  }
    }
  }

  /*void set_action(float* action, float action_t)
  {
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      _action[action_idx] = action[action_idx];

    _action_t = action_t;
    }*/

  void set_action_cont(float action_xt, float action_yt)
  {
    /*for(int action_idx=0; action_idx<_action_size; action_idx++)
      _action[action_idx] = 0.0;
      _action[int(fmod(action_t/PI*180+180,_action_size))] = 1.0;*/
    
    _action_xt = action_xt;
    _action_yt = action_yt;
  } 

  /*double* get_policy(int num_phi, int* phi_idx, float* phi_val)
  {
    float* h = new float[_action_size];  float hmax = std::numeric_limits<float>::min();
    double* policy = new double[_action_size]; double policy_sum = 0.0;

    // h(A)=0
    for(int action_idx=0; action_idx<_action_size; action_idx++)
      h[action_idx] = 0.0;
    
    // forall a: h(s_t,A) = sum_i w_i*phi_i(s_t,A)
    for(int idx=0; idx<num_phi && phi_idx[idx]<_state_size; idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
      {
	if(isnan(_phi_val[idx]) || isinf(_phi_val[idx]))
	  cerr<<"\n===NANs in PHI===";
	
	h[action_idx] += _actor_w[phi_idx[idx]][action_idx] * _phi_val[idx];
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
      if(_num_phi != 0) cout<<"ZERO-SUM '"<<_tag<<"'POLICY!";
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
      for(int idx=0; idx<num_phi && phi_idx[idx]<_state_size; idx++)
	cout<<"{"<<phi_idx[idx]<<","<<phi_val[idx]<<"}, ";
      cout<<"\n    A:";
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	cout<< std::fixed << std::setprecision(2) << policy[action_idx]<<",";
      cout<<"";
    }

    delete[] h;
    return policy;
    }*/

  //
  // compute mean and variance of policy for current state
  //   update (_action_xmean,_action_xstd) (_action_ymean,_action_ystd) 
  double* compute_policy_cont(int num_phi, int* phi_idx, float* phi_val)
  {
    double* policy = new double[_state_size*4];

    double temp_actor_xmean = 0.0; 
    double temp_actor_xstd = 0.0;
    double temp_actor_ymean = 0.0; 
    double temp_actor_ystd = 0.0;

    for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
    {
      temp_actor_xmean += _actor_w[_state_size*0+phi_idx[idx]] * phi_val[idx];    
      temp_actor_xstd  += _actor_w[_state_size*1+phi_idx[idx]] * phi_val[idx];
      temp_actor_ymean += _actor_w[_state_size*2+phi_idx[idx]] * phi_val[idx];
      temp_actor_ystd  += _actor_w[_state_size*3+phi_idx[idx]] * phi_val[idx]; 
    }
    temp_actor_xstd = exp(temp_actor_xstd);
    temp_actor_ystd = exp(temp_actor_ystd);

    if(isinf(temp_actor_xmean) || isnan(temp_actor_xmean)) cerr<<"\nBad mu_x(s)";
    if(isinf(temp_actor_xstd) || isnan(temp_actor_xstd)) cerr<<"\nBad std_x(s)";
    if(isinf(temp_actor_ymean) || isnan(temp_actor_ymean)) cerr<<"\nBad mu_y(s)";
    if(isinf(temp_actor_ystd) || isnan(temp_actor_ystd)) cerr<<"\nBad std_y(s)";

    policy[0] = temp_actor_xmean;
    policy[1] = temp_actor_xstd;
    policy[2] = temp_actor_ymean;
    policy[3] = temp_actor_ystd;

    return policy;
  }
    
  // -----------------------------------
  // action ~ Gaussian(mean=W1.phi(s),var=W2.phi(s))
  // DO IT IN EMPTYBRAIN!
  // -----------------------------------
  /*void get_motor_msg_cont(float* motor_msg)
  {
    seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::normal_distribution<double> N_x(_actor_xmean,_actor_xstd);
    std::normal_distribution<double> N_y(_actor_ymean,_actor_ystd);
    std::default_random_engine generator(seed);

    motor_msg[0] = 0.0;//fmin(N_x(generator),1);
    motor_msg[1] = fmax(fmin(N_y(generator),1),-1);
    
    //if(!_rank)
    //cout<<"\nA: ["<<motor_msg[0]<<","<<motor_msg[1]<<"] ~ N(["<<_actor_xmean<<","<<_actor_ymean<<"],["<<_actor_xstd<<","<<_actor_ystd<<"])";   
    }*/

  // soft-max action-selection from policy pi
  /*void get_motor_msg(double* pi, float* action_t, float* motor_msg)
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
    }*/

  float get_rpe()
  {
    float delta =  _reward
      + get_value(_num_phi_prime, _phi_prime_idx, _phi_prime_val)*_gamma
      - get_value(_num_phi, _phi_idx, _phi_val);

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

    for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	grad_H += _phi_val[idx]*(1.0/_action_size-policy[action_idx]);

    return grad_H;
   }*/

  /*------------- Update rules ----------*/
  
  void update_actor_cont(double delta)
  {
    _actor_xmean = 0.0; 
    _actor_xstd = 0.0;
    _actor_ymean = 0.0; 
    _actor_ystd = 0.0;

    for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
    {
      _actor_xmean += _actor_w[_state_size*0+_phi_idx[idx]] * _phi_val[idx];    
      _actor_xstd  += _actor_w[_state_size*1+_phi_idx[idx]] * _phi_val[idx];
      _actor_ymean += _actor_w[_state_size*2+_phi_idx[idx]] * _phi_val[idx];
      _actor_ystd  += _actor_w[_state_size*3+_phi_idx[idx]] * _phi_val[idx];
    }
    _actor_xstd = exp(_actor_xstd);
    _actor_ystd = exp(_actor_ystd);
    
    float tempx1 = _action_xt-_actor_xmean;
    float tempx2 = 1.0/_actor_xstd/_actor_xstd; tempx2 = (tempx2>10.0)?10:tempx2;
    float tempy1 = _action_yt-_actor_ymean;
    float tempy2 = 1.0/_actor_ystd/_actor_ystd; tempy2 = (tempy2>10.0)?10:tempy2;

    // weight consolidation
    for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
    {
      _actor_w[_state_size*0+_phi_idx[idx]] += _alpha*delta*_phi_val[idx]*tempx1*tempx2;      
      _actor_w[_state_size*1+_phi_idx[idx]] += _alpha*delta*_phi_val[idx]*(tempx1*tempx1*tempx2-1.0);
      _actor_w[_state_size*2+_phi_idx[idx]] += _alpha*delta*_phi_val[idx]*tempy1*tempy2;
      _actor_w[_state_size*3+_phi_idx[idx]] += _alpha*delta*_phi_val[idx]*(tempy1*tempy1*tempy2-1.0);
	
      if(isinf(_actor_w[_state_size*0 + _phi_idx[idx]]) || isnan(_actor_w[_state_size*0 + _phi_idx[idx]]))
	cerr<<"\nBad w_xmean";
      if(isinf(_actor_w[_state_size*1 + _phi_idx[idx]]) || isnan(_actor_w[_state_size*1 + _phi_idx[idx]]))
	cerr<<"\nBad w_xstd";
      if(isinf(_actor_w[_state_size*2 + _phi_idx[idx]]) || isnan(_actor_w[_state_size*2 + _phi_idx[idx]]))
	cerr<<"\nBad w_ymean";
      if(isinf(_actor_w[_state_size*3 + _phi_idx[idx]]) || isnan(_actor_w[_state_size*3 + _phi_idx[idx]]))
	cerr<<"\nBad w_ystd";
    }

    // DEBUGGER
    if(!_rank && false)
    {
      cout<<"\nS:";
      //for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
      //cout<<"{"<<_phi_idx[idx]<<","<<_phi_val[idx]<<"},";
      cout<<" PI: N("<<_actor_ymean<<","<<_actor_ystd<<")";
      cout<<" A:"<<_action_yt<<" R:"<<_reward<<" UPDATE:"<<tempy1*tempy2;
    }
  }

  /*void update_actor(double delta)
  {
    double* policy = get_policy(_num_phi,_phi_idx,_phi_val);

    // e-trace decay
    for(int state_idx=0; state_idx<_state_size; state_idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	_actor_e[state_idx][action_idx] *= _lambda;

    // efference-update of e-trace
    for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
	_actor_e[_phi_idx[idx]][action_idx] += _gamma*_phi_val[idx]*(_action[action_idx]-policy[action_idx]);
    
    // weight consolidation
    for(int state_idx=0; state_idx<_state_size; state_idx++)
      for(int action_idx=0; action_idx<_action_size; action_idx++)
      {
	_actor_w[state_idx][action_idx] += _alpha*delta*_actor_e[state_idx][action_idx]*_rho;

	if(isnan(_actor_w[state_idx][action_idx]) && isinf(_actor_w[state_idx][action_idx]))
	  cerr<<"NANs in w";
      }
  
    delete[] policy;
  }*/

  void update_critic(double delta)
  {
    for(int state_idx=0; state_idx<_state_size; state_idx++)
      _critic_e[state_idx] = _lambda*_critic_e[state_idx];

    for(int idx=0; idx<_num_phi && _phi_idx[idx]<_state_size; idx++)
      _critic_e[_phi_idx[idx]] += _gamma * _phi_val[idx];
    
    for (int state_idx=0; state_idx<_state_size; state_idx++)
    {
      _critic_w[state_idx] += _alpha * delta * _critic_e[state_idx] * _rho;

      if(_critic_w[state_idx]!=_critic_w[state_idx]) cerr<<"NANs in v";
    }
  }

  float get_value(int num_phi, int* phi_idx, float* phi_val)
  {
    float _value=0.0;
    
    // q_cap(s,a) = sum_i w_i*phi_i(s,a)
    for(int idx=0; idx<num_phi; idx++)
    {
      if(_critic_w[phi_idx[idx]]!=_critic_w[phi_idx[idx]]) cerr<<"NANs in RPE";

      _value += _critic_w[phi_idx[idx]]*phi_val[idx];
    }
    
    return _value;
  }

  void advance_timestep(int num_phi_prime, int* phi_prime_idx, float* phi_prime_val, float* action_prime, float reward, int time)    
  {
    _num_phi_prime = num_phi_prime;
    _phi_prime_idx = phi_prime_idx;
    _phi_prime_val = phi_prime_val;
        
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
    
    if(_num_phi<0)
      return;
    
    delta = get_rpe();
    update_actor_cont(delta);
    update_critic(delta);
  }  
};
