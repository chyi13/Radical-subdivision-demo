#include "GlutBase.h"

bool gSwitch[3] = {false};

LODBase* g_LOD;

void init() 
{
	initGL();

	g_LOD = new RadicalLOD();

	if (!g_LOD->loadFromFile("venus_radical.ply"))//"bun_zipper_res2.ase"))
	{
		printf("File not found!\n");
		return;
	}
}

int main(int argc, char** argv)
{
	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_DOUBLE|GLUT_DEPTH|GLUT_RGBA);
	glutInitWindowSize(800, 600);
	
	glutCreateWindow(argv[0]);

	init();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(processSpecialKeys);
	glutMouseFunc(mouse);
	glutMotionFunc(mousemove);

	glutMainLoop();

	return 0;
}