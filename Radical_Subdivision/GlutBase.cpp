#include "GlutBase.h"
#include "functions.h"

float v0[3], v1[3];
float mo[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

int width, height;
float scale_v[3] ={1.0f,1.0f,1.0f};

LOD m_lod;

void initGL()
{
	glClearColor(0.75f,0.75f,0.75f,1.f);
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);

	glEnable(GL_DEPTH_TEST);					// open depth test

	GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_shininess[] = { 50.0 };
	GLfloat globel_ambient[] = { 0.0 , 0.0 , 0.0 , 1.0 };
	GLfloat light_position[] = { 0.0, 0.0, 50.0, 1.0 };
	GLfloat mat_ambient[]={0.0,0.0,0.0,1.0};

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT , globel_ambient);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);

	width = 800 ; height = 600;
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60,width/height,0.1,1000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	{
		gluLookAt(0.0,0.0,50.0+scale_v[0],0,0,0,0,1,0);
		glMultMatrixf( mo );
		//asefile.render();
		//glScalef(scale_v[0],scale_v[1],scale_v[2]);
		//glScalef(10.f,10.f,10.f);
		//renderAse();
		
		//mloader.render();
		m_lod.render();
	}
	glPopMatrix();

	glutSwapBuffers();
}

void reshape(int w,int h)
{
	glViewport(0,0,GLsizei(w),GLsizei(h));
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(60.f,GLfloat(w)/GLfloat(h),0.1f,1000.f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	width = w;
	height = h;
}
void processSpecialKeys(int key, int x, int y) {

	switch(key) 
	{
	case GLUT_KEY_UP:
		scale_v[0] += 1.0f;
		scale_v[1] += 1.0f;
		scale_v[2] += 1.0f;
		break;

	case GLUT_KEY_DOWN:
		scale_v[0] -= 1.0f;
		scale_v[1] -= 1.0f;
		scale_v[2] -= 1.0f;

		break;
	default:
		break;

	}
	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
	case 27:
		exit(0);
		break;

	case 'f':
		glutFullScreen();
		break;
	case 'r':
		m_lod.radicalSubdivision();
//		m_lod.loopSubdivision();
		break;
	case 'w':
		m_lod.setWired();
		break;
	case 'b':
		m_lod.nextLevel();
		break;
	case 'u':
		m_lod.prevLevel();
		break;
	default:
		break;
	}
	
	glutPostRedisplay();		
}

void mouse(int btn, int state, int x, int y) 
{
	switch (btn) {
	  case GLUT_LEFT_BUTTON:
		  {
			  switch(state)
			  {
			  case GLUT_DOWN:
				  vassign( v0, 2.0*x/width-1, -2.0*y/height+1, 1 );
				  normalize(v0);
				  break;
			  }
		  }
		  break;

	  default:
		  break;
	}
	glutPostRedisplay();
}

void mousemove(int x, int y)
{
	float axis[3], angle;

	vassign( v1, 2.0*x/width-1, -2.0*y/height+1, 1 );
	normalize(v1);
	if( dot(v0,v1)>.999 )
		return;
	cross(axis,v0,v1);
	normalize(axis);
	angle = acosf( clamp(dot(v0,v1),-1,1) );
	vassign( v0, v1 );

	glPushMatrix();
	glLoadIdentity();
	glRotatef( angle*180/PI, axis[0], axis[1], axis[2] );
	glMultMatrixf( mo );
	glGetFloatv( GL_MODELVIEW_MATRIX, mo );
	glPopMatrix();
	glutPostRedisplay();
}