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
  
  // initialize previous action policy
  double policy_sum = 0; double _tau_previous = 0.5;
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    policy_prev[action_idx] = exp((action_previous==action_idx)/_tau_previous);
    policy_sum += policy_prev[action_idx];
  }
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_prev[action_idx] /= policy_sum;
  
  // behaviour <- pi_prev * goal * collide
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    policy_behaviour[action_idx] = 1;//pow(policy_prev[action_idx],1);
    policy_behaviour[action_idx] *= pow(policy_goal[action_idx],10); 
    policy_behaviour[action_idx] *= 1;//pow(policy_collide[action_idx],10);
  }  
  
  // compute and normalize pi_final
  policy_sum = 0.0;
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_sum += policy_behaviour[action_idx];  
  if(DEBUG) cout<<"\n\n";
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    policy_behaviour[action_idx] /= policy_sum;
    if(DEBUG) cout<<"["<<round(policy_prev[action_idx]*100)/100<<"x"<<round(policy_goal[action_idx]*100)/100<<"x"<<round(policy_collide[action_idx]*100)/100<<"="<<round(policy_behaviour[action_idx]*100)/100<<"]";
  }
    
  delete[] policy_goal;
  delete[] policy_collide;
  delete[] policy_prev;
}
