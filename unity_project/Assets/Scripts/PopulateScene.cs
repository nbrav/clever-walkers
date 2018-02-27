using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;
using UnityEngine.UI;
using System.Net.Sockets;
using System.Net;
using System;
		      
public class PopulateScene : MonoBehaviour
{
    public int numOfZombies = 9;
    public int numOfWalkers = 2;
  
    [SerializeField]
    bool Learning;

    [SerializeField]
    bool AnimationOff;

    [SerializeField]
    float LearningTimeScale = 5.0f;

    [SerializeField]
    bool TurnOnSector = false;

    [SerializeField]
    bool TurnOnTriangleIndicator = false;

    [SerializeField]
    GameObject ZombiePrefab;

    [SerializeField]
    GameObject HumanPrefab;

    [SerializeField]
    GameObject agentPrefab;

    GameObject[] agent;

    Vector3 IntelligentAgentPosition;
    Quaternion IntelligentAgentRotation;
    Vector3[] ZombieAgentPosition;
    Quaternion[] ZombieAgentRotation;
				    
    /* RL variables (experimental) */
  
    bool waiting_for_transition = false;
    float[] reset_counter;
  
    /* Episode variables */
  
    int FixedUpdateIndex;
    int trial_elapsed=0;
    int frame_rate = 30;
    float trial_duration = 65.0F; //in sec
    float time_per_update = 0.5F; //in sec

    string tring;
    string display_string;
  
    /* UDP socket and channel set-up */
				
    public Socket[] socket;
    string THIS_IP = "127.0.0.1";
    string BESKOW_IP = "193.11.167.133";
    int PORT = 7890;

    void Awake()
    {
	agent = new GameObject[numOfZombies+numOfWalkers];
	
	IntelligentAgentRotation = Quaternion.Euler(0, 240, 0);
	IntelligentAgentPosition = new Vector3(-140.0f, 0.0f, 210.0f);

        GenerateAgent();
    }
  
    // Use this for initialization
    void Start()
    {
        Application.targetFrameRate = frame_rate;
	Time.fixedDeltaTime = time_per_update;

	reset_counter = new float[numOfWalkers];
	for(int i=0;i<numOfWalkers;i++)
	  reset_counter[i]=trial_duration;
	
	/* UDP set-up */

	socket = new Socket[numOfWalkers];
	for(int idx=0; idx<numOfWalkers; idx++)
	{
	  socket[idx] = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
	  IPEndPoint brain_EP = new IPEndPoint(IPAddress.Any, PORT+idx);
	  socket[idx].Bind(brain_EP);
	}
    }

    // Update is called once per frame
    void Update ()
    {
    }
  
    // FixedUpdate is called every 
    void FixedUpdate()
    {
      IPEndPoint sender;
      EndPoint Remote;
      
      Time.timeScale = LearningTimeScale;
      FixedUpdateIndex++;
      
      if(agent.Length<numOfWalkers+numOfZombies)
	return;

      // check for active UDP queue / or abandon
      for(int idx=0; idx<numOfWalkers; idx++)
      {
	if(socket[idx].Available<=0)
	{	
	  agent[idx].GetComponent<QAgent>().set_udp(-1);
	  return;
	}
      }

      float global_reset=0.0f;
      for (int reset_idx=0; reset_idx<numOfWalkers; reset_idx++)
	global_reset += reset_counter[reset_idx];
	  
      // execute brain update
      try
      {
	display_string = "";

	// receive all action
	for(int idx=0; idx<numOfWalkers; idx++)
	{
	  // finding any client
	  sender = new IPEndPoint(IPAddress.Any, PORT+idx);
	  Remote = (EndPoint)(sender);

	  // receive action
	  byte[] data_in = new byte[256];;
	  
	  socket[idx].ReceiveFrom(data_in,sizeof(int),0,ref Remote);	
	  int action = System.BitConverter.ToInt32(data_in, 0);
      	  
	  // if local reset
	  if(reset_counter[idx]<=0.0f && global_reset!=0.0f)
	  {
	    display_string += "AGENT "+idx.ToString()+" RESETED";

	    agent[idx].GetComponent<QAgent>().set_udp(-1);

	    float[] data_out_float_reset = new float[1];	    
	    data_out_float_reset[0] = float.MaxValue;
	    byte[] data_out_reset = new byte[sizeof(float)];
	    Buffer.BlockCopy(data_out_float_reset, 0, data_out_reset, 0, data_out_reset.Length);		    	    
	    socket[idx].SendTo(data_out_reset, sizeof(float), SocketFlags.None, Remote);
	  }
	  else if(global_reset==0.0f)
	  {
	    agent[idx].GetComponent<QAgent>().reset();

	    float[] data_out_float_reset = new float[2];	    
	    data_out_float_reset[0] = float.MaxValue; data_out_float_reset[1] = float.MaxValue;	    
	    byte[] data_out_reset = new byte[sizeof(float)*2];
	    Buffer.BlockCopy(data_out_float_reset, 0, data_out_reset, 0, data_out_reset.Length);		    	    
	    socket[idx].SendTo(data_out_reset, sizeof(float)*2, SocketFlags.None, Remote);

	    for (int reset_idx=0; reset_idx<numOfWalkers; reset_idx++)
	      reset_counter[reset_idx] = trial_duration;

	    display_string += "GLOBAL RESETED";
	  }
	  else
	  {
	    display_string += "AGENT " + idx.ToString();
	    reset_counter[idx]-=1;//Time.fixedDeltaTime;
	    agent[idx].GetComponent<QAgent>().set_udp(action);
	  }
      
	  // sending state,reward
	  List<float> state_reward;
	  state_reward = agent[idx].GetComponent<QAgent>().get_udp();
	  
	  float[] data_out_int = new float[(state_reward.Count+1)];
	  byte[] data_out = new byte[sizeof(float)*(state_reward.Count+1)];
	  
	  data_out_int[0] = state_reward.Count;
	  for(int idx2=0; idx2<state_reward.Count;idx2++)
	    data_out_int[idx2+1] = state_reward[idx2];

	  if(global_reset!=0.0f && reset_counter[idx]!= 0.0f)
	    if(state_reward[state_reward.Count-3]!=0.0f)
	    {
	      reset_counter[idx]=0.0f;
	    }
	  
	  Buffer.BlockCopy(data_out_int, 0, data_out, 0, data_out.Length);	
	  socket[idx].SendTo(data_out, sizeof(float)*data_out_int.Length, SocketFlags.None,Remote);

	  // display
	  display_string += "COUNTER "+reset_counter[idx].ToString()+","+global_reset.ToString();
	  display_string += " (A:"+action.ToString();
	  display_string += ")-->(S':";
	  for(int obstacle_index=0; obstacle_index<state_reward.Count;obstacle_index++)
	    display_string += " "+state_reward[obstacle_index].ToString();
	  display_string += "&"+state_reward[state_reward.Count-3].ToString();
	  display_string += ","+state_reward[state_reward.Count-2].ToString();
	  display_string += ")";	  
	  display_string += "\n";
	}
      }
      catch (Exception err)
      {
	Debug.Log(err.ToString()+" FixedUpdate unknown error: not receiving brain signal..");
      }
      display_string += "\nTrial:"+trial_elapsed.ToString();
    }

    void createZombieAgent(int index)
    {
        GameObject clone = GameObject.Instantiate(ZombiePrefab) as GameObject;

        clone.GetComponent<ZombieAgent>().setSeed(index);
	clone.GetComponent<ZombieAgent>().reset();
        clone.GetComponent<ZombieAgent>().setDummyAgentPrefab(agentPrefab);

	Vector3 location;
	Quaternion pose;
	
        if(index==1)
	{
	  location = new Vector3(-12.5f,0.0f,5.0f);
	  pose = Quaternion.Euler(0.0f,UnityEngine.Random.Range(0,360),0.0f);
	}
	else if (index==2)
	{
	  location = new Vector3(13.0f,0.0f,10.5f);
	  pose = Quaternion.Euler(0.0f,UnityEngine.Random.Range(0,360),0.0f);
	}
	else if (index==3)
	{
	  location = new Vector3(-10.5f,0.0f,-9.0f);
	  pose = Quaternion.Euler(0.0f,UnityEngine.Random.Range(0,360),0.0f);
	}
	else if (index==4)
	{
	  location = new Vector3(14.0f,0.0f,-5.0f);
	  pose = Quaternion.Euler(0.0f,UnityEngine.Random.Range(0,360),0.0f);
	}
	else
	{
	  location = new Vector3(-13.0f,0.0f,-14.0f);
	  pose = Quaternion.Euler(0.0f,UnityEngine.Random.Range(0,360),0.0f);
	}
	
	clone.GetComponent<ZombieAgent>().setResetPose(location,pose);

	if(Learning)
	{
	  clone.GetComponent<ZombieAgent>().setTimeScale(LearningTimeScale);
	}
	else
	{
	  //clone.GetComponent<ZombieAgent>().setTimeScale(1.0f);
	}

	if(AnimationOff)  Destroy(clone.GetComponent<Animator>());	  
	  
	agent[index] = clone;
    }

    void createSmartAgent(int index)
    {
        GameObject clone = GameObject.Instantiate(HumanPrefab);

	clone.name = "agent"+index.ToString();
	clone.GetComponent<QAgent>().setDummyAgentPrefab(agentPrefab);
	clone.AddComponent<Rigidbody>();

	// agent pose
	Vector3 location = new Vector3(0,0,0);
	Quaternion pose;
	
	// goal object
	GameObject sphere = GameObject.CreatePrimitive(PrimitiveType.Sphere);	      
	sphere.name = "goal"+index.ToString();
	
	pose = Quaternion.Euler(0.0f,UnityEngine.Random.Range(0,360),0.0f);

	float square_dist = 7.5f;
	
	if(index==0)
	{
	  location = new Vector3(2-square_dist,0,2-square_dist);
	  sphere.transform.position = new Vector3(square_dist, 1.5F, square_dist);
	}
	else if(index==1)
	{
	  location = new Vector3(2-square_dist,0,square_dist-2);
	  sphere.transform.position = new Vector3(square_dist, 1.5F, -square_dist);
	}
	else if(index==2)
	{
	  location = new Vector3(square_dist-2,0,2-square_dist);
	  sphere.transform.position = new Vector3(-square_dist, 1.5F, square_dist);
	}
	else if(index==3)
	{
	  location = new Vector3(square_dist-2,0,square_dist-2);
	  sphere.transform.position = new Vector3(-square_dist, 1.5F, -square_dist);
	}

	clone.GetComponent<QAgent>().setResetPose(location,pose);

	// draw sector state
        if (!TurnOnSector)
            clone.GetComponent<DrawSector>().enabled = false;

	// draw indicator reward
        if (!TurnOnTriangleIndicator)
            clone.GetComponent<QAgent>().turn_triangle_indicator(false);
        else
            clone.GetComponent<QAgent>().turn_triangle_indicator(true);

	// set time-scale
        if (Learning)
            clone.GetComponent<QAgent>().setTimeScale(LearningTimeScale);
        else
	  clone.GetComponent<QAgent>().setTimeScale(1.0f);

	// set timing details
	clone.GetComponent<QAgent>().setTimePerUpdate(time_per_update);
	clone.GetComponent<QAgent>().reset();

	clone.GetComponent<QAgent>().setGoal(sphere);

	// animation
        if (AnimationOff) Destroy(clone.GetComponent<Animator>());

        agent[index] = clone;
    }

    void GenerateAgent()
    {
        int i=0;
        for (;i<numOfWalkers; i++)
	    createSmartAgent(i);
        for (;i<numOfWalkers+numOfZombies; i++)
	    createZombieAgent(i);
    }
  
    void OnGUI ()
    {
      //tring = GUI.TextField (new Rect (50, 50, 750, 150), display_string);
    }
}
