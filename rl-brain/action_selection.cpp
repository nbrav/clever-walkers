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

void check_nan(double* pi, int pi_size)
{
  for(int idx=0; idx<pi_size; idx++)
    if(pi[idx]!=pi[idx])
      cerr<<"\nNAN ALERT!!";
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

void action_selection (double* policy_behaviour, double* policy_goal, int action_previous, double* policy_collide, float heading_direction, bool DEBUG)
{
  int numDirection = 8, numSpeed = 3; int action_size = numDirection*numSpeed;

  // initalize policy (prev,final) 36x5
  double* policy_prev = new double[numDirection*numSpeed];
  for(int action_idx=0; action_idx<numDirection*numSpeed; action_idx++)
  {
    policy_prev[action_idx] = 0.0;
    policy_behaviour[action_idx] = 0.0;
  }
  
  double policy_sum = 0; 
  // initialize previous action policy
  /*double _tau_previous = 2.0;
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    policy_prev[action_idx] = exp((action_previous==action_idx)/_tau_previous);
    policy_sum += policy_prev[action_idx];
  }
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_prev[action_idx] /= policy_sum;
  */
  
  // behaviour <- pi_prev * goal * collide
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    policy_behaviour[action_idx] = 1;//pow(policy_prev[action_idx];
    policy_behaviour[action_idx] *= policy_goal[action_idx]; 
    policy_behaviour[action_idx] *= policy_collide[action_idx];

    policy_behaviour[action_idx] = policy_behaviour[action_idx];
  }

  // compute and normalize pi_final
  policy_sum = 0.0;
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_sum += policy_behaviour[action_idx];  
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_behaviour[action_idx] /= policy_sum;
      
  check_nan(policy_behaviour,action_size);
  check_nan(policy_goal,action_size);
  check_nan(policy_collide,action_size);

  delete[] policy_goal;
  delete[] policy_collide;
  delete[] policy_prev;
}
