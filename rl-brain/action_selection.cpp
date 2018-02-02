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

std::tuple<int, int>  action_selection(float* policy_geo, float* policy_ego, float heading_direction, float _epsilon)
{
  int action_size = 8;

  float action_geo_to_angle[8] = {-135,-90,-45,0,45,90,135,180};
  float theta=0;

  float* policy = new float[action_size];
  int greedy_action_geo_chosen = rand()%action_size,     greedy_action_ego_chosen = rand()%action_size,
    action_geo_chosen=0, action_ego_chosen=0;

  // finding zero action
  int closest_zero_action = 0;
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    if(abs(int(action_geo_to_angle[action_idx]+360)%360) <
       abs(int(action_geo_to_angle[closest_zero_action]+360)%360-int(action_geo_to_angle[action_idx]+360)%360))
      closest_zero_action = action_idx;
  }
  
  // finding heading action
  int closest_heading_action = 0;
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    if(abs(int(heading_direction+360)%360-int(action_geo_to_angle[action_idx]+360)%360) <
       abs(int(action_geo_to_angle[closest_heading_action]+360)%360-int(action_geo_to_angle[action_idx]+360)%360))
      closest_heading_action = action_idx;
  }

  // initialize policy to zero
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy[action_idx] = 0.0;

  //allo-working
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    policy[action_idx] = policy_geo[action_idx];
  }

  //ego-debugging
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {    
    int action_geo_idx = (action_size+action_idx+closest_heading_action-closest_zero_action)%action_size;    

    //if(action_idx>=2 && action_idx<=4)
      policy[action_geo_idx] *= 1.7*policy_ego[action_idx]; 
      //else
      // policy[action_geo_idx] *= 0.0; 
  }

  bool DEBUG = false;
  if(DEBUG) printf("\n\n");
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {    
    int action_geo_idx = (action_size+action_idx+closest_heading_action-closest_zero_action)%action_size;    
    if(DEBUG) printf("GOAL%d[%0.3f]->",action_geo_idx,policy[action_geo_idx]);
    if(policy[action_geo_idx] > policy[greedy_action_geo_chosen])
    {
      greedy_action_geo_chosen = action_geo_idx;
      //printf("*");
    }
    if(DEBUG) printf("NEW%d[%0.3f]<-EGO%d[%0.3f], ",action_geo_idx,policy[action_geo_idx],action_idx,policy_ego[action_idx]);
    
    if(greedy_action_geo_chosen == action_geo_idx)
      greedy_action_ego_chosen = action_idx;
  }
  if(DEBUG) printf("%d",greedy_action_geo_chosen);
  
  // if completely clueless, better stop
  /*float sum_policy=0.0;
  for(int action_idx=0; action_idx<action_size; action_idx++)
    sum_policy += policy[action_idx];
  if(sum_policy==0)
    return 10;  
  */
  
  // a_t = a_t^greedy (epsilon greedy)
  if (((float)rand()/RAND_MAX) < _epsilon) // P(exploit) = _epsilon
  {
    action_ego_chosen = greedy_action_ego_chosen;
    action_geo_chosen = greedy_action_geo_chosen;
  }
  else // explore
  {
    action_ego_chosen = rand()%action_size;
    action_geo_chosen = (action_size+action_ego_chosen+closest_heading_action-closest_zero_action)%action_size;    
  }
   
  //return std::make_tuple(greedy_action_geo_chosen, greedy_action_ego_chosen);
  return std::make_tuple(action_geo_chosen, action_ego_chosen);
}
