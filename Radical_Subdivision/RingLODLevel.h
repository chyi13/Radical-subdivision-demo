#pragma once
#ifndef RING_LOD_LEVEL_H
#define RING_LOD_LEVEL_H

#include "halfedgebase.h"
#include <ctime>

class RingLODError
{
public:
	int vertexIndex;				// original vertex index
	float errXYZ[3];				// x, y, z errors
	int v1, v2, f1, f2, e1, e2, e3, e4;
};

class RingLODLevel : public HalfEdgeBase
{
public:
	RingLODLevel(void);
	~RingLODLevel(void);

	void initLL(int iVNum, LOD_VERTEX* pVert, int iFNum, LOD_FACE* pFace);	// initialize first lod
	void init(int tVNum, int tFNum, int tENum);
	
	bool buildNextLevel();			// build next level
	
	int			level;				// current LOD level

	RingLODError* m_pError;
	int m_iErrNum;					// Error data

	RingLODLevel* next;					// next LODLevel pointer
	RingLODLevel* prev;					// previous LODLevel

	///////////////////////////////////////////////////////
	bool split();				// 1.split even point
	bool predict(RingLODLevel* nextLOD);// 3.error vertex predict
	bool findRingVert(int o,int e[]);	// 3.1 find 8 Ring Vert
	bool updateLOD(RingLODLevel* nextLOD);		// 4.update build next lod
	bool saveErrorToFile(RingLODLevel* nextLOD); // 5.save error data to file
	bool saveCurrentMesh();		// 6.final step: save current mesh to file

	void computeNormals();
	void computeValence();

	//////////////////////////////////////////////////////////
	int evenNum;
	bool setEven(int index);			// set Even 
	
	float ext_time;						// time to build this level

	float threshold;					// error threshold

	//////////////////////////////////////////////////////////
	char m_sLODName[255];			// lod name
};

class RingLODMeshLevel : public RingLODLevel
{
public:
	RingLODMeshLevel();
	~RingLODMeshLevel(){};
	bool initLL(int t_iVertNum, LOD_VERTEX* pVert, int t_iFaceNum, LOD_FACE* pFace);
	bool init(int t_iVNum, int t_iFNum);

	bool ringReverseSubdivide();
	bool saveLevelToFile(RingLODMeshLevel*);

	bool findErrVert(int a, int b, float* err);
	bool findVertices(int a, int b, int& c, int& f);

	int level;
	int maxLevel;
	RingLODMeshLevel *next, *prev;
};

#endif

