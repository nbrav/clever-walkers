using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;

public class HallwayOrcaPopulateScene : MonoBehaviour {

    [SerializeField]
    GameObject orcaAgentPrefab;

    [SerializeField]
    float LearningTimeScale = 25.0f;

    [SerializeField]
    bool RandomReset;

    [SerializeField]
    bool VizRewards = false;

    public int numOfWalkers = 25;

    GameObject[] agent;

    /* RL variables (experimental) */
  
    float[] reset_counter;

    /* Episode variables */
  
    int FixedUpdateIndex;
    int trial_elapsed=0;
    int frame_rate = 30;
    float trial_duration = 100.0F; //in sec
    float time_per_update = 0.5F; //in sec

    string tring;
    string display_string;
  
    private void Awake()
    {
        agent = new GameObject[numOfWalkers];
	GenerateAgent();
    }

    // Use this for initialization
    void Start () {

        Application.targetFrameRate = frame_rate;
	Time.fixedDeltaTime = time_per_update;

	reset_counter = new float[numOfWalkers];
	
	for(int i=0;i<numOfWalkers;i++)
	  reset_counter[i]=trial_duration;
    }   

    void createSmartAgent(int index)
    {
        GameObject clone = GameObject.Instantiate(orcaAgentPrefab);

        clone.name = "agent" + index.ToString();

        // agent pose
        Vector3 location = new Vector3(0, 0, 0);
        Quaternion pose;

        GameObject goal;

        pose = Quaternion.Euler(0.0f, UnityEngine.Random.Range(0, 360), 0.0f);

        float square_dist = 13f;

        string SCENARIO = "circle";//"narrowpassage";

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
		    
		    location = new Vector3(10.1f+size*(index_in_group%square), 0, 10.0f+size*(index_in_group/square));
		    goal.transform.position = new Vector3(-10.0f-size*(index_in_group%square), 0.0F, -10.0f-size*(index_in_group/square));
		    
		    goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
		    break;
		case 1:
		    clone.GetComponent<Renderer>().material.color = new Color(0.0f, 1.0f, 0.0f);
		    goal.GetComponent<Renderer>().material.color = new Color(0.0f, 1.0f, 0.0f);
		    
		    location = new Vector3(-10.2f-size*(index_in_group%square), 0, 10.0f+size*(index_in_group/square));
		    goal.transform.position = new Vector3(10.0f+size*(index_in_group%square), 0.0F, -10.0f-size*(index_in_group/square));
		    
		    goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
		    break;
		case 2:
		    clone.GetComponent<Renderer>().material.color = new Color(1.0f, 0.0f, 0.0f);
		    goal.GetComponent<Renderer>().material.color = new Color(1.0f, 0.0f, 0.0f);
		    
		    location = new Vector3(10.1f+size*(index_in_group%square), 0, -10.0f-size*(index_in_group/square));
		    goal.transform.position = new Vector3(-10.0f-size*(index_in_group%square), 0.0F, 10.0f+size*(index_in_group/square));
		    
		    goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
		    break;
		case 3:
		    clone.GetComponent<Renderer>().material.color = new Color(1.0f, 0.0f, 1.0f);
		    goal.GetComponent<Renderer>().material.color = new Color(1.0f, 0.0f, 1.0f);
		    
		    location = new Vector3(-10.1f-size*(index_in_group%square), 0, -10.0f-size*(index_in_group/square));
		    goal.transform.position = new Vector3(10.0f+size*(index_in_group%square), 0.0F, 10.0f+size*(index_in_group/square));
		    
		    goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
		    break;		    
	    }

	    clone.GetComponent<AgentComponent>().setGoal(goal);	    
        }
        else if (SCENARIO == "hallway")
        {
            goal = GameObject.CreatePrimitive(PrimitiveType.Cube);
            goal.name = "goal" + index.ToString();

            if (index % 2 == 0)
            {
                clone.GetComponent<Renderer>().material.color = new Color(0.0f, 0.5f, 1.0f);
                goal.GetComponent<Renderer>().material.color = new Color(0.0f, 0.5f, 1.0f);
                location = new Vector3((index - numOfWalkers / 2) * 3.0f, 0, 10);
                goal.transform.position = new Vector3((index-numOfWalkers/2)*3.0f, 0.5F, -15);
                goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
            }
            else
            {
                clone.GetComponent<Renderer>().material.color = new Color(1.0f, 0.5f, 0.0f);
                goal.GetComponent<Renderer>().material.color = new Color(1.0f, 0.5f, 0.0f);
                location = new Vector3((index-numOfWalkers/2-1.00f) * 3.0f, 0, -15);
                goal.transform.position = new Vector3((index-numOfWalkers/2-1.00f) * 3.0f, 0.5F, 10);
                goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
            }
            clone.GetComponent<AgentComponent>().setGoal(goal);
        }
	else if(SCENARIO == "circle")
	{
            goal = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            goal.name = "goal" + index.ToString();

            clone.GetComponent<Renderer>().material.color = new Color(Mathf.Cos(2*Mathf.PI*index/numOfWalkers), 0.5f, Mathf.Sin(2*Mathf.PI*index/numOfWalkers));

            location = new Vector3(Mathf.Cos(2*Mathf.PI*index/numOfWalkers)*square_dist, 0, Mathf.Sin(2*Mathf.PI*index/numOfWalkers)*square_dist);
	    pose = Quaternion.Euler(0.0f, UnityEngine.Random.Range(0, 360), 0.0f);

            goal.transform.position = new Vector3(-Mathf.Cos(2*Mathf.PI*index/numOfWalkers)*square_dist, 0.5F, -Mathf.Sin(2*Mathf.PI*index/numOfWalkers)*square_dist);
            goal.transform.localScale = new Vector3(2.0f, 2.0f, 2.0f);
            goal.GetComponent<Renderer>().material.color = new Color(Mathf.Cos(2*Mathf.PI*index/numOfWalkers), 0.5f, Mathf.Sin(2*Mathf.PI*index/numOfWalkers));

            clone.GetComponent<AgentComponent>().setGoal(goal);
	}
	
        clone.GetComponent<AgentComponent>().setResetPose(location, pose);

        // draw indicator reward
        if (!VizRewards)
            clone.GetComponent<AgentComponent>().turn_triangle_indicator(false);
        else
            clone.GetComponent<AgentComponent>().turn_triangle_indicator(true);

        // set timing details
        clone.GetComponent<AgentComponent>().RandomReset = RandomReset;
        clone.GetComponent<AgentComponent>().reset();


        agent[index] = clone;
    }

    // FixedUpdate is called every 
    void FixedUpdate()
    {	
	FixedUpdateIndex++;
	
	// execute brain update
	try
	{
	    bool global_reset = true;
	    for(int idx=0; idx<numOfWalkers; idx++)
	    {
		if(agent[idx].GetComponent<AgentComponent>().reached_goal()!=0.0f)
		    reset_counter[idx] = 0.0f;
		else
		    reset_counter[idx] -= 1.0f;
				    
		global_reset = global_reset && (reset_counter[idx]<=0);
	    }

	    if(global_reset==true)
	    {
		Debug.Log("RESET!");
		for(int idx=0; idx<numOfWalkers; idx++)
		    reset_counter[idx]=trial_duration;

		for(int idx=0; idx<numOfWalkers; idx++)
		    if (agent[idx] != null)
			Destroy(agent[idx]);

		//agent = new GameObject[numOfWalkers];
		//GenerateAgent();
	    }
	}
	catch (Exception err)
	{
	}
    }

    void GenerateAgent()
    {
        int i = 0;
        for (; i < numOfWalkers; i++)
            createSmartAgent(i);
    }

    // Update is called once per frame
    void Update () {
		
	}
}
