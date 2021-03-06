#pragma once

#ifndef LOD_BASE_H
#define LOD_BASE_H

#include "HalfEdgeBase.h"
#include "ASELoader.h"
#include "PLYLoader.h"

class LODBase : public HalfEdgeBase
{
public:
	LODBase(void);
	~LODBase(void);

	bool loadFromFile(const char* filename);	// load 3d mesh file. (.ase .ply)
	bool load_ase_file(const char* filename);
	bool load_ply_file(const char* filename);

	ASELoader m_ASEfile;				// ASE file class
	int			m_iObjectNum;		// ASE object number
	PLYLoader m_ply;			// ply file object

	char m_sFilename[255];

	///////////////////////////////////////////////////////////////////////

	void computeNormals();
	void computeValence();
	void sortAdjVert();

	virtual bool saveToPly() = 0;	// save current subdivision
	//////////////////////////////////////////////////////////////////////

	int m_iMaxLevel;		// subdivision level
	virtual void subdivision() = 0;

	virtual void buildAllLevels() = 0;
	void nextLevel();
	void prevLevel();

	virtual	bool recoverAllLevels() = 0;

	int			mSubLevel;		// number 1,2,3,4
	int			maxLevel;		// reverse subdivision level

	//////////////////////////////////////////////////////////////////////

	virtual void render() = 0;
	void renderSubdivision();

	void setWired();
	int m_iRenderMethod;

	//////////////////////////////////////////////////////////////////////
	float threshold;			// threshold
	float total_time;			// reverse subdivision building time
};

#endif
