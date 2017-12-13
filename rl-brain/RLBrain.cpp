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
 private:
  int _time;
  int _rank;
  
  int _state, _state_prime;
  std::vector< float > _reward;
  int _action;

  std::vector< float > _rpe;    
  
  int _state_size;
  int _reward_size;
  int _action_size;

  std::vector< std::vector< std::vector<float> > > _qvalue;
  std::vector< std::vector< std::vector<float> > > _etrace;
  
  float _alpha;                // Learning rate
  std::vector< float > _gamma; // Discount factor for Q
  float _epsilon;              // Greedy arbitration exploration
  float _lambda;               // Eligibility decay

  FILE *qvalue_outfile;
  FILE *qvalue_infile;
  //TODO: do we need two files for reading and writing?
  
  FILE *state_outfile;
  FILE *action_outfile;
  FILE *reward_outfile;

  char const *QVALUE_FILE, *REWARD_FILE, *STATE_FILE, *ACTION_FILE;
  
 public:

  bool VERBOSE;
  
  qbrain(int rank)
  {
    _rank = rank;
    parse_param();

    std::string QVALUE_STRING = "qvalue."+std::to_string(_rank)+".log";
    QVALUE_FILE = QVALUE_STRING.c_str();

    std::string STATE_STRING = "state."+std::to_string(_rank)+".log";
    STATE_FILE = STATE_STRING.c_str();

    std::string ACTION_STRING = "action."+std::to_string(_rank)+".log";
    ACTION_FILE = ACTION_STRING.c_str();

    std::string REWARD_STRING = "reward-punishment."+std::to_string(_rank)+".log";
    REWARD_FILE = REWARD_STRING.c_str();
    
    float gaussian[] = {0.1,0.2,0.4,0.7,0.4,0.2,0.1,0.1};

    std::ifstream fs(QVALUE_FILE);
    if(fs.is_open())
    {
      // Read Q matrix from file
      qvalue_infile = fopen(QVALUE_FILE,"rb");
      fseek(qvalue_infile, -sizeof(float)*_reward_size*_state_size*_action_size, SEEK_END);

      _qvalue.resize(_reward_size);
      for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      {
	_qvalue[reward_idx].resize(_state_size);
	for (int state_idx=0; state_idx<_state_size; state_idx++)
	{
	  _qvalue[reward_idx][state_idx].resize(_action_size);
	  fread(&_qvalue[reward_idx][state_idx][0], sizeof(float), _action_size, qvalue_infile);
	}
      }      
      fclose(qvalue_infile);   
    }
    else
    {
      // Initialize Q matrix
      _qvalue.resize(_reward_size);
      for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      {
	_qvalue[reward_idx].resize(_state_size);
	for (int state_idx=0; state_idx<_state_size; state_idx++)
	{
	  _qvalue[reward_idx][state_idx].resize(_action_size,1);
	  for (int action_idx=0; action_idx<_action_size; action_idx++)
	  {
	    _qvalue[reward_idx][state_idx][action_idx] = (float)rand()/RAND_MAX; //gaussian[action_idx]; //
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
    
    // Initiate delta_i
    _rpe.resize(_reward_size);
    std::fill(_rpe.begin(), _rpe.end(), 1.0);

    qvalue_outfile = fopen(QVALUE_FILE, "ab");
    fclose(qvalue_outfile);

    state_outfile = fopen(STATE_FILE, "wb");
    fclose(state_outfile);

    reward_outfile = fopen(REWARD_FILE, "ab");
    fclose(reward_outfile);
  }

  void parse_param()
  {
    _state_size = 100; //TODO: Must use a param file     
    _action_size = 8;  //TODO: Must use a param file     
    _reward_size = 1; //TODO : Must use a param file     
    
    _alpha = 0.1; // learning rate
    _epsilon = 0.8; // epsilon-greedy WARNING: RANDOM ACTIONS!! 
    _lambda = 0.0; // eligibility parameter 0.8
    
    _gamma.resize(_reward_size);
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      _gamma[reward_idx] = 0.8; // temporal-decay rate
  }

  void reset()
  {
    _state = 0;
    _action = 0;
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      _reward[reward_idx] = 0.0;
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      for (int state_idx=0; state_idx<_state_size; state_idx++)
    	for (int action_idx=0; action_idx<_action_size; action_idx++)
    	  _etrace[reward_idx][state_idx][action_idx] = 0.0;
  }

  /*---------- Logger -----------*/
  
  void state_log()
  {
    state_outfile = fopen(STATE_FILE, "ab"); //ab
    fwrite(&_state, sizeof(int), 1, state_outfile);
    fclose(state_outfile);    
  }

  void action_log()
  {
    action_outfile = fopen(ACTION_FILE, "ab"); //ab
    fwrite(&_action, sizeof(int), 1, action_outfile);
    fclose(action_outfile);    
  }

  void reward_log()
  {
    reward_outfile = fopen(REWARD_FILE, "ab"); //ab
    fwrite(&_reward[0], sizeof(float) , _reward_size, reward_outfile);
    fclose(reward_outfile);    
  }

  void qvalue_log()
  {
    qvalue_outfile = fopen(QVALUE_FILE, "ab"); // overwrite every EPOCHs
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      for (int state_idx=0; state_idx<_state_size; state_idx++)
	fwrite(&_qvalue[reward_idx][state_idx][0], sizeof(float) , _action_size, qvalue_outfile);
    fclose(qvalue_outfile);
  }
  
  /* ---------- I/O ----------------*/
  
  void compute_action()
  { 
    int greedy_action =
      std::distance(_qvalue[0][_state].begin(), std::max_element(_qvalue[0][_state].begin(), _qvalue[0][_state].end()));
    
    if (((float)rand()/RAND_MAX) < _epsilon) // P(exploit) = _epsilon
      _action = greedy_action; 
    else // explore
      _action = rand()%_action_size;
    
    //_action = _state;     //WARNING! DEBUGGING!
  }

  void set_state(int state)
  {
    _state = state;
  }

  int get_action()
  {
    return _action;
  }

  /*------------- Update rules ----------*/
  
  void update_etrace()
  {
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      for (int state_idx=0; state_idx<_state_size; state_idx++)
    	for (int action_idx=0; action_idx<_action_size; action_idx++)
	  _etrace[reward_idx][state_idx][action_idx] *= (_lambda*_gamma[reward_idx]);
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      _etrace[reward_idx][_state][_action] = 1.0;
  }

  void update_qvalue()
  {
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
    {
      // releasing dopamine
      _rpe[reward_idx] = _reward[reward_idx] +
	_gamma[reward_idx]*(*std::max_element(_qvalue[reward_idx][_state_prime].begin(), _qvalue[reward_idx][_state_prime].end()))
	- _qvalue[reward_idx][_state][_action];
    }
    
    // cortico-striatal learning
    for (int reward_idx=0; reward_idx<_reward_size; reward_idx++)
      for (int state_idx=0; state_idx<_state_size; state_idx++)
	for (int action_idx=0; action_idx<_action_size; action_idx++)
	  _qvalue[reward_idx][state_idx][action_idx] += _alpha*_rpe[reward_idx]*_etrace[reward_idx][state_idx][action_idx];    
  }
  void advance_timestep(int state_prime, int reward, int time)    
  {
    _state_prime = state_prime;
    _reward[0] = reward;
    _time = time;

    update_etrace();
    update_qvalue();
   
    if(time%1000==0)
      qvalue_log();

    fflush(stdout);
    
    compute_action();

    reward_log();
    state_log();
    action_log();

    //printf("\n BRAIN[%d,%d -> %f,%d]",
    //	   _state,get_action(),_reward[0],_state_prime);
  }  
};
