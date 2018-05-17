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
    private static double DEG_TO_RAD = System.Math.PI / 180.0f;
    public bool RandomReset = false;

    // reinforcement learning variables 
    int _action = 0;
    float reward_collision = 0, reward_goal;

    // ego-centric state-space parameters
    float[,,] state_array;
    int num_disc=6, num_sector=8;

    float[] disc_radii;
    float[] sector_angles;
    
    // allo-centric state-space parameters
    float[,] placecell;
    int NUM_PC = 225;
    float M = 15.0f, N = 15.0f;
    float PC_SIZE = 2.5f; //size of place-cell
    float pc_sigma = 2.0f; //variance of place-cell Gaussian
    float potential_previous = -1.0f;

    // action-space parameters
    float[] speed = new float[] { 1.0f};
    public bool VizTrail = false;

    // visualization parameters
    List<Vector2> colorList = new List<Vector2>();
    float[] color_sector_angles = new float[] {75,90,105}; 
    ShowCollision show_collision;
    private string[] ray_colour_code = new string[] { "ANNULUS_COLOUR", "SECTOR_COLOUR" };
    public bool vizCollisionCells = false;

    // draw circle for place-cells
    public bool vizPlaceCell = false;
    public DrawCircle _circlePrefab;
    List<DrawCircle> circleToDraw = new List<DrawCircle>();

    // draw reward indication
    bool turnOnTriangleIndicator;

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
    int trial_elasped = 0; //in sec  

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
	    sector_angles[sector] = Mathf.Pow(baseline,sector)*baseline_theta/Mathf.PI*180.0f - baseline_theta/Mathf.PI*180.0f;
	
	for (int sector=0; sector<num_sector; sector++)
	    sector_angles[num_sector+sector] = - Mathf.Pow(baseline,sector)*baseline_theta/Mathf.PI*180.0f + baseline_theta/Mathf.PI*180.0f; 
	
	if (vizCollisionCells)
	{
	    for (int sector=0; sector<num_sector*2; sector++)
		for (int disc=0; disc<num_disc; disc++)
		{
		    Debug.Log("SEC["+(sector).ToString()+"]"+sector_angles[sector]);

		    float theta_rad = Mathf.Deg2Rad*sector_angles[sector];		    
		    
		    float r1 = disc_radii[disc]*(baseline-1.0f/baseline)/2.0f;
		    float r2 = 0.5f*Mathf.Sqrt(2-2*Mathf.Cos(theta_rad*baseline)*Mathf.Cos(theta_rad/baseline)-2*Mathf.Sin(theta_rad*baseline)*Mathf.Sin(theta_rad/baseline));
		    
		    circleToDraw.Add(Instantiate(_circlePrefab));
                    circleToDraw[sector*num_disc+disc].SetupCircle
			(new Vector3( disc_radii[disc]*Mathf.Cos(theta_rad),
				      UnityEngine.Random.Range(0.49f,0.51f),
				      disc_radii[disc]*Mathf.Sin(theta_rad)),
			 (r1+r2)/2.0f/1.0f,
			 new Color(0.0f, 0.750f, 0.0f));
                }
	}
	
        state_array = new float[sector_angles.Length, disc_radii.Length, 10]; //TODO: dicretize angle generically

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

                placecell[pc_idx, 0] = (x - M / 2.0f + 0.25f * (2.0f * (y % 2) - 1.0f)) * PC_SIZE; 
                placecell[pc_idx, 1] = (y - N / 2.0f - 0.5f) * PC_SIZE;
                placecell[pc_idx, 2] = 0.0f;

                if (vizPlaceCell)
                {
                    circleToDraw.Add(Instantiate(_circlePrefab));
                    circleToDraw[pc_idx].SetupCircle
                    (new Vector3(placecell[pc_idx, 0], 0.0f, placecell[pc_idx, 1]), pc_sigma, new Color(placecell[pc_idx, 2], 1 - placecell[pc_idx, 2], 0.0f));
                }

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

    public void set_udp(float[] motor_command)
    {
	do_jump_action(motor_command[0]*180.0f/Mathf.PI, motor_command[1]);
    }

    // instantaneous translation+rotation
    void do_jump_action(float action_angle, float action_speed)
    {
        Vector3 ray_origin = transform.position;
        transform.rotation = Quaternion.Euler(0.0f, action_angle, 0.0f); //global rotate 	

        Vector3 next_position = gameObject.GetComponent<Rigidbody>().position + gameObject.transform.forward * (action_speed);

        // FIND IF IN BOUNDARY PLACE-CELL
        if (at_edge(next_position)) return;

        gameObject.GetComponent<Rigidbody>().position += gameObject.transform.forward * (action_speed);
        gameObject.GetComponent<Rigidbody>().velocity = Vector3.zero;
	
        // visualize trail
        Vector3 ray_vector = next_position;
        if(VizTrail) DrawLine(ray_origin, ray_vector, Color.red, 25);
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
        udp_out.Add(ego_state.Count);
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
    void cast_ray_old()
    {
        colorList.Clear();

        // cast a ray around the agent 
        RaycastHit[] hits;
	int cc_idx = 0;

        float ray_length = disc_radii[disc_radii.Length - 1];

        // reset states
        for (int sector = 0; sector < sector_angles.Length - 1; sector++)
            for (int annulus = 0; annulus < disc_radii.Length; annulus++)
                for (int relative_angle = 0; relative_angle < 10; relative_angle++)
                    state_array[sector, annulus, relative_angle] = 0.0f;

        // each ray around the agent (with some angle)
        for (float ray_angle = Mathf.Min(sector_angles); ray_angle <= Mathf.Max(sector_angles); ray_angle += RAYCAST_INTERVAL)
        {
            Vector3 ray_origin = AGENT_HEIGHT * Vector3.up + transform.position; // set origin of ray near the eye of agent
            Vector3 ray_vector = Quaternion.AngleAxis(ray_angle, Vector3.up) * transform.forward; //set ray vector with iterant angle
            hits = Physics.RaycastAll(ray_origin, ray_vector, ray_length); //find raycast hit	

            // each hit of a ray
            for (int i = 0; i < hits.Length; i++)
            {
                RaycastHit hit = hits[i];
                Vector3 velocity_obstacle;

                if (hits[i].collider.gameObject.tag == "pedestrian" && hits[i].collider.gameObject != this.gameObject) // if not another agent
                {
                    velocity_obstacle = hits[i].collider.gameObject.GetComponent<QAgent>().get_velocity();
                }
                else if (hits[i].collider.gameObject.tag == "wall")
                {
                    velocity_obstacle = Vector3.zero;
                }
                else
                {
                    continue;
                }

                Vector3 velocity_relative = velocity_obstacle - get_velocity();
                float relative_angle = Mathf.Round(Vector3.Angle(velocity_relative, transform.forward));
                int relative_angle_idx = (int)relative_angle / 36;
                state_array[angle_to_sector(ray_angle), distance_to_annulus(hit.distance), relative_angle_idx] = 1.0f; //hits[i].distance;

		if(vizCollisionCells)
		{
		    circleToDraw[angle_to_sector(ray_angle)*num_disc+distance_to_annulus(hit.distance)].UpdateMeshColor(new Color(1, 1, 0.0f));

		    //colorList.Add(new Vector2(distance_to_annulus(hit.distance), angle_id(ray_angle)));		    
		    //Vector3 ray_origin_rel = AGENT_HEIGHT*Vector3.up + hits[i].collider.gameObject.transform.position; 
		    //Vector3 ray_vector_rel = Quaternion.AngleAxis(theta, Vector3.up) * transform.forward; //egocentric
		    //Vector3 ray_vector_rel = Quaternion.AngleAxis(10, Vector3.up) *Vector3.forward; //allocentric
		    //Debug.DrawRay(ray_origin_rel, ray_vector_rel*3, Color.green);
		}		
            }
        }
    }

    void cast_ray()
    {
        colorList.Clear();

        // cast a ray around the agent 
        RaycastHit[] hits;
	int cc_idx = 0;

        float ray_length = disc_radii[disc_radii.Length - 1];

	// reset states
        for (int sector = 0; sector < num_sector*2; sector++)
            for (int disc = 0; disc < num_disc; disc++)
	    {
		//circleToDraw[sector*num_disc+disc].UpdateMeshColor(new Color(0, 0.75f, 0.0f));
		
                for (int relative_angle = 0; relative_angle < 10; relative_angle++)
                    state_array[sector, disc, relative_angle] = 0.0f;
	    }
	
        // each ray around the agent (with some angle)
        for (float ray_angle = Mathf.Min(sector_angles); ray_angle <= Mathf.Max(sector_angles); ray_angle += RAYCAST_INTERVAL)
        {
            Vector3 ray_origin = AGENT_HEIGHT * Vector3.up + transform.position; // set origin of ray near the eye of agent
            Vector3 ray_vector = Quaternion.AngleAxis(ray_angle, Vector3.up) * transform.forward; //set ray vector with iterant angle
            hits = Physics.RaycastAll(ray_origin, ray_vector, ray_length); //find raycast hit	

            // each hit of a ray
            for (int i = 0; i < hits.Length; i++)
            {
                RaycastHit hit = hits[i];
                Vector3 velocity_obstacle;

                if (hits[i].collider.gameObject.tag == "pedestrian" && hits[i].collider.gameObject != this.gameObject) // if not another agent
                {
                    velocity_obstacle = hits[i].collider.gameObject.GetComponent<QAgent>().get_velocity();
                }
                else if (hits[i].collider.gameObject.tag == "wall")
                {
                    velocity_obstacle = Vector3.zero;
                }
                else
                {
                    continue;
                }

                Vector3 velocity_relative = velocity_obstacle - get_velocity();
                float relative_angle = Mathf.Round(Vector3.Angle(velocity_relative, transform.forward));
                int relative_angle_idx = (int)relative_angle / 36;
                state_array[angle_to_sector(ray_angle), distance_to_annulus(hit.distance), relative_angle_idx] = 1.0f; //hits[i].distance;

		Debug.Log("RAYYED!!");
		
		if(vizCollisionCells)
		{
		    circleToDraw[angle_to_sector(ray_angle)*num_disc+distance_to_annulus(hit.distance)].UpdateMeshColor(new Color(1.0f, 0.0f, 0.0f));

		    //colorList.Add(new Vector2(distance_to_annulus(hit.distance), angle_id(ray_angle)));		    
		    //Vector3 ray_origin_rel = AGENT_HEIGHT*Vector3.up + hits[i].collider.gameObject.transform.position; 
		    //Vector3 ray_vector_rel = Quaternion.AngleAxis(theta, Vector3.up) * transform.forward; //egocentric
		    //Vector3 ray_vector_rel = Quaternion.AngleAxis(10, Vector3.up) *Vector3.forward; //allocentric
		    //Debug.DrawRay(ray_origin_rel, ray_vector_rel*3, Color.green);
		}		
            }
        }
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

            if (vizPlaceCell) circleToDraw[pc_idx].UpdateMeshColor(new Color(placecell[pc_idx, 2], 1 - placecell[pc_idx, 2], 0.0f));
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
                pc_idx++;
            }
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
            if (placecell[pc_idx, 2] > 0.05)
            {
                phi.Add(pc_idx);
                phi.Add(placecell[pc_idx, 2]);
            }
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

        for (int annulus = 0; annulus < disc_radii.Length; annulus++)
        {
            for (int sector = 0; sector < sector_angles.Length - 1; sector++)
            {
                for (int relative_angle_idx = 0; relative_angle_idx < 10; relative_angle_idx++)
                {
                    if (state_array[sector, annulus, relative_angle_idx] > 0.0f)
                    {
                        phi.Add(sector + annulus * (sector_angles.Length - 1) + relative_angle_idx * (sector_angles.Length - 1) * (disc_radii.Length));
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
        //cast_ray();
        //get_color_list();
        return code_feature();
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
            //Debug.Log(potential_previous+"->"+Vector3.Distance(goalObject.transform.position,transform.position)+"="+Mathf.Max(Mathf.Min(reward_shape,1.0f),-1.0f));
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
        return shapped_reward_goal() + reward_goal;
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
        if (RandomReset) transform.position = new Vector3(UnityEngine.Random.Range(-15, 15), 0, UnityEngine.Random.Range(-15, 15));
        else  transform.position = defaultLocation;

        transform.rotation = Quaternion.Euler(0.0f, UnityEngine.Random.Range(0, 360), 0.0f);

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

    // Get which sector has collision
    float angle_id(float angle)
    {
        for (int i = 0; i < color_sector_angles.Length - 1; i++)
        {
            if (color_sector_angles[i] <= angle && angle <= color_sector_angles[i + 1])
            {
                return (float)i;
            }
        }
        return (float)(color_sector_angles.Length - 1);
    }

    // Return color list to draw sector
    public List<Vector2> get_color_list()
    {
        return colorList;
    }

    // Get radius array
    public float[] get_radius_array()
    {
        return disc_radii;
    }

    // Get angle array
    public float[] get_anlge_array()
    {
        return color_sector_angles;
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
            Debug.Log(actionIndex);
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
        lr.SetWidth(0.1f, 0.1f);
        lr.SetPosition(0, start);
        lr.SetPosition(1, end);
        GameObject.Destroy(myLine, duration);
    }

}
