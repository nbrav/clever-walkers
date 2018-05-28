using UnityEngine;
using System.Collections;
using RVO;

public class SimulatorComponent : MonoBehaviour {

    public float effectTime = 20.0f;
    private float timer;

    private Coroutine currentRoutine = null;
    
    // Use this for initialization
    void Start ()
    {
        timer = effectTime;
	currentRoutine = StartCoroutine(DoStep());
    }

    private void FixedUpdate()
    {
	
	//DoStep();
    }

    IEnumerator DoStep()
    {        
        while (true)
        {            
            yield return new WaitForEndOfFrame();
            Simulator.Instance.setTimeStep(Time.deltaTime);
            yield return Simulator.Instance.doStep();            
        }            
    }    
}
