#include "LODBase.h"


LODBase::LODBase(void)
{
	m_iRenderMethod = GL_LINE_LOOP;
	m_iMaxLevel = 0;
	
	maxLevel = 0;
	mSubLevel = 0;

	total_time = 0;
}


LODBase::~LODBase(void)
{
}

bool LODBase::loadFromFile(const char* filename)
{
	char t_fn[255];
	strcpy(t_fn, filename);

	int i;
	for (i = 0; i<strlen(t_fn); i++)
	{
		if (t_fn[i] == '.')
			break;
	}
	memset(m_sFilename,0, sizeof(m_sFilename));
	strncpy(m_sFilename, t_fn, i);	// get rid of the suffix

	if (strstr(filename, ".ply"))
	{
		return load_ply_file(filename);
	}
	else if (strstr(filename, ".ase"))
	{
		return load_ase_file(filename);
	}
	else
	{
		printf("Error: Unknonw file format!\n");
		return false;
	}
}

bool LODBase::load_ase_file(const char* filename)
{
	if (!m_ASEfile.loadfile(filename))
		return false;
	// some other initial stuff

	//
	//	load ase object detail
	//

	ASE_OBJECT_HEAD* l_pAseObjectHead  = m_ASEfile.getase_head();
	ASE_OBJECT*		 l_pAseObject = l_pAseObjectHead->head;

	m_iObjectNum = l_pAseObjectHead->object_number;	// NUM is not right


	m_iVertNum = l_pAseObject->vertex_num;
	m_iFaceNum = l_pAseObject->face_num;

	m_pVert	   = (LOD_VERTEX*)malloc(sizeof(LOD_VERTEX)*m_iVertNum);
	m_pFace	   = (LOD_FACE*)  malloc(sizeof(LOD_FACE)*m_iFaceNum);

	//	printf("vnum = %d,fnum = %d\n",m_iVertNum,m_iFaceNum);
	int i;
	for (i=0;i<m_iVertNum;i++)
	{
		m_pVert[i].point.x = l_pAseObject->vertex_list[i].x;
		m_pVert[i].point.y = l_pAseObject->vertex_list[i].y;
		m_pVert[i].point.z = l_pAseObject->vertex_list[i].z;

		m_pVert[i].normal.x=m_pVert[i].normal.y=m_pVert[i].normal.z=0.0;
		m_pVert[i].valence=0;

		m_pVert[i].even = UNKNOWN;
	}
	for (i=0;i<m_iFaceNum;i++)
	{
		m_pFace[i].vertIndex[0] = l_pAseObject->face_list[i].index[0];
		m_pFace[i].vertIndex[1] = l_pAseObject->face_list[i].index[1];
		m_pFace[i].vertIndex[2] = l_pAseObject->face_list[i].index[2];

		m_pFace[i].normal.x=m_pFace[i].normal.y=m_pFace[i].normal.z=0.0;
	}

	// compute normals
	computeNormals();

	// compute adjacence
	computeValence();

	// sort
	sortAdjVert();
	
	return true;
}

bool LODBase::load_ply_file(const char* filename)
{
	if (!m_ply.loadfile(filename))
		return false;
	PLY_OBJECT ply_obj;
	ply_obj = m_ply.getObject();

	m_iVertNum = ply_obj.vertex_num;
	m_iFaceNum = ply_obj.face_num;

	m_pVert	   = (LOD_VERTEX*)malloc(sizeof(LOD_VERTEX)*m_iVertNum);
	m_pFace	   = (LOD_FACE*)  malloc(sizeof(LOD_FACE)*m_iFaceNum);

	//	printf("vnum = %d,fnum = %d\n",m_iVertNum,m_iFaceNum);
	int i;
	for (i=0;i<m_iVertNum;i++)
	{
		m_pVert[i].point.x = ply_obj.vertex_list[i].x;
		m_pVert[i].point.y = ply_obj.vertex_list[i].y;
		m_pVert[i].point.z = ply_obj.vertex_list[i].z;

		m_pVert[i].normal.x=m_pVert[i].normal.y=m_pVert[i].normal.z=0.0;
		m_pVert[i].valence=0;

		m_pVert[i].even = UNKNOWN;
	}
	for (i=0;i<m_iFaceNum;i++)
	{
		m_pFace[i].vertIndex[0] = ply_obj.face_list[i].index[0];
		m_pFace[i].vertIndex[1] = ply_obj.face_list[i].index[1];
		m_pFace[i].vertIndex[2] = ply_obj.face_list[i].index[2];

		m_pFace[i].normal.x=m_pFace[i].normal.y=m_pFace[i].normal.z=0.0;
	}

	// compute normals
	computeNormals();

	// compute adjacence
	computeValence();

	// sort 
	sortAdjVert();
	
	return true;
}

void LODBase::computeNormals()
{
	int i,j;

	for(i = 0 ; i < m_iVertNum; i++)
	{
		m_pVert[i].normal.x = m_pVert[i].normal.y = m_pVert[i].normal.z= 0;
	}

	for(i = 0 ; i< m_iFaceNum; i++)
	{
		VERTEX planeVector[2];
		planeVector[0] = createVector(m_pVert[m_pFace[i].vertIndex[0]].point,m_pVert[m_pFace[i].vertIndex[1]].point);
		planeVector[1] = createVector(m_pVert[m_pFace[i].vertIndex[0]].point,m_pVert[m_pFace[i].vertIndex[2]].point);
		m_pFace[i].normal = cross(planeVector[0], planeVector[1]);

		normalize(m_pFace[i].normal);

		//	printf("%f %f %f\n",m_pFace[i].normal.x,m_pFace[i].normal.y,m_pFace[i].normal.z);

		for(j=0;j<3;j++)
		{
			m_pVert[m_pFace[i].vertIndex[j]].normal.x += m_pFace[i].normal.x;
			m_pVert[m_pFace[i].vertIndex[j]].normal.y += m_pFace[i].normal.y;
			m_pVert[m_pFace[i].vertIndex[j]].normal.z += m_pFace[i].normal.z;
		}
	}


	for(i = 0 ; i < m_iVertNum; i++)
	{
		normalize(m_pVert[i].normal);
	}
}

void LODBase::computeValence()
{
	// only for closed surface
	int i,j;
	for (i = 0; i<m_iVertNum; i++)
	{
		m_pVert[i].valence = 0;
	}

	// clockwise left hand 
	for (i = 0; i<m_iFaceNum; i++)
	{
		for (j = 0; j<3; j++)
		{
			m_pVert[m_pFace[i].vertIndex[j]].adj[m_pVert[m_pFace[i].vertIndex[j]].valence] = m_pFace[i].vertIndex[(j+1)%3];
			m_pVert[m_pFace[i].vertIndex[j]].valence++;
		}
	}

}

void LODBase::sortAdjVert()
{
	int i,j;
	// Now the somewhat harder part -- sort them by angle.  Pick an arbitrary one to start
	// with.
	for (i=0; i<m_iVertNum;i++)
	{
		if ( m_pVert[i].valence <= 2 ) continue;
		if ( m_pVert[i].valence > 6) return;
		// We'll use these.  A lot.
		VERTEX normal = m_pVert[i].normal;
		VERTEX vertex = createVector(m_pVert[i].point);

		// Use this to sort them.
		std::map<float, int> sortedVert;

		// Just start with the first one, all the others will be stored relative to it, in
		// CCW-wound order.
		int vertexNum = m_pVert[i].adj[0];
		sortedVert[0.0f] = vertexNum;

		VERTEX refVector = createVector(m_pVert[vertexNum].point);
		minus(refVector, vertex);

		// Project the reference (angle == 0) vector into the tangent plane.
		VERTEX projVec = normal;
		product(projVec, dot(refVector,normal));
		minus(refVector, projVec);
		////for debug
		//if( i == 11  && m_pVert[11].adj[0] == 403 )
		//{
		//	m_dpoint[0].x = refVector.x;
		//	m_dpoint[0].y = refVector.y;
		//	m_dpoint[0].z = refVector.z;
		//}

		normalize(refVector);

		for (j=1; j<m_pVert[i].valence; j++)
		{
			vertexNum = m_pVert[i].adj[j];
			if ( vertexNum == -1 ) continue;


			// Find the vector from the vertex along this edge.
			VERTEX vertVector = createVector(m_pVert[vertexNum].point);
			minus(vertVector,vertex);

			// Find the vector projected into the tangent plane.
			projVec = normal;
			product(projVec, dot(vertVector,normal));
			minus(vertVector, projVec);
			
			////for debug
			//if( i == 11  && m_pVert[11].adj[0] == 403 )
			//{
			//	m_dpoint[j].x = vertVector.x;
			//	m_dpoint[j].y = vertVector.y;
			//	m_dpoint[j].z = vertVector.z;
			//}

			normalize(vertVector);


			// Find the angle between it and the refence vector.  
			// Remember that v1 dot v2 = |v1||v2|cos(theta) = cos(theta) for normalized vectors.
			float cosTheta = dot(vertVector,refVector);
			if (cosTheta < -1.0f) cosTheta = -1.0f;
			if (cosTheta > 1.0f) cosTheta = 1.0f;
			float angle = acos(cosTheta);
			angle *= 180.0f / (float)PI;

			// To find the sign, it's a clockwise (negative) angle if the cross product points 
			// away from the tangent plane normal, so map it properly.
			VERTEX crossProd = cross(vertVector,refVector);
			if (dot(crossProd,normal) < 0)
			{
				angle = 360 - angle;
			}

			sortedVert[angle] = vertexNum;
		}

		// Okay, it's all sorted, put it into the vertex's edge list in the right order.
		std::map<float, int>::iterator it;
		j=1;
		for (it=sortedVert.begin(),it++; it!=sortedVert.end(); it++)
		{
			m_pVert[i].adj[j] = it->second;
			j++;
		}
	}
}

void LODBase::setWired()
{
	if (m_iRenderMethod == GL_LINE_LOOP)
	{
		m_iRenderMethod = GL_TRIANGLES;
		glEnable(GL_LIGHTING);
	}
	else
	{
		m_iRenderMethod = GL_LINE_LOOP;
		glDisable(GL_LIGHTING);
	}
}

void LODBase::renderSubdivision()
{
	int i;
	glColor3f(1.0f,0.0f,0.0f);			// Set The Color
	for(i = 0; i< m_iFaceNum; i++){
		glBegin(m_iRenderMethod);// 用OpenGL命令绘制三角形网格

		//这里设置面法向量...........
		glNormal3f(m_pFace[i].normal.x,m_pFace[i].normal.y,m_pFace[i].normal.z);

		//...........
		glNormal3f(m_pVert[m_pFace[i].vertIndex[0]].normal.x,m_pVert[m_pFace[i].vertIndex[0]].normal.y,m_pVert[m_pFace[i].vertIndex[0]].normal.z);
		glVertex3f(m_pVert[m_pFace[i].vertIndex[0]].point.x,m_pVert[m_pFace[i].vertIndex[0]].point.y,m_pVert[m_pFace[i].vertIndex[0]].point.z);

		glNormal3f(m_pVert[m_pFace[i].vertIndex[1]].normal.x,m_pVert[m_pFace[i].vertIndex[1]].normal.y,m_pVert[m_pFace[i].vertIndex[1]].normal.z);
		glVertex3f(m_pVert[m_pFace[i].vertIndex[1]].point.x,m_pVert[m_pFace[i].vertIndex[1]].point.y,m_pVert[m_pFace[i].vertIndex[1]].point.z);

		glNormal3f(m_pVert[m_pFace[i].vertIndex[2]].normal.x,m_pVert[m_pFace[i].vertIndex[2]].normal.y,m_pVert[m_pFace[i].vertIndex[2]].normal.z);
		glVertex3f(m_pVert[m_pFace[i].vertIndex[2]].point.x,m_pVert[m_pFace[i].vertIndex[2]].point.y,m_pVert[m_pFace[i].vertIndex[2]].point.z);

		/*glVertex3f(plod->x[plod->b[k]],plod->y[plod->b[k]],plod->z[plod->b[k]]);
		glVertex3f(plod->x[plod->c[k]],plod->y[plod->c[k]],plod->z[plod->c[k]]);*/
		glEnd();
	}
}

void LODBase::nextLevel()
{
	if (maxLevel == 0)
		buildAllLevels();

	if ((mSubLevel+1) > maxLevel*4)
		return;
	else
		mSubLevel++;
}

void LODBase::prevLevel()
{
	if (mSubLevel >0)
		mSubLevel--;
}
