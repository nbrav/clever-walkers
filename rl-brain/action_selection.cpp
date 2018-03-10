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

std::tuple<int, int> action_selection
(double* policy_geo, int action_previous, double* policy_ego, float* preference, float heading_direction, float _epsilon, bool DEBUG)
{
  int numDirection = 8, numSpeed = 3;
  int action_size = numDirection*numSpeed;  

  // initalize policy (geo,ego,prev,final) 36x5
  double* policy_prev = new double[numDirection*numSpeed];
  double* policy_final = new double[numDirection*numSpeed];
  
  for(int action_idx=0; action_idx<numDirection*numSpeed; action_idx++)
  {
    policy_prev[action_idx] = 0.0;
    policy_final[action_idx] = 0.0;
  }

  int action_geo_chosen = rand()%action_size,
      action_ego_chosen = rand()%action_size;

  // computing difference between ego and geo
  // initializing action -> angle
  float action_geo_to_angle[numDirection];
  for(int idx=0; idx<numDirection; idx++)
    action_geo_to_angle[idx]=float(idx)*10.0f;    
  
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
  
  // initialize previous action policy
  double policy_sum = 0; double _tau_previous = 0.20;
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    policy_prev[action_idx] = exp((action_previous==action_idx)/_tau_previous);
    policy_sum += policy_prev[action_idx];
    
    if(policy_prev[action_idx] != policy_prev[action_idx]) cerr<<"\n---NANs in PI_prev!---";
  }
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_prev[action_idx] /= policy_sum;
    
  //allo-centric
  if(DEBUG) cout<<"\n\nPI_goal[";
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    if(DEBUG) cout<<policy_geo[action_idx]<<",";
    policy_final[action_idx] = pow(policy_prev[action_idx],1) * pow(policy_geo[action_idx],1); 

    if(policy_final[action_idx]!=policy_final[action_idx])
      cerr<<"\nNANs in PI_goal!";
  }
  if(DEBUG)cout<<"]";
  
  /*//ego-centric
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {    
    int action_geo_idx = (action_size+action_idx+closest_heading_action-closest_zero_action)%action_size;    

    //if(action_idx>=3 && action_idx<=5)
    policy[action_geo_idx] *= pow(policy_ego[action_idx],2);
    //else
    //policy[action_geo_idx] *= 0.0; 
  }*/
  
  // normalize pi_f
  policy_sum = 0.0;
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_sum += policy_final[action_idx];
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_final[action_idx] /= policy_sum;

  // pi_final action-selection

  if(SOFTMAX)
  {  
    float rand_prob = ((float)rand()/RAND_MAX), policy_bucket = 0.0;
    for(int action_idx=0; action_idx<action_size; action_idx++)
    {    
      int action_geo_idx = (action_size+action_idx+closest_heading_action-closest_zero_action)%action_size;
      policy_bucket += (policy_final[action_geo_idx]);
      if(rand_prob <= policy_bucket)
      {
	action_geo_chosen = action_geo_idx;      
	action_ego_chosen = action_idx;
	break;
      }
    }
  }
  else
  {
    for(int action_idx=0; action_idx<action_size; action_idx++)
    {    
      if(policy_final[action_idx]>policy_final[action_geo_chosen])
      {
	action_geo_chosen = action_idx;    
	action_ego_chosen = action_idx;
      }
    }

    if (((float)rand()/RAND_MAX)>_epsilon) //P(exploit)=_epsilon
    {
      action_ego_chosen = rand()%action_size;
      action_geo_chosen = rand()%action_size; //(action_size+action_ego_chosen+closest_heading_action-closest_zero_action)%action_size;
    }
  }
  
  
  delete[] policy_geo;
  delete[] policy_ego;
  delete[] policy_prev;
  delete[] policy_final;
  
  return std::make_tuple(action_geo_chosen, action_ego_chosen);
}
