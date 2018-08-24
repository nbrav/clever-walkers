#include <iostream>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <time.h>
#include <algorithm>
#include <math.h>
#include <string>

#include "Utils.cpp"

#define PI 3.14159265

using namespace std;

void rectify_zero(double* pi, int pi_size)
{
    for(int action_idx=0; action_idx<pi_size; action_idx++)
      pi[action_idx] = 1.0/float(pi_size);
    cout<<"\nZero behave-policy!";
}

void check_nan(double* pi, int pi_size)
{
  for(int idx=0; idx<pi_size; idx++)
    if(pi[idx]!=pi[idx])
    {
      cerr<<"\nNAN ALERT!!";
      exit(0);
    }
}

void print_policy(double* pi, int pi_size, string name)
{
  cout<<"\n"<<name<<"[";
  for(int idx=0; idx<pi_size; idx++)
    cout<<round(pi[idx]*100)/100<<","<<setw(4);
  cout<<"]";
}

// compute IS()
void IS(double* pi, double* b, int pi_size, int action)
{
  if(action<0 || action>=pi_size)
    cerr<<"Invalided action for importance sampling!";
  if(!b[action]==0.0)
    cout<<" IS="<<pi[action]/b[action];  
}

// compute KL(pi1||pi2)
void KL(double* pi1, double* pi2, int pi_size)
{
  double div = 0.0;
  for(int idx=0; idx<pi_size; idx++)
    if(!pi1[idx]==0.0)
      div += pi1[idx]*(log(pi1[idx])-log(pi2[idx]));
  cout<<" KL="<<div;
}

void action_selection (double* policy_behaviour, double* policy_goal, double* policy_collide, float* action_previous, int action_size, float heading_direction, bool DEBUG)
{
  // initalize policy (prev,final) 36x5
  double* policy_prev = new double[action_size];
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_behaviour[action_idx] = 0.0;
  
  double policy_sum = 0;
  
  // initialize previous action policy
  double _tau2_previous = 1.0;
  int action_previous_index = rand()%action_size;
  for(int action_idx=0; action_idx<action_size; action_idx++)
    if(action_previous[action_idx]==1)
      action_previous_index = action_idx;
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    int action_diff = (action_idx-action_previous_index)%action_size;
    policy_prev[action_idx] = sqrt(2*PI*_tau2_previous)*exp(-action_diff*action_diff/2/_tau2_previous);
    policy_sum += policy_prev[action_idx];
  }
  if(policy_sum==0)
    rectify_zero(policy_prev, action_size);
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_prev[action_idx] /= policy_sum;
  
  // behaviour <- pi_prev * goal * collide
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    policy_behaviour[action_idx] = 1;//pow(policy_prev[action_idx];
    policy_behaviour[action_idx] *= policy_goal[action_idx];
    //policy_behaviour[action_idx] *= policy_prev[action_idx];    
    //policy_behaviour[action_idx] *= policy_collide[action_idx];
  }

  // compute and normalize pi_final
  policy_sum = 0.0;
  
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_sum += policy_behaviour[action_idx];
  
  if(policy_sum==0)
    rectify_zero(policy_behaviour, action_size);

  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_behaviour[action_idx] /= policy_sum;
  
  check_nan(policy_behaviour,action_size);
  check_nan(policy_goal,action_size);
  check_nan(policy_collide,action_size);

  // debugging
  if(DEBUG)
  {
    cout<<"\n";
    print_policy(policy_goal,36,"goal");     //KL(policy_goal,policy_behaviour,direction_size*speed_size);
    //print_policy(policy_prev,36,"prev");
    print_policy(policy_collide,36,"coll");  //KL(policy_collide,policy_behaviour,direction_size*speed_size);
    print_policy(policy_behaviour,36,"beha"); 
  }

  delete[] policy_goal;
  delete[] policy_collide;
  delete[] policy_prev;
}
