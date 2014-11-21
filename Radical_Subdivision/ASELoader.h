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
	ASE_OBJECT_HEAD* getase_head(){return head;}	//�����ļ�ͷ��ָ��
	void render();									//����aseģ��
	void print();									//���ase�ļ���Ϣ
	bool outputPLY();		// save .ase file as .ply format
	~ASELoader(void);
private:
	void centralize();	//���Ļ���ʹ��ȡase�ļ��Ķ��������ƶ����е�

	void getRealName(const char* fn);	// trim the file string to get rid of the format suffix
private:	
	ASE_OBJECT_HEAD* head;
	char m_sFileN[255];
};

#endif