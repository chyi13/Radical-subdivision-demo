#include "ASEfile.h"

ASEfile::ASEfile(void)
{
}

bool ASEfile::loadfile(const char* filename)
{
	FILE* file;
	ASE_OBJECT* p,*q;
	char line[200];
	file=fopen(filename,"r");
	if (file==NULL)
	{
		printf("FILE IS NOT FOUND!\n");
		return false;
	}


	head=(ASE_OBJECT_HEAD*)malloc(sizeof(ASE_OBJECT_HEAD));
	head->head=NULL;
	head->object_number=0;
	char* ptr;
	while(!feof(file))
	{
		fgets(line,200,file);
		if (strstr(line,"GEOMOBJECT"))
		{
			q=(ASE_OBJECT*)malloc(sizeof(ASE_OBJECT));
			q->next=NULL;

			if (head->head==NULL)
			{
				p=head->head=q;
			}
			else
			{
				p->next=q;
				p=p->next;
			}
			head->object_number++;
		}
		else if (strstr(line,"*MESH_NUMVERTEX"))
		{
			ptr=strstr(line,"*MESH_NUMVERTEX");
			ptr+=strlen("*MESH_NUMVERTEX");
			q->vertex_num=atoi(ptr);
			q->vertex_list=(VERTEX*)malloc((q->vertex_num)*sizeof(VERTEX));
		}
		else if (strstr(line,"*MESH_NUMFACES"))
		{
			ptr=strstr(line,"*MESH_NUMFACES");
			ptr+=strlen("*MESH_NUMFACES");
			q->face_num=atoi(ptr);
			q->face_list=(FACE*)malloc((q->face_num)*sizeof(FACE));

		}
		else if (strstr(line,"*MESH_VERTEX "))
		{
			int vi;
			float vf1,vf2,vf3;
			ptr=strstr(line,"*MESH_VERTEX");
			ptr+=strlen("*MESH_VERTEX ");
			sscanf(ptr,"%d %f %f %f ",&vi,&vf1,&vf2,&vf3);
			q->vertex_list[vi].x=vf1;
			q->vertex_list[vi].y=vf2;
			q->vertex_list[vi].z=vf3;

		}
		else if (strstr(line,"*MESH_FACE "))
		{
			int fi;
			ptr=strstr(line,"*MESH_FACE ");
			ptr+=strlen("*MESH_FACE ");
			fi=atoi(ptr);
			ptr=strstr(line,"A:");
			ptr+=strlen("A:");
			q->face_list[fi].index[0]=atoi(ptr);

			ptr=strstr(line,"B:");
			ptr+=strlen("B:");
			q->face_list[fi].index[1]=atoi(ptr);
			
			ptr=strstr(line,"C:");
			ptr+=strlen("C:");
			q->face_list[fi].index[2]=atoi(ptr);
		}
	}
	fclose(file);
	//ÖÐÐÄ»¯
	centralize();
	return true;

}
void ASEfile::render()
{
	int it;
	ASE_OBJECT* temp;
	temp=head->head;
	glScalef(0.2,0.2,0.2);
	while(temp!=NULL)
	{
		glColor3f(1.0,0.0,0.0);
		
		//glBegin(GL_TRAINGLE_STRIPS);
		for (it=0;it<temp->face_num;it++)
		{
			glBegin(GL_LINE_LOOP);
			glVertex3f(temp->vertex_list[temp->face_list[it].index[0]].x,temp->vertex_list[temp->face_list[it].index[0]].y,temp->vertex_list[temp->face_list[it].index[0]].z);
			glVertex3f(temp->vertex_list[temp->face_list[it].index[1]].x,temp->vertex_list[temp->face_list[it].index[1]].y,temp->vertex_list[temp->face_list[it].index[1]].z);
			glVertex3f(temp->vertex_list[temp->face_list[it].index[2]].x,temp->vertex_list[temp->face_list[it].index[2]].y,temp->vertex_list[temp->face_list[it].index[2]].z);
			glEnd();
		}
		
		temp=temp->next;
	}
}
void ASEfile::print()
{
	FILE* fw;
	fw=fopen("1.txt","w");
	ASE_OBJECT* temp;
	temp=head->head;
	int it;
	while(temp!=NULL)
	{
		for (it=0;it<temp->vertex_num;it++)
			fprintf(fw,"vertex %d %f %f %f \n",it,temp->vertex_list[it].x,temp->vertex_list[it].y,temp->vertex_list[it].z);
		for (it=0;it<temp->face_num;it++)
			fprintf(fw,"face   %d %d %d %d \n",it,temp->face_list[it].index[0],temp->face_list[it].index[1],temp->face_list[it].index[2]);
		temp=temp->next;
	}
}
void ASEfile::centralize()
{
	if (!head!=NULL)
		return;
	int i;
	ASE_OBJECT* p;
	p=head->head;

	double maxX,maxY,maxZ,minX,minY,minZ,midX,midY,midZ;
	maxX = maxY = maxZ = -10000000;
	minX = minY = minZ = 10000000;

	while(p!=NULL)
	{
		for (i=0;i<p->vertex_num;i++)
		{
			if (p->vertex_list[i].x>maxX) maxX=p->vertex_list[i].x;
			if (p->vertex_list[i].x<minX) minX=p->vertex_list[i].x;

			if (p->vertex_list[i].y>maxY) maxY=p->vertex_list[i].y;
			if (p->vertex_list[i].y<minY) minY=p->vertex_list[i].y;

			if (p->vertex_list[i].z>maxZ) maxZ=p->vertex_list[i].z;
			if (p->vertex_list[i].z<minZ) minZ=p->vertex_list[i].z;
		}
		p=p->next;
	}

	midX=(maxX+minX)/2;
	midY=(maxY+minY)/2;
	midZ=(maxZ+minZ)/2;
	
	p=head->head;
	while(p!=NULL)
	{
		for (i=0;i<p->vertex_num;i++)
		{
			p->vertex_list[i].x-=midX;
			p->vertex_list[i].y-=midY;
			p->vertex_list[i].z-=midZ;
		}
		p=p->next;
	}

}
ASEfile::~ASEfile(void)
{
}
