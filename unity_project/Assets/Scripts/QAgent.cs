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
    int _action=7;
    float reward = 0;

    // ego-centric state-space parameters
    float[,,] state_array;
    float[] radius_annulus = new float[] {5.0f,10.0f,15.0f,20.0f};
    //int[] angle_sector = new int[] {-105,-95,-85,-75,-65,-55,-45,-35,-25,-15,-5,5,15,25,35,45,55,65,75,85,95,105,255};
    int[] angle_sector = new int[] {0,15,30,45,60,75,90,105,120,135,150,165,180,195,210,225,240,255,270,285,300,315,330,345,360};

    // allo-centric state-space parameters
    float [,] placecell;
    int NUM_PC=50;
    float M=6.0f, N=6.0f;

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
  
    float AGENT_HEIGHT = 3.0f; // to set-up raycast visuals 
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
      
      Transform[] trans = this.gameObject.GetComponentsInChildren<Transform>(true);
      foreach (Transform t in trans)
      {
	if (t.name == "Canvas")
	    show_collision = t.GetComponent<ShowCollision>();
      }
      show_collision.UntriggerIndicator();

      /* allcentric place-cell set-up */

      /*int _s_x = (int)Mathf.Floor(M*(transform.position.x-X_MIN)/(X_MAX-X_MIN));
      int _s_y = (int)Mathf.Floor(N*(transform.position.z-Y_MIN)/(Y_MAX-Y_MIN));      

      if(_s_x<0) {_s_x=0;}
      else if (_s_x>M-1) _s_x=M-1;

      if(_s_y<0) _s_y=0;
      else if (_s_y>N-1) _s_y=N-1;*/

      float PC_SIZE = 3.0f;
      int pc_idx=0;
      placecell = new float[NUM_PC,3];

      for(float x=0.0f; x<=M; x++)
      {
	for(float y=0.0f; y<=N; y++)
	{
	  placecell[pc_idx,0] = (x - M/2.0f)*PC_SIZE;// + UnityEngine.Random.Range(-2,2);
	  placecell[pc_idx,1] = (y - N/2.0f)*PC_SIZE;// + UnityEngine.Random.Range(-2,2);
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

    // warning! setting velocity only gives old states, dummy!!
    void do_smooth_action(int action, float speed_default)
    {
      /*var rb_velocity_local = transform.InverseTransformDirection(rb.velocity);
	
	if(action==7) //halt
	{
	  rb_velocity_local = new Vector3(0, 0, -1.0f*speed_default);

	  gameObject.transform.Rotate(new Vector3(0, 0, 0) * Time.fixedDeltaTime);
	  gameObject.transform.position += gameObject.transform.forward * (0.0F * Time.fixedDeltaTime);
	}
	else
	{
	  Debug.Log(time_per_update);
	  float theta = action_to_angle(action) * Time.deltaTime / time_per_update;
	  gameObject.transform.Rotate(new Vector3(0, theta, 0));	  
	  
	  rb_velocity_local.x = Mathf.Sin(Mathf.Deg2Rad * action_to_angle(_action));
	  rb_velocity_local.y = 0;
	  rb_velocity_local.z = Mathf.Cos(Mathf.Deg2Rad * action_to_angle(_action)) * speed_default;

	  gameObject.transform.position += gameObject.transform.forward * (speed_default * Time.deltaTime);
	  gameObject.transform.Rotate(new Vector3(0, action_to_angle(_action), 0)*Time.deltaTime);	  
	}

	rb.velocity = transform.TransformDirection(rb_velocity_local);*/
    }

    // instantaneous translation+rotation
    void do_jump_action(int action, float speed_default)
    {
        // visualize action vectors
        Vector3 ray_origin = AGENT_HEIGHT*Vector3.up + transform.position; 
	Vector3 ray_vector = Quaternion.AngleAxis(egocentric_action_to_angle(_action), Vector3.up) * transform.forward;
	//Debug.DrawRay(ray_origin, ray_vector * 2, Color.green);

	var rb_velocity_local = transform.InverseTransformDirection(rb.velocity);	
	float theta = allocentric_action_to_angle(action);

	transform.rotation = Quaternion.Euler(0.0f,theta,0.0f);;	
	gameObject.GetComponent<Rigidbody>().position += gameObject.transform.forward * (speed_default);
    }

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
    
    public List<float> get_udp()
    {
      List<float> udp_out = new List<float>();
      //udp_out = get_features_egocentric();
      udp_out = get_features_allocentric();
      udp_out.Add(get_reward());      
      return udp_out;
    }

    public void set_udp(int action)
    {
      _action = action;

      if (_action >= 0 && _action < 8)
	do_jump_action(_action, 1.0f);
      else
      {
	Debug.Log("Update Error: invalid action range..");
	do_jump_action(3, 0.0f);	    
      }
    }

    public void reset()
    {
      transform.position = new Vector3(UnityEngine.Random.Range(-9,9), 0.0f, UnityEngine.Random.Range(-9,9)); //defaultLocation;
      transform.rotation = Quaternion.Euler(0.0f,UnityEngine.Random.Range(0,360),0.0f); //defaultPose;

      position_previous = Vector3.zero;
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
	  
	  Vector3 velocity_obstacle = hits[i].collider.gameObject.GetComponent<QAgent>().get_velocity();
	  Vector3 velocity_relative = velocity_obstacle - get_velocity();
	  int relative_angle_idx = (int)Mathf.Round(Vector3.Angle(velocity_relative, transform.forward)/36);

	  state_array[angle_to_sector(ray_angle),distance_to_annulus(hit.distance),relative_angle_idx] = 1.0f; //hits[i].distance;

	  Debug.DrawRay(ray_origin, velocity_relative, Color.red);
	  
	  //Debug.Log(Vector3.Angle(velocity_relative, transform.forward));

	  colorList.Add(new Vector2(distance_to_annulus(hit.distance), angle_id(ray_angle)));
	}
      }
    }

    /* --------------------- 
       hippocampus place-cell
    --------------------- */
    void simulate_hippocampus()
    {
      float sigma = 3.0f;
      for(int pc_idx=0; pc_idx<NUM_PC; pc_idx++)
      {
	float dis1 = transform.position.x - placecell[pc_idx,0]; // TODO /2sigma2
	float dis2 = transform.position.z - placecell[pc_idx,1];
	placecell[pc_idx,2] = Mathf.Round(Mathf.Exp(-(dis1*dis1+dis2*dis2)/2/sigma/sigma)*100)/100;

	if(placecell[pc_idx,2]<0.2f) placecell[pc_idx,2]=0.0f; //WARNING clipping approximation
	
	Debug.DrawRay(new Vector3(placecell[pc_idx,0], 0.0f, placecell[pc_idx,1]), Vector3.up, Color.red);
      }
    }

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

    List<float> code_placecells()
    {
      //TODO DEBUG
      
      List<float> phi = new List<float>(); 

      // ORIGINAL PLACE CELL
      /*for(int pc_idx=0; pc_idx<NUM_PC; pc_idx++)
	phi.Add(placecell[pc_idx,2]);            
      */
      
      // NEAREST PLACE CELL
      int nearest_pc_idx = 0;
      for(int pc_idx=0; pc_idx<NUM_PC; pc_idx++)
      {
	if(placecell[pc_idx,2]>placecell[nearest_pc_idx,2])
	  nearest_pc_idx = pc_idx;	  
      }

      for(int pc_idx=0; pc_idx<NUM_PC; pc_idx++)	
	if(pc_idx == nearest_pc_idx)
	  phi.Add(1);
	else
	  phi.Add(0);            

      for(int pc_idx=0; pc_idx<NUM_PC; pc_idx++)
	Debug.DrawRay(new Vector3(placecell[pc_idx,0], 0.0f, placecell[pc_idx,1]) + Vector3.up,		      
		    Vector3.up*phi[pc_idx],
		    Color.green);	

      /*int _s_x = (int)Mathf.Floor(M*(transform.position.x+15.0f)/30.0f);
      int _s_y = (int)Mathf.Floor(N*(transform.position.z+15.0f)/30.0f);      

      if(_s_x<0) {_s_x=0;}
      else if (_s_x>M-1) _s_x=(int)M-1;

      if(_s_y<0) _s_y=0;
      else if (_s_y>N-1) _s_y=(int)N-1;
      */
      
      return phi;
    }

    /* --------------------
       convert agent direction into place-cell state representations
       egocentric
    --------------------- */
    int code_state_pole()
    {
      int pose = ((int) (transform.rotation.eulerAngles.y/10.0));
      return pose;
    }

    /* ---------------------
       convert agent state to features {#entries, unitIdx_obs1, unitIdx_obs2,..}
       egocentric
    ----------------------- */
    List<int> code_feature()
    {
      List<int> phi = new List<int>();

      for(int annulus=0; annulus<radius_annulus.Length; annulus++)
      {
	for(int sector=0; sector<angle_sector.Length-1; sector++)
	{
	  for(int relative_angle_idx=0; relative_angle_idx<10; relative_angle_idx++)
	  {
	    if(state_array[sector,annulus,relative_angle_idx] > 0.0f)
	    {
	      phi.Add(sector + annulus*(angle_sector.Length-1) + relative_angle_idx*(angle_sector.Length-1)*(radius_annulus.Length));
	      //phi.Add(sector+annulus*(angle_sector.Length-1));
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
    List<int> get_features_egocentric()
    {
        cast_ray();
        get_color_list();
        return code_feature();
    }

    /* ------------------------
        get reward
    ----------------------------*/
    void OnTriggerEnter(Collider col)
    {
      //Debug.Log("Collision +"+col.gameObject.name);

      if(col.gameObject == goalObject)
      {
	if (turnOnTriangleIndicator)
	  show_collision.TriggerIndicator();
	
	reward = 1.0f;
      }
    }

    void OnTriggerExit(Collider col)
    {
      if(col.gameObject == goalObject)
      {
	if (turnOnTriangleIndicator)
	  show_collision.UntriggerIndicator();
	
	reward = 0.0f;
      }
    }

    float get_reward()
    {      
      return reward;
    }

    /* utilities: ray angle to sector index */
    int angle_to_sector(int angle)
    {
      // angle_sector = new int[] {-105,-95,-85,-75,-65,-55,-45,-35,-25,-15,-5,5,15,25,35,45,55,65,75,85,95,105,255};

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
      // radius_annulus = new float[] {5.0f,10.0f,15.0f,20.0f};

      for(int annulus = 0; annulus < radius_annulus.Length; annulus++)
      {
	if (dist <= radius_annulus[annulus])
	  return annulus;
      }
      return -1;
    }

    /* utilities: get angle from action coding */
    float egocentric_action_to_angle(int actionIndex)
    {
        if (actionIndex == 0)                 return -45.0f;
        else if (actionIndex == 1)            return -30.0f;
        else if (actionIndex == 2)            return -15.0f;
        else if (actionIndex == 3)            return 0.0f;
        else if (actionIndex == 4)            return 15.0f;
        else if (actionIndex == 5)            return 30.0f;
        else if (actionIndex == 6)            return 45.0f;
	else if (actionIndex == 7)            return 0.0f;
	else return -1.0f;
    }

    /* utilities: get angle from action coding */
    float allocentric_action_to_angle(int actionIndex)
    {
        if (actionIndex == 0)                 return -135.0f;
        else if (actionIndex == 1)            return -90.0f;
        else if (actionIndex == 2)            return -45.0f;
        else if (actionIndex == 3)            return 0.0f;
        else if (actionIndex == 4)            return 45.0f;
        else if (actionIndex == 5)            return 90.0f;
        else if (actionIndex == 6)            return 135.0f;
	else if (actionIndex == 7)            return 180.0f;
	else return -1.0f;
    }

    public void turn_triangle_indicator(bool flag)
    {
        turnOnTriangleIndicator = flag;
    }
}
