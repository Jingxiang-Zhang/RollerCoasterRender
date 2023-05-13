#pragma once
#ifndef _ROLLER_COASTER_H_
#define _ROLLER_COASTER_H_
#include "openGL/shader.h"
#include "openGL/openGLMatrix.h"
#include "jpg/loadJPG.h"
#include <iostream>
#include <cstring>
#include <vector>

#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#ifndef OUT
#define OUT
#endif
#ifndef IN
#define IN
#endif

extern char shaderBasePath[1024];

namespace rollerCoaster {
	// declear variables
	struct Point {
		double x;
		double y;
		double z;
	};

	struct Spline {
		int numControlPoints;
		Point* points;
	};
	// the mourse button state
	struct mouse_button_state {
		int leftMouseButton;	 // 1 if pressed, 0 if not 
		int middleMouseButton;	 // 1 if pressed, 0 if not
		int rightMouseButton;	 // 1 if pressed, 0 if not
	};

	enum class splineType {
		Catmull_Rom,   // none shading
		B_Splines // Phone shading
	};

	enum class splineCreateType {
		brute_force,
		recursive_subdivision
	};

	// graph transformation state
	typedef enum {
		ROTATE,
		TRANSLATE,
		SCALE
	} control_state;

	// load spline function
	int loadSplines(char* argv);
	// load texture function
	int initTexture(const char* imageFilename, GLuint textureHandle,
		GLuint boarderStyle = GL_REPEAT);

	void saveScreenshot(const char* filename);
	// perform animation inside idleFunc
	void idleFunc();
	// callback for resizing the window
	void reshapeFunc(int w, int h);
	// callback for idle mouse movement
	void mouseMotionDragFunc(int x, int y);
	// callback for mouse drags
	void mouseMotionFunc(int x, int y);
	// callback for mouse button changes
	void mouseButtonFunc(int button, int state, int x, int y);
	// callback for pressing the keys on the keyboard
	void keyboardFunc(unsigned char key, int x, int y);
	// do initialization
	void initScene();
	void initSceneTrack();
	void initPhoneShading();
	void initSceneTexture();
	void initSkyBox();
	void initPhysicalEffect();
	void initShadowProgram();

	// control output FPS
	void timerFunc(int t);

	// display mode, switch by keyboard
	enum class viewMode {
		normal, // normal view position
		ride // ride roller coaster position
	};
	// shading mode, switch by keyboard
	enum class shadingMode {
		None,   // none shading
		PhoneShading, // Phone shading
		PhoneShadingPlusTexture
	};
	void matrixMode_normal();
	void matrixMode_ride();
	void shadingMode_None();
	void shadingMode_PhoneShading();
	void drawTrack();
	void drawGroundTexture();
	void drawSkybox();
	void drawShadow();

	// tells glut to use a particular display function to redraw 
	void displayFunc();

	void render(const char* frame_save_path = nullptr,
		unsigned int FPS = 60);

	// generate the whole spline by splines
	void createSplinesArray(
		OUT std::vector<std::vector<Point>>& position_list,
		OUT std::vector<std::vector<Point>>& tangent_list,
		OUT std::vector<std::vector<Point>>& normal_list,
		OUT std::vector<std::vector<Point>>& binormal_list);

	// generate one part of the spline by brute force
	void splineInterpolationBF(IN Point p0, IN Point p1, IN Point p2, IN Point p3,
		OUT std::vector<Point>& interpolation_list,
		OUT std::vector<Point>& interpolation_tangent);

	// generate one part of the spline by recursive subdivision
	void splineInterpolationRS(IN Point p0, IN Point p1, IN Point p2, IN Point p3,
		OUT std::vector<Point>& interpolation_list,
		OUT std::vector<Point>& interpolation_tangent);

	// convert std::vector<Point> into float vertex array, and index element array
	void convertListToGLData(IN std::vector<std::vector<Point>>& input_point_list,
		OUT float*& vertex, OUT unsigned int*& index, OUT unsigned int& nVertex,
		OUT unsigned int& nIndex);

	void generate_gl_buffer(GLuint& glBuffer, unsigned int size,
		float* vertices_mesh);

	void generate_gl_elem_buffer(GLuint& indicesBuffer, unsigned int size,
		unsigned int* indices_mesh);

	void gl_layout(GLuint& vertexBuffer, const char* variable_name,
		unsigned int step, GLuint program);

	void flatten_vector(IN const std::vector<std::vector<Point>> vec_in,
		OUT std::vector<Point>& vec_out);

	void generate_T_track_new(IN const std::vector<std::vector<Point>>& position_list,
		IN const std::vector<std::vector<Point>>& tangent_list,
		IN const std::vector<std::vector<Point>>& normal_list,
		IN const std::vector<std::vector<Point>>& binormal_list,
		OUT std::vector<float>& track_vertex_stream,
		OUT std::vector<float>& track_normal_stream,
		OUT std::vector<float>& track_color_vertex_stream,
		OUT std::vector<unsigned int>& track_index_stream,
		OUT std::vector<float>& track_texture_coordinate);

	void generate_T_track_one_strip(
		IN const std::vector<std::vector<Point>>& position_list,
		IN const std::vector<std::vector<Point>>& tangent_list,
		IN const std::vector<std::vector<Point>>& normal_list,
		IN const std::vector<std::vector<Point>>& binormal_list,
		IN const float strip1_alpha, IN const float strip1_beta,
		IN const float strip2_alpha, IN const float strip2_beta,
		OUT std::vector<float>& track_vertex_stream,
		OUT std::vector<float>& track_normal_stream,
		OUT std::vector<float>& track_color_vertex_stream,
		OUT std::vector<unsigned int>& track_index_stream,
		OUT std::vector<float>& track_texture_coordinate);

	void generate_ground_mesh(IN const float boarder, IN const float boarder_coordinate,
		OUT std::vector<float>& vertex, OUT std::vector<float>& texture_coordinate,
		OUT std::vector<unsigned int>& index);

	void generate_skybox_mesh(IN unsigned int* vertex_index, IN Point* sky_box_vertices,
		OUT std::vector<float>& vertex, OUT std::vector<float>& texture_coordinate,
		OUT std::vector<unsigned int>& index);

	void setTextureUnit(GLint unit);

	void spline_statistic(std::vector<std::vector<Point>>& position_list);
}

#endif