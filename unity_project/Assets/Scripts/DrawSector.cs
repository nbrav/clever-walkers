using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class DrawSector : MonoBehaviour {

    class Sector
    {
        public GameObject m_gSector = null;
        public float m_fConeLenght = 5.0f;
        public float m_fAngleOfView = 90.0f;
        public float m_vStartDistanceCone = 2.0f;
        public bool m_bShowCone = true;
        public float m_fSpan;
        public float m_fStartRadians;
        public float m_fCurrentRadians;
        public float m_fAngleRotation;
        public int m_iConeVisibilityPrecision = 20;
        public int m_iVertMax = 120;
        public int m_iTrianglesMax = 120;

        public Sector(float length, float angle, float startDistance)
        {
            m_fConeLenght = length;
            m_fAngleOfView = angle;
            m_vStartDistanceCone = startDistance;
        }

        public Sector()
        {
        }
    }

    /* Fov Properties */
    private Material m_matVisibilityCone = null;
    public Material m_matCollision = null;
    public Material m_matFree = null;
    public bool m_bHasStartDistance = true;
    private float m_fFixedCheckNextTime;

    /* Render Properties */
    //public int m_iConeVisibilityPrecision = 20;
    //public  float       m_fDistanceForRender        = 600.0f;

    //float[] radius_annulus = new float[] { 10.0f, 20.0f };
    //float[] angle_sector = new float[] { -45.0f, -15.0f, 15.0f, 45.0f };
    float[] radius_annulus;
    float[] angle_sector;
    float[] rotation_angle;
    private Sector[,] m_Sectors;
    float[] angleOfViews;
    private GameObject m_goVisibilityCone = null;
    //private int m_iVertMax = 120;
    //private int m_iTrianglesMax = 120;

    private QAgent m_sQAgent;
    private List<Vector2> m_lColorList = new List<Vector2>();

    private void Awake()
    {
        m_sQAgent = this.GetComponent<QAgent>();
        if (m_sQAgent == null)
        {
            Debug.LogError("Draw sector should be attached to Q Agent");
        }
        radius_annulus = m_sQAgent.get_radius_array();
        angle_sector = m_sQAgent.get_anlge_array();
    }

    void Start()
    {
        InitAIConeDetection();
        m_matVisibilityCone = m_matFree;
    }

    void Update()
    {
        UpdateAIConeDetection();
    }

    private void InitAIConeDetection()
    {
        m_Sectors = new Sector[radius_annulus.Length, angle_sector.Length];
        angleOfViews = new float[angle_sector.Length];
        rotation_angle = new float[angle_sector.Length];

        for (int i = 0; i < angleOfViews.Length-1; i++)
        {
            angleOfViews[i] = (angle_sector[i + 1] - angle_sector[i])/2.0f;
            rotation_angle[i] = (angle_sector[i] + angle_sector[i + 1]) / 2.0f;
        }
        angleOfViews[angleOfViews.Length - 1] = (360.0f - (angle_sector[angle_sector.Length - 1]-angle_sector[0]))/2.0f;
        rotation_angle[angleOfViews.Length - 1] = 180.0f;
        float startLength = 0.0f;
        float startLengthCache = startLength;
        //float radiusAccum = 0.0f;

        for (int i=0;i< radius_annulus.Length;i++) 
        {            
            for (int j = 0; j < angle_sector.Length; j++)
            {
                m_Sectors[i, j] = new Sector(radius_annulus[i], angleOfViews[j], startLength);
                m_Sectors[i, j].m_iConeVisibilityPrecision = (int)(m_Sectors[i, j].m_fAngleOfView / 360.0f * 200.0f);
                m_Sectors[i, j].m_iVertMax = m_Sectors[i, j].m_iConeVisibilityPrecision * 2 + 2;
                m_Sectors[i, j].m_iTrianglesMax = m_Sectors[i, j].m_iConeVisibilityPrecision * 2;

                float fStartRadians = (360.0f - (angleOfViews[j])) * Mathf.Deg2Rad;
                float fCurrentRadians = fStartRadians;
                float fSpan = (angleOfViews[j]) / m_Sectors[i, j].m_iConeVisibilityPrecision * Mathf.Deg2Rad * 2.0f;
                //angleOfViews[j] *= 0.5f;

                
                m_Sectors[i, j].m_fStartRadians = fStartRadians;
                m_Sectors[i, j].m_fCurrentRadians = fCurrentRadians;
                m_Sectors[i, j].m_fSpan = fSpan;
                m_Sectors[i, j].m_fAngleRotation = rotation_angle[j];
                m_Sectors[i,j].m_gSector = GameObject.CreatePrimitive(PrimitiveType.Cube);
                Component.Destroy(m_Sectors[i,j].m_gSector.GetComponent<BoxCollider>());
                m_Sectors[i,j].m_gSector.name = this.name + "_VisConeMesh_radium"+i+"_sector"+j;
                Mesh sectorMesh=new Mesh();
                m_Sectors[i,j].m_gSector.GetComponent<MeshFilter>().mesh = sectorMesh;
                Vector3[] sectorVertices = new Vector3[m_Sectors[i, j].m_iVertMax];
                int[] sectorTriangles = new int[m_Sectors[i, j].m_iTrianglesMax * 3];                
                Vector3[] sectorvNormals = new Vector3[m_Sectors[i, j].m_iVertMax];
                sectorMesh.vertices = sectorVertices;
                sectorMesh.triangles = sectorTriangles;

                Vector2[] sectorvUV = new Vector2[sectorMesh.vertices.Length];
                sectorMesh.uv = sectorvUV;
                m_Sectors[i,j].m_gSector.GetComponent<Renderer>().material = m_matVisibilityCone;

                for (int k = 0; k < m_Sectors[i, j].m_iVertMax; k++)
                {
                    sectorvNormals[k] = Vector3.up;
                }

                sectorMesh.normals = sectorvNormals;

            }
            startLength = startLengthCache + radius_annulus[i];
            
        }

    }

    private void UpdateAIConeDetection()
    {
        m_lColorList = m_sQAgent.get_color_list();
        if (m_lColorList.Count != 0)
        {
            for (int i = 0; i < radius_annulus.Length; i++)
            {
                for (int j = 0; j < angle_sector.Length; j++)
                {
                    if (m_lColorList.Contains(new Vector2((float)i, (float)j)))
                    {
                        m_matVisibilityCone = m_matCollision;
                        m_Sectors[i, j].m_gSector.GetComponent<Renderer>().material = m_matVisibilityCone;
                        DrawVisibilityCone2(m_Sectors[i, j]);
                    }
                    else
                    {
                        m_matVisibilityCone = m_matFree;
                        m_Sectors[i, j].m_gSector.GetComponent<Renderer>().material = m_matVisibilityCone;
                        DrawVisibilityCone2(m_Sectors[i, j]);
                    }

                }
            }
        }
        else
        {
            for (int i = 0; i < radius_annulus.Length; i++)
            {
                for (int j = 0; j < angle_sector.Length; j++)
                {
                    m_matVisibilityCone = m_matCollision;
                }
            }
        }
        
    }

    public void DisableCone()
    {
        for (int i = 0; i < radius_annulus.Length; i++)
        {
            for (int j = 0; j < angle_sector.Length; j++)
            {
                m_Sectors[i, j].m_gSector.GetComponent<MeshFilter>().mesh.Clear();
            }
        }
    }

    private void DrawVisibilityCone2(Sector sector)
    {
        sector.m_fCurrentRadians = sector.m_fStartRadians;        
        Vector3 CurrentVector = this.transform.forward;
        CurrentVector = Quaternion.Euler(0, sector.m_fAngleRotation, 0) * CurrentVector;
        Vector3 DrawVectorCurrent = this.transform.forward;
        Vector3[] cacheVertices = new Vector3[sector.m_iVertMax];
        Vector3[] cachevNormals = new Vector3[sector.m_iVertMax];

        int index = 0;
        for (int i = 0; i < sector.m_iConeVisibilityPrecision + 1; ++i)
        {

            float newX = CurrentVector.x * Mathf.Cos(sector.m_fCurrentRadians) - CurrentVector.z * Mathf.Sin(sector.m_fCurrentRadians);
            //float newZ = CurrentVector.y * Mathf.Sin( m_fCurrentRadians ) + CurrentVector.z * Mathf.Cos( m_fCurrentRadians );
            float newZ = CurrentVector.x * Mathf.Sin(sector.m_fCurrentRadians) + CurrentVector.z * Mathf.Cos(sector.m_fCurrentRadians);

            DrawVectorCurrent.x = newX;
            DrawVectorCurrent.y = 0.0f;
            DrawVectorCurrent.z = newZ;

            //float Angle       = 90.0f;
            //DrawVectorCurrent = Quaternion.Euler( 0.0f, 0.0f, Angle ) * DrawVectorCurrent;

            sector.m_fCurrentRadians += sector.m_fSpan;

            /* Calcoliamo dove arriva il Ray */
            float FixedLenght = sector.m_fConeLenght;
            /* Adattiamo la mesh alla superfice sulla quale tocca */

            Vector3 startPosition = Vector3.zero;
            startPosition = this.transform.position;
            startPosition.y += 1.0f;

            if (m_bHasStartDistance)
            {
                cacheVertices[index]= startPosition + DrawVectorCurrent.normalized * sector.m_vStartDistanceCone;
                //m_vVertices[index] = this.transform.position + DrawVectorCurrent.normalized * sector.m_vStartDistanceCone;
            }
            else
            {
                cacheVertices[index] = startPosition;
                //m_vVertices[index] = this.transform.position;
            }

            cacheVertices[index+1] = startPosition + DrawVectorCurrent.normalized * FixedLenght;
            //m_vVertices[ index + 1 ].y  = this.transform.position.y;

            //Color color;
            //if (bFoundWall)
            //    color = Color.red;
            //else
            //    color = Color.yellow;

            //Debug.DrawLine(startPosition, cacheVertices[index + 1], Color.yellow);
            index += 2;
        }

        if (sector.m_bShowCone)
        {
            int[] _iTriangles = new int[sector.m_iTrianglesMax * 3];

            int localIndex = 0;
            for (int j = 0; j < sector.m_iTrianglesMax * 3; j = j + 6)
            {
                _iTriangles[j] = localIndex;
                _iTriangles[j + 1] = localIndex + 3;
                _iTriangles[j + 2] = localIndex + 1;

                _iTriangles[j + 3] = localIndex + 2;
                _iTriangles[j + 4] = localIndex + 3;
                _iTriangles[j + 5] = localIndex;

                localIndex += 2;
            }
            Mesh sectorMesh = sector.m_gSector.GetComponent<MeshFilter>().mesh;

            cachevNormals = sectorMesh.normals;
            sectorMesh.Clear();
            sectorMesh.vertices = cacheVertices;
            sectorMesh.triangles = _iTriangles;
            sectorMesh.normals = cachevNormals;
            sectorMesh.RecalculateNormals();
            //sector.m_gSector.GetComponent<MeshFilter>().mesh = sectorMesh;
        }
        else
        {
            DisableCone();
        }

    }
}
