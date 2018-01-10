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
    float trial_duration = 45.0F; //in sec
    float time_per_update = 0.5F; //in sec

    string tring;
    string display_string;
    int RESET_CODE_INT = 255;
  
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
	  reset_counter[i]=0.0f;
	
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

    void FixedUpdate()
    {
      display_string = "";
      
      Time.timeScale = LearningTimeScale;
      FixedUpdateIndex++;
      
      if(agent.Length<numOfWalkers+numOfZombies)
	return;

      //if(FixedUpdateIndex%1000==0)
      //Debug.Log(FixedUpdateIndex.ToString()+" updates and going strong..");
      
      // check for active UDP queue / or abandon
      for(int idx=0; idx<numOfWalkers; idx++)
      {
	if(socket[idx].Available<=0)
	{	
	  //display_string = "Agent ";
	  //display_string += idx.ToString()+",";
	  //display_string += " Broken Connection!";
	  agent[idx].GetComponent<QAgent>().set_udp(-1);
	  return;
	}
      }
      
      // execute brain update
      try
      {
	for(int idx=0; idx<numOfWalkers; idx++)
	{
	  // finding any client
	  IPEndPoint sender = new IPEndPoint(IPAddress.Any, PORT+idx);
	  EndPoint Remote = (EndPoint)(sender);
  
	  // receive action
	  byte[] data_in = new byte[256];;
	  
	  socket[idx].ReceiveFrom(data_in,sizeof(int),0,ref Remote);	
	  int action = System.BitConverter.ToInt32(data_in, 0);
	  
	  agent[idx].GetComponent<QAgent>().set_udp(action);
	  
	  // check for reset
	  // TODO: how to reset inside loop for all agents
	  if(reset_counter[idx]>=trial_duration)
	  {
	    display_string += "RESET";

	    int agent_idx=0;

	    agent[idx].GetComponent<QAgent>().reset();

	    reset_counter[idx]=0.0f;

	    byte[] data_out_reset = new byte[sizeof(int)];
	    data_out_reset[0] = Convert.ToByte(RESET_CODE_INT); // send reset code
	    	    
	    socket[0].SendTo(data_out_reset, sizeof(int), SocketFlags.None, Remote);
	  }
	  else
	  {
	    reset_counter[idx]+=1;//Time.fixedDeltaTime;
	  }
      
	  // sending state,reward
	  List<int> state_reward;
	  state_reward = agent[idx].GetComponent<QAgent>().get_udp();

	  // display
	  /*display_string += "A:"+action.ToString();
	  display_string += " S:";
	  for(int obstacle_index=0; obstacle_index<state_reward.Count-1;obstacle_index++)
	    display_string += " "+state_reward[obstacle_index].ToString();
	    display_string += " R:"+state_reward[state_reward.Count-1].ToString();*/
	  
	  int[] data_out_int = new int[(state_reward.Count+1)];
	  byte[] data_out = new byte[sizeof(int)*(state_reward.Count+1)];
	  
	  data_out_int[0] = state_reward.Count;
	  for(int idx2=0; idx2<state_reward.Count;idx2++)
	    data_out_int[idx2+1] = state_reward[idx2];
	  
	  /*if(state_reward[state_reward.Count-1]!=0.0f)
	    {
	    display_string = "RESET!";
	    reset_counter[idx]=trial_duration;
	    }*/
	  
	  Buffer.BlockCopy(data_out_int, 0, data_out, 0, data_out.Length);	
	  socket[idx].SendTo(data_out, sizeof(int)*data_out_int.Length, SocketFlags.None,Remote);

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

	clone.GetComponent<QAgent>().setDummyAgentPrefab(agentPrefab);
	clone.AddComponent<Rigidbody>();

	Vector3 location;
	Quaternion pose;
	
	location = new Vector3(UnityEngine.Random.Range(-10,10),0.0f,UnityEngine.Random.Range(-10,10));
	pose = Quaternion.Euler(0.0f,UnityEngine.Random.Range(0,360),0.0f);

	clone.GetComponent<QAgent>().setResetPose(location,pose);

        if (!TurnOnSector)
        {
            clone.GetComponent<DrawSector>().enabled = false;
        }

        if (!TurnOnTriangleIndicator)
        {
            clone.GetComponent<QAgent>().turn_triangle_indicator(false);
        }
        else
        {
            clone.GetComponent<QAgent>().turn_triangle_indicator(true);
        }

        if (Learning)
        {
            clone.GetComponent<QAgent>().setTimeScale(LearningTimeScale);
        }
        else
        {
	  //clone.GetComponent<QAgent>().setTimeScale(1.0f);
        }
	
	clone.GetComponent<QAgent>().setTimePerUpdate(time_per_update);
	clone.GetComponent<QAgent>().reset();
        
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
      tring = GUI.TextField (new Rect (50, 50, 500, 70), display_string);
    }
}
