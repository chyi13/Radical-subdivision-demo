#pragma once
#ifndef PLY_LOADER_H
#define PLY_LOADER_H

#include <stdio.h>
#include "data_struct.h"

class PLYLoader
{
public:
	PLYLoader(void);
	~PLYLoader(void);

	bool loadfile(const char* filename);
	void render();
	PLY_OBJECT getObject(){return *ply;}

private:
	void centralize();
	PLY_OBJECT* ply;
};

#endif

