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

    /* Episode variables */
    int FixedUpdateIndex;
    int trial_elapsed=0;
    float trial_duration = 20.0F;
    int frame_rate = 30;
    float time_scale = 1.0F; 
    float time_per_update = 1.0F; //in sec
    
    /* UDP socket and channel set-up */

    public Socket socket = null;
    string THIS_IP = "127.0.0.1";
    string BESKOW_IP = "193.11.167.133";
    int PORT = 7890;

    // Use this for initialization
    void Start()
    {
        Application.targetFrameRate = frame_rate;
	Time.fixedDeltaTime = time_per_update;
	
	agent = new GameObject[numOfZombies+numOfWalkers];
	
	IntelligentAgentRotation = Quaternion.Euler(0, 240, 0);
	IntelligentAgentPosition = new Vector3(-140.0f, 0.0f, 210.0f);	

        GenerateAgent();

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
      String logger = "";
      
      if(agent.Length<numOfWalkers+numOfZombies)
	return;
      
      if(FixedUpdateIndex%1000==0)
	Debug.Log(FixedUpdateIndex.ToString()+" updates and going strong..");
      
      // check for active UDP queue
      if(socket.Available<=0)
      {	
	Debug.Log("PopulateScene FixedUpdate Error: UDP connection unavailabe!");
	for(int idx=0; idx<numOfWalkers; idx++)
	  agent[idx].GetComponent<QAgent>().set_udp(7);
	return;	
      }

      // check for reset 
      if(Time.fixedTime>=trial_duration*trial_elapsed)
      {
	trial_elapsed++;
	for(int i=0; i<numOfWalkers;i++)
	 agent[i].GetComponent<QAgent>().reset();
      }

      // execute brain update
      try
      {
	// finding any client
	IPEndPoint sender = new IPEndPoint(IPAddress.Any, 0);
	EndPoint Remote = (EndPoint)(sender);

	Debug.Log("Found client:"+sender.ToString());
	
	// receiving UDP
	byte[] data_in = new byte[256];;
	socket.ReceiveFrom(data_in,numOfWalkers*sizeof(int),0,ref Remote);

	logger += " A[";	
	for(int idx=0; idx<numOfWalkers; idx++)
	{
	  int action = System.BitConverter.ToInt32(data_in, idx*sizeof(int));
	  agent[idx].GetComponent<QAgent>().set_udp(action);
	  logger += action.ToString()+",";
	}
	logger += "]";
	
	// sending UDP; TODO: for multi-agent
	byte[] data_out = new byte[sizeof(int)*numOfWalkers*2];
	int[] data_out_int = new int[numOfWalkers*2];
	int[] state_reward = new int[2];

	logger += "S,R[";
	for(int idx=0; idx<numOfWalkers; idx++)
	{
	  state_reward = agent[idx].GetComponent<QAgent>().get_udp();	  
	  data_out_int[idx*2] = state_reward[0];
	  data_out_int[idx*2+1] = state_reward[1];
	  logger = logger + "(" + state_reward[0].ToString() + "," + state_reward[1].ToString() + "),";
	}
	Buffer.BlockCopy(data_out_int, 0, data_out, 0, data_out.Length);	
	socket.SendTo(data_out, sizeof(int)*numOfWalkers*2, SocketFlags.None, Remote);
      }
      catch (Exception err)
      {
	Debug.Log(err.ToString()+" FixedUpdate unknown error: not receiving brain signal..");
      }
      //Debug.Log(logger);
    }

    void createZombieAgent(int index)
    {
        GameObject clone = GameObject.Instantiate(ZombiePrefab) as GameObject;

        clone.GetComponent<ZombieAgent>().setSeed(index);
	clone.GetComponent<ZombieAgent>().reset();
        //clone.GetComponent<ZombieAgent>().setGoalObject(ZombieAgentPosition[0]);
        clone.GetComponent<ZombieAgent>().setDummyAgentPrefab(agentPrefab);

	Rigidbody clone_body =  clone.AddComponent<Rigidbody>();
	clone_body.useGravity = false;

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

        clone.GetComponent<QAgent>().setDummyAgentPrefab(agentPrefab);
	clone.GetComponent<QAgent>().reset();

	Rigidbody clone_body =  clone.AddComponent<Rigidbody>();
	clone_body.useGravity = false;

        if (Learning)
        {
            clone.GetComponent<QAgent>().setTimeScale(LearningTimeScale);
        }
        else
        {
            clone.GetComponent<QAgent>().setTimeScale(1.0f);
        }

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
}
