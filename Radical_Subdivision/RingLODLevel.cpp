#include "RingLODLevel.h"


RingLODLevel::RingLODLevel(void)
{
}


RingLODLevel::~RingLODLevel(void)
{
}

void RingLODLevel::initLL(int iVNum, LOD_VERTEX* pVert, int iFNum, LOD_FACE* pFace)
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
//??
	computeNormals();

	computeValence();
}

void RingLODLevel::init(int tVNum, int tFNum, int tENum)
{
	ext_time = 0;
	next = nullptr;

	m_iVertNum = tVNum;
	m_iFaceNum = tFNum;
	m_iErrNum = tENum;

	m_pVert = (LOD_VERTEX*)malloc(sizeof(LOD_VERTEX)*m_iVertNum);
	m_pFace = (LOD_FACE*)  malloc(sizeof(LOD_FACE)*m_iFaceNum);
	m_pError = (RingLODError*)malloc(sizeof(RingLODError)*m_iErrNum);		// Error data
}

void RingLODLevel::computeNormals()
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

void RingLODLevel::computeValence()
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

bool RingLODLevel::buildNextLevel()
{
	// 00000000000000000000000000
	printf("\r\n------------------building level %d-------------------\n",level+1);
//	createHalfEdge();

	clock_t start,finish;
	start = clock();

	// 11111111111111111111111111
	if(split()){
		printf("Success! Split\n");

		// 222222222222222222222222222222222 initialize
		next = new RingLODLevel();
		next->level = level+1;
		next->init(evenNum, m_iFaceNum/4, m_iVertNum - evenNum);
		next->threshold = threshold;
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

bool RingLODLevel::split()
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

bool RingLODLevel::setEven(int index)
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

	int o, oo;
	for (int i = 0; i< m_iFaceNum; i++)
	{
		o = -1;
		if (m_pFace[i].vertIndex[0] == index)
		{
			o = m_pFace[i].vertIndex[1];
			oo = m_pFace[i].vertIndex[2];
		}
		else if (m_pFace[i].vertIndex[1] == index)
		{
			o = m_pFace[i].vertIndex[2];
			oo = m_pFace[i].vertIndex[0];
		}
		else if (m_pFace[i].vertIndex[2] == index)
		{
			o = m_pFace[i].vertIndex[0];
			oo = m_pFace[i].vertIndex[1];
		}
		if (o>-1)
		{
			if (m_pVert[o].valence != 6)
			{
				printf("Error: Even point cannot been near Even\n");
				return false;
			}
			if (m_pVert[o].even == EVEN)
			{
				printf("Error: Already been set Even\n");
				return false;
			}

			m_pVert[o].even = ODD;

			oo = findThirdVert(oo, o);
			oo = findThirdVert(oo, o);

			if (oo == -1)
			{
				printf("Error: Cannot find opposite point\n");
				return false;
			}
			if(!setEven(oo))					// (2)
			{
				printf("Error: setEven error\n");
				return false;
			}
		}
	}
	return true;
}

bool RingLODLevel::predict(RingLODLevel* nextLOD)
{
	///////////////////
	//   e1___f1___e2
	//     \  /\  /
	//    v1\/__\/v2
	//      /\  /\
	//   e3/__\/__\e4
	//        f2
	int val[8];
	int v1, v2, f1, f2, e1, e2, e3, e4;
	float w = 1.0/16.0;

	int errorIndex = 0;
	for (int i = 0; i<m_iVertNum; i++)
	{
		if (m_pVert[i].even == ODD)
		{
			// step 3.1
			if (!findRingVert(i, val))
			{
				printf("Error: Cannot find Ring Vert!\n");
				return false;
			}
			v1 = val[0];
			v2 = val[1];
			f1 = val[2];
			f2 = val[3];
			e1 = val[4];
			e2 = val[5];
			e3 = val[6];
			e4 = val[7];

			float ex, ey, ez;
			ex = 8*w *(m_pVert[v1].point.x + m_pVert[v2].point.x)
				+ 2*w *(m_pVert[f1].point.x + m_pVert[f2].point.x)
				- w* (m_pVert[e1].point.x + m_pVert[e2].point.x + m_pVert[e3].point.x + m_pVert[e4].point.x);
			ey = 8*w *(m_pVert[v1].point.y + m_pVert[v2].point.y)
				+ 2*w *(m_pVert[f1].point.y + m_pVert[f2].point.y)
				- w* (m_pVert[e1].point.y + m_pVert[e2].point.y + m_pVert[e3].point.y + m_pVert[e4].point.y);
			ez = 8*w *(m_pVert[v1].point.z + m_pVert[v2].point.z)
				+ 2*w *(m_pVert[f1].point.z + m_pVert[f2].point.z)
				- w* (m_pVert[e1].point.z + m_pVert[e2].point.z + m_pVert[e3].point.z + m_pVert[e4].point.z);

		//	printf("%f %f %f\n", ex, ey, ez);
			ex = m_pVert[i].point.x - ex;
			ey = m_pVert[i].point.y - ey;
			ez = m_pVert[i].point.z - ez;
			
			nextLOD->m_pError[errorIndex].vertexIndex = i;
			nextLOD->m_pError[errorIndex].errXYZ[0] = ex;
			nextLOD->m_pError[errorIndex].errXYZ[1] = ey;
			nextLOD->m_pError[errorIndex].errXYZ[2] = ez;

			nextLOD->m_pError[errorIndex].v1 = v1;
			nextLOD->m_pError[errorIndex].v2 = v2;
			nextLOD->m_pError[errorIndex].e1 = e1;
			nextLOD->m_pError[errorIndex].e2 = e2;
			nextLOD->m_pError[errorIndex].e3 = e3;
			nextLOD->m_pError[errorIndex].e4 = e4;
			nextLOD->m_pError[errorIndex].f1 = f1;
			nextLOD->m_pError[errorIndex].f2 = f2;

			++errorIndex;
		}
	}
	nextLOD->m_iErrNum = errorIndex;
	return true;
}

bool RingLODLevel::findRingVert(int o,int e[])
{
	//
	//    4___ 2___5
	//     \  /\  /
	//     0\/ o\/1 
	//      /\  /\
	//    6/__\/__\7
	//         3
	int k = 0;
	for (int i = 0; i<m_iFaceNum; i++)
	{
		int a = m_pFace[i].vertIndex[0];
		int b = m_pFace[i].vertIndex[1];
		int c = m_pFace[i].vertIndex[2];

		if (a == o && m_pVert[b].even == EVEN)
		{
			e[k] = b;
			k++;
		}
		else if (b == o && m_pVert[c].even == EVEN)
		{
			e[k] = c;
			k++;
		}
		else if (c == o && m_pVert[a].even == EVEN)
		{
			e[k] = a;
			k++;
		}
		if (k>1) break;
	}

	if (k == 0)
		return false;

	// vert 2
	int t[12];
	t[0] = findThirdVert(e[0], o);
	t[1] = findThirdVert(o, e[1]);
	e[2] = findThirdVert(t[0], t[1]);

	// vert 3
	t[2] = findThirdVert(o, e[0]);
	t[3] = findThirdVert(e[1], o);
	e[3] = findThirdVert(t[3], t[2]);

	// vert 4
	t[4] = findThirdVert(e[0], t[0]);
	t[5] = findThirdVert(t[0], e[2]);
	e[4] = findThirdVert(t[4], t[5]);

	// vert 5
	t[6] = findThirdVert(e[2], t[1]);
	t[7] = findThirdVert(t[1], e[1]);
	e[5] = findThirdVert(t[6], t[7]);

	// vert 6
	t[8] = findThirdVert(e[3], t[2]);
	t[9] = findThirdVert(t[2], e[0]);
	e[6] = findThirdVert(t[8], t[9]);

	// vert 7
	t[10] = findThirdVert(e[1], t[3]);
	t[11] = findThirdVert(t[3], e[3]);
	e[7] = findThirdVert(t[10], t[11]);

	return true;
}

bool RingLODLevel::updateLOD(RingLODLevel* nextLOD)
{
	// 1. update vertices
	std::map<int,int> tempMap;			// <original id, current id>
	std::map<int,int>::iterator it;

	int newVertex = 0;
	for (int i = 0; i< m_iVertNum; i++)
	{
		if (m_pVert[i].even == EVEN)
		{
			tempMap.insert(std::pair<int,int>(i, newVertex));
			nextLOD->m_pVert[newVertex].point = m_pVert[i].point;
			nextLOD->m_pVert[newVertex].even = UNKNOWN;
			nextLOD->m_pVert[newVertex].index = newVertex;

			newVertex++;
		}
	}

	// 2. update faces
	int na, nb, nc;
	int newFace = 0;

	for (int i = 0; i< m_iFaceNum; i++)
	{
		if (m_pVert[m_pFace[i].vertIndex[0]].even == EVEN
			|| m_pVert[m_pFace[i].vertIndex[1]].even == EVEN
			|| m_pVert[m_pFace[i].vertIndex[2]].even == EVEN)
			continue;

		na = findThirdVert(m_pFace[i].vertIndex[0], m_pFace[i].vertIndex[2]);
		nb = findThirdVert(m_pFace[i].vertIndex[1], m_pFace[i].vertIndex[0]);
		nc = findThirdVert(m_pFace[i].vertIndex[2], m_pFace[i].vertIndex[1]);

		// find the index in current index
		it = tempMap.find(na);
		nextLOD->m_pFace[newFace].vertIndex[0] = it->second;

		it = tempMap.find(nb);
		nextLOD->m_pFace[newFace].vertIndex[1] = it->second;

		it = tempMap.find(nc);
		nextLOD->m_pFace[newFace].vertIndex[2] = it->second;

		newFace++;

		if (newFace >= nextLOD->m_iFaceNum)
			break;
	}

	return true;
}

bool RingLODLevel::saveErrorToFile(RingLODLevel* nextLOD)
{
	FILE* t_pFile;
	char t_sFileN[255];

	sprintf(t_sFileN, "./generate/%s_ring-%d.err", m_sLODName, nextLOD->level);
	t_pFile = fopen(t_sFileN, "w");

	if (t_pFile == nullptr)
	{
		printf("Error: Cannot create SAVE-FILE!\n");
		return false;
	}
	fprintf(t_pFile, "%d\n", nextLOD->m_iErrNum);	// ?????????

	for (int i = 0; i< nextLOD->m_iErrNum; i++)
	{
		if (abs(nextLOD->m_pError[i].errXYZ[0]) > threshold
			|| abs(nextLOD->m_pError[i].errXYZ[1]) > threshold
			|| abs(nextLOD->m_pError[i].errXYZ[2]) > threshold)
		{
			fprintf(t_pFile,"%d %f %f %f %d %d %d %d\n",nextLOD->m_pError[i].vertexIndex,
				nextLOD->m_pError[i].errXYZ[0], nextLOD->m_pError[i].errXYZ[1], nextLOD->m_pError[i].errXYZ[2],
				nextLOD->m_pError[i].v1, nextLOD->m_pError[i].v2, 
				nextLOD->m_pError[i].e1, nextLOD->m_pError[i].e2);
		}
	}

	fclose(t_pFile);
	return true;
}

bool RingLODLevel::saveCurrentMesh()
{
	FILE* t_pFile;
	char t_sFileN[255];
	sprintf(t_sFileN, "./generate/%s_ring.cor", m_sLODName);

	t_pFile = fopen(t_sFileN, "w");

	// 0. vert num, face num
	fprintf(t_pFile,"ply\nformat ascii 1.0\n"
		"comment File created by Chai Yi's thesis project\n"
		"element vertex %d\n"
		"property float x\n"
		"property float y\n"
		"property float z\n"
		"element face %d\n"
		"property list uchar int vertex_indices\n"
		"end_header\n",m_iVertNum, m_iFaceNum);

	// 1.save vertex first
	for (int i = 0; i<m_iVertNum; i++)
	{
		fprintf(t_pFile, "%f %f %f\n", m_pVert[i].point.x, m_pVert[i].point.y, m_pVert[i].point.z);
	}

	// 2.save faces
	for (int i = 0; i<m_iFaceNum; i++)
	{
		fprintf(t_pFile, "3 %d %d %d\n", m_pFace[i].vertIndex[0], m_pFace[i].vertIndex[1], m_pFace[i].vertIndex[2]);
	}
	fclose(t_pFile);
	return true;
}

//////////////////////////////////////////////////////////////////////////////
// RingLODMeshLevel.cpp

RingLODMeshLevel::RingLODMeshLevel()
{
	maxLevel = 0;
	next = NULL;
	prev = NULL;
}

bool RingLODMeshLevel::initLL(int t_iVertNum, LOD_VERTEX* pVert, int t_iFaceNum, LOD_FACE* pFace)
{
	m_iVertNum = t_iVertNum;
	m_iFaceNum = t_iFaceNum;

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

bool RingLODMeshLevel::init(int t_iVNum, int t_iFNum)
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

bool RingLODMeshLevel::ringReverseSubdivide()
{
	char t_sErrFName[255];
	FILE* t_pFile;
	sprintf(t_sErrFName, "./generate/%s_ring-%d.err", m_sLODName, level);
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

	m_pError = (RingLODError*) malloc(sizeof(RingLODError) * m_iErrNum);

	for (int i = 0; i<m_iErrNum; i++)
	{
		fscanf(t_pFile, "%d %f %f %f %d %d %d %d", &(m_pError[i].vertexIndex),
			&(m_pError[i].errXYZ[0]), &(m_pError[i].errXYZ[1]),&(m_pError[i].errXYZ[2]),
			&(m_pError[i].v1), &(m_pError[i].v2),
			&(m_pError[i].e1), &(m_pError[i].e2));
	}
	fclose(t_pFile);
	
	// step 000000000000
//	createHalfEdge();

	// 111111111111111111
	next = new RingLODMeshLevel();
	next->level = level - 1;
	strcpy(next->m_sLODName, m_sLODName);

	int t_iNewVertNum = m_iVertNum + edgemap.size()/2;
	int t_iNewFaceNum = m_iFaceNum * 4;
	int t_iCurVert;

	int* va = new int[t_iNewVertNum];
	int* vb = new int[t_iNewVertNum];

	if (!next->init(t_iNewVertNum, t_iNewFaceNum))
		return false;

	// step 1221212121212121
	// copy vertex to the new Vertex space
	for (t_iCurVert = 0; t_iCurVert<m_iVertNum; t_iCurVert++)
	{
		next->m_pVert[t_iCurVert] = m_pVert[t_iCurVert];
	}
	printf("Recovery: step 1 completed\n");

	///////////////////
	//   e1___f1___e2
	//     \  /\  /
	//    v1\/__\/v2
	//      /\  /\
	//   e3/__\/__\e4
	//        f2
	// step 22222222222222
	int v1,v2,e1,e2,e3,e4,f1,f2,newVert[4];
	float w = 1.0/16.0;				// subdivision coefficient
	for (int i = 0; i< m_iFaceNum; i++)
	{
		for (int j = 0; j< 3; j++)
		{
			newVert[j] = findIndex(m_pFace[i].vertIndex[j], m_pFace[i].vertIndex[(j+1)%3],va,vb, t_iCurVert-m_iVertNum);
			if(newVert[j] == -1)	// not find means the new vertex has not been created
			{
				v1 = m_pFace[i].vertIndex[j];
				v2 = m_pFace[i].vertIndex[(j+1)%3];

				f1 = findThirdVert(v1,v2);
				f2 = findThirdVert(v2,v1);

				e1 = findThirdVert(v1,f1);
				e2 = findThirdVert(f1,v2);
				e3 = findThirdVert(v2,f2);
				e4 = findThirdVert(f2,v1);

				// new vert position
				next->m_pVert[t_iCurVert].index = t_iCurVert;
				next->m_pVert[t_iCurVert].point.x = 8*w* (m_pVert[v1].point.x + m_pVert[v2].point.x)
					+ 2*w* (m_pVert[f1].point.x + m_pVert[f2].point.x)
					- w* (m_pVert[e1].point.x + m_pVert[e2].point.x + m_pVert[e3].point.x + m_pVert[e4].point.x);
				next->m_pVert[t_iCurVert].point.y = 8*w* (m_pVert[v1].point.y + m_pVert[v2].point.y)
					+ 2*w* (m_pVert[f1].point.y + m_pVert[f2].point.y)
					- w* (m_pVert[e1].point.y + m_pVert[e2].point.y + m_pVert[e3].point.y + m_pVert[e4].point.y);
				next->m_pVert[t_iCurVert].point.z = 8*w* (m_pVert[v1].point.z + m_pVert[v2].point.z)
					+ 2*w* (m_pVert[f1].point.z + m_pVert[f2].point.z)
					- w* (m_pVert[e1].point.z + m_pVert[e2].point.z + m_pVert[e3].point.z + m_pVert[e4].point.z);
				next->m_pVert[t_iCurVert].valence = 0;
				next->m_pVert[t_iCurVert].normal.x = next->m_pVert[t_iCurVert].normal.y 
					= next->m_pVert[t_iCurVert].normal.z
					= 0;

				// add error data
				float t_fErr[3] = {0};
				if (findErrVert(v1, v2, t_fErr))
				{
					next->m_pVert[t_iCurVert].point.x += t_fErr[0];
					next->m_pVert[t_iCurVert].point.y += t_fErr[1];
					next->m_pVert[t_iCurVert].point.z += t_fErr[2];
				}

				// mark subdivided edge
				va[t_iCurVert - m_iVertNum] = v1;
				vb[t_iCurVert - m_iVertNum] = v2;
				newVert[j] = t_iCurVert;

				t_iCurVert++;
			}
			else
			{
				newVert[j] += m_iVertNum;	// i is index in [va,vb], so we need add m_iVertNum
			}
		}

		//step 333333333333
		//          i2
		//          /\
		//       n2/__\n1
		//        /\  /\   
		//     i0/__\/__\ i1
		//          n0

		// no.1 face
		next->m_pFace[i*4].vertIndex[0] = m_pFace[i].vertIndex[0];
		next->m_pFace[i*4].vertIndex[1] = newVert[0];
		next->m_pFace[i*4].vertIndex[2] = newVert[2];
		next->m_pFace[i*4].normal.x = next->m_pFace[i*4].normal.y
			= next->m_pFace[i*4].normal.z
			= 0;

		// no.2 face
		next->m_pFace[i*4+1].vertIndex[0] = m_pFace[i].vertIndex[1];
		next->m_pFace[i*4+1].vertIndex[1] = newVert[1];
		next->m_pFace[i*4+1].vertIndex[2] = newVert[0];
		next->m_pFace[i*4+1].normal.x = next->m_pFace[i*4+1].normal.y
			= next->m_pFace[i*4+1].normal.z
			= 0;

		// no.3 face
		next->m_pFace[i*4+2].vertIndex[0] = m_pFace[i].vertIndex[2];
		next->m_pFace[i*4+2].vertIndex[1] = newVert[2];
		next->m_pFace[i*4+2].vertIndex[2] = newVert[1];
		next->m_pFace[i*4+2].normal.x = next->m_pFace[i*4+2].normal.y
			= next->m_pFace[i*4+2].normal.z
			= 0;

		// no.4 face
		next->m_pFace[i*4+3].vertIndex[0] = newVert[0];
		next->m_pFace[i*4+3].vertIndex[1] = newVert[1];
		next->m_pFace[i*4+3].vertIndex[2] = newVert[2];
		next->m_pFace[i*4+3].normal.x = next->m_pFace[i*4+3].normal.y
			= next->m_pFace[i*4+3].normal.z
			= 0;
	}
	printf("Recovery: step 2 completed\n");

	

	// 4444444444444444444444444
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

bool RingLODMeshLevel::findErrVert(int a, int b, float* err)
{
	for (int i = 0; i<m_iErrNum; i++)
	{
		if ((m_pError[i].v1 == a && m_pError[i].v2 == b) ||
			(m_pError[i].v1 == b && m_pError[i].v2 == a))
		{
			err[0] = m_pError[i].errXYZ[0];
			err[1] = m_pError[i].errXYZ[1];
			err[2] = m_pError[i].errXYZ[2];
			return true;
		}
	}
	return false;
}

bool RingLODMeshLevel::saveLevelToFile(RingLODMeshLevel* nextLevel)
{
	FILE* t_pFile;
	char t_sFName[255];
	sprintf(t_sFName, "./generate/%s_ring_lod-%d.ply", nextLevel->m_sLODName, nextLevel->level);

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