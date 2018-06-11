using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

public class DrawEllipse : MonoBehaviour {
    
    //private Color _fillColor;
    private MeshFilter _meshFilter;
    private LineRenderer _lineRenderer;

    private void Awake()
    {
        _meshFilter = GetComponent<MeshFilter>();
        _lineRenderer = GetComponent<LineRenderer>();
    }

    public void SetupEllipse(float sigmaR, float sigmaTheta, Vector3 center, Color fillColor, GameObject parent)
    {
        // Set the gameobject's position to be the center of mass

	if (parent != null)
	    transform.parent = parent.transform;

	transform.localPosition = Vector3.zero;
	transform.localRotation = Quaternion.Euler(90, 0, 90);
	
        // Update the mesh relative to the transform
        _meshFilter.mesh = EllipseMesh(sigmaR, sigmaTheta, center, fillColor);

        // Update the shape's outline
        _lineRenderer.positionCount = _meshFilter.mesh.vertices.Length;
        _lineRenderer.SetPositions(_meshFilter.mesh.vertices);
    }

    public void UpdateMeshColor(Color fillColor)
    {
	Vector3[] vertices = _meshFilter.mesh.vertices;

        // create new colors array where the colors will be created.
        Color[] colors = new Color[vertices.Length];

        for (int i = 0; i < vertices.Length; i++)
            colors[i] = fillColor;

	_meshFilter.mesh.colors = colors;
    }

    /// <summary>
    /// Creates and returns a circle mesh given two vertices on its center 
    /// and any outer edge point.
    /// </summary>
    private static Mesh EllipseMesh(float sigmaR, float sigmaTheta, Vector3 center, Color fillColor)
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
	
        var numSegments = 360; //(int)(r*segmentMultiplier + segmentOffset);

        // Create an array of points arround a cricle
        var circleVertices = Enumerable.Range(0, numSegments)
            .Select(i => {
                var angle = 2 * Mathf.PI * i / numSegments;
                var pos = new Vector2(r + sigmaR * Mathf.Cos(angle), theta + sigmaTheta * Mathf.Sin(angle));
                return PolarToCartesian(pos);
            })
            .ToArray();

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

    private static Vector2 CartesianToPolar(Vector3 point)
    {
        return new Vector2(point.magnitude, Mathf.Atan2(point.x, point.z));
    }

    private static Vector2 PolarToCartesian(Vector2 polar)
    {
        return new Vector2(polar.x*Mathf.Cos(polar.y), polar.x*Mathf.Sin(polar.y));
    }
}
