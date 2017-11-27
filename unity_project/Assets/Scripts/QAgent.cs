using UnityEngine;
#if UNITY_EDITOR
using UnityEditor;
#endif
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System;

public class QAgent : MonoBehaviour
{
    private static double DEG_TO_RAD = System.Math.PI / 180.0f;

    float[] radius_annulus = new float[] { 10.0f, 20.0f };
    int[] angle_sector = new int[] { -45, -15, 15, 45, 315 };

    float[,] state_array;

    int action = 0;
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
    float time_per_update = 1.0F; //in sec
    float trial_duration = 10.0F;
    int trial_elasped = 0; //in sec

    /* UDP socket and channel set-up */

    public Socket socket = null;
    private string IP;  // define in init
    string strMessage = "";

    string THIS_IP = "127.0.0.1";
    string BESKOW_IP = "193.11.167.133";
    int PORT = 7890;

    /* constants that you don't have to care about */
    float AGENT_HEIGHT = 3.0f; // to set-up raycast visuals 
    int RAYCAST_INTERVAL = 10;


    // Color of Sector setup
    List<Vector2> colorList = new List<Vector2>();
    float[] color_radius_annulus = new float[] { 10.0f, 20.0f };
    float[] color_angle_sector = new float[] { -45.0f, -15.0f, 15.0f, 45.0f };


    // Set rate
    void Awake()
    {
        Application.targetFrameRate = frame_rate;
        Time.fixedDeltaTime = time_per_update;
        Screen.fullScreen = true;
    }

    // Intializating Unity
    void Start()
    {
        QualitySettings.vSyncCount = 0;

        /* UDP set-up */

        socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
        IPEndPoint brain_EP = new IPEndPoint(IPAddress.Any, PORT);
        socket.Bind(brain_EP);

        /* agent-env set-up */

        state_array = new float[angle_sector.Length - 1, radius_annulus.Length];
        start_position = transform.position;
    }

    public void setTimeScale(float global_time_scale)
    {
        time_scale = global_time_scale;
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
        if (action == 7) //halt
        {
            gameObject.transform.Rotate(new Vector3(0, 0, 0) * Time.fixedDeltaTime);
            gameObject.transform.position += gameObject.transform.forward * (0.0F * Time.fixedDeltaTime);
        }
        else
        {
            int actionSelected = action;
            float turning_speed = action_to_angle(action) / time_per_update;

            gameObject.transform.Rotate(new Vector3(0, turning_speed, 0) * Time.deltaTime);
            gameObject.transform.position += gameObject.transform.forward * (speed_default * Time.deltaTime);
        }
    }

    // Update is called once per frame
    void Update()
    {
        UpdateIndex++;

        float avgFrameRate = Time.frameCount / Time.time;

        Application.targetFrameRate = frame_rate;
        Time.timeScale = time_scale;

        if (action >= 0 && action < 8)
            do_action(action, 1.5f);
        else
        {
            Debug.Log("Update Error: invalid action range..");
            do_action(7, 0.0f);
        }
    }

    void FixedUpdate()
    {
        FixedUpdateIndex++;

        if (FixedUpdateIndex % 1000 == 0)
            Debug.Log(FixedUpdateIndex.ToString());

        // check for active UDP queue
        if (socket.Available <= 0)
        {
            //Debug.Log("FixedUpdate Error: UDP connection unavailabe!");
            action = 7;
            return;
        }

        // check for reset 
        if (Time.fixedTime >= trial_duration * trial_elasped)
        {
            trial_elasped++;
            reset();
        }

        // execute brain update
        try
        {
            // receiving action commands
            IPEndPoint sender = new IPEndPoint(IPAddress.Any, 0);
            EndPoint Remote = (EndPoint)(sender);

            byte[] data_in = new byte[256]; ;
            socket.ReceiveFrom(data_in, 256, 0, ref Remote);
            string text = Encoding.UTF8.GetString(data_in);
            action = int.Parse(text);

            // sending state+reward data
            int state_reward = get_state();
            if (get_reward() == -1) state_reward = -state_reward;

            byte[] data_out = Encoding.UTF8.GetBytes(state_reward.ToString());
            socket.SendTo(data_out, SocketFlags.None, Remote);
        }
        catch (Exception err)
        {
            Debug.Log(err.ToString() + " FixedUpdate unknown error: not receiving brain signal..");
            action = 7;
        }

        //Vector3 ray_origin = AGENT_HEIGHT*Vector3.up + transform.position; 
        //Vector3 ray_vector = Quaternion.AngleAxis(action_to_angle(action), Vector3.up) * transform.forward;
        //Debug.DrawRay(ray_origin, ray_vector * 2, Color.green); 
    }

    void reset()
    {
        Quaternion IntelligentAgentRotation = Quaternion.Euler(0, 0, 0);
        Vector3 IntelligentAgentPosition = new Vector3(-140.0f, 0.0f, 215.0f);

        transform.position = IntelligentAgentPosition;
        transform.rotation = IntelligentAgentRotation;
        //Debug.Log("Reset!");
    }

    /* --------------------- 
       castray
    --------------------- */
    void cast_ray()
    {
        colorList.Clear();
        // cast a ray around the agent 
        RaycastHit[] hits;

        float ray_length = radius_annulus[radius_annulus.Length - 1];

        // reset states
        for (int sector = 0; sector < angle_sector.Length - 1; sector++)
            for (int annulus = 0; annulus < radius_annulus.Length; annulus++)
                state_array[sector, annulus] = 0.0f;

        // each ray around the agent (with some angle)
        for (int ray_angle = Mathf.Min(angle_sector); ray_angle <= Mathf.Max(angle_sector); ray_angle += RAYCAST_INTERVAL)
        {
            Vector3 ray_origin = AGENT_HEIGHT * Vector3.up + transform.position; // set origin of ray near the eye of agent
            Vector3 ray_vector = Quaternion.AngleAxis(ray_angle, Vector3.up) * transform.forward; //set ray vector with iterant angle
            hits = Physics.RaycastAll(ray_origin, ray_vector, ray_length); //find raycast hit	

            Debug.DrawRay(ray_origin, ray_vector * ray_length, Color.green); // paint all rays green

            // each hit of a ray
            for (int i = 0; i < hits.Length; i++)
            {
                RaycastHit hit = hits[i];

                // TODO: make a distance_to_annulus function
                if (hit.distance < radius_annulus[0]) //inner annulus
                {
                    state_array[angle_to_sector(ray_angle), 0] = 1.0f;
                    Debug.DrawRay(ray_origin, ray_vector * ray_length, Color.red); // paint all nearer hit rays orange
                                                                                   //Debug.Log("("+ray_angle+","+hit.distance.ToString()+") -> ("+angle_to_sector(ray_angle).ToString()+",0)");

                    //List of sector colors
                    colorList.Add(new Vector2(0.0f, angle_id(ray_angle)));

                }
                else //outer annulus
                {
                    state_array[angle_to_sector(ray_angle), 1] = 1.0f;
                    Debug.DrawRay(ray_origin, ray_vector * ray_length, Color.yellow); // paint all hit rays orange	  
                                                                                      //Debug.Log("("+ray_angle+","+hit.distance.ToString()+") -> ("+angle_to_sector(ray_angle).ToString()+",1)");

                    //List of sector colors
                    colorList.Add(new Vector2(1.0f, angle_id(ray_angle)));
                }
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
        return color_radius_annulus;
    }

    // Get angle array
    public float[] get_anlge_array()
    {
        return color_angle_sector;
    }

    /* --------------------
       convert sensory state matrix to coded state:
       TODO: name this style
    --------------------- */
    int code_state_halit()
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
       distributed: 
    --------------------- */
    int code_state()
    {
        float state = 0;
        int unit_tracker = 0;
        for (int sector = 0; sector < angle_sector.Length - 1; sector++)
            for (int annulus = 0; annulus < radius_annulus.Length; annulus++)
                state += state_array[sector, annulus] * Mathf.Pow(2, unit_tracker++);
        return (int)state;
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
        //Debug.Log("Collision!");
    }

    void OnTriggerExit(Collider col)
    {
        reward = 0;
    }

    int get_reward()
    {
        return reward;
    }


    /* utilities: ray angle to sector index */
    int angle_to_sector(int angle)
    {
        // angle_sector = {-45,-15,15,45,405};

        for (int sector = 0; sector < angle_sector.Length - 1; sector++)
        {
            if (angle > angle_sector[sector] && angle <= angle_sector[sector + 1])
                return sector;
        }
        return 0;
    }

    /* utilities: get angle from action coding */
    float action_to_angle(int actionIndex)
    {
        if (actionIndex == 0) return -45.0f;
        else if (actionIndex == 1) return -30.0f;
        else if (actionIndex == 2) return -15.0f;
        else if (actionIndex == 3) return 0.0f;
        else if (actionIndex == 4) return 15.0f;
        else if (actionIndex == 5) return 30.0f;
        else if (actionIndex == 6) return 45.0f;
        else return -1.0f;
    }
}
