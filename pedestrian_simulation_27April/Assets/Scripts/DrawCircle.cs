using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

public class DrawCircle : MonoBehaviour {
    
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

    public void SetupCircle(Vector3 center, float radius, Color fillColor, GameObject parent)
    {
        // Set the gameobject's position to be the center of mass
        transform.position = center;

	if (parent != null)
	    transform.parent = parent.transform;

        // Update the mesh relative to the transform
        var v0Relative = Vector2.zero;        
        _meshFilter.mesh = CircleMesh(v0Relative, radius, fillColor);

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
    private static Mesh CircleMesh(Vector2 center, float radius, Color fillColor)
    {
        // We want to make sure that the circle appears to be curved.
        // This can be approximated by drawing a regular polygon with lots of segments.
        // The number of segments can be increased based on the radius so that large circles also appear curved.
        // We use an offset and multiplier to create a tunable linear function.
        const float segmentOffset = 40f;
        const float segmentMultiplier = 5f;
        var numSegments = (int)(radius * segmentMultiplier + segmentOffset);

        // Create an array of points arround a cricle
        var circleVertices = Enumerable.Range(0, numSegments)
            .Select(i => {
                var theta = 2 * Mathf.PI * i / numSegments;
                return new Vector2(Mathf.Cos(theta), Mathf.Sin(theta)) * radius;
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
}
