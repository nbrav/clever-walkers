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
    public Rigidbody rb;
    public bool RandomReset = false;

    GameObject[] _agents;

    /* reinforcement learning parameters */

    // ego-centric state-space parameters
    int num_disc=4, num_sector=15, num_relvel=8;
    float[] disc_radii, sector_angles;
    float[,] collisioncell; // [cc_x, cc_y, sigma_radial, sigma_tangential, value, direction]

    // draw ellipe for collison-cells
    public bool vizCollisionCells = false;
    public DrawEllipse _ellipsePrefab;
    List<DrawEllipse> ellipseCollisionCell = new List<DrawEllipse>();

    // allo-centric state-space parameters
    int NUM_PC = 225;
    float M = 15.0f, N = 15.0f;
    float[,] placecell;
    float PC_SIZE = 2.5f; //size of place-cell
    float pc_sigma = 2.5f; //variance of place-cell Gaussian
    float potential_previous = -1.0f;

    // draw circle for place-cells
    public bool vizPlaceCell = false;
    public DrawCircle _circlePrefab;
    List<DrawCircle> circlePlaceCell = new List<DrawCircle>();

    // action-space parameters
    float[] speed = new float[] {1.0f};
    int _action = 0;
    public bool VizTrail = false;

    // reward parameters
    float reward_collision = 0, reward_goal;
    bool turnOnTriangleIndicator;
    ShowCollision show_collision;

    // whatever this is for..
    //[SerializeField]
    //GameObject dummyNavMeshAgent;

    Vector3 defaultLocation;
    Quaternion defaultPose;
    GameObject goalObject;

    /* timing setup */

    //float time_scale = 1.0F;

    /* velocity component */
    
    Vector3 position_previous;

    /* agent simulation details */

    float AGENT_HEIGHT = 0.0f; // to set-up raycast visuals 
    float RAYCAST_INTERVAL = Mathf.Deg2Rad*5.0f;
    public Color thisColor = new Color(0.5F, 0.5F, 0.5F, 0.5F);
 
    // Set rate
    void Awake()
    {
        /* visualize egocentric states */

        Transform[] trans = this.gameObject.GetComponentsInChildren<Transform>(true);
        foreach (Transform t in trans)
        {
            if (t.name == "Canvas")
            {
                show_collision = t.GetComponent<ShowCollision>();
            }
        }
        show_collision.UntriggerIndicator(Color.red);
        show_collision.UntriggerIndicator(Color.blue);
    }

    // Intializating Unity
    void Start()
    {
        QualitySettings.vSyncCount = 0;

	/* egocentric state set-up */

	disc_radii = new float[num_disc]; 
	sector_angles = new float[num_sector*2]; 
	
	float baseline = 1.5f;

	for (int disc=0; disc<num_disc; disc++)
	    disc_radii[disc] = Mathf.Pow(baseline, disc);
	
	float baseline_theta = Mathf.PI/Mathf.Pow(baseline,num_sector-1);
	
	for (int sector=0; sector<num_sector; sector++)
	    sector_angles[sector] = (Mathf.Pow(baseline,sector)-1)*baseline_theta;
	
	for (int sector=0; sector<num_sector; sector++)
	    sector_angles[num_sector+sector] = - (Mathf.Pow(baseline,sector)-1)*baseline_theta;

        collisioncell = new float[num_sector*2*num_disc*8, 6];
	
	/* EQUI-COLLISION_CELLS
	disc_radii = new float[] {1.0f,2.0f,4.0f,8.0f};
	
	for (int sector=0; sector<num_sector; sector++)
	    sector_angles[sector] = sector*Mathf.PI/num_sector;
	
	for (int sector=0; sector<num_sector; sector++)
	    sector_angles[num_sector+sector] = -(sector+1)*Mathf.PI/num_sector;
	*/

	for (int sector=0; sector<num_sector*2; sector++)
	    for (int disc=0; disc<num_disc; disc++)
	    {		
		float radius_radial = Mathf.Pow(baseline,disc)*(baseline-1.0f/baseline)/2.0f;
		float radius_tangential = Mathf.Pow(baseline,sector%num_sector)*(baseline-1.0f/baseline)/2.0f*baseline_theta;

		int cc_idx = sector*(num_disc) + disc;
		
		collisioncell[cc_idx,0] = disc_radii[disc]*Mathf.Cos(sector_angles[sector]);
		collisioncell[cc_idx,1] = disc_radii[disc]*Mathf.Sin(sector_angles[sector]);
		collisioncell[cc_idx,2] = radius_radial;
		collisioncell[cc_idx,3] = radius_tangential;
		collisioncell[cc_idx,4] = 0.0f;
		collisioncell[cc_idx,4] = -1.0f;
		
		if(vizCollisionCells)
		{
		    ellipseCollisionCell.Add(Instantiate(_ellipsePrefab));
		    
		    ellipseCollisionCell[sector*num_disc+disc].SetupEllipse
			(radius_radial, radius_tangential,
			 new Vector3( disc_radii[disc]*Mathf.Sin(-sector_angles[sector]),
				      UnityEngine.Random.Range(-0.1f, 0.1f),
				      disc_radii[disc]*Mathf.Cos(sector_angles[sector])),
			 new Color(0.0f, 1.0f, 0.0f, 0.1f),
			 this.gameObject);
		}
	    }

        /* allcentric place-cell set-up */

        placecell = new float[NUM_PC, 3];

        int pc_idx = 0;
        for (float x = 0.0f; x<M; x++)
        {
            for (float y = 0.0f; y<N; y++)
            {
                if (pc_idx > NUM_PC)
                {
                    Debug.Log("Wrong index while initializing place-cells!!");
                    break;
                }

                placecell[pc_idx, 0] = (x - M/2.0f + 0.25f*(2.0f*(y%2) - 1.0f))*PC_SIZE + 1.0f; 
                placecell[pc_idx, 1] = (y - N/2.0f - 0.5f)*PC_SIZE + 2.0f;
                placecell[pc_idx, 2] = 0.0f;

                if (vizPlaceCell)
                {
                    circlePlaceCell.Add(Instantiate(_circlePrefab));
                    circlePlaceCell[pc_idx].SetupCircle
			(new Vector3(placecell[pc_idx, 0], 0.0f, placecell[pc_idx, 1]),
			 pc_sigma,
			 new Color(0.5f, 0.0f, 0.0f, 0.5f),
			 null);
                }

                pc_idx++;
            }
        }

        /* rigidbody setup */

        rb = this.gameObject.GetComponent<Rigidbody>();
        rb.constraints = RigidbodyConstraints.FreezePositionY | RigidbodyConstraints.FreezeRotationX | RigidbodyConstraints.FreezeRotationY | RigidbodyConstraints.FreezeRotationZ;
    }

    public void set_parent_array(GameObject[] agents)
    {
	_agents = agents;
    }
    
    public void setResetPose(Vector3 location, Quaternion pose)
    {
        defaultLocation = location;
        defaultPose = pose;
    }

    /*public void setTimeScale(float global_time_scale)
    {
        time_scale = global_time_scale;
    }*/

    /*public void setDummyAgentPrefab(GameObject newPref)
    {
        dummyNavMeshAgent = newPref;
    }*/

    public void setGoal(GameObject goal)
    {
        goalObject = goal;
    }

    /*---------------------------------------------------------------
                             motor system
    ---------------------------------------------------------------*/

    public void set_udp(float[] motor_command, bool pause)
    {
	if(pause)
	    do_jump_action(0.0f, 0.0f);
	else    
	    do_jump_action(motor_command[0]*180.0f/Mathf.PI, motor_command[1]);
    }

    // instantaneous translation+rotation
    void do_jump_action(float action_angle, float action_speed)
    {
        Vector3 ray_origin = transform.position;

        transform.rotation = Quaternion.Euler(0.0f, action_angle, 0.0f); //global rotate 	

        Vector3 next_position = gameObject.GetComponent<Rigidbody>().position + gameObject.transform.forward * (action_speed);

        if (at_edge(next_position)) return;

        gameObject.GetComponent<Rigidbody>().position += gameObject.transform.forward * (action_speed);
        gameObject.GetComponent<Rigidbody>().velocity = Vector3.zero;
	
        // visualize trail
        Vector3 ray_vector = next_position;
        if(VizTrail) DrawLine(ray_origin, ray_vector, thisColor, 25);
    }

    /*---------------------------------------------------------------
                             sensory system
    ---------------------------------------------------------------*/

    public List<float> get_udp()
    {
	List<float> udp_out = new List<float>();

        List<float> allo_state = get_features_allocentric();
        udp_out.Add(allo_state.Count / 2);
        udp_out.AddRange(allo_state);

        List<float> ego_state = get_features_egocentric();
        udp_out.Add(ego_state.Count / 2);
        udp_out.AddRange(ego_state);

        udp_out.Add(get_reward_goal());
        udp_out.Add(get_reward_collision());
        udp_out.Add(transform.localEulerAngles.y); //heading direction

        return udp_out;
    }

    public Vector3 get_velocity()
    {
        Vector3 velocity = (transform.position - position_previous) / Time.fixedDeltaTime;
        position_previous = transform.position;
        return velocity;
    }

    /* --------------------- 
       castray
    --------------------- */
    List<float> cast_ray_simple()
    {
        List<float> phi = new List<float>();
	
	// reset states
        for (int sector = 0; sector < num_sector*2; sector++)
            for (int disc = 0; disc < num_disc; disc++)
		collisioncell[sector*num_disc+disc,4] = 0.0f;
	
        // check all agents that are close worth including
        float sqrTolerance = 4.0f*disc_radii[disc_radii.Length-1]*disc_radii[disc_radii.Length-1];
	
	for(int idx=0; idx<_agents.Length; idx++)
	{
	    Vector3 agent2obs = transform.position-_agents[idx].transform.position;
	    if(_agents[idx]!=this.gameObject && agent2obs.sqrMagnitude<sqrTolerance)
	    {
		float sqrDistanceMin = 10000.0f; // TEMP
		int cc_idxMin = 0;
			    
		Vector3 velocity_relative = _agents[idx].gameObject.GetComponent<QAgent>().get_velocity() - get_velocity();

		float relvel_theta = (num_relvel*Vector3.Angle(velocity_relative, transform.forward)/360.0f);
		float relvel_speed = velocity_relative.magnitude;
		float relvel_idx = relvel_theta;

		for (int sector = 0; sector < num_sector*2; sector++)
		{
		    for (int disc = 0; disc < num_disc; disc++)
		    {
			int cc_idx = sector*num_disc + disc;
			
			Vector3 cc_vect = transform.rotation * new Vector3(collisioncell[cc_idx,1], 0, collisioncell[cc_idx,0]);
			Vector3 cc2obs_cart  = transform.position - _agents[idx].transform.position + cc_vect;	
			Vector2 cc2obs_polar = new Vector2(cc2obs_cart.magnitude, Mathf.Atan2(cc2obs_cart[0], cc2obs_cart[2]));
			
			float sqrDistance = (cc2obs_polar[0]*cc2obs_polar[0]/collisioncell[cc_idx,2]/collisioncell[cc_idx,2] +
					     cc2obs_polar[1]*cc2obs_polar[1]/collisioncell[cc_idx,3]/collisioncell[cc_idx,3]);

			/*// CONTINUOUS
			collisioncell[cc_idx,4] = Mathf.Max(collisioncell[cc_idx,4],
							    Mathf.Exp(-0.5f*sqrDistance));
			collisioncell[cc_idx,5] = relvel_idx;			
			*/

			// BINARY
			if(sqrDistance < sqrDistanceMin)
			{
			    cc_idxMin = cc_idx;
			    sqrDistanceMin = sqrDistance;
			}
		    }
		}
		collisioncell[cc_idxMin,4] = 1.0f; // TEMP
		collisioncell[cc_idxMin,5] = relvel_idx; // TEMP
	    }
	}

	for (int sector = 0; sector < num_sector*2; sector++)
	    for (int disc = 0; disc < num_disc; disc++)
	    {
		int cc_idx = sector*num_disc + disc;

		if(collisioncell[cc_idx,4]>0.05f)
		{
		    phi.Add(collisioncell[cc_idx,5]*(num_sector*2*num_disc) + cc_idx);
		    phi.Add(collisioncell[cc_idx,4]);
		}		
	    }

	if(vizCollisionCells) 
	    for (int sector = 0; sector < num_sector*2; sector++)
		for (int disc = 0; disc < num_disc; disc++)
		    ellipseCollisionCell[sector*num_disc+disc].UpdateMeshColor
			(new Color(0.0f, 1.0f, 0.0f, collisioncell[sector*num_disc+disc,4]));

	return phi;
    }

    /* --------------------- 
       castray
    --------------------- */
    List<float> cast_ray()
    {
        List<float> phi = new List<float>();

	// cast a ray around the agent 
	RaycastHit[] hits;

	float ray_length = disc_radii[num_disc-1];
	
	// reset states
        for (int sector = 0; sector < num_sector*2; sector++)
            for (int disc = 0; disc < num_disc; disc++)
		collisioncell[sector*num_disc+disc,4] = 0.0f;
	
        // check all agents that are close worth including
        float sqrTolerance = 4.0f*disc_radii[disc_radii.Length-1]*disc_radii[disc_radii.Length-1];

	// each ray around the agent (with some angle)
	for (float ray_angle=Mathf.Min(sector_angles); ray_angle<=Mathf.Max(sector_angles); ray_angle += RAYCAST_INTERVAL)
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
		    velocity_obstacle = hits[i].collider.gameObject.GetComponent<QAgent>().get_velocity();
		else if (hits[i].collider.gameObject.tag=="wall")
		    velocity_obstacle = Vector3.zero;
		else
		    continue;
	  
		Vector3 agent2obs = transform.position-hits[i].collider.gameObject.transform.position;

		float sqrDistanceMin = 10000.0f; // TEMP
		int cc_idxMin = 0;
		
		Vector3 velocity_relative = velocity_obstacle - get_velocity();
		
		float relvel_theta = (num_relvel*Vector3.Angle(velocity_relative, transform.forward)/360.0f);
		float relvel_speed = velocity_relative.magnitude;
		float relvel_idx = relvel_theta;
		
		for (int sector = 0; sector < num_sector*2; sector++)
		{
		    for (int disc = 0; disc < num_disc; disc++)
		    {
			int cc_idx = sector*num_disc + disc;
			
			Vector3 cc_vect = transform.rotation * new Vector3(collisioncell[cc_idx,1], 0, collisioncell[cc_idx,0]);
			Vector3 cc2obs_cart  = transform.position - hits[i].collider.gameObject.transform.position + cc_vect;	
			Vector2 cc2obs_polar = new Vector2(cc2obs_cart.magnitude, Mathf.Atan2(cc2obs_cart[0], cc2obs_cart[2]));
			
			float sqrDistance = (cc2obs_polar[0]*cc2obs_polar[0]/collisioncell[cc_idx,2]/collisioncell[cc_idx,2] +
					     cc2obs_polar[1]*cc2obs_polar[1]/collisioncell[cc_idx,3]/collisioncell[cc_idx,3]);
			
			/* CONTINUOUS
			   collisioncell[cc_idx,4] = Mathf.Max(collisioncell[cc_idx,4],
							    Mathf.Exp(-0.5f*sqrDistance));
			   collisioncell[cc_idx,5] = relvel_idx;			
			*/

			// BINARY
			if(sqrDistance < sqrDistanceMin)
			{
			    cc_idxMin = cc_idx;
			    sqrDistanceMin = sqrDistance;
			}
		    }
		}
		collisioncell[cc_idxMin,4] = 1.0f; // TEMP
		collisioncell[cc_idxMin,5] = relvel_idx; // TEMP
	    }
	}

	for (int sector = 0; sector < num_sector*2; sector++)
	    for (int disc = 0; disc < num_disc; disc++)
	    {
		int cc_idx = sector*num_disc + disc;

		if(collisioncell[cc_idx,4]>0.05f)
		{
		    phi.Add(collisioncell[cc_idx,5]*(num_sector*2*num_disc) + cc_idx);
		    phi.Add(collisioncell[cc_idx,4]);
		}		
	    }

	if(vizCollisionCells) 
	    for (int sector = 0; sector < num_sector*2; sector++)
		for (int disc = 0; disc < num_disc; disc++)
		    ellipseCollisionCell[sector*num_disc+disc].UpdateMeshColor
			(new Color(0.0f, 1.0f, 0.0f, collisioncell[sector*num_disc+disc,4]));

	return phi;
    }

    /* --------------------- 
       hippocampus place-cell
    --------------------- */
    void simulate_hippocampus()
    {
        for (int pc_idx = 0; pc_idx < NUM_PC; pc_idx++)
        {
            float dis1 = transform.position.x - placecell[pc_idx, 0];
            float dis2 = transform.position.z - placecell[pc_idx, 1];
            placecell[pc_idx, 2] = Mathf.Round(Mathf.Exp(-(dis1 * dis1 + dis2 * dis2) / 2 / pc_sigma / pc_sigma) * 100) / 100;

	    if (vizPlaceCell) circlePlaceCell[pc_idx].UpdateMeshColor(new Color(0.0f, 1.0f,  0.0f, placecell[pc_idx, 2]));
				
        }
    }

    // is the agent at the end of arena
    bool at_edge(Vector3 position)
    {
        float closest_x = -1.0f, closest_y = -1.0f, closest_dist = -1.0f; int pc_idx = 0;

        for (float x = 0.0f; x < M; x++)
        {
            for (float y = 0.0f; y < N; y++)
            {
                float dist1 = position.x - placecell[pc_idx, 0];
                float dist2 = position.z - placecell[pc_idx, 1];
                float dist = Mathf.Round(Mathf.Exp(-(dist1 * dist1 + dist2 * dist2) / 2 / pc_sigma / pc_sigma) * 100) / 100;

                if (dist > closest_dist)
                {
                    closest_x = x; closest_y = y; closest_dist = dist;
                }
                pc_idx++;            }
        }
        if (closest_x == 0 || closest_x == M - 1 || closest_y == 0 || closest_y == N - 1)
            return true;
        else
            return false;
    }

    List<float> code_placecells()
    {
        List<float> phi = new List<float>();

        for (int pc_idx = 0; pc_idx < NUM_PC; pc_idx++)
        {
            if (placecell[pc_idx, 2] > 0.05f)
            {
                phi.Add(pc_idx);
                phi.Add(placecell[pc_idx, 2]);
            }
        }
        return phi;
    }

    /* ---------------------
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
        return cast_ray_simple();
    }

    /*---------------------------------------------------------------
                                 reward system
    ---------------------------------------------------------------*/

    public float reached_goal()
    {
        return reward_goal;
    }

    void OnTriggerEnter(Collider col)
    {
        if (col.gameObject == goalObject)
        {
            if (turnOnTriangleIndicator)
                show_collision.TriggerIndicator(Color.blue);

            reward_goal = 1.0f;
        }

        if (col.gameObject.tag == "pedestrian" || col.gameObject.tag == "wall")
        {
            if (turnOnTriangleIndicator)
                show_collision.TriggerIndicator(Color.red);

            reward_collision = -1.0f;
        }
    }

    void OnTriggerExit(Collider col)
    {
        if (col.gameObject == goalObject)
        {
        }

        if (col.gameObject.tag == "pedestrian" || col.gameObject.tag == "wall")
        {
            if (turnOnTriangleIndicator)
                show_collision.UntriggerIndicator(Color.red);

            reward_collision = 0.0f;
        }
    }

    float shapped_reward_goal()
    {
        if (potential_previous != -1.0f)
        {
            float reward_shape = potential_previous - Vector3.Distance(goalObject.transform.position, transform.position);
            potential_previous = Vector3.Distance(goalObject.transform.position, transform.position);
            return Mathf.Max(Mathf.Min(reward_shape, 1.0f), -1.0f);
        }
        else
        {
            potential_previous = Vector3.Distance(goalObject.transform.position, transform.position);
            return 0.0f;
        }
    }

    float get_reward_goal()
    {
        return shapped_reward_goal(); //reward_goal
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
        //float avgFrameRate = Time.frameCount / Time.time;

        //Application.targetFrameRate = frame_rate;
        //Time.timeScale = time_scale;
    }

    void FixedUpdate()
    {

    }

    public void reset()
    {
        if (RandomReset)
	{
	    transform.position = new Vector3(UnityEngine.Random.Range(-15, 15), 0, UnityEngine.Random.Range(-15, 15));
	    transform.rotation = Quaternion.Euler(0.0f, UnityEngine.Random.Range(0, 360), 0.0f);
	}
        else
	{
	    transform.position = defaultLocation;
	    transform.rotation = defaultPose;
	}

        reward_goal = 0.0f; reward_collision = 0.0f;

        if (turnOnTriangleIndicator)
            show_collision.UntriggerIndicator(Color.blue);

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

    /* utilities: ray angle to sector index */
    int angle_to_sector(float angle)
    {
        for (int sector = 0; sector < sector_angles.Length - 1; sector++)
        {
            if (angle > sector_angles[sector] && angle <= sector_angles[sector + 1])
                return sector;
        }
        return 0;
    }

    /* utilities: ray angle to sector index */
    int distance_to_annulus(float dist)
    {
        for (int annulus = 0; annulus < disc_radii.Length; annulus++)
        {
            if (dist <= disc_radii[annulus])
                return annulus;
        }
        return -1;
    }

    /* utilities: get angle from action coding (in allocentric frame) */
    float action_to_angle(int actionIndex)
    {
        return (float)(actionIndex % 8) * 45.0f;
    }

    /* utilities: get angle from action coding (in allocentric frame) */
    float action_to_speed(int actionIndex)
    {
        if ((int)(Math.Floor((double)actionIndex / 8)) < 0 || (int)(Math.Floor((double)actionIndex / 8)) >= 3)
        {
            return 2;
        }
        return speed[(int)(Math.Floor((double)actionIndex / 8))];
    }

    public void turn_triangle_indicator(bool flag)
    {
        turnOnTriangleIndicator = flag;
    }

    void DrawLine(Vector3 start, Vector3 end, Color color, float duration = 10f)
    {
        GameObject myLine = new GameObject();
        myLine.transform.position = start;
        myLine.AddComponent<LineRenderer>();
        LineRenderer lr = myLine.GetComponent<LineRenderer>();
        lr.material = new Material(Shader.Find("Particles/Alpha Blended Premultiply"));
        lr.SetColors(color, color);
        lr.SetWidth(0.3f, 0.3f);
        lr.SetPosition(0, start);
        lr.SetPosition(1, end);
        GameObject.Destroy(myLine, duration);
    }

}
