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

std::tuple<int, int>
action_selection(double* policy_geo, int action_previous, double* policy_ego, float* preference, float heading_direction, float _epsilon, bool DEBUG)
{
  int action_size = 8; _epsilon = 1.0;

  float action_geo_to_angle[8] = {-135,-90,-45,0,45,90,135,180};
  float theta=0;

  float* policy = new float[action_size];
  double* policy_geo_previous = new double[action_size];
  int greedy_action_geo_chosen = rand()%action_size, greedy_action_ego_chosen = rand()%action_size,
    action_geo_chosen=rand()%action_size, action_ego_chosen=rand()%action_size;
  
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

  // initialize previous action policy
  double policy_sum = 0;
  double _tau_previous = 0.05;
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    policy_geo_previous[action_idx] = exp((action_previous==action_idx)/_tau_previous);
    policy_sum += policy_geo_previous[action_idx];
    if(policy_geo_previous[action_idx] != policy_geo_previous[action_idx]) cerr<<"\n---NANs in PI_{t-1}!---";
  }
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_geo_previous[action_idx] /= policy_sum;
    
  //float k1=preference[0], k2=10*preference[1];
  float k1=1, k2=1;
  
  //allo-centric
  if(DEBUG) cout<<"\nPI[";
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {
    if(DEBUG) cout<<policy_geo[action_idx]<<",";//<<"<-"<<policy_geo_previous[action_idx]<<" ";
    policy[action_idx] = pow(policy_geo_previous[action_idx],2) * pow(policy_geo[action_idx],1); //k1/(k1+k2)

    if(policy[action_idx]!=policy[action_idx]) cerr<<"\n---NANs in PI_f!---"<<policy_geo[action_idx]<<","<<policy_geo_previous[action_idx];
  }
  if(DEBUG)cout<<"]";

  //ego-centric
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {    
    int action_geo_idx = (action_size+action_idx+closest_heading_action-closest_zero_action)%action_size;    

    //if(action_idx>=3 && action_idx<=5)
    policy[action_geo_idx] *= pow(policy_ego[action_idx],2);//k2/(k1+k2));
    //else
    //policy[action_geo_idx] *= 0.0; 
  }

  // normalize pi_f
  policy_sum = 0.0;
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy_sum += policy[action_idx];
  for(int action_idx=0; action_idx<action_size; action_idx++)
    policy[action_idx] /= policy_sum;
  
  // soft-max action-selection
  float rand_prob = ((float)rand()/RAND_MAX), policy_bucket = 0.0;
  //cout<<"\n";
  for(int action_idx=0; action_idx<action_size; action_idx++)
  {    
    int action_geo_idx = (action_size+action_idx+closest_heading_action-closest_zero_action)%action_size;
    policy_bucket += (policy[action_geo_idx]);
    if(rand_prob <= policy_bucket)    
    {
      greedy_action_geo_chosen = action_geo_idx;      
      greedy_action_ego_chosen = action_idx;
      break;
    }
    //cout<<policy[action_geo_idx]<<",";
  }

  // greedy action_selection
  /*for(int action_idx=0; action_idx<action_size; action_idx++)
  {    
    int action_geo_idx = (action_size+action_idx+closest_heading_action-closest_zero_action)%action_size;
    
    if(policy[action_geo_idx] > policy[greedy_action_geo_chosen])
    {
      greedy_action_geo_chosen = action_geo_idx;    
      greedy_action_ego_chosen = action_idx;
    }
    }*/
  
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

  if(DEBUG) cout<<" A:"<<action_geo_chosen<<" A':"<<action_previous;
  return std::make_tuple(action_geo_chosen, action_ego_chosen);
}
