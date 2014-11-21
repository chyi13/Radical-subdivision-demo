#pragma once

#ifndef RADICAL_LOD_H
#define RADICAL_LOD_H

#include "RadicalLODLevel.h"
#include "LODBase.h"

class RadicalLOD : public LODBase
{
public:
	RadicalLOD(void);
	~RadicalLOD(void);

public:
	virtual void render();			// render
public:

	virtual void subdivision();
	virtual bool saveToPly();

	virtual void buildAllLevels();
	// reverse subdivision end
	/////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////
	// reverse subdivision
public:
	virtual bool recoverAllLevels(); 
private:
	RadicalLODMeshLevel lodMesh;			// recovery from reverse subdivision
	bool buildRecover;
	
	RadicalLODLevel lod;					// reverse subdivision

};
#endif