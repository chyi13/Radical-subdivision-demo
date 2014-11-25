#include "PLYLoader.h"


PLYLoader::PLYLoader(void)
{
}


PLYLoader::~PLYLoader(void)
{
}

bool PLYLoader::loadfile(const char* filename)
{
	FILE* file;
	file = fopen(filename,"r");
	if (file == NULL)
	{
		printf("PLY file not found!\n");
		return false;
	}

	ply = (PLY_OBJECT*)malloc(sizeof(PLY_OBJECT));

	bool hasConfidence = false, hasIntensity = false;

	char line[200];
	char* ptr;
	while(!feof(file))
	{
		fgets(line,200,file);
		if (strstr(line,"element vertex"))
		{
			ptr = strstr(line, "element vertex");
			ptr+= strlen("element vertex");

			ply->vertex_num = atoi(ptr);
			ply->vertex_list = (VERTEX*)malloc(ply->vertex_num * sizeof(VERTEX));
		}

		if (strstr(line,"element face"))
		{
			ptr = strstr(line, "element face");
			ptr+= strlen("element face");

			ply->face_num = atoi(ptr);
			ply->face_list = (FACE*)malloc(ply->face_num * sizeof(FACE));
		}

		if (strstr(line, "property float confidence"))
		{
			hasConfidence = true;
		}
		if (strstr(line, "property float intensity"))
		{
			hasConfidence = true;
		}
		if (strstr(line,"end_header")) // the end of header description
		{
			break;
		}
	}

	int i;
	float x, y, z, confidence, intensity;

	if (hasConfidence && hasIntensity)
	{
		for (i = 0; i<ply->vertex_num; i++)
		{
			fscanf(file, "%f %f %f %f %f", &x, &y, &z, &confidence, &intensity);
			ply->vertex_list[i].x = x * 100.f;
			ply->vertex_list[i].y = y * 100.f;
			ply->vertex_list[i].z = z * 100.f;
		}
	}
	else
	{
		for (i = 0; i<ply->vertex_num; i++)
		{
			fscanf(file, "%f %f %f", &x, &y, &z);
			ply->vertex_list[i].x = x;
			ply->vertex_list[i].y = y;
			ply->vertex_list[i].z = z;
		}
	}
	int count, a, b, c;
	for (i = 0; i<ply->face_num; i++)
	{
		fscanf(file, "%d %d %d %d",&count, &a, &b, &c);
		ply->face_list[i].index[0] = a;
		ply->face_list[i].index[1] = b;
		ply->face_list[i].index[2] = c;
	}

	centralize();	// centralize

	return true;
}

void PLYLoader::render()
{
	int i;
	for (i = 0; i<ply->face_num; i++)
	{
		glBegin(GL_LINE_LOOP);

		glVertex3f(ply->vertex_list[ply->face_list[i].index[0]].x, ply->vertex_list[ply->face_list[i].index[0]].y,
			ply->vertex_list[ply->face_list[i].index[0]].z);
		glVertex3f(ply->vertex_list[ply->face_list[i].index[1]].x, ply->vertex_list[ply->face_list[i].index[1]].y,
			ply->vertex_list[ply->face_list[i].index[1]].z);
		glVertex3f(ply->vertex_list[ply->face_list[i].index[2]].x, ply->vertex_list[ply->face_list[i].index[2]].y,
			ply->vertex_list[ply->face_list[i].index[2]].z);

		glEnd();
	}
}

void PLYLoader::centralize()
{
	int i;

	float maxX, maxY, maxZ;
	float minX, minY, minZ;

	maxX = maxY = maxZ = -1000000;
	minX = minY = minZ = 1000000;

	for (i = 0; i<ply->vertex_num; i++)
	{
		if (ply->vertex_list[i].x > maxX) maxX = ply->vertex_list[i].x;
		if (ply->vertex_list[i].x < minX) minX = ply->vertex_list[i].x;

		if (ply->vertex_list[i].y > maxY) maxY = ply->vertex_list[i].y;
		if (ply->vertex_list[i].y < minY) minY = ply->vertex_list[i].y;

		if (ply->vertex_list[i].z > maxZ) maxZ = ply->vertex_list[i].z;
		if (ply->vertex_list[i].z < minZ) minZ = ply->vertex_list[i].z;
	}

	float midX = (maxX + minX)/2;
	float midY = (maxY + minY)/2;
	float midZ = (maxZ + minZ)/2;

	float scaleCoef = 30./(maxX - minX + maxY - minY + maxZ - minZ);

	for (i = 0; i<ply->vertex_num; i++)
	{
		ply->vertex_list[i].x = scaleCoef* (ply->vertex_list[i].x - midX);
		ply->vertex_list[i].y = scaleCoef* (ply->vertex_list[i].y - midY);
		ply->vertex_list[i].z = scaleCoef* (ply->vertex_list[i].z - midZ);
	}
}

void PLYLoader::print()
{
	FILE* fw;
	fw=fopen("1.txt","w");
	PLY_OBJECT plyobj = getObject();
	int it;
	for (it=0;it<plyobj.vertex_num;it++)
		fprintf(fw,"vertex %d %f %f %f \n",it,plyobj.vertex_list[it].x,plyobj.vertex_list[it].y,plyobj.vertex_list[it].z);
	for (it=0;it<plyobj.face_num;it++)
		fprintf(fw,"face   %d %d %d %d \n",it,plyobj.face_list[it].index[0],plyobj.face_list[it].index[1],plyobj.face_list[it].index[2]);

	fclose(fw);
}