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

/* a ~ pi(.) sample an action from policy pi */
int get_softmax_action(double* pi, int pi_size)  
{
  float rand_prob = (float)rand()/RAND_MAX, pi_bucket = 0.0;
  for(int action_idx=0; action_idx<pi_size; action_idx++)
  {
      pi_bucket += pi[action_idx];

      if(rand_prob <= pi_bucket)
	return action_idx;	
  }    
}

/* a = argmax_a pi(a) */
int get_greedy_action(double* pi, int pi_size)  
{
  int action_greedy = (int)rand()%pi_size;

  for(int action_idx=0; action_idx<pi_size; action_idx++)
    if(pi[action_idx] > pi[action_greedy])
      action_greedy = action_idx;
  return action_greedy;
}

void gain_policy(double* pi, int pi_size, float gain)
{
  double pi_sum = 0.0;
  for(int action_idx=0; action_idx<pi_size; action_idx++)
  {
    pi[action_idx] = pow(pi[action_idx],gain);
    pi_sum += pi[action_idx];    
  }
  for(int action_idx=0; action_idx<pi_size; action_idx++)
    pi[action_idx] /= pi_sum;      
}

/* return closest action (in action-space) for heading angle */
int bearing_to_action(int theta, int numDirection)
{
  return round(theta/360.0*numDirection);
}

/* transform policy (DxS) to global action-space (along D-axis by theta) */
void geocentricate(double* pi, int numDirection, int numSpeed, int theta)
{
  double* pi_old = new double[numDirection*numSpeed];
  for(int idx=0; idx<numDirection*numSpeed; idx++)
  {
    pi_old[idx]=pi[idx];
    pi[idx]=0;
  }

  int heading_action = bearing_to_action(theta, numDirection);
  for(int numSpeed_idx=0; numSpeed_idx<numSpeed; numSpeed_idx++)
  {
    for(int numDirection_idx=0; numDirection_idx<numDirection; numDirection_idx++)
    {
      int action_geo_idx = numSpeed_idx*numDirection+(numDirection+numDirection_idx+heading_action)%numDirection;
      pi[action_geo_idx] += pi_old[numSpeed_idx*numDirection+numDirection_idx];      
    }
  }
  delete[] pi_old;
}

/* transform action (in local action-space) to global action (along D-axis by theta) */
int geocentricate(int action, int numDirection, int numSpeed, int theta)
{
  int heading_action = bearing_to_action(theta, numDirection);
  return numDirection*floor(action/numDirection) + (action%numDirection + heading_action + numDirection)%numDirection;
}

/* transform action (in global action-space) to local action (along D-axis by -theta) */
int egocentricate(int action, int numDirection, int numSpeed, int theta)
{
  int heading_action = bearing_to_action(theta, numDirection);
  return numDirection*floor(action/numDirection) + (action%numDirection - heading_action + numDirection)%numDirection;
}
