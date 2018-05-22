using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

public class DrawEllipse : MonoBehaviour {
    
    //private Color _fillColor;
    private MeshFilter _meshFilter;
    private LineRenderer _lineRenderer;

    //public Color FillColor {
    //    get { return _fillColor; }
    //    set {
    //        _fillColor = value;
    //    }
    //}


    private void Awake()
    {
        _meshFilter = GetComponent<MeshFilter>();
        _lineRenderer = GetComponent<LineRenderer>();
    }


    public void SetupEllipse(float sigmaR, float sigmaTheta, float dis, Vector2 center, Color fillColor)
    {
        // Set the gameobject's position to be the center of mass
        transform.position = center;

        // Update the mesh relative to the transform
        var v0Relative = Vector2.zero;        
        _meshFilter.mesh = EllipseMesh(sigmaR, sigmaTheta, dis, center, fillColor);


        // Update the shape's outline
        _lineRenderer.positionCount = _meshFilter.mesh.vertices.Length;
        _lineRenderer.SetPositions(_meshFilter.mesh.vertices);
    }

    /// <summary>
    /// Creates and returns a circle mesh given two vertices on its center 
    /// and any outer edge point.
    /// </summary>
    private static Mesh EllipseMesh(float sigmaR, float sigmaTheta, float dis, Vector2 center, Color fillColor)
    {
        // We want to make sure that the circle appears to be curved.
        // This can be approximated by drawing a regular polygon with lots of segments.
        // The number of segments can be increased based on the radius so that large circles also appear curved.
        // We use an offset and multiplier to create a tunable linear function.
        const float segmentOffset = 40f;
        const float segmentMultiplier = 5f;
        Vector2 polarCenter = CartesianToPolar(center);
        float r = polarCenter.x;
        float theta = polarCenter.y;
        var numSegments = (int)(r * segmentMultiplier + segmentOffset);
        //Quaternion q = Quaternion.AngleAxis(angle, Vector3.forward);

        // Create an array of points arround a cricle
        var circleVertices = Enumerable.Range(0, numSegments)
            .Select(i => {
                var angle = 2 * Mathf.PI * i / numSegments;
                var pos = new Vector2(r + dis * sigmaR * Mathf.Cos(angle), theta + dis * sigmaTheta * Mathf.Sin(angle));
                return PolarToCartesian(pos);
                //return pos+center;
            })
            .ToArray();

        foreach (var point in circleVertices)
        {

        }
            

        // Find all the triangles in the shape
        var triangles = new Triangulator(circleVertices).Triangulate();

        // Assign each vertex the fill color
        var colors = Enumerable.Repeat(fillColor, circleVertices.Length).ToArray();

        var mesh = new Mesh
        {
            name = "Circle",
            vertices = circleVertices.ToVector3(),
            triangles = triangles,
            colors = colors
        };

        mesh.RecalculateNormals();
        mesh.RecalculateBounds();
        mesh.RecalculateTangents();

        return mesh;
    }

    private static Vector2 CartesianToPolar(Vector2 point)
    {
        var polar = Vector2.zero;
        polar.x = new Vector2(point.x, point.y).magnitude;
        polar.y = Mathf.Atan2(point.x, point.y);
        return polar;
    }


    private static Vector2 PolarToCartesian(Vector2 polar)
    {
        return new Vector2(polar.x*Mathf.Cos(polar.y), polar.x * Mathf.Sin(polar.y));
    }
}
