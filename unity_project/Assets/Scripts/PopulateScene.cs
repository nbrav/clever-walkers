using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;
using UnityEngine.UI;

public class PopulateScene : MonoBehaviour
{
    public int numOfZombies = 4;

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

    // Use this for initialization
    void Start()
    {
        agent = new GameObject[numOfZombies + 1];

        IntelligentAgentRotation = Quaternion.Euler(0, 240, 0);
        IntelligentAgentPosition = new Vector3(-140.0f, 0.0f, 210.0f);

        ZombieAgentPosition = new Vector3[numOfZombies];
        ZombieAgentRotation = new Quaternion[numOfZombies];

        ZombieAgentPosition[0] = new Vector3(-140.0f, 0.0f, 217.0f);
        ZombieAgentRotation[0] = Quaternion.Euler(0, 90, 0);

        GenerateAgent();
    }

    // Update is called once per frame
    void Update()
    {
    }

    void createZombieAgent(int index)
    {
        GameObject clone = GameObject.Instantiate(ZombiePrefab, ZombieAgentPosition[0], ZombieAgentRotation[0]) as GameObject;

        clone.GetComponent<ZombieAgent>().setSeed(index);
        clone.GetComponent<ZombieAgent>().setGoalObject(ZombieAgentPosition[0]);
        clone.GetComponent<ZombieAgent>().setDummyAgentPrefab(agentPrefab);
        Rigidbody clone_body = clone.AddComponent<Rigidbody>();
        clone_body.useGravity = false;

        if (Learning)
        {
            clone.GetComponent<ZombieAgent>().setTimeScale(LearningTimeScale);
        }
        else
        {
            clone.GetComponent<ZombieAgent>().setTimeScale(1.0f);
        }

        if (AnimationOff) Destroy(clone.GetComponent<Animator>());

        agent[index] = clone;
    }

    void createSmartAgent(int index)
    {
        GameObject clone = GameObject.Instantiate(HumanPrefab, IntelligentAgentPosition, IntelligentAgentRotation);

        clone.GetComponent<QAgent>().setDummyAgentPrefab(agentPrefab);

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
        createSmartAgent(0);

        for (int i = 0; i < numOfZombies; i++)
        {
            createZombieAgent(i);
        }
    }
}
