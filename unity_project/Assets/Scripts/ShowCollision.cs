using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class ShowCollision : MonoBehaviour {

    bool onTrigger = false;
    Transform camTrans;
    Canvas canvas;
    Image image;

    private void Start()
    {
        camTrans = Camera.main.transform;
        Transform[] trans = this.gameObject.GetComponentsInChildren<Transform>(true);
        foreach (Transform t in trans)
        {
            if (t.name == "Image")
            {
                image = t.GetComponent<Image>();
            }
        }
        image.enabled = false;
    }

    void Update()
    {
        if (onTrigger)
        {
            ShowIndicator();
        }
        else
        {
            HideIndicator();
        }

    }

    void ShowIndicator()
    {
        transform.rotation = Quaternion.LookRotation(transform.position - camTrans.position);
        image.enabled = true;
    }

    void HideIndicator()
    {
        image.enabled = false;
    }

    public void TriggerIndicator()
    {
        onTrigger = true;
    }

    public void UntriggerIndicator()
    {
        onTrigger = false;
    }
}
