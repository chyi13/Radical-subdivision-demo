#include "LoopLOD.h"


LoopLOD::LoopLOD(void)
{
}


LoopLOD::~LoopLOD(void)
{
}

void LoopLOD::subdivision()
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

	// save
	saveToPly();

	m_iMaxLevel++;
}

bool LoopLOD::saveToPly()
{
	char t_sFileN[255];
	sprintf(t_sFileN, "./generate/%s_loop-%d.ply", m_sFilename, m_iMaxLevel);

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

void LoopLOD::render()
{
	if (mSubLevel >0)
	{
		LoopLODLevel *plod = &m_lod;

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

void LoopLOD::buildAllLevels()
{
	LoopLODLevel* p = &m_lod;
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

bool LoopLOD::recoverAllLevels()
{
	return true;
}

