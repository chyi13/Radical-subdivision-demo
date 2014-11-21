#pragma once

#ifndef HALF_EDGE_BASE_H
#define HALF_EDGE_BASE_H

#include "data_struct.h"
#include "pair.h"

#include <vector>
#include <map>

class HalfEdgeBase
{
public:
	HalfEdgeBase(void);
	~HalfEdgeBase(void);

	LOD_VERTEX	*m_pVert;			// vertex array
	LOD_FACE	*m_pFace;			// face array
	int			m_iVertNum;			// vertex number
	int			m_iFaceNum;			// face number

	typedef std::map<Pair,HalfEdge*,std::less<Pair> > EdgeMap;
	EdgeMap edgemap;

	Vertex *vertices;
	Face *faces;
	HalfEdge *he;

	// create Half Edge struct
	void createHalfEdge();

	// 
	int hf_findPairVert(int ia,int ib);		// 另外一面的面点
	int hf_findFaceVert(int ia,int ib);
	int hf_findThirdVert(int ia,int ib);	// ia ib 之外的第三个顶点
	int hf_findPairThirdVert(int ia, int ib);

	int HalfEdgeBase::findIndex(int a,int b,int* va,int* vb,int len);	// check if edge<a, b> is in set <va, vb> which has 
																		// length of len.
};

#endif

