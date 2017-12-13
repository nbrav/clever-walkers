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
    float time_scale = 1.0F; 
    float trial_duration = 20.0F; //in sec
    float time_per_update = 0.5F; //in sec

    string tring;
    string display_string;
  
    /* UDP socket and channel set-up */
				
    public Socket socket = null;
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

	socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
	IPEndPoint brain_EP = new IPEndPoint(IPAddress.Any, PORT);
	socket.Bind(brain_EP);
    }

    // Update is called once per frame
    void Update ()
    {
    }

    void FixedUpdate()
    {
      FixedUpdateIndex++;
      
      if(agent.Length<numOfWalkers+numOfZombies)
	return;
      
      if(FixedUpdateIndex%1000==0)
	Debug.Log(FixedUpdateIndex.ToString()+" updates and going strong..");
      
      // check for active UDP queue
      if(socket.Available<=0)
      {	
	//Debug.Log("PopulateScene FixedUpdate Error: UDP connection unavailabe!");
	display_string = "No UDPs!";
	for(int idx=0; idx<numOfWalkers; idx++)
	  agent[idx].GetComponent<QAgent>().set_udp(-1);
	return;	
      }

      // check for reset
      for(int idx=0; idx<numOfWalkers;idx++)
      {
	if(reset_counter[idx]>trial_duration)
	{
	  agent[idx].GetComponent<QAgent>().reset();
	  reset_counter[idx]=0.0f;
	}
	else
	{
	  reset_counter[idx]+=Time.fixedDeltaTime;
	}  
      }
      
      // execute brain update
      try
      {
	// finding any client
	IPEndPoint sender = new IPEndPoint(IPAddress.Any, 0);
	EndPoint Remote = (EndPoint)(sender);

	// receive action
	byte[] data_in = new byte[256];;
	socket.ReceiveFrom(data_in,numOfWalkers*sizeof(int),0,ref Remote);
	
	for(int idx=0; idx<numOfWalkers; idx++)
	{
	  int action = System.BitConverter.ToInt32(data_in, idx*sizeof(int));
	  agent[idx].GetComponent<QAgent>().set_udp(action);
	  display_string = "A:"+action.ToString();
	}	
	
	// sending state,reward
	byte[] data_out = new byte[sizeof(int)*numOfWalkers*2];
	int[] data_out_int = new int[numOfWalkers*2];
	int[] state_reward = new int[2];

	for(int idx=0; idx<numOfWalkers; idx++)
	{
	  state_reward = agent[idx].GetComponent<QAgent>().get_udp();

	  display_string += "\nS:"+state_reward[0].ToString()+ "\nR:"+state_reward[1].ToString();;
		
	  data_out_int[idx*2] = state_reward[0];
	  data_out_int[idx*2+1] = state_reward[1];

	  if(state_reward[1]!=0.0f)
	    reset_counter[idx]=trial_duration;
	}
	Buffer.BlockCopy(data_out_int, 0, data_out, 0, data_out.Length);	
	socket.SendTo(data_out, sizeof(int)*numOfWalkers*2, SocketFlags.None, Remote);	  
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

	if(Learning)
	{
	  clone.GetComponent<ZombieAgent>().setTimeScale(LearningTimeScale);
	}
	else
	{
	  clone.GetComponent<ZombieAgent>().setTimeScale(1.0f);
	}

	if(AnimationOff)  Destroy(clone.GetComponent<Animator>());	  
	  
	agent[index] = clone;
    }

    void createSmartAgent(int index)
    {
        GameObject clone = GameObject.Instantiate(HumanPrefab);

	clone.GetComponent<QAgent>().reset();
        clone.GetComponent<QAgent>().setDummyAgentPrefab(agentPrefab);
	clone.AddComponent<Rigidbody>();

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
            clone.GetComponent<QAgent>().setTimeScale(1.0f);
        }
	clone.GetComponent<QAgent>().setTimePerUpdate(time_per_update);

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
      tring = GUI.TextField (new Rect (50, 50, 60, 80), display_string);
    }
}
