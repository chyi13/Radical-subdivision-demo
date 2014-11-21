#pragma once

#ifndef LOOP_LOD_LEVEL_H
#define LOOP_LOD_LEVEL_H

#include "halfedgebase.h"
#include <ctime>

class LoopLODError
{
public:
	int vertexIndex;				// original vertex index
	float errXYZ[3];				// x, y, z errors
	int v1, v2;
};

class LoopLODLevel : public HalfEdgeBase
{
public:
	LoopLODLevel(void);
	~LoopLODLevel(void);

	void initLL(int iVNum, LOD_VERTEX* pVert, int iFNum, LOD_FACE* pFace);	// initialize first lod
	void init(int tVNum, int tFNum, int tENum);
	
	bool buildNextLevel();			// build next level
	
	int			level;				// current LOD level

	LoopLODError* m_pError;
	int m_iErrNum;					// Error data

	LoopLODLevel* next;					// next LODLevel pointer
	LoopLODLevel* prev;					// previous LODLevel

	///////////////////////////////////////////////////////
	bool split();					// 1.split even point
	bool predict(LoopLODLevel* nextLOD);// 3.error vertex predict
	bool findLoopVert(int o,int e[]);	// 3.1 
	bool updateLOD(LoopLODLevel* nextLOD);		// 4.update build next lod
	bool saveErrorToFile(LoopLODLevel* nextLOD); // 5.save error data to file
	bool saveCurrentMesh(); // 6.final step: save current mesh to file

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

#endif

