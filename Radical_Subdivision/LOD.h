#pragma once

#ifndef LOD_H
#define LOD_H

#include "ASEfile.h"
#include "PLYLoader.h"
#include "pair.h"

#include <map>

#include "LODLevel.h"
#include "data_struct.h"

//class LODLevel;
/////////////////////////////////////////////////////////////////////////
//	LOD class
//
//////////////////////////////////////////////////////////////////////////
class LOD
{
public:
	LOD(void);
	~LOD(void);

	bool load_ase_file(const char* filename);
	bool load_ply_file(const char* filename);

	void render();
	void renderAse();
	void renderSubdivision();

	void radicalSubdivision();
	void loopSubdivision();

	void nextLevel();
	void prevLevel();
	//////////////////////////////////////////////////////////////////
	//	build LOD level
	

	LOD_VERTEX	*m_pVert;			// vertex array
	LOD_FACE	*m_pFace;			// face array

private:
	bool trans2EulerPoly();

private:
	void buildAllLevels();
	LODLevel lod;					// lod structure
	
	float threshold;
	//////////////////////////////////////////////////////////////////
	// Render arguments
public:
	void setWired();
private:
	int m_iRenderMethod;

	float total_time;
private:
	void computeNormals();

	void computeValence();

	int findIndex(int a,int b,int* va,int* vb,int len);
private:
	ASEfile m_ASEfile;				// ASE file class
	int			m_iObjectNum;		// ASE object number

	PLYLoader m_ply;			// ply file object
	

	int			m_iVertNum;		// vertex number
	int			m_iFaceNum;			// face number

	int			mSubLevel;		// 1,2,3,4 number
	int			maxLevel;		// max lod level
	//////////////////////////////////////////////////////////////////
	// Half edge
	
	//define edgemap using STL map
	typedef std::map<Pair,HalfEdge*,std::less<Pair> > EdgeMap;
	EdgeMap edgemap;

	Vertex *vertices;
	Face *faces;
	HalfEdge *he;
private:
	// create Half Edge struct
	void createHalfEdge();

	// 
	int hf_findPairVert(int ia,int ib);		// 另外一面的面点
	int hf_findFaceVert(int ia,int ib);
	int hf_findThirdVert(int ia,int ib);	// ia ib 之外的第三个顶点
	int hf_findPairThirdVert(int ia, int ib);
};
#endif