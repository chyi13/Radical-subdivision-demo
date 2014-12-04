#pragma once
#ifndef RING_LOD_H
#define RING_LOD_H

#include "lodbase.h"
#include "RingLODLevel.h"

class RingLOD : public LODBase
{
public:
	RingLOD();
	~RingLOD();
	virtual void subdivision();
	virtual void buildAllLevels();
	virtual bool saveToPly();
	virtual	bool recoverAllLevels();
	virtual void render();		// render stuff

	void subdivision1();
	void subdivision2();

private:
	RingLODLevel m_LOD;
	RingLODMeshLevel m_LODMesh;
	bool buildRecover;
};

#endif