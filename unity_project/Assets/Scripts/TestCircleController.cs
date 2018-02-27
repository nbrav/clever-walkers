using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;

public class TestCircleController : MonoBehaviour {

    public DrawCircle _circlePrefab;
    public float _radius;
    public Color _fillColor;

    private DrawCircle circleToDraw;
	// Use this for initialization
	void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
        
        if (Input.GetMouseButtonDown(0))
        {
            var inputMousePos = Input.mousePosition;
            inputMousePos.z = 10.0f;
            var mousePos = (Vector2)Camera.main.ScreenToWorldPoint(inputMousePos);
            circleToDraw = Instantiate(_circlePrefab);
            //circleToDraw.FillColor = _fillColor;
            circleToDraw.SetupCircle(mousePos, _radius, _fillColor);

        }

    }

}
