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

    private static double DEG_TO_RAD = System.Math.PI / 180.0f;

    float[] radius_annulus = new float[] {5.0f,10.0f,15.0f,20.0f};
    //float[] radius_annulus = new float[] { 5.0f, 10.0f, 20.0f };
    int[] angle_sector = new int[] {-105,-95,-85,-75,-65,-55,-45,-35,-25,-15,-5,5,15,25,35,45,55,65,75,85,95,105,255};

    // Color of Sector setup
    List<Vector2> colorList = new List<Vector2>();
    float[] color_angle_sector = new float[] {-105,-95,-85,-75,-65,-55,-45,-35,-25,-15,-5,5,15,25,35,45,55,65,75,85,95,105};

    // Indicator of collision
    ShowCollision show_collision;

    float[,] state_array;
    int _action=7;
    int reward = 0;

    private string[] ray_colour_code = new string[] { "ANNULUS_COLOUR", "SECTOR_COLOUR" };

    GameObject goalGameObject;

    [SerializeField]
    GameObject dummyNavMeshAgent;

    Vector3 goalAgentPosition;
    Vector3 start_position;

    int FixedUpdateIndex = 0;
    int UpdateIndex = 0;

    /* Timing setup */

    int frame_rate = 30;
    float time_scale = 1.0F; // TODO: get from PopulateScene
    float time_per_update;
  
    /* constants that you don't have to care about */
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
            
      /* agent-env set-up */
      
      state_array = new float[angle_sector.Length-1,radius_annulus.Length];
      start_position = transform.position;
        

        //show_collision = this.gameObject.GetComponent<Canvas>().GetComponent<ShowCollision>();
        Transform[] trans = this.gameObject.GetComponentsInChildren<Transform>(true);
        foreach (Transform t in trans)
        {
            if (t.name == "Canvas")
            {
                show_collision = t.GetComponent<ShowCollision>();
            }
        }

        
    }

    public void setTimeScale(float global_time_scale)
    {
        time_scale = global_time_scale;
    }

    public void setTimePerUpdate(float global_time_per_update)
    {
        time_per_update = global_time_per_update;
    }

    public void setGoalObject(Vector3 newGoalObj)
    {
        goalAgentPosition = newGoalObj;
    }

    public void setDummyAgentPrefab(GameObject newPref)
    {
        dummyNavMeshAgent = newPref;
    }

    void do_action(int action, float speed_default)
    {
        if(action==7) //halt
	{
	  gameObject.transform.Rotate(new Vector3(0, 0, 0) * Time.fixedDeltaTime);
	  gameObject.transform.position += gameObject.transform.forward * (0.0F * Time.fixedDeltaTime);
	}
	else
	{
	  gameObject.transform.position += gameObject.transform.forward * (speed_default * Time.deltaTime);
	}
    }

    // Update is called once per frame
    void Update()
    {

        float avgFrameRate = Time.frameCount / Time.time;

        Application.targetFrameRate = frame_rate;
        Time.timeScale = time_scale;

        if (_action >= 0 && _action < 8)
            do_action(_action, 1.5f);
        else
        {
            Debug.Log("Update Error: invalid action range..");
            do_action(7, 0.0f);
        }
    }

    void FixedUpdate()
    {
      // Visualize action vectors
      Vector3 ray_origin = AGENT_HEIGHT*Vector3.up + transform.position; 
      Vector3 ray_vector = Quaternion.AngleAxis(action_to_angle(_action), Vector3.up) * transform.forward;
      Debug.DrawRay(ray_origin, ray_vector * 2, Color.green);

      //float turning_speed = 0.01f*action_to_angle(action)/time_per_update;	  
      //gameObject.transform.Rotate(new Vector3(0, turning_speed, 0) * Time.deltaTime);
      gameObject.transform.Rotate(new Vector3(0, time_per_update*action_to_angle(_action), 0));
    }
    
    public int[] get_udp()
    {
      int[] udp_out = new int[2];
      udp_out[0] = get_state(); //get state from QAgent
      udp_out[1] = get_reward();
      return udp_out;      
    }

    public void set_udp(int action)
    {
      _action = action;      
    }

    public void reset()
    {
      float rand_pos_x = -140.0f + UnityEngine.Random.Range(-20,20);
      float rand_pos_y = 0.0f;
      float rand_pos_z = 215.0f + UnityEngine.Random.Range(-20,20);

      float rand_theta_x = 0.0f;
      float rand_theta_y = UnityEngine.Random.Range(0,360);
      float rand_theta_z = 0.0f;

      transform.position = new Vector3(rand_pos_x,rand_pos_y,rand_pos_z);
      transform.rotation = Quaternion.Euler(rand_theta_x,rand_theta_y,rand_theta_z);
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
	  state_array[sector,annulus] = 0.0f;

      // each ray around the agent (with some angle)
      for (int ray_angle=Mathf.Min(angle_sector); ray_angle<=Mathf.Max(angle_sector); ray_angle += RAYCAST_INTERVAL)
      {
	Vector3 ray_origin = AGENT_HEIGHT*Vector3.up + transform.position; // set origin of ray near the eye of agent
	Vector3 ray_vector = Quaternion.AngleAxis(ray_angle, Vector3.up) * transform.forward; //set ray vector with iterant angle
	hits = Physics.RaycastAll(ray_origin, ray_vector, ray_length); //find raycast hit	
	//Debug.DrawRay(ray_origin, ray_vector * ray_length, Color.green); // paint all rays green
	
	// each hit of a ray
	for (int i=0; i<hits.Length; i++)
	{	    
	  RaycastHit hit = hits[i];
	  state_array[angle_to_sector(ray_angle),distance_to_annulus(hit.distance)] = 1.0f;
	  
	  //List of sector colors
	  colorList.Add(new Vector2(distance_to_annulus(hit.distance), angle_id(ray_angle)));
	}
      }
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

    /* --------------------
       convert sensory state matrix to coded state:
       DEPRECATED; TODO: add appropriate name
    --------------------- */
    int code_state_combinatoric_sectorized()
    {
        // TODO: complete for new multi-dimensional state_array
        float state = 0;
        //for(int sector=0; sector<state_array.Length; sector++)
        //{
        //state += state_array[sector] * Mathf.Pow(radius_annulus.Length+1, state_array.Length-1-sector);
        //Debug.Log("["+state_array[0].ToString()+" "+state_array[1].ToString()+" "+state_array[2].ToString()+"] "+state.ToString());
        //}
        return (int)state;
    }

    /* --------------------
       convert sensory state matrix to coded state:
       DEPRECATED; combinatoric: all combinations of sensor grid 
    --------------------- */
    int code_state_combinatoric()
    {
        float state = 0;
        int unit_tracker = 0;
        for (int sector = 0; sector < angle_sector.Length - 1; sector++)
            for (int annulus = 0; annulus < radius_annulus.Length; annulus++)
                state += state_array[sector, annulus] * Mathf.Pow(2, unit_tracker++);
        return (int)state;
    }

    /* --------------------
       convert sensory state matrix to coded state:
       nearest-winner
    --------------------- */
    int code_state()
    {
      for(int annulus=0; annulus<radius_annulus.Length; annulus++)
      {
	System.Random rand = new System.Random();
	List<int> closest_sectors = new List<int>();
	
	for(int sector=0; sector<angle_sector.Length-1; sector++)
	{
	  if(state_array[sector,annulus] == 1.0f)
	  {
	    closest_sectors.Add(sector);
	  }
	}

	if(!closest_sectors.Count.Equals(0))
	{
	  int index = rand.Next(closest_sectors.Count);
	  //Debug.Log("("+closest_sectors[index].ToString()+","+annulus.ToString()+")");	  
	  return closest_sectors[index]+annulus*(angle_sector.Length-1);
	}
      }
      return (int)0;
    }

    /* --------------------
       get state (encoded, egocentric)       
    --------------------- */
    int get_state()
    {
        cast_ray();//cast ray around the agent and fill up sensor matrix
        get_color_list();
        return code_state();
    }

    /* ------------------------
        get reward
    ----------------------------*/
    void OnTriggerEnter(Collider col)
    {
      reward = -1;
        if (turnOnTriangleIndicator)
        {
            show_collision.TriggerIndicator();
        }
        
      Debug.Log("Collision!");
    }

    void OnTriggerExit(Collider col)
    {
        if (turnOnTriangleIndicator)
        {
            show_collision.UntriggerIndicator();
        }
        
        reward = 0;
    }

    int get_reward()
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
    float action_to_angle(int actionIndex)
    {
        if (actionIndex == 0)                 return -90.0f;
        else if (actionIndex == 1)            return -60.0f;
        else if (actionIndex == 2)            return -30.0f;
        else if (actionIndex == 3)            return 0.0f;
        else if (actionIndex == 4)            return 30.0f;
        else if (actionIndex == 5)            return 60.0f;
        else if (actionIndex == 6)            return 90.0f;
	else return -1.0f;
    }

    public void turn_triangle_indicator(bool flag)
    {
        turnOnTriangleIndicator = flag;
    }
}
