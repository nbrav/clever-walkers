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
void geocentricate(double* pi, int direction_size, int speed_size, int theta)
{
  double* pi_old = new double[direction_size*speed_size];
  for(int action_idx=0; action_idx<direction_size*speed_size; action_idx++)
  {
    pi_old[action_idx]=pi[action_idx];
    pi[action_idx]= 0;
  }

  int heading_direction = bearing_to_action(theta, direction_size);
  for(int speed_idx=0; speed_idx<speed_size; speed_idx++)
  {
    for(int direction_idx=0; direction_idx<direction_size; direction_idx++)
    {
      int action_geo_idx =  speed_idx*direction_size + (direction_size+direction_idx+heading_direction)%direction_size;
      pi[action_geo_idx] += pi_old[speed_idx*direction_size + direction_idx];
    }
  }
  delete[] pi_old;
}

/* transform action (in global action-space) to local action (along D-axis by -theta) */
void egocentricate(float* action_ego, float* action, int direction_size, int speed_size, int theta)
{
  int heading_direction = bearing_to_action(theta, direction_size);  
  for(int speed_idx=0; speed_idx<speed_size; speed_idx++)
  {
    for(int direction_idx=0; direction_idx<direction_size; direction_idx++)
    {
      int action_ego_idx = speed_idx*direction_size + (direction_size+direction_idx-heading_direction)%direction_size;	
      action_ego[speed_idx*direction_size + direction_idx] = action[action_ego_idx];
    }
  }  
}
