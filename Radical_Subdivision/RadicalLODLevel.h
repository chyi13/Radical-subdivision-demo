#pragma once
#ifndef RADICAL_LOD_LEVEL_H
#define RADICAL_LOD_LEVEL_H
#include <time.h>

#include "HalfEdgeBase.h"

class RadicalLODError
{
public:
	int vertexIndex;				// original vertex index
	VERTEX point;

	float errXYZ[3];				// error 
	int faceVertex[3];				// three vertex around the error vertex
};

class RadicalLODLevel : public HalfEdgeBase
{
public:
	RadicalLODLevel();
	~RadicalLODLevel();

	void initLL(int iVNum, LOD_VERTEX* pVert, int iFNum, LOD_FACE* pFace);	// initialize first lod
	void init(int tVNum, int tFNum, int tENum);
	bool buildNextLevel();			// build next level
	
	int			level;				// current LOD level

	RadicalLODError* m_pError;
	int m_iErrNum;					// Error data

	RadicalLODLevel* next;					// next LODLevel pointer
	RadicalLODLevel* prev;					// previous LODLevel

	///////////////////////////////////////////////////////
	bool split();					// 1.split even point
	bool predict(RadicalLODLevel* nextLOD);// 3.error vertex predict
	bool findRadicalVert(int o,int e[]);	// 3.1 
	bool findRadicalVert1(int o,int e[]);	// 3.1 
	bool updateLOD(RadicalLODLevel* nextLOD);		// 4.update build next lod
	bool saveErrorToFile(RadicalLODLevel* nextLOD); // 5.save error data to file
	bool saveCurrentMesh(); // 6.final step: save current mesh to file

	void computeNormals();
	void computeValence();

	//////////////////////////////////////////////////////////
	int evenNum;
	bool setEven(int index);			// set Even 
	
	float ext_time;						// time to build this level

	float threshold;					// error threshold

	///////////////////////////////////////////////////////////
	char m_sLODName[255];
};



class RadicalLODMeshLevel : public RadicalLODLevel
{
public:
	RadicalLODMeshLevel();
	bool initLL(int t_iVNum, LOD_VERTEX* pVert, int t_iFNum, LOD_FACE* pFace);
	bool init(int t_iVNum, int t_iFNum);

	bool radicalReverseSubdivide();
	bool saveLevelToFile(RadicalLODMeshLevel*);

	bool findErrVert(int a, int b, int c, float* err);
	bool findVertices(int a, int b, int& c, int& f);

	int level;
	int maxLevel;
	RadicalLODMeshLevel *next, *prev;
};
#endif