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

int action_selection(float* policy_geo, float* policy_ego, float heading_direction, float _epsilon)
{
  int action_size = 8;
  float action_ego_to_angle[8] = {-45,-30,-15,0,15,30,45,0};
  float action_geo_to_angle[8] = {-135,-90,-45,0,45,90,135,180};

  float* policy = new float[action_size];
  int greedy_action_ego_chosen = rand()%action_size, action_ego_chosen=0;
      
  for(int action_ego=0; action_ego<action_size; action_ego++)
  {
    policy[action_ego] = 0.0;
    for(int action_geo=0; action_geo<action_size; action_geo++)
    {
      float theta = heading_direction + action_ego_to_angle[action_ego] - action_geo_to_angle[action_geo];
      policy[action_ego] += policy_ego[action_ego] * policy_geo[action_geo] * cos((theta)*PI/180.0);   
    }
    if(policy[action_ego] > policy[greedy_action_ego_chosen])
      greedy_action_ego_chosen = action_ego;
  }

  // if completely clueless, better stop
  float sum_policy=0.0;
  for(int action_idx=0; action_idx<action_size; action_idx++)
    sum_policy += policy[action_idx];
  if(sum_policy==0)
    return 10;

  // a_t = a_t^greedy (epsilon greedy)
  if (((float)rand()/RAND_MAX) < _epsilon) // P(exploit) = _epsilon
    action_ego_chosen = greedy_action_ego_chosen; 
  else // explore
    action_ego_chosen = rand()%action_size;

  return action_ego_chosen;
}
