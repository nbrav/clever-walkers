using UnityEngine;
using System.Collections;
using RVO;

public class AgentComponent : MonoBehaviour {

    private int _agentHandler;
    public float neighborDist;
    public int maxNeighbors;
    public float timeHorizon;
    public float timeHorizonObst;
    public float radius;
    public float maxSpeed;
    public float agentHeight;
    public bool isKinematic = false;
    public bool RandomReset = false;

    Vector3 defaultLocation;
    Quaternion defaultPose;
    GameObject goalObject;

    Random random;

    private Rigidbody rb;
    
    /* velocity component */
    Vector3 position_previous;

    /* pseudo-reward components */
    float reward_goal = 0.0f;
    
    // draw reward indication
    bool turnOnTriangleIndicator;

    ShowCollision show_collision;

    private void Awake()
    {
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

    // Use this for initialization
    void Start () {
        random = new Random();
        _agentHandler = Simulator.Instance.addAgent(transform.position, agentHeight, neighborDist, maxNeighbors, timeHorizon, timeHorizonObst, radius, maxSpeed, Vector2.zero, isKinematic);


        /* rigidbody setup */

        rb = this.gameObject.GetComponent<Rigidbody>();
        rb.constraints = RigidbodyConstraints.FreezePositionY | RigidbodyConstraints.FreezeRotationX | RigidbodyConstraints.FreezeRotationY | RigidbodyConstraints.FreezeRotationZ;
    }

    public void setGoal(GameObject goal)
    {
        goalObject = goal;
    }

    public float reached_goal()
    {
	return reward_goal;
    }


    public void setResetPose(Vector3 location, Quaternion pose)
    {
        defaultLocation = location;
        defaultPose = pose;
    }

    void OnTriggerEnter(Collider col)
    {
        if (col.gameObject == goalObject)
        {
            if (turnOnTriangleIndicator)
            {                
                show_collision.TriggerIndicator(Color.blue);
		reward_goal = 1.0f;
            }             
        }

        if (col.gameObject.tag == "pedestrian" || col.gameObject.tag == "wall")
        {
            if (turnOnTriangleIndicator)
                show_collision.TriggerIndicator(Color.red);
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
        }
    }

    public void reset()
    {
	//this.gameObject.transform.position = Vector3();


        if (RandomReset) transform.position = defaultLocation;
        else transform.position = new Vector3(UnityEngine.Random.Range(-15, 15), 0, UnityEngine.Random.Range(-15, 15));

	/*reward_goal = 0.0f;
	
        transform.rotation = Quaternion.Euler(0.0f, UnityEngine.Random.Range(0, 360), 0.0f);

        if (turnOnTriangleIndicator)
            show_collision.UntriggerIndicator(Color.blue);

        position_previous = Vector3.zero;
	*/
    }

    public void setPosition(Vector3 pos)
    {
        Simulator.Instance.setAgentPosition(_agentHandler, pos);
    }

    void setPreferredVelocities(Vector3 velocity)
    {
        float angle = (float)Random.value * 2.0f * (float)Mathf.PI;
        float dist = (float)Random.value * 0.0001f;
        Simulator.Instance.setAgentPrefVelocity(_agentHandler, velocity + dist * new Vector3((float)Mathf.Cos(angle), 0, (float)Mathf.Sin(angle)));
    }

    public void turn_triangle_indicator(bool flag)
    {
        turnOnTriangleIndicator = flag;
    }

    // Update is called once per frame
    void Update () {
        Vector3 pos = Simulator.Instance.getAgentPosition(_agentHandler);
        DrawLine(transform.position, pos, Color.red, 3.0f);
        transform.position = pos;
        Simulator.Instance.setAgentPrefVelocity(_agentHandler, goalObject.transform.position - transform.position);        
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
