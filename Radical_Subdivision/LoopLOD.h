#pragma once

#ifndef LOOP_LOD_H
#define LOOP_LOD_H

#include "lodbase.h"
#include "LoopLODLevel.h"

class LoopLOD : public LODBase
{
public:
	LoopLOD(void);
	~LoopLOD(void);

	virtual void subdivision();
	virtual void buildAllLevels();
	virtual bool saveToPly();
	virtual	bool recoverAllLevels();
	virtual void render();		// render stuff

private:
	LoopLODLevel m_lod;
};

#endif

