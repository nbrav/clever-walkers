using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System;
			      
public class ZombieAgent : MonoBehaviour
{
    private static double DEG_TO_RAD = System.Math.PI/180.0f;

    float[] radius_annulus = new float[] {10.0f, 20.0f};
    int[] angle_sector = new int[] {-45,-15,15,45};
  
    float[,] state_array;
    int action=0;

    private string[] ray_colour_code = new string[] {"ANNULUS_COLOUR", "SECTOR_COLOUR"};
    
    GameObject goalGameObject;

    [SerializeField]
    GameObject dummyNavMeshAgent;

    Vector3 goalAgentPosition;
    Vector3 defaultLocation;
    Quaternion defaultPose;
  
    int reward = 0;
    int FixedUpdateIndex = 0;
    int UpdateIndex = 0;
  
    /* Timing setup */
  
    int frame_rate = 30;
    float time_scale = 1.0F;
    float trial_duration = 20.0F;
    int trial_elasped=0; //in sec

    /* velocity component */
    Vector3 position_previous;

    /* UDP channel initialize */
			  
    private static int localPort;
    private string IP;  // define in init
    public int port;  // define in init
    IPEndPoint remoteEndPoint;
    UdpClient client;   
    string strMessage="";
  
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

    public void setGoalObject(Vector3 newGoalObj)
    {
        goalAgentPosition = newGoalObj;
    }

    public void setDummyAgentPrefab(GameObject newPref)
    {
        dummyNavMeshAgent = newPref;
    }

    public void setSeed(int seed)
    {
      UnityEngine.Random.seed = seed;
    }
  
    void do_action(int action, float speed_default)
    {
        if(action==7) //halt
	{
	  float speed = 0;

	  gameObject.transform.Rotate(new Vector3(0, 0, 0) * Time.deltaTime);
	  gameObject.transform.position += gameObject.transform.forward * (0.0F * Time.deltaTime);
	}
	else
	{
	  int actionSelected = action;
	  
	  gameObject.transform.Rotate(new Vector3(0, action_to_angle(actionSelected), 0) * Time.deltaTime);
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

      action = 3;
      
      do_action(action, 1.5f);
    }

    void FixedUpdate()
    {
      //FixedUpdateIndex++;      
      //Debug.Log("UpdateLatency:"+Time.deltaTime);

      //if(Time.fixedTime>=trial_duration*trial_elasped)
      //{
      //trial_elasped++;
      //reset();
      //}     
    }
  
    public void reset()
    {
      transform.position = defaultLocation;
      transform.rotation = Quaternion.Euler(0.0f,UnityEngine.Random.Range(0,360),0.0f); //defaultPose;
    }

    public Vector3 get_velocity()
    {
      Vector3 velocity = (transform.position-position_previous);///Time.fixedDeltaTime;
      position_previous = transform.position;
      return velocity;
    }
    
    /* ------------------------
        get reward
    ----------------------------*/
    void OnTriggerEnter (Collider col)
    {
      reward = -1;
      //Debug.Log("Collision!");
    }

    void OnTriggerExit (Collider col)
    {
      reward = 0;
    }
  
    /* utilities: get angle from action coding */
    float action_to_angle(int actionIndex)
    {
        if (actionIndex == 0)                 return -45.0f;
        else if (actionIndex == 1)            return -30.0f;
        else if (actionIndex == 2)            return -15.0f;
        else if (actionIndex == 3)            return 0.0f;
        else if (actionIndex == 4)            return 15.0f;
        else if (actionIndex == 5)            return 30.0f;
        else if (actionIndex == 6)            return 45.0f;
	else return -1.0f;
    }
}
