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
    public int numOfWalkers;

    [SerializeField]
    bool Learning;

    [SerializeField]
    float LearningTimeScale = 5.0f;

    [SerializeField]
    bool RandomReset;

    [SerializeField]
    bool AnimationOff;

    [SerializeField]
    bool VizCollisionCells = true;

    [SerializeField]
    bool VizPlaceCell = false;

    [SerializeField]
    bool VizRewards = false;

    [SerializeField]
    bool VizTrail = true;

    [SerializeField]
    GameObject HumanPrefab;

    [SerializeField]
    GameObject agentPrefab;

    GameObject[] agent;
    
    /* UDP socket and channel set-up */

    public Socket[] socket;
    string THIS_IP = "127.0.0.1";
    string BESKOW_IP = "193.11.167.133";
    int PORT = 7890;

    /* Episode variables */

    int frameRate = 30;
    float trial_duration = 100.0F; // sec
    float time_per_update = 0.5F; // in sec
    float[] reset_counter;

    /* Timing */
    float[] TimeStart;
    float[] TimeToGoal;
    StreamWriter[] sr;
    
    void Awake()
    {
        agent = new GameObject[numOfWalkers];

        GenerateAgent();
    }

    // Use this for initialization
    void Start()
    {
        Application.targetFrameRate = frameRate;
        Time.fixedDeltaTime = time_per_update;

        reset_counter = new float[numOfWalkers];
        for (int i = 0; i < numOfWalkers; i++)
            reset_counter[i] = trial_duration;

        /* UDP set-up */

        socket = new Socket[numOfWalkers];
        for (int idx = 0; idx < numOfWalkers; idx++)
        {
            socket[idx] = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            IPEndPoint brain_EP = new IPEndPoint(IPAddress.Any, PORT + idx);
            socket[idx].Bind(brain_EP);
        }

	/* Timing */
	
	sr = new StreamWriter[numOfWalkers];
	TimeStart = new float[numOfWalkers];
	TimeToGoal = new float[numOfWalkers];
        for (int idx = 0; idx < numOfWalkers; idx++)
	{
	    sr[idx] = File.CreateText("/home/naba/Desktop/clever-walkers/rl-brain/data/TimeToGoal"+idx+".log");
	    TimeStart[idx] = 0.0f;
	    TimeToGoal[idx] = -1.0f;
	}
    }

    // Update is called once per frame
    void Update()
    {
    }

    // FixedUpdate is called every 
    void FixedUpdate()
    {
        IPEndPoint sender;
        EndPoint Remote;

        Time.timeScale = LearningTimeScale;

        if (agent.Length<numOfWalkers)
            return;

        // check for active UDP queue or abandon
        for (int idx = 0; idx < numOfWalkers; idx++)
        {
            if (socket[idx].Available <= 0)
            {
                agent[idx].GetComponent<QAgent>().set_udp(new float[2]{0.0f,0.0f}, true);
                return;
            }
        }

	// compute globalReset variable
        float global_reset = 0.0f;
        for (int reset_idx = 0; reset_idx < numOfWalkers; reset_idx++)
            global_reset += reset_counter[reset_idx];

        // execute brain update
        try
        {
            // receive all action
            for (int idx = 0; idx < numOfWalkers; idx++)
            {
                // finding any client
                sender = new IPEndPoint(IPAddress.Any, PORT+idx);
                Remote = (EndPoint)(sender);

                // receive action
                byte[] data_in = new byte[256];
		float[] motor_command = new float[2];

                socket[idx].ReceiveFrom(data_in, 2*sizeof(float), 0, ref Remote);
		
		motor_command[0] = BitConverter.ToSingle(data_in, 0);
		motor_command[1] = BitConverter.ToSingle(data_in, sizeof(float));
		
                // if local reset
                if (reset_counter[idx] <= 0.0f && global_reset != 0.0f)
                {
		    if(reset_counter[idx]<=0.0f && TimeToGoal[idx]==-1.0f)
			TimeToGoal[idx] = Time.time - TimeStart[idx];
		    
                    agent[idx].GetComponent<QAgent>().set_udp(new float[2]{0.0f,0.0f}, true);

                    float[] data_out_float_reset = new float[1];
                    data_out_float_reset[0] = float.MaxValue;
                    byte[] data_out_reset = new byte[sizeof(float)];
                    Buffer.BlockCopy(data_out_float_reset, 0, data_out_reset, 0, data_out_reset.Length);
                    socket[idx].SendTo(data_out_reset, sizeof(float), SocketFlags.None, Remote);
                }
                else if (global_reset == 0.0f)
                {
		    write_TimeToGoal(idx,TimeToGoal[idx]);

		    agent[idx].GetComponent<QAgent>().reset(); //NO RESET

                    float[] data_out_float_reset = new float[2];
                    data_out_float_reset[0] = float.MaxValue; data_out_float_reset[1] = float.MaxValue;
                    byte[] data_out_reset = new byte[sizeof(float) * 2];
                    Buffer.BlockCopy(data_out_float_reset, 0, data_out_reset, 0, data_out_reset.Length);
                    socket[idx].SendTo(data_out_reset, sizeof(float) * 2, SocketFlags.None, Remote);

                    for (int reset_idx = 0; reset_idx < numOfWalkers; reset_idx++)
                        reset_counter[reset_idx] = trial_duration;

		    TimeStart[idx] = Time.time;
		    TimeToGoal[idx] = -1.0f;
                }
                else
                {
                    reset_counter[idx] -= 1; //Time.fixedDeltaTime;
                    agent[idx].GetComponent<QAgent>().set_udp(motor_command, false);
		    TimeToGoal[idx] = -1.0f;
                }

                // sending state,reward
                List<float> state_reward;
                state_reward = agent[idx].GetComponent<QAgent>().get_udp();

                float[] data_out_int = new float[(state_reward.Count + 1)];
                byte[] data_out = new byte[sizeof(float) * (state_reward.Count + 1)];

                data_out_int[0] = state_reward.Count;
                for (int idx2 = 0; idx2 < state_reward.Count; idx2++)
                    data_out_int[idx2 + 1] = state_reward[idx2];

                if (global_reset != 0.0f && reset_counter[idx] != 0.0f)
                    if (agent[idx].GetComponent<QAgent>().reached_goal() != 0.0f)
                        reset_counter[idx] = 0.0f;

                Buffer.BlockCopy(data_out_int, 0, data_out, 0, data_out.Length);
                socket[idx].SendTo(data_out, sizeof(float) * data_out_int.Length, SocketFlags.None, Remote);
            }
        }
        catch (Exception err)
        {
            Debug.Log(err.ToString() + " FixedUpdate unknown error: not receiving brain signal..");
        }
    }

    void write_TimeToGoal(int agentIdx, float t)
    {
	sr[agentIdx].WriteLine ("{0}", t);
	sr[agentIdx].Flush(); 
    }

    void createSmartAgent(int index)
    {
        GameObject clone = GameObject.Instantiate(HumanPrefab);

        clone.name = "agent" + index.ToString();
        //clone.GetComponent<QAgent>().setDummyAgentPrefab(agentPrefab);
        clone.AddComponent<Rigidbody>();

        // agent pose
        Vector3 location = new Vector3(0, 0, 0);
        Quaternion pose = Quaternion.identity;
        GameObject goal;

        float square_dist = 13f;

        string SCENARIO =  "circle"; //"crossing";; //"hallway"; //"narrowpassage";

        if (SCENARIO == "narrowpassage")
        {
            goal = GameObject.CreatePrimitive(PrimitiveType.Cube);
            goal.name = "goal" + index.ToString();

	    int group = index/4;
	    int square = 2;
	    float size = 3.0f;
	    int index_in_group = index%4;
	    
	    switch(group)
	    {
		case 0:
		    clone.GetComponent<Renderer>().material.color = new Color(0.0f, 0.0f, 1.0f);
		    goal.GetComponent<Renderer>().material.color = new Color(0.0f, 0.0f, 1.0f);
		    
		    location = new Vector3(10.0f+size*(index_in_group%square), 0, 10.0f+size*(index_in_group/square));
		    goal.transform.position = new Vector3(-10.0f-size*(index_in_group%square), 0.0F, -10.0f-size*(index_in_group/square));
		    
		    goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
		    break;
		case 1:
		    clone.GetComponent<Renderer>().material.color = new Color(0.0f, 1.0f, 0.0f);
		    goal.GetComponent<Renderer>().material.color = new Color(0.0f, 1.0f, 0.0f);
		    
		    location = new Vector3(-10.0f-size*(index_in_group%square), 0, 10.0f+size*(index_in_group/square));
		    goal.transform.position = new Vector3(10.0f+size*(index_in_group%square), 0.0F, -10.0f-size*(index_in_group/square));
		    
		    goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
		    break;
		case 2:
		    clone.GetComponent<Renderer>().material.color = new Color(1.0f, 0.0f, 0.0f);
		    goal.GetComponent<Renderer>().material.color = new Color(1.0f, 0.0f, 0.0f);
		    
		    location = new Vector3(10.0f+size*(index_in_group%square), 0, -10.0f-size*(index_in_group/square));
		    goal.transform.position = new Vector3(-10.0f-size*(index_in_group%square), 0.0F, 10.0f+size*(index_in_group/square));
		    
		    goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
		    break;
		case 3:
		    clone.GetComponent<Renderer>().material.color = new Color(1.0f, 0.0f, 1.0f);
		    goal.GetComponent<Renderer>().material.color = new Color(1.0f, 0.0f, 1.0f);
		    
		    location = new Vector3(-10.0f-size*(index_in_group%square), 0, -10.0f-size*(index_in_group/square));
		    goal.transform.position = new Vector3(10.0f+size*(index_in_group%square), 0.0F, 10.0f+size*(index_in_group/square));
		    
		    goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
		    break;		    
	    }
	    clone.GetComponent<QAgent>().setGoal(goal);	    
        }
	else if(SCENARIO=="crossing")
	{
            goal = GameObject.CreatePrimitive(PrimitiveType.Cube);
            goal.name = "goal" + index.ToString();

	    Color our_color = new Color(0.0f,0.0f,0.0f);

	    switch(index/8)
	    {
		case 0:
		    our_color = new Color(0.0f, 0.5f, 1.0f);
		    pose = Quaternion.Euler(0.0f, 90.0f, 0.0f);
		    break;
		case 1:
		    our_color = new Color(1.0f, 0.5f, 0.0f);
		    pose = Quaternion.Euler(0.0f, 0.0f, 0.0f);
		    break;
	    }

	    switch(index)
	    {
		case 0:
		    location = new Vector3(-17.5f, 0.0f, -2.625f);
		    goal.transform.position = new Vector3(17.5f, 0.0f, -2.625f);
		    break;
		case 1:
		    location = new Vector3(-17.5f, 0.0f, -2.625f+1.75f);
		    goal.transform.position = new Vector3(17.5f, 0.0f, -2.625f+1.75f);		    
		    break;
		case 2:
		    location = new Vector3(-17.5f, 0.0f, -2.625f+1.75f*2);
		    goal.transform.position = new Vector3(17.5f, 0.0f, -2.625f+1.75f*2);
		    break;
		case 3:
		    location = new Vector3(-17.5f, 0.0f, -2.625f+1.75f*3);
		    goal.transform.position = new Vector3(17.5f, 0.0f, -2.625f+1.75f*3);
		    break;
		case 4:
		    location = new Vector3(-17.5f+1.75f, 0.0f, -2.625f);
		    goal.transform.position = new Vector3(17.5f-1.75f, 0.0f, -2.625f);
			break;
		case 5:
		    location = new Vector3(-17.5f+1.75f, 0.0f, -2.625f+1.75f);
		    goal.transform.position = new Vector3(17.5f-1.75f, 0.0f, -2.625f+1.75f);
		    break;
		case 6:
		    location = new Vector3(-17.5f+1.75f, 0.0f, -2.625f+1.75f*2);
		    goal.transform.position = new Vector3(17.5f-1.75f, 0.0f, -2.625f+1.75f*2);
		    break;
		case 7:
		    location = new Vector3(-17.5f+1.75f, 0.0f, -2.625f+1.75f*3);
		    goal.transform.position = new Vector3(17.5f-1.75f, 0.0f, -2.625f+1.75f*3);
		    break;
		case 8:
		    location = new Vector3(2.625f, 0.0f, -17.5f);
		    goal.transform.position = new Vector3(2.625f, 0.0f, 17.5f);
		    break;
		case 9:
		    location = new Vector3(2.625f-1.75f, 0.0f, -17.5f);
		    goal.transform.position = new Vector3(2.625f-1.75f, 0.0f, 17.5f);
		    break;
		case 10:
		    location = new Vector3(2.625f-1.75f*2, 0.0f, -17.5f);
		    goal.transform.position = new Vector3(2.625f-1.75f*2, 0.0f, 17.5f);
		    break;
		case 11:
		    location = new Vector3(2.625f-1.75f*3, 0.0f, -17.5f);
		    goal.transform.position = new Vector3(2.625f-1.75f*3, 0.0f, 17.5f);
		    break;
		case 12:
		    location = new Vector3(2.625f, 0.0f, -17.5f-1.75f);
		    goal.transform.position = new Vector3(2.625f, 0.0f, 17.5f+1.75f);
		    break;
		case 13:
		    location = new Vector3(2.625f-1.75f*1, 0.0f, -17.5f-1.75f);
		    goal.transform.position = new Vector3(2.625f-1.75f*1, 0.0f, 17.5f+1.75f);
		    break;
		case 14:
		    location = new Vector3(2.625f-1.75f*2, 0.0f, -17.5f-1.75f);
		    goal.transform.position = new Vector3(2.625f-1.75f*2, 0.0f, 17.5f+1.75f);
		    break;
		case 15:
		    location = new Vector3(2.625f-1.75f*3, 0.0f, -17.5f-1.75f);
		    goal.transform.position = new Vector3(2.625f-1.75f*3, 0.0f, 17.5f+1.75f);
		    break;
	    }
		    
	    clone.GetComponent<Renderer>().material.color = our_color;

            goal.transform.localScale = new Vector3(1.0f, 1.0f, 1.0f);
            goal.GetComponent<Renderer>().material.color = our_color;

            clone.GetComponent<QAgent>().setGoal(goal);	    	    
	}
	else if(SCENARIO == "photoshoot")
	{
            goal = GameObject.CreatePrimitive(PrimitiveType.Cube);
            goal.name = "goal" + index.ToString();

	    Color our_color = new Color(0.0f,0.0f,0.0f);
	    
	    switch(index)
	    {
		case 0:
		    our_color = new Color(0.5f, 0.5f, 0.5f);
		    location = new Vector3(6.0f, 0.0f, 4.0f);
		    break;
		case 1:
		    our_color = new Color(0.5f, 0.5f, 0.5f);
		    location = new Vector3(4.0f, 0.0f, 1.0f);
		    break;
		case 2:
		    our_color = new Color(1.0f, 0.0f, 0.0f);
		    location = new Vector3(-1.0f, 0.0f, 4.0f);
		    break;
		case 3:
		    our_color = new Color(0.5f, 0.5f, 0.5f);
		    location = new Vector3(3.0f, 0.0f, -3.0f);
		    break;
		case 4:
		    our_color = new Color(0.5f, 0.5f, 0.5f);
		    location = new Vector3(2.0f, 0.0f,7.0f);
		    break;		    
	    }
            clone.GetComponent<Renderer>().material.color = our_color;

	    pose = Quaternion.Euler(0.0f, UnityEngine.Random.Range(0, 360), 0.0f);

            goal.transform.position = new Vector3(-Mathf.Cos(2*Mathf.PI*index/numOfWalkers)*square_dist, 0.5F, -Mathf.Sin(2*Mathf.PI*index/numOfWalkers)*square_dist);
            goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
            goal.GetComponent<Renderer>().material.color = our_color;

            clone.GetComponent<QAgent>().setGoal(goal);	    
	}
	else if (SCENARIO == "circle")
        {
            goal = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            goal.name = "goal" + index.ToString();

            clone.GetComponent<Renderer>().material.color = new Color(Mathf.Cos(2*Mathf.PI*index/numOfWalkers), 0.5f, Mathf.Sin(2*Mathf.PI*index/numOfWalkers));

            location = new Vector3(Mathf.Cos(2*Mathf.PI*index/numOfWalkers)*square_dist, 0, Mathf.Sin(2*Mathf.PI*index/numOfWalkers)*square_dist);
	    pose = Quaternion.Euler(0.0f, UnityEngine.Random.Range(0, 360), 0.0f);

            goal.transform.position = new Vector3(-Mathf.Cos(2*Mathf.PI*index/numOfWalkers)*square_dist, 0.5F, -Mathf.Sin(2*Mathf.PI*index/numOfWalkers)*square_dist);
            goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
            goal.GetComponent<Renderer>().material.color = new Color(Mathf.Cos(2*Mathf.PI*index/numOfWalkers), 0.5f, Mathf.Sin(2*Mathf.PI*index/numOfWalkers));

            clone.GetComponent<QAgent>().setGoal(goal);
        }
        else if (SCENARIO == "hallway")
        {
            goal = GameObject.CreatePrimitive(PrimitiveType.Cube);
            goal.name = "goal" + index.ToString();

	    // DEBUGGIN!
	    /*if(index == 0)
	    {
		clone.GetComponent<Renderer>().material.color = new Color(0.0f, 0.5f, 1.0f);
                goal.GetComponent<Renderer>().material.color = new Color(0.0f, 0.5f, 1.0f);

		location = new Vector3(-5.0f, 0.0f, -15.0f);
		pose = Quaternion.Euler(0.0f, 0.0f, 0.0f);

                goal.transform.position = new Vector3((index-numOfWalkers/2)*3.0f, 0.5F, -15);
                goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
	    }
	    else*/ if (index % 2 == 0)
            {
                clone.GetComponent<Renderer>().material.color = new Color(0.0f, 0.5f, 1.0f);
                goal.GetComponent<Renderer>().material.color = new Color(0.0f, 0.5f, 1.0f);

		location = new Vector3(2.5f+(index-numOfWalkers/2)*3.0f, 0, 10);
		pose = Quaternion.Euler(0.0f, 0.0f, 0.0f);

                goal.transform.position = new Vector3(2.5f+(index-numOfWalkers/2)*3.0f, 0.0F, -15);
                goal.transform.localScale = new Vector3(1.0f, 1.0f, 1.0f);
            }
            else
            {
                clone.GetComponent<Renderer>().material.color = new Color(1.0f, 0.5f, 0.0f);
                goal.GetComponent<Renderer>().material.color = new Color(1.0f, 0.5f, 0.0f);
		
                location = new Vector3(2.5f+(index-numOfWalkers/2-1.0f)*3.0f, 0, -15);
		pose = Quaternion.Euler(0.0f, 180.0f, 0.0f);

                goal.transform.position = new Vector3(2.5f+(index-numOfWalkers/2-1.0f)*3.0f, 0.0F, 10);
                goal.transform.localScale = new Vector3(1.0f, 1.0f, 1.0f);
            }

            clone.GetComponent<QAgent>().setGoal(goal);
        }

        clone.GetComponent<QAgent>().setResetPose(location, pose);

        // draw PlaceCells state
        if (VizPlaceCell && index==2)
            clone.GetComponent<QAgent>().vizPlaceCell = true;

        // draw sector state
        if (VizCollisionCells && index == 2)
	    clone.GetComponent<QAgent>().vizCollisionCells = true;

        // draw indicator reward
        if (!VizRewards)
            clone.GetComponent<QAgent>().turn_triangle_indicator(false);
        else
            clone.GetComponent<QAgent>().turn_triangle_indicator(true);

	if(VizTrail)
            clone.GetComponent<QAgent>().VizTrail = true;

        // set time-scale
        /*if (Learning)
            clone.GetComponent<QAgent>().setTimeScale(LearningTimeScale);
        else
            clone.GetComponent<QAgent>().setTimeScale(1.0f);
	    */
	
        // set timing details
        clone.GetComponent<QAgent>().RandomReset = RandomReset;
        clone.GetComponent<QAgent>().reset();

        // animation
        if (AnimationOff) Destroy(clone.GetComponent<Animator>());

        agent[index] = clone;
    }

    void GenerateAgent()
    {
        for (int i=0; i < numOfWalkers; i++)
            createSmartAgent(i);

	for (int i=0; i < numOfWalkers; i++)
	    agent[i].GetComponent<QAgent>().set_parent_array(agent);
    }
}
