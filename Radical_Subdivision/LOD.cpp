#include "LOD.h"
#include <vector>

LOD::LOD(void)
{
	m_iRenderMethod = GL_LINE_LOOP;
	maxLevel = 0;
	mSubLevel = 0;
	total_time = 0;
	buildRecover = false;
}

bool LOD::loadFromFile(const char* filename)
{
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

bool LOD::load_ase_file(const char* filename)
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

bool LOD::load_ply_file(const char* filename)
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

void LOD::computeNormals()
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

void LOD::computeValence()
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

	//
	FILE* temp;
	temp = fopen("debug.log","a");
	fprintf(temp,"*******\n");
	for (i = 0; i<m_iVertNum; i++)
		fprintf(temp,"%d\n",m_pVert[i].valence);
	fclose(temp);
}

void LOD::sortAdjVert()
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

void LOD::renderAse()
{
	m_ASEfile.render();
}

void LOD::render()
{
	if (mSubLevel >0)
	{
		LODLevel *plod = &lod;

		// find the right level
		int level = 0;
		level = mSubLevel/4;
		while(plod && plod->level<level)
			plod = plod->next;

		// subLevel
		// 0 current lod
		// 1 even and odd vertices
		// 2 predict error
		// 3 next lod
		int subLevel = mSubLevel - level*4;
		if (subLevel == 0 || subLevel == 1 || subLevel == 3)
		{
			for (int i = 0; i<plod->m_iFaceNum; i++)
			{
				glBegin(m_iRenderMethod);
				glNormal3f(plod->m_pFace[i].normal.x,
					plod->m_pFace[i].normal.y,
					plod->m_pFace[i].normal.z);
				for (int j = 0; j<3; j++)
				{
					glVertex3f(plod->m_pVert[plod->m_pFace[i].vertIndex[j]].point.x,
						plod->m_pVert[plod->m_pFace[i].vertIndex[j]].point.y,
						plod->m_pVert[plod->m_pFace[i].vertIndex[j]].point.z);
				}
				glEnd();
			}
		}

		// display even and odd vertices
		if (subLevel == 1)
		{
			glPointSize(8.0f);
			glBegin(GL_POINTS);

			for (int i =0; i<plod->m_iVertNum; i++)
			{
				// even is red
				if (plod->m_pVert[i].even == EVEN)
					glColor3f(1.0f,0.0,0.0);
				else
					glColor3f(0.0,0.0,1.0f);
				glVertex3f(plod->m_pVert[i].point.x,
					plod->m_pVert[i].point.y,
					plod->m_pVert[i].point.z);
			}

			glEnd();
		}

		// display predict error
		if (subLevel == 2 && plod->next)
		{
			glColor3f(1.0f,0.0f,1.0f);
			for (int i =0; i<plod->next->m_iErrNum; i++)
			{
				glBegin(GL_LINES);
				// original vertex
				glVertex3f(plod->m_pVert[plod->next->m_pError[i].vertexIndex].point.x,
					plod->m_pVert[plod->next->m_pError[i].vertexIndex].point.y,
					plod->m_pVert[plod->next->m_pError[i].vertexIndex].point.z);
				// new vertex
				glVertex3f(plod->m_pVert[plod->next->m_pError[i].vertexIndex].point.x - plod->next->m_pError[i].errXYZ[0],
					plod->m_pVert[plod->next->m_pError[i].vertexIndex].point.y - plod->next->m_pError[i].errXYZ[1],
					plod->m_pVert[plod->next->m_pError[i].vertexIndex].point.z - plod->next->m_pError[i].errXYZ[2]);

				glEnd();
			}
		}

		// next lod
		if (subLevel == 3 && plod->next)
		{
			glColor3f(1.0f,0.0,1.0);

			for (int i =0; i<plod->m_iVertNum; i++)
			{
				if (plod->m_pVert[i].even == EVEN)
				{
					glBegin(GL_LINES);
					glVertex3f(plod->m_pVert[i].point.x,
						plod->m_pVert[i].point.y,
						plod->m_pVert[i].point.z);
					//		glVertex3f(plod->next->m_p
					glEnd();
				}
			}
		}

	}
	if (maxLevel == 0)
	{
		renderSubdivision();
	}
}

void LOD::renderSubdivision()
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

void LOD::setWired()
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

bool LOD::trans2EulerPoly()
{
	int i,j,k;
	int errorFaceNum = 0;
	for (i = 0; i< m_iFaceNum; i++)
	{
		int edgeCount = 0;
		int a,b,c;
		a = m_pFace[i].vertIndex[0];
		b = m_pFace[i].vertIndex[1];
		c = m_pFace[i].vertIndex[2];

		for (j = 0; j< m_iFaceNum; j++)
		{
			if (i == j)
				continue;
			else
			{
				int d,e,f;
				d = m_pFace[j].vertIndex[0];
				e = m_pFace[j].vertIndex[1];
				f = m_pFace[j].vertIndex[2];

				if ((a == d && ( b == e || b == f)) ||
					(a == e && ( b == d || b == f)) ||
					(a == f && ( b == d || b == e)))
				{
					edgeCount ++;
					continue;
				}
				if ((a == d && ( c == e || b == f)) ||
					(a == e && ( c == d || b == f)) ||
					(a == f && ( c == d || b == e)))
				{
					edgeCount ++;
					continue;
				}
				if ((c == d && ( b == e || b == f)) ||
					(c == e && ( b == d || b == f)) ||
					(c == f && ( b == d || b == e)))
				{
					edgeCount ++;
					continue;
				}
			}
		}
		if (edgeCount != 0)
		{
			//		printf("Error: edge count not larger than 3!\n");
			//		printf("%d\n",edgeCount);
			errorFaceNum ++;
		}
	}
	printf("Error: error face num: %d\n\r",errorFaceNum);
	return false;
}

void LOD::radicalSubdivision()
{
	LOD_VERTEX *pNewVert;			// new vertex
	LOD_FACE   *pNewFace;			// new face
	int iNewVertNum = m_iVertNum + m_iFaceNum; 	// vertex number
	int iNewFaceNum = m_iFaceNum * 3;			// face number

	trans2EulerPoly();
	printf("Current Vertex: %d\nNew   Vertex: %d\n",m_iVertNum,iNewVertNum);

	// allocate memory space
	pNewVert = (LOD_VERTEX*)malloc(sizeof(LOD_VERTEX) * iNewVertNum);
	pNewFace = (LOD_FACE *) malloc(sizeof(LOD_FACE)* iNewFaceNum);

	// step 111111111111111
	// copy vertex to the new Vertex space
	int iCurVert;

	for (iCurVert = 0; iCurVert<m_iVertNum; iCurVert++)
	{
		pNewVert[iCurVert].index = iCurVert;
		pNewVert[iCurVert].point.x = m_pVert[iCurVert].point.x;
		pNewVert[iCurVert].point.y = m_pVert[iCurVert].point.y;
		pNewVert[iCurVert].point.z = m_pVert[iCurVert].point.z;
		pNewVert[iCurVert].valence = 0;
		pNewVert[iCurVert].normal.x = pNewVert[iCurVert].normal.y = pNewVert[iCurVert].normal.z = 0;
	}
	printf("step 1 completed\n");

	// step 22222222222222
	// update every face

	int v1,v2;
	int *va,*vb;			// mark edge id
	va = new int[iNewVertNum];
	vb = new int[iNewVertNum];

	// (1)add a face vertex 
	for (int i = 0; i<m_iFaceNum; i++)
	{

		// 
		//			a
		//		   / \
		//        / d \
		//      b/_____\c
		//				

		// save new VERTEX
		pNewVert[iCurVert].point.x = (m_pVert[m_pFace[i].vertIndex[0]].point.x+m_pVert[m_pFace[i].vertIndex[1]].point.x+m_pVert[m_pFace[i].vertIndex[2]].point.x)/3;
		pNewVert[iCurVert].point.y = (m_pVert[m_pFace[i].vertIndex[0]].point.y+m_pVert[m_pFace[i].vertIndex[1]].point.y+m_pVert[m_pFace[i].vertIndex[2]].point.y)/3;
		pNewVert[iCurVert].point.z = (m_pVert[m_pFace[i].vertIndex[0]].point.z+m_pVert[m_pFace[i].vertIndex[1]].point.z+m_pVert[m_pFace[i].vertIndex[2]].point.z)/3;
		pNewVert[iCurVert].index = iCurVert;
		pNewVert[iCurVert].valence = 0;
		pNewVert[iCurVert].normal.x = pNewVert[iCurVert].normal.y = pNewVert[iCurVert].normal.z = 0;

		m_pFace[i].faceVertexID = iCurVert; // add new face vertex id to current face

		iCurVert++;
	}
	printf("step 2 completed\n");
	// step 333333333333
	// create half edge
	createHalfEdge();

	printf("Creating hf completed\n");
	printf("Edges %d\n",edgemap.size()/2);

	printf("v-e+f: %d\n", m_iVertNum - edgemap.size()/2 + m_iFaceNum );
	// (2)find face
	// mark subdivided edge
	// traverse all three edge

	int tempFaceCount = 0;
	int edgeMarkCount = 0;

	for (int i = 0; i<m_iFaceNum; i++)
	{
		for (int j = 0; j<3; j++)
		{
			v1 = m_pFace[i].vertIndex[j];
			v2 = m_pFace[i].vertIndex[(j+1)%3];
			// check if edge<v1,v2> is marked.
			int edgeIndex = findIndex(v1,v2,va,vb,edgeMarkCount);

			if (edgeIndex == -1)
			{
				va[edgeMarkCount] = v1;
				vb[edgeMarkCount] = v2;
				edgeMarkCount++;
				// (3)every edge add two faces
				//			c
				//		   / \
				//        / L \
				//     v1/_____\v2
				//		 \     /
				//		  \ R /
				//		   \ /
				//			d	
				///////////////////////////////
				int c = hf_findThirdVert(v1,v2);
				int d = hf_findThirdVert(v2,v1);
				int L = hf_findFaceVert(v1,v2);
				int R = hf_findPairVert(v1,v2);

				// Face 1
				pNewFace[tempFaceCount].vertIndex[0] = v1;
				pNewFace[tempFaceCount].vertIndex[1] = R;
				pNewFace[tempFaceCount].vertIndex[2] = L;
				tempFaceCount++;

				// Face 2
				pNewFace[tempFaceCount].vertIndex[0] = v2;
				pNewFace[tempFaceCount].vertIndex[1] = L;
				pNewFace[tempFaceCount].vertIndex[2] = R;
				tempFaceCount++;
			}

		}

	}
	printf("step 3 completed\n");
	// step 44444444444
	// update vertices position

	for (int i = 0; i<m_iVertNum; i++)
	{
		//////////////////////////////////////////
		//
		//	V = (1-an)v + an/n (sigma(vi))
		//	
		//	an = (4-2cos(2pi/n))/9
		//
		/////////////////////////////////////////

		float an = 0.f;
		float sigma[3] = {0};
		int N = m_pVert[i].valence;

		for (int j = 0; j<N; j++)
		{
			sigma[0] += m_pVert[m_pVert[i].adj[j]].point.x;
			sigma[1] += m_pVert[m_pVert[i].adj[j]].point.y;
			sigma[2] += m_pVert[m_pVert[i].adj[j]].point.z;
		}

		an = (4.0 - 2*cos(2*PI/N))/9;

		pNewVert[i].point.x = (1-an)*m_pVert[i].point.x + an/N*sigma[0];
		pNewVert[i].point.y = (1-an)*m_pVert[i].point.y + an/N*sigma[1];
		pNewVert[i].point.z = (1-an)*m_pVert[i].point.z + an/N*sigma[2];

	}


	// step 55555555555
	// update data pointers
	iNewVertNum = iCurVert;
	LOD_VERTEX* pTempVert;
	pTempVert = (LOD_VERTEX*)malloc(sizeof(LOD_VERTEX)*iNewVertNum);
	memcpy(pTempVert,pNewVert,sizeof(LOD_VERTEX)*iNewVertNum);
	free(pNewVert);
	pNewVert = pTempVert;

	m_iVertNum = iNewVertNum;
	//	printf("face count1 = %d count2 = %d\n",m_iFaceNum,tempFaceCount);
	m_iFaceNum = tempFaceCount;
	free(m_pVert);
	m_pVert = pNewVert;
	free(m_pFace);
	m_pFace = pNewFace;

	// update valence & normals
	computeNormals();

	computeValence();

	// sort
	sortAdjVert();
}

void LOD::loopSubdivision()
{
	LOD_VERTEX *pNewVert;			// new vertex
	LOD_FACE   *pNewFace;			// new face
	int *va,*vb;					//新点对应边的顶点index
	int iNewVertNum = m_iVertNum * 4; 	// vertex number
	int iNewFaceNum = m_iFaceNum * 4;			// face number
	float w = 1.0/8.0;				//细分w参数	

	// allocate memory space
	pNewVert = (LOD_VERTEX*)malloc(sizeof(LOD_VERTEX) * iNewVertNum);
	pNewFace = (LOD_FACE *) malloc(sizeof(LOD_FACE)* iNewFaceNum);
	va = new int[iNewVertNum];
	vb = new int[iNewVertNum];

	// step 111111111111111
	// copy vertex to the new Vertex space
	int iCurVert;

	for (iCurVert = 0; iCurVert<m_iVertNum; iCurVert++)
	{
		pNewVert[iCurVert].index = iCurVert;
		pNewVert[iCurVert].point.x = m_pVert[iCurVert].point.x;
		pNewVert[iCurVert].point.y = m_pVert[iCurVert].point.y;
		pNewVert[iCurVert].point.z = m_pVert[iCurVert].point.z;
		pNewVert[iCurVert].valence = 0;
		pNewVert[iCurVert].normal.x = pNewVert[iCurVert].normal.y = pNewVert[iCurVert].normal.z = 0;
	}

	// step 1212121212121212
	createHalfEdge();

	// step 222222222222222
	// add face vertex to every face
	int v1,v2,f1,f2,newVert[3];
	for (int iCurFace = 0; iCurFace<m_iFaceNum; iCurFace++)
	{
		for (int i = 0; i<3; i++)
		{
			newVert[i] = findIndex(m_pFace[iCurFace].vertIndex[i],m_pFace[iCurFace].vertIndex[(i+1)%3],va,vb,iCurVert-m_iVertNum);
			if(newVert[i] == -1)
			{	//未找到，表示未细分，开始细分
				//找Butterfly细分点
				//   e1___f1___e2
				//     \  /\  /
				//    v1\/__\/v2
				//      /\  /\
				//   e4/__\/__\e3
				//        f2

				v1 = m_pFace[iCurFace].vertIndex[i];
				v2 = m_pFace[iCurFace].vertIndex[(i+1)%3];
				f1 = hf_findThirdVert(v2,v1);
				f2 = m_pFace[iCurFace].vertIndex[(i+2)%3];

				// new vert position
				pNewVert[iCurVert].index = iCurVert;
				pNewVert[iCurVert].point.x = 3*w*(m_pVert[v1].point.x + m_pVert[v2].point.x )
					+  w * ( m_pVert[f1].point.x + m_pVert[f2].point.x );
				pNewVert[iCurVert].point.y = 3*w*(m_pVert[v1].point.y + m_pVert[v2].point.y )
					+  w * ( m_pVert[f1].point.y + m_pVert[f2].point.y );
				pNewVert[iCurVert].point.z = 3*w*(m_pVert[v1].point.z + m_pVert[v2].point.z )
					+  w * ( m_pVert[f1].point.z + m_pVert[f2].point.z );
				pNewVert[iCurVert].valence = 0;
				pNewVert[iCurVert].normal.x 
					= pNewVert[iCurVert].normal.y 
					= pNewVert[iCurVert].normal.z
					= 0;

				// mark subdivided edge
				va[iCurVert - m_iVertNum] = v1;
				vb[iCurVert - m_iVertNum] = v2;
				newVert[i] = iCurVert;

				iCurVert++;

				if(iCurVert>iNewVertNum){//新点数超出预算，重新分配空间，
					LOD_VERTEX *pTmpVert;	//顶点数组
					int *piTmp;
					pTmpVert = (LOD_VERTEX*)malloc(sizeof(LOD_VERTEX)*(iNewVertNum+m_iVertNum));
					memcpy(pTmpVert,pNewVert,sizeof(LOD_VERTEX)*iNewVertNum);
					free(pNewVert);
					pNewVert = pTmpVert;

					piTmp = new int[iNewVertNum+m_iVertNum];
					memcpy(piTmp,va,sizeof(int)*iNewVertNum);
					delete []va;
					va = piTmp;

					piTmp = new int[iNewVertNum+m_iVertNum];
					memcpy(piTmp,vb,sizeof(int)*iNewVertNum);
					delete []vb;
					vb = piTmp;

					iNewVertNum = iNewVertNum+m_iVertNum;
				}
			}
			else	// if find 
			{
				newVert[i] += m_iVertNum;	// i is index in [va,vb], so we need add m_iVertNum
			}
		}
		// step 33333333333333333
		// update with 4 new faces
		//           i0
		//          /\
		//       n2/__\n0
		//        /\  /\   
		//     i2/__\/__\ i1
		//          n1

		pNewFace[iCurFace*4].vertIndex[0] = m_pFace[iCurFace].vertIndex[0];
		pNewFace[iCurFace*4].vertIndex[1] = newVert[0];
		pNewFace[iCurFace*4].vertIndex[2] = newVert[2];
		pNewFace[iCurFace*4].normal.x 
			= pNewFace[iCurFace*4].normal.y
			= pNewFace[iCurFace*4].normal.z
			= 0;
		//每二个面:
		pNewFace[iCurFace*4+1].vertIndex[0] = newVert[0];
		pNewFace[iCurFace*4+1].vertIndex[1] = newVert[1];
		pNewFace[iCurFace*4+1].vertIndex[2] = newVert[2];
		pNewFace[iCurFace*4+1].normal.x 
			= pNewFace[iCurFace*4+1].normal.y
			= pNewFace[iCurFace*4+1].normal.z
			= 0;
		//每三个面:
		pNewFace[iCurFace*4+2].vertIndex[0] = newVert[0];
		pNewFace[iCurFace*4+2].vertIndex[1] = m_pFace[iCurFace].vertIndex[1];
		pNewFace[iCurFace*4+2].vertIndex[2] = newVert[1];
		pNewFace[iCurFace*4+2].normal.x 
			= pNewFace[iCurFace*4+2].normal.y
			= pNewFace[iCurFace*4+2].normal.z
			= 0;
		//每四个面:
		pNewFace[iCurFace*4+3].vertIndex[0] = newVert[1];
		pNewFace[iCurFace*4+3].vertIndex[1] = m_pFace[iCurFace].vertIndex[2];
		pNewFace[iCurFace*4+3].vertIndex[2] = newVert[2];
		pNewFace[iCurFace*4+3].normal.x 
			= pNewFace[iCurFace*4+3].normal.y
			= pNewFace[iCurFace*4+3].normal.z
			= 0;
	}
	// step 44444444444444444444444444444444
	// update original vertices

	int ver1, ver2, find,nearpoint[100];
	float mid,kb,midx,midy,midz;
	for (int iC = 0; iC<m_iVertNum; iC++)
	{
		int i=0, nearnum=0, find0=0;
		for(int iCurface=0;iCurface<m_iFaceNum;iCurface++){ //for1
			if (m_pFace[iCurface].vertIndex[0]==iC)
			{ 
				ver1=m_pFace[iCurface].vertIndex[1];
				ver2=m_pFace[iCurface].vertIndex[2];
				find0=1;
			}
			if (m_pFace[iCurface].vertIndex[1]==iC)
			{
				ver1=m_pFace[iCurface].vertIndex[2];
				ver2=m_pFace[iCurface].vertIndex[0];
				find0=1;
			}
			if (m_pFace[iCurface].vertIndex[2]==iC)
			{ 
				ver1=m_pFace[iCurface].vertIndex[0];
				ver2=m_pFace[iCurface].vertIndex[1];
				find0=1;
			}
			if (find0==1) //找到含有该顶点的面
			{
				find=0;
				for (i=1;i<nearnum+1;i++)
					if (nearpoint[i]==ver1)
					{ 
						find=1;   
						break;  
					}

					if (find==0) 
					{
						nearnum++;
						nearpoint[nearnum]=ver1; 
					}
					find=0;
					for (i=1;i<nearnum+1;i++)
						if (nearpoint[i]==ver2) 
						{find=1;   break; }
						if (find==0) { 
							nearnum++;
							nearpoint[nearnum]=ver2; }
			} //end find0

			find0=0;
		} //end iface

		nearpoint[0]=nearnum;  
		if (nearnum==3) kb=3.0/16.0;
		else {  mid= 3.0/8.0+1.0/4.0*cos(2*PI/nearnum);
		kb=1.0/nearnum*(5.0/8.0-mid*mid);
		}

		midx=midy=midz=0.0;
		for (i=1;i<=nearnum;i++)
		{
			midx += m_pVert[nearpoint[i]].point.x;
			midy += m_pVert[nearpoint[i]].point.y;
			midz += m_pVert[nearpoint[i]].point.z;
		}

		pNewVert[iC].point.x = (1.0-nearnum*kb)*m_pVert[iC].point.x + kb * midx;
		pNewVert[iC].point.y = (1.0-nearnum*kb)*m_pVert[iC].point.y + kb * midy;
		pNewVert[iC].point.z = (1.0-nearnum*kb)*m_pVert[iC].point.z + kb * midz;

	}

	// step 55555555555555555555555555555555
	// copy data
	iNewVertNum = iCurVert;
	LOD_VERTEX *pTmpVert;	//顶点数组
	pTmpVert = (LOD_VERTEX*)malloc(sizeof(LOD_VERTEX)*(iNewVertNum));
	memcpy(pTmpVert,pNewVert,sizeof(LOD_VERTEX)*iNewVertNum);
	free(pNewVert);
	pNewVert = pTmpVert;

	//
	m_iVertNum = iNewVertNum;
	m_iFaceNum = iNewFaceNum;
	free(m_pVert);
	m_pVert = pNewVert;
	free(m_pFace);
	m_pFace = pNewFace;

	computeNormals();

	computeValence();
}

int LOD::findIndex(int a,int b,int* va,int* vb,int len)
{
	for (int i = 0; i<len; i++)
	{
		if ( ((va[i] == a)&&(vb[i] == b)) ||
			((va[i] == b)&&(vb[i] == a)))
			return i;
	}
	return -1; // cannot find edge<a,b>
}

void LOD::createHalfEdge()
{
	int numOfVertices=0;
	int numOfFaces=0;

	numOfVertices = m_iVertNum;
	numOfFaces = m_iFaceNum;

	vertices = new Vertex[numOfVertices];
	faces = new Face[numOfFaces];

	//	nFace = numOfFaces;


	int i;

	for(i=0 ; i<numOfVertices; i++){
		vertices[i].coord.x = m_pVert[i].point.x;
		vertices[i].coord.y = m_pVert[i].point.y;
		vertices[i].coord.z = m_pVert[i].point.z;
	}		


	for(i=0; i<numOfFaces ; i++){		

		int vi;
		std::vector<int> v;  //use vector to store vertices' id of which the current face consists

		std::vector<int>::const_iterator p;

		/*file>>type;
		while(file>>vi){					
		v.push_back(vi);			
		}*/
		v.push_back(m_pFace[i].vertIndex[0]);
		v.push_back(m_pFace[i].vertIndex[1]);
		v.push_back(m_pFace[i].vertIndex[2]);

		faces[i].ver = new Vertex[v.size()];
		faces[i].nPolygon = v.size();
		faces[i].faceVertexId = m_pFace[i].faceVertexID;	// add inner new face vertex id 

		he = new HalfEdge[v.size()];

		int j = 0;
		for(p= v.begin(); p != v.end(); p++){			
			//更改 *p
			faces[i].ver[j] = vertices[*p];		//access vertex from array of vertices according to its' id
			faces[i].ver[j].id = *p;			//store its id				
			j++;			
		}


		faces[i].firstEdge = &he[0];

		//construct halfedge
		for(j=0, p=v.begin() ; j<v.size(); j++,p++){
			faces[i].ver[j].startEdge = &he[j];
			he[j].head = &faces[i].ver[(j+1)%v.size()];
			he[j].leftf = &faces[i];
			he[j].next = &he[(j+1)%v.size()];

			if((p+1)!= v.end()){				
				Pair pair(*p, *(p+1));				
				edgemap.insert(EdgeMap::value_type(pair, &he[j]));  //insert to edgemap
			}else{				
				Pair pair(*p, *v.begin());
				edgemap.insert(EdgeMap::value_type(pair, &he[j]));	//insert to edgemap
			}			
		}


	}

	EdgeMap::const_iterator iter;
	//set up halfedge sym pointer
	for(iter = edgemap.begin(); iter != edgemap.end(); iter++){

		EdgeMap::const_iterator iter1;
		EdgeMap::const_iterator iter2;
		Pair p1(iter->first.a,iter->first.b);
		Pair p2(iter->first.b,iter->first.a);
		iter1 = edgemap.find(p1);
		iter2 = edgemap.find(p2);

		iter1->second->sym = iter2->second;
	}
}
int LOD::hf_findPairVert(int ia,int ib)
{
	Pair hfpair(ia,ib);
	EdgeMap::const_iterator hfiterator;
	hfiterator = edgemap.find(hfpair);  // find half edge pair

	if (hfiterator == edgemap.end())
		return -1;
	else
		return hfiterator->second->sym->leftf->faceVertexId;

}

int LOD::hf_findThirdVert(int ia,int ib)
{
	Pair hfpair(ia,ib);
	EdgeMap::const_iterator hfiterator;
	hfiterator=edgemap.find(hfpair);
	Vertex* hfvertex;
	hfvertex=hfiterator->second->leftf->ver;

	int i;
	for (i=0;i<3;i++)
	{
		if (hfvertex[i].id != ia && hfvertex[i].id != ib)
			return hfvertex[i].id;
	}

	return -1;
}

int LOD::hf_findFaceVert(int ia,int ib)
{
	Pair hfpair(ia,ib);
	EdgeMap::const_iterator hfiterator;
	hfiterator = edgemap.find(hfpair);

	if (hfiterator == edgemap.end())
		return -1;
	else
		return hfiterator->second->leftf->faceVertexId;
}

int LOD::hf_findPairThirdVert(int ia, int ib)
{
	Pair hfpair(ia,ib);
	EdgeMap::const_iterator hfiterator;
	hfiterator = edgemap.find(hfpair);
	Vertex* hfvertex = hfiterator->second->sym->leftf->ver;
	//////////////

	return hfvertex->id;
}

/////////////////////////////////////////////////////////////////////////////////////////

void LOD::nextLevel()
{
	if (maxLevel == 0)
		buildAllLevels();

	if ((mSubLevel+1) > maxLevel*4)
		return;
	else
		mSubLevel++;
}

void LOD::prevLevel()
{
	if (mSubLevel >0)
		mSubLevel--;
}

void LOD::buildAllLevels()
{
	LODLevel* p = &lod;

	p->initLL(m_iVertNum,m_pVert,m_iFaceNum,m_pFace);

	// input the threshold
	printf("Enter the threshold: ");
	scanf("%f", &(p->threshold));

	while(p->buildNextLevel())
	{
		total_time +=p->ext_time;
		p = p->next;
		maxLevel = p->level;
	}

	printf("Total building time: %f\n",total_time);
}

bool LOD::recoverAllLevels()
{
	if (buildRecover)
	{
		printf("Error: Already built!\n\r");
		return false;
	}
	else	// if the recover sequence has not been built. then lodMesh is initialized by the coarest lodlevel
	{

		LODLevel* p = &lod;
		while(p->next) p = p->next;
		lodMesh.loadLODLevel(p);

		LODMeshLevel* mp = &lodMesh; 
		while (mp->radicalReverseSubdivide())
		{
			mp = mp->next;
		}
		printf("Recovery success!\n");
		printf("Compressed model has been generated!\n");
	}
	return true;
}

LOD::~LOD(void)
{
}



