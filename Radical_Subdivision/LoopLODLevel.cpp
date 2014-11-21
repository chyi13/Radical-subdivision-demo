#include "LoopLODLevel.h"


LoopLODLevel::LoopLODLevel(void)
{
}


LoopLODLevel::~LoopLODLevel(void)
{
}

void LoopLODLevel::initLL(int iVNum, LOD_VERTEX* pVert, int iFNum, LOD_FACE* pFace)
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

void LoopLODLevel::init(int tVNum, int tFNum, int tENum)
{
	ext_time = 0;
	next = nullptr;

	m_iVertNum = tVNum;
	m_iFaceNum = tFNum;
	m_iErrNum = tENum;

	m_pVert = (LOD_VERTEX*)malloc(sizeof(LOD_VERTEX)*m_iVertNum);
	m_pFace = (LOD_FACE*)  malloc(sizeof(LOD_FACE)*m_iFaceNum);
	m_pError = (LoopLODError*)malloc(sizeof(LoopLODError)*m_iErrNum);		// Error data
}

void LoopLODLevel::computeNormals()
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

void LoopLODLevel::computeValence()
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

bool LoopLODLevel::buildNextLevel()
{
	// 00000000000000000000000000
	printf("\r\n------------------building level %d-------------------\n",level+1);
	createHalfEdge();

	clock_t start,finish;
	start = clock();
	
	// 11111111111111111111111111
	if(split()){
		
		printf("Success! Split\n");
		// 222222222222222222222222222222222 initialize
		next = new LoopLODLevel();
		next->level = level+1;
		next->init(evenNum, m_iFaceNum/4, m_iVertNum - evenNum);
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

bool LoopLODLevel::split()
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

bool LoopLODLevel::setEven(int index)
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

			oo = hf_findThirdVert(oo, o);
			oo = hf_findThirdVert(oo, o);

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

bool LoopLODLevel::predict(LoopLODLevel *nextLOD)
{
	////////////////////
	//        f1
	//        /\  
	//    v1 /__\ v2
	//       \  /
	//        \/
	//        f2
	//
	int v1,v2,f1,f2;
	float w = 1.0/ 8.0;		// loop subdivision coefficient
	int errorIndex = 0;

	for (int i = 0; i < m_iVertNum; i++)
	{
		if (m_pVert[i].even == ODD)			// 1.if the vertex is ODD then predict
		{
			// find 4 Loop vertices
			int val[4];
			if (!findLoopVert(i,val))
			{
				printf("Error: Predict-Cannot find 3 vertices!\n");
				return false;
			}
			v1 = val[0];
			v2 = val[1];
			f1 = val[2];
			f2 = val[3];
	
			// calculate the difference
			// the predicted vert should be:
			// 8*w*(v1+v2) + 2*w*(f1+f2) - w*(e1+e2+e3+e4)
			
			float ex, ey, ez;
			ex = 3 * w * (m_pVert[v1].point.x + m_pVert[v2].point.x) + 1 * w * (m_pVert[f1].point.x + m_pVert[f2].point.x);
			ey = 3 * w * (m_pVert[v1].point.y + m_pVert[v2].point.y) + 1 * w * (m_pVert[f1].point.y + m_pVert[f2].point.y);
			ez = 3 * w * (m_pVert[v1].point.z + m_pVert[v2].point.z) + 1 * w * (m_pVert[f1].point.z + m_pVert[f2].point.z);

			ex = ex - m_pVert[i].point.x;
			ey = ey - m_pVert[i].point.y;
			ez = ez - m_pVert[i].point.z;
			
			// save the error information
			nextLOD->m_pError[errorIndex].vertexIndex = i;//第k个error对应的顶点ID是i. 顶点ID是指原始网格的顶点INDEX
			nextLOD->m_pError[errorIndex].errXYZ[0] = ex;
			nextLOD->m_pError[errorIndex].errXYZ[1] = ey;
			nextLOD->m_pError[errorIndex].errXYZ[2] = ez;

			//此时将v1,v2在vert_id中的index保存下来作为误差的标识:
			//(保存index是因其数字相对较小,因为顶点保存是以ID为序,故保存index也能唯一标识)
			//思考:既然这里标识了误差,那么err_vert是否必要? 答案是肯定的.
			nextLOD->m_pError[errorIndex].v1 = v1;
			nextLOD->m_pError[errorIndex].v2 = v2;
			
			errorIndex++;
		}
	}
	return true;
}

bool LoopLODLevel::findLoopVert(int o,int e[])
{
	int k = 0,t[4];

	//		e2
	//	   /  \
	//    t0   t1
	//   /      \
	//  e0-- o --e1
	//   \      /
	//    t2   t3
	//     \  /
	//      e3

	for(int i= 0; i< m_iFaceNum; i++)
	{
		if (m_pFace[i].vertIndex[0] == o && 
			m_pVert[m_pFace[i].vertIndex[1]].even == EVEN)
		{
			e[k] = m_pFace[i].vertIndex[1]; 
			k++;
		}
		else if (m_pFace[i].vertIndex[1] == o && 
			m_pVert[m_pFace[i].vertIndex[2]].even == EVEN)
		{
			e[k] = m_pFace[i].vertIndex[2]; 
			k++;
		}
		else if (m_pFace[i].vertIndex[2] == o && 
			m_pVert[m_pFace[i].vertIndex[0]].even == EVEN)
		{
			e[k] = m_pFace[i].vertIndex[0]; 
			k++;
		}
		if(k>1)	break;
	}
	
	//e[2]:
	t[0] = hf_findThirdVert(e[0],o);
	t[1] = hf_findThirdVert(o,e[1]);
	e[2] = hf_findThirdVert(t[0],t[1]);

	//e[3]:
	t[3] = hf_findThirdVert(e[1],o);
	t[2] = hf_findThirdVert(o,e[0]);
	e[3] = hf_findThirdVert(t[3],t[2]);

	return true;
}

bool LoopLODLevel::updateLOD(LoopLODLevel* nextLOD)
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
		
		na = hf_findThirdVert(m_pFace[i].vertIndex[0], m_pFace[i].vertIndex[2]);
		nb = hf_findThirdVert(m_pFace[i].vertIndex[1], m_pFace[i].vertIndex[0]);
		nc = hf_findThirdVert(m_pFace[i].vertIndex[2], m_pFace[i].vertIndex[1]);

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

bool LoopLODLevel::saveErrorToFile(LoopLODLevel* nextLOD)
{
	FILE* t_pFile;
	char t_sFileN[255];

	sprintf(t_sFileN, "./generate/%s_loop-%d.err", m_sLODName, nextLOD->level);
	t_pFile = fopen(t_sFileN, "w");

	if (t_pFile == nullptr)
	{
		printf("Error: Cannot create SAVE-FILE!\n");
		return false;
	}
	fprintf(t_pFile, "%d\n", nextLOD->m_iErrNum);	// ?????????

	for (int i = 0; i< nextLOD->m_iErrNum; i++)
	{
		if (nextLOD->m_pError[i].errXYZ[0] > threshold
			|| nextLOD->m_pError[i].errXYZ[1] > threshold
			|| nextLOD->m_pError[i].errXYZ[2] > threshold)
		{
			fprintf(t_pFile,"%d %f %f %f %d %d\n",nextLOD->m_pError[i].vertexIndex,
			nextLOD->m_pError[i].errXYZ[0], nextLOD->m_pError[i].errXYZ[1], nextLOD->m_pError[i].errXYZ[2],
			nextLOD->m_pError[i].v1, nextLOD->m_pError[i].v2);
		}
	}

	fclose(t_pFile);
	return true;
}

bool LoopLODLevel::saveCurrentMesh()
{
	FILE* t_pFile;
	char t_sFileN[255];
	sprintf(t_sFileN, "./generate/%s_loop.cor", m_sLODName);
	
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