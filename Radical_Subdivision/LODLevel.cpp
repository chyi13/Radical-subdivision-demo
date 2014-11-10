#include "LODLevel.h"
LODLevel::LODLevel()
{
}

void LODLevel::initLL(int iVNum, LOD_VERTEX* pVert, int iFNum, LOD_FACE* pFace)
{
	ext_time = 0;
	level = 0;

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

void LODLevel::init(int tVNum, int tFNum, int tENum)
{
	ext_time = 0;
	m_iVertNum = tVNum;
	m_iFaceNum = tFNum;
	m_iErrNum = tENum;
	m_pVert = (LOD_VERTEX*)malloc(sizeof(LOD_VERTEX)*m_iVertNum);
	m_pFace = (LOD_FACE*)  malloc(sizeof(LOD_FACE)*m_iFaceNum);

	m_pError = (LOD_ERROR*)malloc(sizeof(LOD_ERROR)*m_iErrNum);		// Error data
}

void LODLevel::computeNormals()
{
	int i,j;

	for(i = 0 ; i < m_iVertNum; i++)
	{
		m_pVert[i].normal.x = m_pVert[i].normal.y = m_pVert[i].normal.z= 0;
	}
	
	for(i = 0 ; i< m_iFaceNum; i++)
	{
		VERTEX planeVector[2];
		
//		printf("%d %d %d\n",m_pFace[i].vertIndex[0],m_pFace[i].vertIndex[1],m_pFace[i].vertIndex[2]);

		planeVector[0] = createVector(m_pVert[m_pFace[i].vertIndex[0]].point,m_pVert[m_pFace[i].vertIndex[1]].point);
		planeVector[1] = createVector(m_pVert[m_pFace[i].vertIndex[0]].point,m_pVert[m_pFace[i].vertIndex[2]].point);
		m_pFace[i].normal = cross(planeVector[0], planeVector[1]);

		normalize(m_pFace[i].normal);

//		printf("%f %f %f\n",m_pFace[i].normal.x,m_pFace[i].normal.y,m_pFace[i].normal.z);
	
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

void LODLevel::computeValence()
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

bool LODLevel::buildNextLevel()
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
		next = new LODLevel();
		next->level = level+1;				// level ++
		next->init(evenNum, m_iFaceNum/3, m_iVertNum - evenNum);		// face num = current faceNum/3

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


bool LODLevel::split()
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

bool LODLevel::predict(LODLevel* nextLOD)
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
			nx = float(m_pVert[abcVertices[0]].point.x + m_pVert[abcVertices[1]].point.x+m_pVert[abcVertices[2]].point.x)/3;
			ny = float(m_pVert[abcVertices[0]].point.y + m_pVert[abcVertices[1]].point.y+m_pVert[abcVertices[2]].point.y)/3;
			nz = float(m_pVert[abcVertices[0]].point.z + m_pVert[abcVertices[1]].point.z+m_pVert[abcVertices[2]].point.z)/3;

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

bool LODLevel::saveErrorToFile(LODLevel* nextLOD)
{
	FILE *file1,*file2;
	
	char filename1[255], filename2[255];
	sprintf(filename1,"error-%d.err",nextLOD->level);
	sprintf(filename2,"errorXYZ-%d.err",nextLOD->level);

	file1 = fopen(filename1,"w");
	file2 = fopen(filename2,"w");
	if (!file1 || !file2)
	{
		printf("Error: SAVE-File cannot open!\n");
		return false;
	}
	for (int i =0; i<nextLOD->m_iErrNum; i++)
	{
		fprintf(file1,"%d,%f,%f,%f,%d,%d,%d\n",nextLOD->m_pError[i].vertexIndex,
			nextLOD->m_pError[i].errXYZ[0],nextLOD->m_pError[i].errXYZ[1],nextLOD->m_pError[i].errXYZ[2],
			nextLOD->m_pError[i].faceVertex[0],nextLOD->m_pError[i].faceVertex[1],nextLOD->m_pError[i].faceVertex[2]);

		fprintf(file2,"%f %f %f\n",nextLOD->m_pError[i].errXYZ[0],nextLOD->m_pError[i].errXYZ[1],nextLOD->m_pError[i].errXYZ[2]);
	}
	fclose(file1);
	fclose(file2);

	return true;
}

bool LODLevel::saveCurrentMesh()
{
	FILE* file;
	char filename[] = "coarest.cor";
	file = fopen(filename, "w");

	// 0. vert num, face num
	fprintf(file, "%d\n %d\n",m_iVertNum, m_iFaceNum);

	// 1.save vertex first
	for (int i = 0; i<m_iVertNum; i++)
	{
		fprintf(file, "%d %f %f %f\n",m_pVert[i].index, m_pVert[i].point.x, m_pVert[i].point.y, m_pVert[i].point.z);
	}
	
	// 2.save faces
	for (int i = 0; i<m_iFaceNum; i++)
	{
		fprintf(file, "%d %d %d\n", m_pFace[i].vertIndex[0], m_pFace[i].vertIndex[1], m_pFace[i].vertIndex[2]);
	}
	fclose(file);
	return true;
}


bool LODLevel::updateLOD(LODLevel* nextLOD)
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
	/*for (int i= 0; i<next->m_iErrNum; i++)
	{
		for (int j= 0; j<3; j++)
		{
			nextLOD->m_pError[i].faceVertex[j] = tempMap.find(nextLOD->m_pError[i].faceVertex[j])->second;
		}
	}*/
	return true;
}

bool LODLevel::findRadicalVert1(int o, int e[])
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
bool LODLevel::findRadicalVert(int o, int e[])
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


bool LODLevel::setEven(int index)
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

int LODLevel::findThirdVert(int ia, int ib)
{
	for (int i =0; i<m_iFaceNum; i++)
	{
		for (int j=0; j<3; j++)
		{
			if ((m_pFace[i].vertIndex[j]==ia) && (m_pFace[i].vertIndex[(j+1)%3]==ib))
			{
				return m_pFace[i].vertIndex[(j+2)%3];
			}
		}
	}
	return -1;
}

void LODLevel::createHalfEdge()
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

		v.push_back(m_pFace[i].vertIndex[0]);
		v.push_back(m_pFace[i].vertIndex[1]);
		v.push_back(m_pFace[i].vertIndex[2]);
				
		faces[i].ver = new Vertex[v.size()];
		faces[i].nPolygon = v.size();
		faces[i].faceVertexId = m_pFace[i].faceVertexID;	// add inner new face vertex id 

		he = new HalfEdge[v.size()];

		int j = 0;
		for(p= v.begin(); p != v.end(); p++){			
			//¸ü¸Ä *p
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
int LODLevel::hf_findPairVert(int ia,int ib)
{
	Pair hfpair(ia,ib);
	EdgeMap::const_iterator hfiterator;
	hfiterator = edgemap.find(hfpair);  // find half edge pair
	
	int tempVertexId = hfiterator->second->sym->leftf->faceVertexId;

	return tempVertexId;

}

int LODLevel::hf_findThirdVert(int ia,int ib)
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
		{
			return hfvertex[i].id;
		}
	}

	return -1;
}

int LODLevel::hf_findFaceVert(int ia,int ib)
{
	Pair hfpair(ia,ib);
	EdgeMap::const_iterator hfiterator;
	hfiterator = edgemap.find(hfpair);

	return hfiterator->second->leftf->faceVertexId;
}



LODLevel::~LODLevel(void)
{
}
