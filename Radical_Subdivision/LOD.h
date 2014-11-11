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

	//////////////////////////////////////////////////////////////////////
	// load file
public:
	bool loadFromFile(const char* filename);	// load 3d mesh file. (.ase .ply)
private:
	bool load_ase_file(const char* filename);
	bool load_ply_file(const char* filename);

	ASEfile m_ASEfile;				// ASE file class
	int			m_iObjectNum;		// ASE object number
	PLYLoader m_ply;			// ply file object
	// load file end
	////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////
	// render
public:
	void render();
	void renderAse();
	void renderSubdivision();
	void setWired();
private:
	int m_iRenderMethod;
	// render end
	///////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////
	// subdivision
public:
	void radicalSubdivision();
	void loopSubdivision();
private:
	bool trans2EulerPoly();
	void computeNormals();
	void computeValence();
	void sortAdjVert();
	// subdivision end
	//////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////
	// reverse subdivision
public:
	void nextLevel();
	void prevLevel();
private:
	void buildAllLevels();
	int findIndex(int a,int b,int* va,int* vb,int len);
	// reverse subdivision end
	/////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////
	// reverse subdivision
public:
	bool recoverAllLevels(); 
private:
	LODMeshLevel lodMesh;
	bool buildRecover;
	// reverse subdivision end
	/////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////
	//	LOD data
private:
	LOD_VERTEX	*m_pVert;			// vertex array
	LOD_FACE	*m_pFace;			// face array
	int			m_iVertNum;			// vertex number
	int			m_iFaceNum;			// face number

	int			mSubLevel;		// number 1,2,3,4
	int			maxLevel;		// max lod level
	
	LODLevel lod;					// lod structure
	float threshold;
	float total_time;
	// data end
	/////////////////////////////////////////////////

	
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
	// half edge end
	////////////////////////////////////////////////////
};
#endif