#include "HalfEdgeBase.h"


HalfEdgeBase::HalfEdgeBase(void)
{
}


HalfEdgeBase::~HalfEdgeBase(void)
{
}

void HalfEdgeBase::createHalfEdge()
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

		/*file>>type;
		while(file>>vi){					
		v.push_back(vi);			
		}*/
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
int HalfEdgeBase::hf_findPairVert(int ia,int ib)
{
	Pair hfpair(ia,ib);
	EdgeMap::const_iterator hfiterator;
	hfiterator = edgemap.find(hfpair);  // find half edge pair

	if (hfiterator == edgemap.end())
		return -1;
	else
		return hfiterator->second->sym->leftf->faceVertexId;

}

int HalfEdgeBase::hf_findThirdVert(int ia,int ib)
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
			return hfvertex[i].id;
	}

	return -1;
}

int HalfEdgeBase::hf_findFaceVert(int ia,int ib)
{
	Pair hfpair(ia,ib);
	EdgeMap::const_iterator hfiterator;
	hfiterator = edgemap.find(hfpair);

	if (hfiterator == edgemap.end())
		return -1;
	else
		return hfiterator->second->leftf->faceVertexId;
}

int HalfEdgeBase::hf_findPairThirdVert(int ia, int ib)
{
	Pair hfpair(ia,ib);
	EdgeMap::const_iterator hfiterator;
	hfiterator = edgemap.find(hfpair);
	Vertex* hfvertex = hfiterator->second->sym->leftf->ver;
	//////////////

	return hfvertex->id;
}

int HalfEdgeBase::findIndex(int a,int b,int* va,int* vb,int len)
{
	for (int i = 0; i<len; i++)
	{
		if ( ((va[i] == a)&&(vb[i] == b)) ||
			((va[i] == b)&&(vb[i] == a)))
			return i;
	}
	return -1; // cannot find edge<a,b>
}