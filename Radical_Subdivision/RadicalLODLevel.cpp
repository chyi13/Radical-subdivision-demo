#include "RadicalLODLevel.h"

RadicalLODLevel::RadicalLODLevel()
{
}

void RadicalLODLevel::initLL(int iVNum, LOD_VERTEX* pVert, int iFNum, LOD_FACE* pFace)
{
	ext_time = 0;
	level = 0;
	next = nullptr;

	m_iVertNum = iVNum;
	m_iFaceNum = iFNum;

	m_pVert = (LOD_VERTEX*)malloc(sizeof(LOD_VERTEX)*iVNum);
	m_pFace = (LOD_FACE*)  malloc(sizeof(LOD_FACE)*iFNum);

	for (int i=0; i<iVNum; i++)
		m_pVert[i] = pVert[i];

	for (int i=0; i<iFNum; i++)
		m_pFace[i] = pFace[i];

	computeNormals();

	computeValence();
}

void RadicalLODLevel::init(int tVNum, int tFNum, int tENum)
{
	ext_time = 0;
	next = nullptr;

	m_iVertNum = tVNum;
	m_iFaceNum = tFNum;
	m_iErrNum = tENum;

	m_pVert = (LOD_VERTEX*)malloc(sizeof(LOD_VERTEX)*m_iVertNum);
	m_pFace = (LOD_FACE*)  malloc(sizeof(LOD_FACE)*m_iFaceNum);
	m_pError = (RadicalLODError*)malloc(sizeof(RadicalLODError)*m_iErrNum);		// Error data
}

void RadicalLODLevel::computeNormals()
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

void RadicalLODLevel::computeValence()
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

bool RadicalLODLevel::buildNextLevel()
{
	// 00000000000000000000000000
	printf("\r\n------------------building level %d-------------------\n",level+1);

	createHalfEdge();				// build current half edge structure

	clock_t start,finish;
	start = clock();
	// 11111111111111111111111111
	if (split())
	{
		printf("Success! Split\n");
		// 222222222222222222222222222222222 initialize
		next = new RadicalLODLevel();
		next->level = level+1;				// level ++
		next->init(evenNum, m_iFaceNum/3, m_iVertNum - evenNum);		// face num = current faceNum/3
		strcpy(next->m_sLODName, m_sLODName);

		for (int i = 0,j = 0; i<m_iVertNum; i++)			// save current even vertex
		{
			if (m_pVert[i].even == EVEN) // save even vertex 
			{
				next->m_pVert[j] = m_pVert[i];
				j++;
			}
		}

		// 333333333333333333333333333333333 predict error
		if (predict(next))
		{
			printf("Success! Predict\n");

			// 444444444444444444444444444444444 save even vertices to next lod 
			// reconstruct next lod faces
			if (updateLOD(next))
			{
				// 55555555555555555555555555555 save error to file
				if (saveErrorToFile(next))
				{
					printf("Success! Reconstruct\n");
					// 666666666666666666666666 normals and valence
					next->computeNormals();
					next->computeValence();

					next->prev = this;

					finish = clock();
					ext_time+=(finish-start)/float(CLOCKS_PER_SEC);
					printf("Success! Level %d build successfully\n",next->level);
					printf("-----------------------------------------------------\n");
					return true;
				}
			}
		}
	}
	else
	{
		if (level == 0)
		{
			printf("Error: Split Failed!\n");
		}
		else
		{
			printf("PM has been built!\n");
			printf("saving PM..\n");
			saveCurrentMesh();
		}

	}
	return false;
}


bool RadicalLODLevel::split()
{
	int i;
	for (i = 0; i<m_iVertNum;i++)
	{
		if (m_pVert[i].valence != 6)
			break;
	}
	evenNum = 0;
	return setEven(i);
}

bool RadicalLODLevel::predict(RadicalLODLevel* nextLOD)
{
	///////////////////////////////
	//			a
	//		   / \
	//        /   \
	//       /  e  \
	//		b ----- c
	// predict e in face
	////////////////////////////////
	
	int abcVertices[3];
	int errorIndex = 0;
	
	for (int i = 0; i<m_iVertNum; i++)
	{
		if (m_pVert[i].even == ODD)			// 1.if the vertex is ODD then predict
		{
			// find a b c
			if (!findRadicalVert1(i,abcVertices))
			{
				printf("Error: Predict-Cannot find 3 vertices!\n");
				return false;
			}

			// found a b c
			nextLOD->m_pError[errorIndex].vertexIndex = i;
			nextLOD->m_pError[errorIndex].faceVertex[0] = abcVertices[0];
			nextLOD->m_pError[errorIndex].faceVertex[1] = abcVertices[1];
			nextLOD->m_pError[errorIndex].faceVertex[2] = abcVertices[2];

			float nx,ny,nz;			// new vertex position
			nx = float(m_pVert[abcVertices[0]].point.x+ m_pVert[abcVertices[1]].point.x+ m_pVert[abcVertices[2]].point.x)/3;
			ny = float(m_pVert[abcVertices[0]].point.y+ m_pVert[abcVertices[1]].point.y+ m_pVert[abcVertices[2]].point.y)/3;
			nz = float(m_pVert[abcVertices[0]].point.z+ m_pVert[abcVertices[1]].point.z+ m_pVert[abcVertices[2]].point.z)/3;

			float ex,ey,ez;			// error = origin - new
			ex = m_pVert[i].point.x - nx;
			ey = m_pVert[i].point.y - ny;
			ez = m_pVert[i].point.z - nz;

			nextLOD->m_pError[errorIndex].errXYZ[0] = ex;
			nextLOD->m_pError[errorIndex].errXYZ[1] = ey;
			nextLOD->m_pError[errorIndex].errXYZ[2] = ez;
			
			errorIndex++;
		}
	}
	return true;
}

bool RadicalLODLevel::saveErrorToFile(RadicalLODLevel* nextLOD)
{
	FILE* t_pFile;
	
	char t_sFileN[255];
	sprintf(t_sFileN,"./generate/%s_radical-%d.err", m_sLODName, nextLOD->level);

	t_pFile = fopen(t_sFileN,"w");
	if (t_pFile == nullptr)
	{
		printf("Error: Cannot open SAVE-File!\n");
		return false;
	}

	// 
	fprintf(t_pFile, "%d\n", nextLOD->m_iErrNum);

	for (int i =0; i<nextLOD->m_iErrNum; i++)
	{
		if (nextLOD->m_pError[i].errXYZ[0] > threshold
			|| nextLOD->m_pError[i].errXYZ[1] > threshold
			|| nextLOD->m_pError[i].errXYZ[2] > threshold)
		{
			fprintf(t_pFile,"%d %f %f %f %d %d %d\n",nextLOD->m_pError[i].vertexIndex,
			nextLOD->m_pError[i].errXYZ[0],nextLOD->m_pError[i].errXYZ[1],nextLOD->m_pError[i].errXYZ[2],
			nextLOD->m_pError[i].faceVertex[0],nextLOD->m_pError[i].faceVertex[1],nextLOD->m_pError[i].faceVertex[2]);
		}
	}
	fclose(t_pFile);
	return true;
}

bool RadicalLODLevel::saveCurrentMesh()
{
	FILE* t_pFile;
	char t_sFileN[255];
	sprintf(t_sFileN, "./generate/%s_radical.cor", m_sLODName);
	printf("shit%s\n",t_sFileN);
	t_pFile = fopen(t_sFileN, "w");

	// 0. vert num, face num
	fprintf(t_pFile, "%d\n%d\n",m_iVertNum, m_iFaceNum);

	// 1.save vertex first
	for (int i = 0; i<m_iVertNum; i++)
	{
		fprintf(t_pFile, "%d %f %f %f\n",m_pVert[i].index, m_pVert[i].point.x, m_pVert[i].point.y, m_pVert[i].point.z);
	}
	
	// 2.save faces
	for (int i = 0; i<m_iFaceNum; i++)
	{
		fprintf(t_pFile, "%d %d %d\n", m_pFace[i].vertIndex[0], m_pFace[i].vertIndex[1], m_pFace[i].vertIndex[2]);
	}
	fclose(t_pFile);
	return true;
}


bool RadicalLODLevel::updateLOD(RadicalLODLevel* nextLOD)
{
	std::map<int,int> tempMap;			// <original id, current id>
	std::map<int,int>::iterator it;
	int tempIndex = 0;

	// create a new face for every error vertex
	for (int i= 0; i<nextLOD->m_iErrNum; i++)
	{
		for (int j =0; j<3; j++)
		{
			if (!tempMap.count(nextLOD->m_pError[i].faceVertex[j]))
			{
				// 1.if not in tempMap, add vertex to tempMap and next lod vertices
				tempMap.insert(std::pair<int,int>(nextLOD->m_pError[i].faceVertex[j],tempIndex));
				nextLOD->m_pVert[tempIndex].point = this->m_pVert[nextLOD->m_pError[i].faceVertex[j]].point;
				nextLOD->m_pVert[tempIndex].even = UNKNOWN;
				nextLOD->m_pVert[tempIndex].index = tempIndex;

				tempIndex++;
			}
		}
		// 2.every error vertex corresponds with a face in next lod
		nextLOD->m_pFace[i].faceVertexID = i;
		for (int j =0; j<3; j++)
		{
			it = tempMap.find(nextLOD->m_pError[i].faceVertex[j]);
			nextLOD->m_pFace[i].vertIndex[j] = it->second;
		}
	}
	// update error vertex index(index from previous level to next lod level)
	for (int i= 0; i<next->m_iErrNum; i++)
	{
		for (int j= 0; j<3; j++)
		{
			nextLOD->m_pError[i].faceVertex[j] = tempMap.find(nextLOD->m_pError[i].faceVertex[j])->second;
		}
	}
	return true;
}

bool RadicalLODLevel::findRadicalVert1(int o, int e[])
{
	int abcIndex= 0;
	for (int i = 0; i<m_iFaceNum; i++)
	{
		if (m_pFace[i].vertIndex[0] == o && 
			m_pVert[m_pFace[i].vertIndex[1]].even == EVEN)
		{
			e[abcIndex] = m_pFace[i].vertIndex[1]; // 1st even vertex
			abcIndex++;

			int opThirdVertex = hf_findThirdVert(o,m_pFace[i].vertIndex[2]);
			if (m_pVert[opThirdVertex].even == EVEN)
			{
				e[abcIndex] = opThirdVertex;		// 2nd even vertex
				abcIndex++;
			}

			opThirdVertex = hf_findThirdVert(o,opThirdVertex);
			opThirdVertex = hf_findThirdVert(o,opThirdVertex);

			if (m_pVert[opThirdVertex].even == EVEN)
			{
				e[abcIndex] = opThirdVertex;		// 3rd even vertex
			}
			if (abcIndex == 2)
				break;
			else
				abcIndex=0;
		}
		///////////////////////////////////////////////////////////////////////
		if (m_pFace[i].vertIndex[1] == o && 
			m_pVert[m_pFace[i].vertIndex[2]].even == EVEN)
		{
			e[abcIndex] = m_pFace[i].vertIndex[2]; // 1st even vertex
			abcIndex++;

			int opThirdVertex = hf_findThirdVert(o,m_pFace[i].vertIndex[0]);
			if (m_pVert[opThirdVertex].even == EVEN)
			{
				e[abcIndex] = opThirdVertex;		// 2nd even vertex
				abcIndex++;
			}

			opThirdVertex = hf_findThirdVert(o,opThirdVertex);
			opThirdVertex = hf_findThirdVert(o,opThirdVertex);

			if (m_pVert[opThirdVertex].even == EVEN)
			{
				e[abcIndex] = opThirdVertex;		// 3rd even vertex
			}
			if (abcIndex == 2)
				break;
			else
				abcIndex=0;
		}
		///////////////////////////////////////////////////////////////////////
		if (m_pFace[i].vertIndex[2] == o && 
			m_pVert[m_pFace[i].vertIndex[0]].even == EVEN)
		{
			e[abcIndex] = m_pFace[i].vertIndex[0]; // 1st even vertex
			abcIndex++;

			int opThirdVertex = hf_findThirdVert(o,m_pFace[i].vertIndex[1]);
			if (m_pVert[opThirdVertex].even == EVEN)
			{
				e[abcIndex] = opThirdVertex;		// 2nd even vertex
				abcIndex++;
			}

			opThirdVertex = hf_findThirdVert(o,opThirdVertex);
			opThirdVertex = hf_findThirdVert(o,opThirdVertex);

			if (m_pVert[opThirdVertex].even == EVEN)
			{
				e[abcIndex] = opThirdVertex;		// 3rd even vertex
			}
			if (abcIndex == 2)
				break;
			else
				abcIndex=0;
		}
	}
	if (abcIndex == 2)
		return true;
	else
		return false;
}
bool RadicalLODLevel::findRadicalVert(int o, int e[])
{
	// iterate faces 
	int abcIndex = 0; // a,b,c 0,1,2

	for (int i = 0; i<m_iFaceNum; i++)
	{
		if ((m_pFace[i].vertIndex[0]==o))
		{
			if (m_pVert[m_pFace[i].vertIndex[1]].even == EVEN)
			{
				e[abcIndex] = m_pFace[i].vertIndex[1];
				abcIndex++;
			}
		}
		if ((m_pFace[i].vertIndex[1]==o))
		{
			if (m_pVert[m_pFace[i].vertIndex[2]].even == EVEN)
			{
				e[abcIndex] = m_pFace[i].vertIndex[2];
				abcIndex++;
			}
		}
		if ((m_pFace[i].vertIndex[2]==o))
		{
			if (m_pVert[m_pFace[i].vertIndex[0]].even == EVEN)
			{
				e[abcIndex] = m_pFace[i].vertIndex[0];
				abcIndex++;
			}
		}
		if (abcIndex>2)			// find 0,1,2, stop for-loop to save time
		{
			break;
		}
	}
	
	if (abcIndex<2)	// error
	{
		printf("Error: Predict-Less than 3 vertices!\n");
		return false;
	}
	return true;

}


bool RadicalLODLevel::setEven(int index)
{
	if (m_pVert[index].even == EVEN)
		return true;
	if (m_pVert[index].even == ODD)
	{
		printf("Error: Already been set ODD\n");
		return false;
	}

	// set even
	m_pVert[index].even = EVEN;
	evenNum++;

	// recurive loop
	// (1) set vertex around current vertex to odd
	// (2) set opposite vertex to even
	int a,b;
	for (int i = 0; i<m_iFaceNum; i++)
	{
		a = -1;
		if (m_pFace[i].vertIndex[0] == index)
		{
			a = m_pFace[i].vertIndex[1];
			b = m_pFace[i].vertIndex[2];
		}
		if (m_pFace[i].vertIndex[1] == index)
		{
			a = m_pFace[i].vertIndex[2];
			b = m_pFace[i].vertIndex[0];
		}
		if (m_pFace[i].vertIndex[2] == index)
		{
			a = m_pFace[i].vertIndex[0];
			b = m_pFace[i].vertIndex[1];
		}

		if (a>-1)
		{
			if (m_pVert[a].valence != 6)
			{
				printf("Error: Even point cannot been near Even\n");
				return false;
			}
			if (m_pVert[a].even == EVEN)
			{
				printf("Error: Already been set Even\n");
				return false;
			}
			m_pVert[a].even = ODD;					// (1)

			//	set opposite EVEN point;
			int opPoint = hf_findThirdVert(b,a);
		//	int opPoint = findThirdVert(b,a);
			if (opPoint == -1)
			{
				printf("Error: Cannot find opposite point\n");
				return false;
			}
			if(!setEven(opPoint))					// (2)
			{
				printf("Error: setEven error\n");
				return false;
			}
		}
	}

	return true;
}

RadicalLODLevel::~RadicalLODLevel(void)
{
}

//////////////////////////////////////////////////////////////////////////////////////
// LODMeshLevel

RadicalLODMeshLevel::RadicalLODMeshLevel()
{
	maxLevel = 0;
	next = NULL;
	prev = NULL;
}

bool RadicalLODMeshLevel::initLL(int t_iVNum, LOD_VERTEX* pVert, int t_iFNum, LOD_FACE* pFace)
{
	m_iVertNum = t_iVNum;
	m_iFaceNum = t_iFNum;

	m_pVert = (LOD_VERTEX*)malloc(sizeof(LOD_VERTEX) * m_iVertNum);
	m_pFace = (LOD_FACE  *)malloc(sizeof(LOD_FACE) * m_iFaceNum);

	for (int i = 0; i< m_iVertNum; i++)
	{
		m_pVert[i] = pVert[i];
	}
	for (int i = 0; i< m_iFaceNum; i++)
	{
		m_pFace[i] = pFace[i];
	}
	if ((m_pVert != nullptr) && (m_pFace != nullptr))
	{
		computeNormals();
		computeValence();
		return true;
	}
	else
		return false;
}

bool RadicalLODMeshLevel::init(int t_iVNum, int t_iFNum)
{
	m_iVertNum = t_iVNum;
	m_iFaceNum = t_iFNum;

	m_pVert = (LOD_VERTEX*) malloc(sizeof(LOD_VERTEX) * m_iVertNum);
	m_pFace = (LOD_FACE*) malloc(sizeof(LOD_VERTEX) * m_iFaceNum);

	if ((m_pVert != nullptr) && (m_pFace != nullptr))
		return true;
	else
		return false;
}

bool RadicalLODMeshLevel::radicalReverseSubdivide()
{
	char t_sErrFName[255];
	FILE* t_pFile;
	sprintf(t_sErrFName, "./generate/%s_radical-%d.err", m_sLODName, level);
	printf("reading file: %s\n",t_sErrFName);
	t_pFile = fopen(t_sErrFName, "r");
	if (t_pFile == nullptr)
	{
		if (level != 0)
		{	
			printf("Error: Cannot load error file %d\n", level);
		}
		return false;
	}
	// start load err information
	fscanf(t_pFile, "%d", &m_iErrNum);	// error number

	m_pError = (RadicalLODError*) malloc(sizeof(RadicalLODError) * m_iErrNum);

	for (int i = 0; i<m_iErrNum; i++)
	{
		fscanf(t_pFile, "%d %f %f %f %d %d %d", &(m_pError[i].vertexIndex),
			&(m_pError[i].errXYZ[0]), &(m_pError[i].errXYZ[1]),&(m_pError[i].errXYZ[2]),
			&(m_pError[i].faceVertex[0]), &(m_pError[i].faceVertex[1]), &(m_pError[i].faceVertex[2]));
	}
	fclose(t_pFile);

	next = new RadicalLODMeshLevel();
	next->level = level-1;
	strcpy(next->m_sLODName, m_sLODName);

	int t_iNewVertNum = m_iVertNum + m_iFaceNum;
	int t_iNewFaceNum = m_iFaceNum *3;
	int t_iCurVert;
	
	if (!(next->init(t_iNewVertNum, t_iNewFaceNum)) )// initialize next lod mesh
		return false;

	// step 111111111111111
	// copy vertex to the new Vertex space
	for (t_iCurVert = 0; t_iCurVert<m_iVertNum; t_iCurVert++)
	{
		next->m_pVert[t_iCurVert] = m_pVert[t_iCurVert];
	}
	printf("Recovery: step 1 completed\n");

	// step 22222222222222
	// update every face
	// (1)add a face vertex 
	for (int i = 0; i<m_iFaceNum; i++)
	{

		// 
		//			a
		//		   / \
		//        / d \
		//      b/_____\c
		//				
		next->m_pVert[t_iCurVert].point.x = float(m_pVert[m_pFace[i].vertIndex[0]].point.x+ m_pVert[m_pFace[i].vertIndex[1]].point.x+ m_pVert[m_pFace[i].vertIndex[2]].point.x)/3;
		next->m_pVert[t_iCurVert].point.y = float(m_pVert[m_pFace[i].vertIndex[0]].point.y+ m_pVert[m_pFace[i].vertIndex[1]].point.y+ m_pVert[m_pFace[i].vertIndex[2]].point.y)/3;
		next->m_pVert[t_iCurVert].point.z = float(m_pVert[m_pFace[i].vertIndex[0]].point.z+ m_pVert[m_pFace[i].vertIndex[1]].point.z+ m_pVert[m_pFace[i].vertIndex[2]].point.z)/3;
		next->m_pVert[t_iCurVert].index = t_iCurVert;
		next->m_pVert[t_iCurVert].valence = 0;
		next->m_pVert[t_iCurVert].normal.x = next->m_pVert[t_iCurVert].normal.y = next->m_pVert[t_iCurVert].normal.z = 0;

		m_pFace[i].faceVertexID = t_iCurVert;

		int a = m_pFace[i].vertIndex[0];
		int b = m_pFace[i].vertIndex[1];
		int c = m_pFace[i].vertIndex[2];
		float t_fErr[3];

		if (findErrVert(a, b, c, t_fErr))		// check if we have saved the error difference
		{
			next->m_pVert[t_iCurVert].point.x += t_fErr[0];
			next->m_pVert[t_iCurVert].point.y += t_fErr[1];
			next->m_pVert[t_iCurVert].point.z += t_fErr[2];
		}

		t_iCurVert++;
	}
	printf("Recovery: step 2 completed\n");

	// step 333333333333
	// create half edge
	createHalfEdge();
	// (2)find face
	// mark subdivided edge
	// traverse all three edge

	int tempFaceCount = 0;
	int edgeMarkCount = 0;
	int v1,v2;
	int *va,*vb;			// mark edge id
	va = new int[t_iNewVertNum];
	vb = new int[t_iNewVertNum];

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
				next->m_pFace[tempFaceCount].vertIndex[0] = v1;
				next->m_pFace[tempFaceCount].vertIndex[1] = R;
				next->m_pFace[tempFaceCount].vertIndex[2] = L;
				tempFaceCount++;

				// Face 2
				next->m_pFace[tempFaceCount].vertIndex[0] = v2;
				next->m_pFace[tempFaceCount].vertIndex[1] = L;
				next->m_pFace[tempFaceCount].vertIndex[2] = R;
				tempFaceCount++;


			}

		}

	}
	printf("Recovery: step 3 completed\n");

	//
	// step 44444444444
	// update vertices position
	//for (int i = 0; i<m_iVertNum; i++)
	//{
	//	//////////////////////////////////////////
	//	//
	//	//	V = (1-an)v + an/n (sigma(vi))
	//	//	
	//	//	an = (4-2cos(2pi/n))/9
	//	//
	//	/////////////////////////////////////////

	//	float an = 0.f;
	//	float sigma[3] = {0};
	//	int N = m_pVert[i].valence;

	//	for (int j = 0; j<N; j++)
	//	{
	//		sigma[0] += m_pVert[m_pVert[i].adj[j]].point.x;
	//		sigma[1] += m_pVert[m_pVert[i].adj[j]].point.y;
	//		sigma[2] += m_pVert[m_pVert[i].adj[j]].point.z;
	//	}

	//	an = (4.0 - 2*cos(2*PI/N))/9;

	//	next->m_pVert[i].point.x = (1-an)*m_pVert[i].point.x + an/N*sigma[0];
	//	next->m_pVert[i].point.y = (1-an)*m_pVert[i].point.y + an/N*sigma[1];
	//	next->m_pVert[i].point.z = (1-an)*m_pVert[i].point.z + an/N*sigma[2];

	//}

	// 444444444444444444444
	// compute normals and valence
	next->computeNormals();
	next->computeValence();

	// 555555555555555555555
	// save current generated level to file
	if (!saveLevelToFile(next))
	{
		printf("Error: Cannot save current generated level!\n");
		return false;
	}
	return true;
}

bool RadicalLODMeshLevel::saveLevelToFile(RadicalLODMeshLevel* nextLevel)
{
	FILE* t_pFile;
	char t_sFName[255];
	sprintf(t_sFName, "./generate/%s_radical_lod-%d.ply", nextLevel->m_sLODName, nextLevel->level);

	t_pFile = fopen(t_sFName, "w");
	if (t_pFile == nullptr)
		return false;
	fprintf(t_pFile,"ply\nformat ascii 1.0\n"
		"comment File created by Chai Yi's thesis project\n"
		"element vertex %d\n"
		"property float x\n"
		"property float y\n"
		"property float z\n"
		"element face %d\n"
		"property list uchar int vertex_indices\n"
		"end_header\n",nextLevel->m_iVertNum, nextLevel->m_iFaceNum);

	for (int i = 0; i<nextLevel->m_iVertNum; i++)
	{
		fprintf(t_pFile, "%f %f %f\n", nextLevel->m_pVert[i].point.x,
			nextLevel->m_pVert[i].point.y,
			nextLevel->m_pVert[i].point.z);
	}

	for (int i = 0; i<nextLevel->m_iFaceNum; i++)
	{
		fprintf(t_pFile, "3 %d %d %d\n", nextLevel->m_pFace[i].vertIndex[0],
			nextLevel->m_pFace[i].vertIndex[1],
			nextLevel->m_pFace[i].vertIndex[2]);
	}
	fclose(t_pFile);
	return true;
}

bool RadicalLODMeshLevel::findErrVert(int a, int b, int c, float* err)
{
	for (int i = 0; i<m_iErrNum; i++)
	{
		int A = m_pError[i].faceVertex[0];
		int B = m_pError[i].faceVertex[1];
		int C = m_pError[i].faceVertex[2];
		if ((A == a && B == b && C == c)||
			(A == a && B == c && C == b)||
			(A == b && B == a && C == c)||
			(A == b && B == c && C == a)||
			(A == c && B == a && C == b)||
			(A == c && B == b && C == a))
		{
			err[0]  = m_pError[i].errXYZ[0];
			err[1]  = m_pError[i].errXYZ[1];
			err[2]  = m_pError[i].errXYZ[2];
			return true;
		}
	}
	return false;
}

bool RadicalLODMeshLevel::findVertices(int a, int b, int& c, int& f)
{
	for (int i = 0; i<m_iFaceNum; i++)
	{
		for (int j = 0; j<3; j++)
		{
			if ((m_pFace[i].vertIndex[j] == a) && (m_pFace[i].vertIndex[(j+1)%3] == b))
			{
				c = m_pFace[i].vertIndex[(j+2)%3];
				f = m_pFace[i].faceVertexID;
				return true;
			}
		}
	}
	return false;
}