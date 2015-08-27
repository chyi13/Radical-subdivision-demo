#include "RadicalLOD.h"

RadicalLOD::RadicalLOD(void)
{
	buildRecover = false;
}

void RadicalLOD::render()
{
	if (mSubLevel >0)
	{
		RadicalLODLevel *plod = &lod;

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
			glColor3f(1.0f,1.0f,1.0f);
			glLineWidth(2.f);
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
			glLineWidth(1.f);
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



void RadicalLOD::subdivision()
{
	LOD_VERTEX *pNewVert;			// new vertex
	LOD_FACE   *pNewFace;			// new face
	int iNewVertNum = m_iVertNum + m_iFaceNum; 	// vertex number
	int iNewFaceNum = m_iFaceNum * 3;			// face number

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
		pNewVert[iCurVert].point.x = float(m_pVert[m_pFace[i].vertIndex[0]].point.x+m_pVert[m_pFace[i].vertIndex[1]].point.x+m_pVert[m_pFace[i].vertIndex[2]].point.x)/3;
		pNewVert[iCurVert].point.y = float(m_pVert[m_pFace[i].vertIndex[0]].point.y+m_pVert[m_pFace[i].vertIndex[1]].point.y+m_pVert[m_pFace[i].vertIndex[2]].point.y)/3;
		pNewVert[iCurVert].point.z = float(m_pVert[m_pFace[i].vertIndex[0]].point.z+m_pVert[m_pFace[i].vertIndex[1]].point.z+m_pVert[m_pFace[i].vertIndex[2]].point.z)/3;
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
	
	printf("V - E + F = %d\n", m_iVertNum - edgemap.size()/2 + m_iFaceNum);

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
				int R = hf_findFaceVert(v2,v1);

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

	// save
	saveToPly();

	m_iMaxLevel++;
}

bool RadicalLOD::saveToPly()
{
	char t_sFileN[255];
	sprintf(t_sFileN, "./generate/%s_radical-%d.ply", m_sFilename, m_iMaxLevel);

	printf("%s\n", t_sFileN);

	FILE* t_pFile = fopen(t_sFileN, "r");
	if (t_pFile != nullptr)		// check if the file already exists.
	{
		printf("Error: Already has .ply file\n");	// exist just return;
	}
	else				// if not, create the file
	{
		t_pFile = fopen(t_sFileN, "w");

		fprintf(t_pFile,"ply\nformat ascii 1.0\n"
			"comment File created by Chai Yi's thesis project\n"
			"element vertex %d\n"
			"property float x\n"
			"property float y\n"
			"property float z\n"
			"element face %d\n"
			"property list uchar int vertex_indices\n"
			"end_header\n",m_iVertNum, m_iFaceNum);		// vertex num, face num

		for (int i = 0; i<m_iVertNum; i++)
		{
			fprintf(t_pFile, "%f %f %f\n", m_pVert[i].point.x,
				m_pVert[i].point.y, m_pVert[i].point.z);
		}

		for (int i = 0; i<m_iFaceNum; i++)
		{
			fprintf(t_pFile, "3 %d %d %d\n", m_pFace[i].vertIndex[0],
				m_pFace[i].vertIndex[1], m_pFace[i].vertIndex[2]);
		}
	}

	fclose(t_pFile);
	return true;
}

void RadicalLOD::buildAllLevels()
{
	RadicalLODLevel* p = &lod;

	p->initLL(m_iVertNum,m_pVert,m_iFaceNum,m_pFace);
	strcpy(p->m_sLODName, m_sFilename);

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

bool RadicalLOD::recoverAllLevels()
{
	if (buildRecover)
	{
		printf("Error: Already built!\n\r");
		return false;
	}
	else	// if the recover sequence has not been built. then lodMesh is initialized by the coarest lodlevel
	{
		printf("Recovering from files...\n");

		RadicalLODLevel* p = &lod;
		while(p->next) p = p->next;

		RadicalLODMeshLevel* mp = &lodMesh;
		mp->initLL(p->m_iVertNum, p->m_pVert, p->m_iFaceNum, p->m_pFace);	// initialize the first level
		mp->level = p->level;
		strcpy(mp->m_sLODName, p->m_sLODName);

		while (mp->radicalReverseSubdivide())
		{
			mp = mp->next;
		}
		printf("Recovery success!\n");
		printf("Compressed model has been generated!\n");
		
		buildRecover = true;
	}
	return true;
}

RadicalLOD::~RadicalLOD(void)
{
}



