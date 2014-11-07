#pragma once
#ifndef LODLEVEL_H
#define LODLEVEL_H
#include "data_struct.h"
#include "pair.h"
#include <map>
#include <vector>
#include <time.h>
class LODLevel
{
public:
	LODLevel();
	~LODLevel();

	void initLL(int iVNum, LOD_VERTEX* pVert, int iFNum, LOD_FACE* pFace);	// initialize first lod
	void init(int tVNum, int tFNum, int tENum);
	bool buildNextLevel();			// build next level
	
	LOD_VERTEX* m_pVert;
	LOD_FACE*	m_pFace;
	int			level;				// current LOD level

	LOD_ERROR* m_pError;
	int m_iErrNum;					// Error data

	LODLevel* next;					// next LODLevel pointer
	LODLevel* prev;					// previous LODLevel
private:
	bool split();					// 1.split even point
	bool predict(LODLevel* nextLOD);// 3.error vertex predict
	bool findRadicalVert(int o,int e[]);	// 3.1 
	bool findRadicalVert1(int o,int e[]);	// 3.1 
	bool updateLOD(LODLevel* nextLOD);		// 4.update build next lod
	bool saveErrorToFile(LODLevel* nextLOD); // 5.save error data to file
	
	void computeNormals();
	void computeValence();

public:
	int evenNum;
	bool setEven(int index);			// set Even 

	int m_iVertNum;						// vertex number
	int m_iFaceNum;						// face number
	
	float ext_time;						// time to build this level

	float threshold;					// error threshold
private:
	///////////////////////////////////////////////////////////
	//	half edge
	//	define edgemap using STL map
	typedef std::map<Pair,HalfEdge*,std::less<Pair> > EdgeMap;
	EdgeMap edgemap;

	Vertex *vertices;
	Face *faces;
	HalfEdge *he;
private:
	// create Half Edge struct
	void createHalfEdge();			// 0. create Half edge

	// 
	int hf_findPairVert(int ia,int ib);
	int hf_findFaceVert(int ia,int ib);
	int hf_findThirdVert(int ia,int ib);

	int findThirdVert(int ia, int ib);
};

#endif