#pragma once

#ifndef __ASE_FILE_H__
#define __ASE_FILE_H__

#include "data_struct.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>

class ASEfile
{
public:
	ASEfile(void);
	bool loadfile(const char* filename);
	ASE_OBJECT_HEAD* getase_head(){return head;}	//返回文件头部指针
	void render();									//绘制ase模型
	void print();									//输出ase文件信息
	~ASEfile(void);
private:
	void centralize();	//中心化。使读取ase文件的顶点坐标移动到中点

private:	
	ASE_OBJECT_HEAD* head;
};

#endif