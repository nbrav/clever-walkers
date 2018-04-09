using UnityEngine;
#if UNITY_EDITOR
using UnityEditor;
#endif
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Threading;
using System;

public class QAgent : MonoBehaviour
{
    bool turnOnTriangleIndicator;

    public Rigidbody rb;				  
    private static double DEG_TO_RAD = System.Math.PI / 180.0f;

    // reinforcement learning variables 
    int _action=0;
    float reward_collision = 0, reward_goal;
    float agent_goal_distance = -1.0f;

    // ego-centric state-space parameters
    float[,,] state_array;
    float[] radius_annulus = new float[] {1.0f,2.0f,4.0f,8.0f};
    int[] angle_sector = new int[] {0,15,30,45,60,75,90,105,120,135,150,165,180,195,210,225,240,255,270,285,300,315,330,345,360};

    // allo-centric state-space parameters
    float [,] placecell;
    int NUM_PC=225;
    float M=15.0f,N=15.0f;
    float PC_SIZE = 2.5f;

    // action-space parameters
    float[] speed = new float[]{0.0f, 0.5f, 1.0f};

    // visualization parameters
    List<Vector2> colorList = new List<Vector2>();
    float[] color_angle_sector = new float[] {0,15,30,45,60,75,90,105,120,135,150,165,180,195,210,225,240,255,270,285,300,315,330,345,360};
    ShowCollision show_collision;
    private string[] ray_colour_code = new string[] { "ANNULUS_COLOUR", "SECTOR_COLOUR" };

    // whatever this is for..
    [SerializeField]
    GameObject dummyNavMeshAgent;

    Vector3 defaultLocation;
    Quaternion defaultPose;
    GameObject goalObject;
  
    /* timing setup */

    int frame_rate = 30;
    float time_scale = 1.0F;
    float time_per_update;
    float trial_duration = 30.0F;
    int trial_elasped=0; //in sec  

    int FixedUpdateIndex = 0;
    int UpdateIndex = 0;

    /* velocity component */
    Vector3 position_previous;

    /* agent simulation details */
  
    float AGENT_HEIGHT = 0.0f; // to set-up raycast visuals 
    int RAYCAST_INTERVAL = 5;  

    // Set rate
    void Awake()
    {
        Application.targetFrameRate = frame_rate;
        Screen.fullScreen = true;
    }

    // Intializating Unity
    void Start()
    {
      QualitySettings.vSyncCount = 0;
            
      /* egocentric state set-up */
      
      state_array = new float[angle_sector.Length-1, radius_annulus.Length, 10]; //TODO: dicretize angle generically
              
      /* visualize egocentric states */
      
      /*Transform[] trans = this.gameObject.GetComponentsInChildren<Transform>(true);
      foreach (Transform t in trans)
      {
	if (t.name == "Canvas")
	    show_collision = t.GetComponent<ShowCollision>();
      }
      show_collision.UntriggerIndicator();
      */
      
      /* allcentric place-cell set-up */

      int pc_idx=0;
      placecell = new float[NUM_PC,3];

      for(float x=0.0f; x<M; x++)
      {
	for(float y=0.0f; y<N; y++)
	{
	  if(pc_idx>NUM_PC)
	  {
	      Debug.Log("Wrong index while initializing place-cells!!");
	      break;
	  }
	  
	  placecell[pc_idx,0] = (x - M/2.0f + 0.5f)*PC_SIZE; // + UnityEngine.Random.Range(-2,2);
	  placecell[pc_idx,1] = (y - N/2.0f + 0.5f)*PC_SIZE; // + UnityEngine.Random.Range(-2,2);
	  placecell[pc_idx,2] = 0.0f;
	  pc_idx++;
	}
      }
      
      /* rigidbody setup */
      
      rb = this.gameObject.GetComponent<Rigidbody>();
      rb.constraints = RigidbodyConstraints.FreezePositionY | RigidbodyConstraints.FreezeRotationX | RigidbodyConstraints.FreezeRotationY | RigidbodyConstraints.FreezeRotationZ;
    }

    public void setResetPose(Vector3 location, Quaternion pose)
    {
      defaultLocation = location;
      defaultPose = pose;
    }

    public void setTimeScale(float global_time_scale)
    {
        time_scale = global_time_scale;
    }

    public void setTimePerUpdate(float global_time_per_update)
    {
        time_per_update = global_time_per_update;
    }

    public void setDummyAgentPrefab(GameObject newPref)
    {
        dummyNavMeshAgent = newPref;
    }

    public void setGoal(GameObject goal)
    {
      goalObject = goal;
    }

    /*---------------------------------------------------------------
                             motor system
    ---------------------------------------------------------------*/

    public void set_udp(int action)
    {
      if(action<0 || action>36*speed.Length)
      {
	//Debug.Log("Invalid action value!");
	return;
      }
    
      _action = action;

      do_jump_action(action_to_angle(_action), action_to_speed(_action)); 
    }

    // instantaneous translation+rotation
    void do_jump_action(float action_angle, float action_speed)
    {
        Vector3 ray_origin = transform.position; 
	transform.rotation = Quaternion.Euler(0.0f,action_angle,0.0f); //global rotate 	

	Vector3 next_position = gameObject.GetComponent<Rigidbody>().position + gameObject.transform.forward * (action_speed);
	
	// FIND IF IN BOUNDARY PLACE-CELL
	if(at_edge(next_position)) return;
	  
	gameObject.GetComponent<Rigidbody>().position += gameObject.transform.forward * (action_speed);
	gameObject.GetComponent<Rigidbody>().velocity = Vector3.zero;
	
	// visualize action vectors
	//Vector3 ray_vector = transform.position;
	//DrawLine.DrawLine(ray_origin, ray_vector, Color.black, 1.0);
    }

    /*---------------------------------------------------------------
                             sensory system
    ---------------------------------------------------------------*/

    public List<float> get_udp()
    {
      List<float> udp_out = new List<float>();

      List<float> allo_state = get_features_allocentric();
      udp_out.Add(allo_state.Count/2);
      udp_out.AddRange(allo_state);

      List<float> ego_state = get_features_egocentric();
      udp_out.Add(ego_state.Count);
      udp_out.AddRange(ego_state);
     
      udp_out.Add(get_reward_goal());
      udp_out.Add(get_reward_collision());      
      udp_out.Add(transform.localEulerAngles.y); //heading direction
      
      return udp_out;
    }

    public Vector3 get_velocity()
    {
      Vector3 velocity = (transform.position-position_previous)/Time.fixedDeltaTime;
      position_previous = transform.position;
      return velocity;
    }

    /* --------------------- 
       castray
    --------------------- */
    void cast_ray()
    {
      colorList.Clear();
      
      // cast a ray around the agent 
      RaycastHit[] hits;

      float ray_length = radius_annulus[radius_annulus.Length-1];

      // reset states
      for(int sector=0; sector<angle_sector.Length-1; sector++)
	for(int annulus=0; annulus<radius_annulus.Length; annulus++)
	  for(int relative_angle=0; relative_angle<10; relative_angle++)
	    state_array[sector,annulus,relative_angle] = 0.0f;

      // each ray around the agent (with some angle)
      for (int ray_angle=Mathf.Min(angle_sector); ray_angle<=Mathf.Max(angle_sector); ray_angle += RAYCAST_INTERVAL)
      {
	Vector3 ray_origin = AGENT_HEIGHT*Vector3.up + transform.position; // set origin of ray near the eye of agent
	Vector3 ray_vector = Quaternion.AngleAxis(ray_angle, Vector3.up) * transform.forward; //set ray vector with iterant angle
	hits = Physics.RaycastAll(ray_origin, ray_vector, ray_length); //find raycast hit	
	
	// each hit of a ray
	for (int i=0; i<hits.Length; i++)
	{	    
	  RaycastHit hit = hits[i];
	  Vector3 velocity_obstacle;
	  
	  if(hits[i].collider.gameObject.tag=="pedestrian" && hits[i].collider.gameObject!=this.gameObject) // if not another agent
	  {   
	    velocity_obstacle = hits[i].collider.gameObject.GetComponent<QAgent>().get_velocity();
	  }
	  else if (hits[i].collider.gameObject.tag=="wall")
	  {
	    velocity_obstacle = Vector3.zero;
	  }
	  else
	  {
	    continue;
	  }
	  
	  Vector3 velocity_relative = velocity_obstacle - get_velocity();
	  float relative_angle = Mathf.Round(Vector3.Angle(velocity_relative, transform.forward));
	  int relative_angle_idx = (int)relative_angle/36;
	  state_array[angle_to_sector(ray_angle),distance_to_annulus(hit.distance),relative_angle_idx] = 1.0f; //hits[i].distance;

	  colorList.Add(new Vector2(distance_to_annulus(hit.distance), angle_id(ray_angle)));
	  
	  /*Vector3 ray_origin_rel = AGENT_HEIGHT*Vector3.up + hits[i].collider.gameObject.transform.position; 
	  //Vector3 ray_vector_rel = Quaternion.AngleAxis(theta, Vector3.up) * transform.forward; //egocentric
	  Vector3 ray_vector_rel = Quaternion.AngleAxis(10, Vector3.up) *Vector3.forward; //allocentric
	  Debug.DrawRay(ray_origin_rel, ray_vector_rel*3, Color.green);	 	    */
	}
      }
    }

    /* --------------------- 
       hippocampus place-cell
    --------------------- */
    void simulate_hippocampus()
    {
      float sigma = 1.0f;
      for(int pc_idx=0; pc_idx<NUM_PC; pc_idx++)
      {
	float dis1 = transform.position.x - placecell[pc_idx,0]; // TODO /2sigma2
	float dis2 = transform.position.z - placecell[pc_idx,1];
	placecell[pc_idx,2] = Mathf.Round(Mathf.Exp(-(dis1*dis1+dis2*dis2)/2/sigma/sigma)*100)/100;

	Debug.DrawRay(new Vector3(placecell[pc_idx,0], 0.0f, placecell[pc_idx,1]), Vector3.up, Color.red);
      }
    }

    // is the agent at the end of arena
    bool at_edge(Vector3 position)
    {
	float sigma=1.0f, closest_x=-1.0f, closest_y=-1.0f, closest_dist=-1.0f; int pc_idx=0;
	
	for(float x=0.0f; x<M; x++)
	{
	    for(float y=0.0f; y<N; y++)
	    {
		float dist1 = position.x - placecell[pc_idx,0];
		float dist2 = position.z - placecell[pc_idx,1];
		float dist = Mathf.Round(Mathf.Exp(-(dist1*dist1+dist2*dist2)/2/sigma/sigma)*100)/100;
		
		if(dist>closest_dist)
		{
		    closest_x=x; closest_y=y; closest_dist=dist;  
		}
		pc_idx++;
	    }
	}
	if(closest_x==0 || closest_x==M-1 || closest_y==0 || closest_y==N-1)
	    return true;
	else
	    return false;
    }	
    
    List<float> code_placecells()
    {
      List<float> phi = new List<float>(); 

      for(int pc_idx=0; pc_idx<NUM_PC; pc_idx++)
      {
	if(placecell[pc_idx,2]>0.05)
	{
	  phi.Add(pc_idx);
	  phi.Add(placecell[pc_idx,2]);
	}
	Debug.DrawRay(new Vector3(placecell[pc_idx,0], 0.0f, placecell[pc_idx,1])+Vector3.up,		      
		      Vector3.up*placecell[pc_idx,2],
		      Color.green);	
      }      
      return phi;
    }

    /* ---------------------
       convert agent state to features {#entries, unitIdx_obs1, unitIdx_obs2,..}
       egocentric
    ----------------------- */
    List<float> code_feature()
    {
      List<float> phi = new List<float>();

      for(int annulus=0; annulus<radius_annulus.Length; annulus++)
      {
	for(int sector=0; sector<angle_sector.Length-1; sector++)
	{
	  for(int relative_angle_idx=0; relative_angle_idx<10; relative_angle_idx++)
	  {
	    if(state_array[sector,annulus,relative_angle_idx] > 0.0f)
	    {
	      phi.Add(sector + annulus*(angle_sector.Length-1) + relative_angle_idx*(angle_sector.Length-1)*(radius_annulus.Length));
	      /*if(this.gameObject.name=="agent0")
		  Debug.Log("Theta:"+sector.ToString()+" Ring:"+annulus.ToString()+" RelVel:"+relative_angle_idx.ToString());*/
	    }
	  }
	}
      }
      return phi;
    }

   /* --------------------
       get feature (sector x ring x agent_vel_dir, encoded, allocentric)       
    --------------------- */
    List<float> get_features_allocentric()
    {
        simulate_hippocampus();
        return code_placecells();
    }

    /* --------------------
       get feature (sector x ring x agent_vel_dir, encoded, egocentric)       
    --------------------- */
    List<float> get_features_egocentric()
    {
        cast_ray();
        get_color_list();
        return code_feature();
    }

    /*---------------------------------------------------------------
                                 reward system
    ---------------------------------------------------------------*/

    
    
    void OnTriggerEnter(Collider col)
    {
      if(col.gameObject == goalObject)
      {
	//Debug.Log("Reward!");
        if (turnOnTriangleIndicator)
	  show_collision.TriggerIndicator();
	
	reward_goal = 1.0f;
      }      
      
      if(col.gameObject.tag == "pedestrian" || col.gameObject.tag == "wall" )
      {
	//Debug.Log("Collision!");
	if (turnOnTriangleIndicator)
	  show_collision.TriggerIndicator();
	
	reward_collision = -1.0f;
      }
    }

    void OnTriggerExit(Collider col)
    {
      if(col.gameObject == goalObject)
      {
	if (turnOnTriangleIndicator)
	  show_collision.UntriggerIndicator();
	
	//reward_goal = 0.0f;
      }
      
      if(col.gameObject.tag == "pedestrian" || col.gameObject.tag == "wall" )
      {
	if (turnOnTriangleIndicator)
	  show_collision.UntriggerIndicator();
	
	reward_collision = 0.0f;
      } 
    }

    float shapped_reward_goal()
    {
	float dist=0.0f;
	if(agent_goal_distance != -1)
	{
	    dist=agent_goal_distance-Vector3.Distance(goalObject.transform.position, transform.position);
	}
	agent_goal_distance=Vector3.Distance(goalObject.transform.position, transform.position);
	return dist/10.0f;
    }
    
    float get_reward_goal()
    {
	return reward_goal;
    }

    float get_reward_collision()
    {      
	return reward_collision;
    }

    /*---------------------------------------------------------------
                                 update
    ---------------------------------------------------------------*/

    // Update is called once per frame
    void Update()
    {
      float avgFrameRate = Time.frameCount / Time.time;
      
      Application.targetFrameRate = frame_rate;
      Time.timeScale = time_scale;
    }

    void FixedUpdate()
    {
      
    }

    public void reset()
    {
	//transform.position = defaultLocation;
	transform.position = new Vector3(UnityEngine.Random.Range(-15,15),0,UnityEngine.Random.Range(-15,15));
	
	transform.rotation = Quaternion.Euler(0.0f,UnityEngine.Random.Range(0,360),0.0f); //defaultPose;
	
	reward_goal = 0.0f; reward_collision = 0.0f;
	agent_goal_distance = -1.0f;
	
	position_previous = Vector3.zero;
    }
  
    /*---------------------------------------------------------------
                             utility files
    ---------------------------------------------------------------*/
    
    void OnDrawGizmosSelected()
    {
      Gizmos.color = Color.yellow;
      Gizmos.DrawSphere(transform.position, 1);
    }
  
    // Get which sector has collision
    float angle_id(float angle)
    {
        for (int i = 0; i < color_angle_sector.Length - 1; i++)
        {
            if (color_angle_sector[i] <= angle && angle <= color_angle_sector[i + 1])
            {
                return (float)i;
            }
        }
        return (float)(color_angle_sector.Length - 1);
    }

    // Return color list to draw sector
    public List<Vector2> get_color_list()
    {
        return colorList;
    }

    // Get radius array
    public float[] get_radius_array()
    {
        return radius_annulus;
    }

    // Get angle array
    public float[] get_anlge_array()
    {
        return color_angle_sector;
    }

    /* utilities: ray angle to sector index */
    int angle_to_sector(int angle)
    {
      for(int sector = 0; sector < angle_sector.Length-1; sector++)
      {
	if (angle > angle_sector[sector] && angle <= angle_sector[sector+1])
	  return sector;
      }
      return 0;
    }

    /* utilities: ray angle to sector index */
    int distance_to_annulus(float dist)
    {
      for(int annulus = 0; annulus < radius_annulus.Length; annulus++)
      {
	if (dist <= radius_annulus[annulus])
	  return annulus;
      }
      return -1;
    }

    /* utilities: get angle from action coding (in allocentric frame) */
    float action_to_angle(int actionIndex)
    {
      //return (float)(actionIndex%36)*10.0f;
      return (float)(actionIndex%8)*45.0f;
    }

    /* utilities: get angle from action coding (in allocentric frame) */
    float action_to_speed(int actionIndex)
    {
      return speed[(int)(Math.Floor((double)actionIndex/8))];
    }
  
    public void turn_triangle_indicator(bool flag)
    {
        turnOnTriangleIndicator = flag;
    }
}
