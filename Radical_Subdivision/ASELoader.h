#pragma once

#ifndef ASE_LOADER_H
#define ASE_LOADER_H

#include "data_struct.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>

class ASELoader
{
public:
	ASELoader(void);
	bool loadfile(const char* filename);
	ASE_OBJECT_HEAD* getase_head(){return head;}	//返回文件头部指针
	void render();									//绘制ase模型
	void print();									//输出ase文件信息
	bool outputPLY();		// save .ase file as .ply format
	~ASELoader(void);
private:
	void centralize();	//中心化。使读取ase文件的顶点坐标移动到中点

	void getRealName(const char* fn);	// trim the file string to get rid of the format suffix
private:	
	ASE_OBJECT_HEAD* head;
	char m_sFileN[255];
};

#endif