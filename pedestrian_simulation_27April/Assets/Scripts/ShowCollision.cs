using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class ShowCollision : MonoBehaviour {

    bool onTrigger = false;
    Transform camTrans;
    Canvas canvas;
    Image imageRed;
    Image imageBlue;
    Color CurrentColor;

    private void Start()
    {
        camTrans = Camera.main.transform;
        Transform[] trans = this.gameObject.GetComponentsInChildren<Transform>(true);
        foreach (Transform t in trans)
        {
            if (t.name == "Red")
            {
                imageRed = t.GetComponent<Image>();
            }
	    else if (t.name == "Blue")
	    {
                imageBlue = t.GetComponent<Image>();
	    }
	    
        }
        imageRed.enabled = false;
        imageBlue.enabled = false;
    }

    void Update()
    {
        if (onTrigger)
        {
	    transform.rotation = Quaternion.LookRotation(transform.position - camTrans.position);
	    if(CurrentColor == Color.red)
		imageRed.enabled = true;
	    else if(CurrentColor == Color.blue)
		imageBlue.enabled = true;
        }
        else
        {
	    //if(CurrentColor == Color.red)
		imageRed.enabled = false;
	    //else if(CurrentColor == Color.blue)
		imageBlue.enabled = false;
        }
    }

    public void TriggerIndicator(Color color)
    {
        onTrigger = true;
	CurrentColor = color;
    }

    public void UntriggerIndicator(Color color)
    {
        onTrigger = false;
	CurrentColor = color;
    }
}
