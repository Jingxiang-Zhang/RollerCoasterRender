#include "rollerCoaster.h"
#include <algorithm>
#include <random>
#include <stack>
#include <GL/glut.h>

char shaderBasePath[1024] = "openGL";

// declear variable
// global parameters
rollerCoaster::splineType spline_type = rollerCoaster::splineType::B_Splines; // type of spline
float lookAtHeight = 40.0f; // camera position at (0, 0, lookAtHeight)
// type of spline creation
rollerCoaster::splineCreateType spline_create_type =
rollerCoaster::splineCreateType::brute_force;
// number of interpolation point in spline when using brute force
unsigned int interpolate_point = 50;
// maximum distance between two contiguous point for interpolation in spline when using 
// recursive subdivision
float length_threshold = 0.1f;
// the light position
glm::vec3 lightDirection(150, 200, 100);

// other variable
int numSplines;
rollerCoaster::Spline* splines;
int mousePos[2];
rollerCoaster::mouse_button_state mouseButtonState = { 0, 0, 0 };
rollerCoaster::control_state controlState;

// physical information
float base_height;
float energy; // suppose the weight of object is m = 1kg (unit)
rollerCoaster::Point current_position; // current speed
rollerCoaster::Point current_tangent; // current speed
rollerCoaster::Point current_normal; // current speed

// control variable
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "hw2";

// frame save path
const char* frame_save_path = nullptr;
// windows FPS
unsigned int frame_save_count = 0;
// frame save count
unsigned int FPS = 60;

// vertex information
GLuint splineTrackVertexArray;
GLuint splineTrackVertexBuffer_1;
GLuint splineTrackTextureCoordinateBuffer_1;
GLuint splineTrackColorVertexBuffer_1;
GLuint splineTrackVertexBuffer_1_index;
GLuint splineTextureHandle;
OpenGLMatrix matrix;
Shader* pipelineProgram;
// number of indices that need to be plot
//unsigned int nSplineVertexBuffer_1;
unsigned int nSplineTrackVertexBuffer_1;

// ground texture part
float groundHeight = -1.0f;
Shader* textureProgram;
GLuint textureHandle;
GLuint textureVertexArray;
GLuint textureVertexBuffer_1;
GLuint textureVertexBuffer_1_coordinate;
GLuint textureVertexBuffer_1_index;
unsigned int nTextureVertexBuffer_1;

// shadow part
Shader* shadowProgram;
float shadowMatrix[16];

// sky box part, this will use some variable (textureProgram) of the texture part
float sky_box_distance = 1000.0f;
GLuint skyboxHandle[6];
GLuint skyboxVertexArray[6];
GLuint skyboxVertexBuffer[6];
GLuint skyboxVertexBuffer_coordinate[6];
GLuint skyboxVertexBuffer_index[6];
unsigned int nSkyboxVertexBuffer[6];

// shading part
GLuint splineTrackNormalVectorBuffer;

rollerCoaster::viewMode view_mode = rollerCoaster::viewMode::normal;
rollerCoaster::shadingMode shading_mode = rollerCoaster::shadingMode::None;
unsigned int ride_index = 0;

// spline information
std::vector<rollerCoaster::Point> spline_position;
std::vector<rollerCoaster::Point> spline_tangent;
std::vector<rollerCoaster::Point> spline_normal;
std::vector<rollerCoaster::Point> spline_binormal;

void rollerCoaster::saveScreenshot(const char* filename) {
    unsigned char* screenshotData = new unsigned char[windowWidth * windowHeight * 3];
    glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

    LoadJPG screenshotImg(windowWidth, windowHeight, 3, screenshotData);
    if (screenshotImg.save(filename) == LoadJPG::OK)
        std::cout << "File " << filename << " saved successfully." << std::endl;
    else std::cout << "Failed to save file " << filename << '.' << std::endl;

    delete[] screenshotData;
}

void rollerCoaster::matrixMode_normal() {
    matrix.LoadIdentity();
    matrix.LookAt(0, 0, lookAtHeight, 0, 0, 0, 0, 1, 0);
    matrix.Scale(landScale[0], landScale[1], landScale[2]);
    matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
    matrix.Rotate(landRotate[0] / 5, 1, 0, 0);
    matrix.Rotate(landRotate[1] / 5, 0, 1, 0);
    matrix.Rotate(landRotate[2] / 5, 0, 0, 1);
}

void rollerCoaster::matrixMode_ride() {
    float relative_height = current_position.z - base_height;
    float current_energy = relative_height * 9.8;
    float speed = std::sqrt((energy - current_energy) * 2);
    float total_distance = speed / FPS / 2;
    Point& position = spline_position[ride_index + 1];
    Point& tangent = spline_tangent[ride_index + 1];
    Point& normal = spline_normal[ride_index + 1];
    // calculation the distance from current_position to the next vertices
    float distance = (position.x - current_position.x) * (position.x - current_position.x) +
        (position.y - current_position.y) * (position.y - current_position.y) +
        (position.z - current_position.z) * (position.z - current_position.z);
    distance = std::sqrt(distance);
    // std::cout << position.x << " " << position.y << " " << position.z << std::endl;

    while (distance <= total_distance) {
        total_distance -= distance;
        current_position.x = position.x;
        current_position.y = position.y;
        current_position.z = position.z;
        current_tangent.x = tangent.x;
        current_tangent.y = tangent.y;
        current_tangent.z = tangent.z;
        current_normal.x = normal.x;
        current_normal.y = normal.y;
        current_normal.z = normal.z;
        ride_index++;
        if (ride_index >= spline_position.size() - 1) {
            view_mode = viewMode::normal;
            return;
        }
        position = spline_position[ride_index + 1];
        tangent = spline_tangent[ride_index + 1];
        normal = spline_normal[ride_index + 1];
        distance = (position.x - current_position.x) * (position.x - current_position.x) +
            (position.y - current_position.y) * (position.y - current_position.y) +
            (position.z - current_position.z) * (position.z - current_position.z);
        distance = std::sqrt(distance);
    }
    if (distance > total_distance) {
        float ratio = total_distance / distance;
        current_position.x = (current_position.x * (1 - ratio)) + position.x * ratio;
        current_position.y = (current_position.y * (1 - ratio)) + position.y * ratio;
        current_position.z = (current_position.z * (1 - ratio)) + position.z * ratio;
        current_tangent.x = (current_tangent.x * (1 - ratio)) + tangent.x * ratio;
        current_tangent.y = (current_tangent.y * (1 - ratio)) + tangent.y * ratio;
        current_tangent.z = (current_tangent.z * (1 - ratio)) + tangent.z * ratio;
        current_normal.x = (current_normal.x * (1 - ratio)) + normal.x * ratio;
        current_normal.y = (current_normal.y * (1 - ratio)) + normal.y * ratio;
        current_normal.z = (current_normal.z * (1 - ratio)) + normal.z * ratio;
    }

    matrix.LoadIdentity();
    matrix.LookAt(current_position.x, current_position.y, current_position.z,
        current_position.x + current_tangent.x, current_position.y +
        current_tangent.y, current_position.z + current_tangent.z,
        current_normal.x, current_normal.y, current_normal.z);

    //matrix.LoadIdentity();
    //Point& position = spline_position[ride_index];
    //Point& tangent = spline_tangent[ride_index];
    //Point& normal = spline_normal[ride_index];
    //matrix.LookAt(position.x, position.y, position.z, 
    //    position.x + tangent.x, position.y + tangent.y, position.z + tangent.z,
    //    normal.x, normal.y, normal.z);
    //ride_index++;
    //if (ride_index >= spline_position.size()) {
    //    view_mode = viewMode::normal;
    //}
}

void rollerCoaster::shadingMode_None() {
    // set mode equal to 0
    pipelineProgram->Bind();
    GLuint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode_v");
    glUniform1i(loc, 0);
    loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode_f");
    glUniform1i(loc, 0);
}

void rollerCoaster::shadingMode_PhoneShading() {
    // set mode equal to 1
    pipelineProgram->Bind();
    int mode_flag = 1;
    if (shading_mode == shadingMode::PhoneShading) {
        mode_flag = 1;
    }
    else {
        mode_flag = 2;
    }
    GLuint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode_v");
    glUniform1i(loc, mode_flag);
    loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode_f");
    glUniform1i(loc, mode_flag);
    // set normalMatrix matrix
    loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "normalMatrix");
    float n[16];
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.GetNormalMatrix(n);
    // float n[16] = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
    GLboolean isRowMajor = GL_FALSE;
    glUniformMatrix4fv(loc, 1, isRowMajor, n);

    // set viewLightDirection
    loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "viewLightDirection");
    glm::vec3 lightDirectionTemp(lightDirection);
    glm::normalize(lightDirectionTemp);
    glm::vec4 lightDirectionV4(lightDirectionTemp[0], lightDirectionTemp[1], lightDirectionTemp[2], 1.0f);

    float m[16];
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.GetMatrix(m);

    glm::mat4 viewMatrix(n[0], n[1], n[2], n[3],
        n[4], n[5], n[6], n[7],
        n[8], n[9], n[10], n[11],
        n[12], n[13], n[14], n[15]);

    //glm::mat4 viewMatrix(n[0], n[4], n[8], n[12],
    //    n[1], n[5], n[9], n[13],
    //    n[2], n[6], n[10], n[14],
    //    n[3], n[7], n[11], n[15]);

    // light direction in the view space
    glm::vec4 viewLightDirection_ = viewMatrix * lightDirectionV4;
    glm::vec3 viewLightDirection(viewLightDirection_[0], viewLightDirection_[1],
        viewLightDirection_[2]);
    viewLightDirection = glm::normalize(viewLightDirection);

    //std::cout << viewLightDirection[0] << " " <<
    //    viewLightDirection[1] << " " <<
    //    viewLightDirection[2] << " " << std::endl;

    // glUniform3f(loc, 1,1,1);

    glUniform3f(loc, viewLightDirection[0], viewLightDirection[1],
        viewLightDirection[2]);
}

void rollerCoaster::drawTrack() {
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    float m[16];
    matrix.GetMatrix(m);
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    float p[16];
    matrix.GetMatrix(p);

    // draw track
    pipelineProgram->Bind();
    pipelineProgram->SetModelViewMatrix(m);
    pipelineProgram->SetProjectionMatrix(p);
    setTextureUnit(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, splineTextureHandle);

    glBindVertexArray(splineTrackVertexArray);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, splineTrackVertexBuffer_1_index);
    glDrawElements(GL_TRIANGLES, nSplineTrackVertexBuffer_1, GL_UNSIGNED_INT, nullptr);
}

void rollerCoaster::drawGroundTexture() {
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    float m[16];
    matrix.GetMatrix(m);
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    float p[16];
    matrix.GetMatrix(p);

    textureProgram->Bind();
    textureProgram->SetModelViewMatrix(m);
    textureProgram->SetProjectionMatrix(p);
    setTextureUnit(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glBindVertexArray(textureVertexArray);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textureVertexBuffer_1_index);
    glDrawElements(GL_TRIANGLES, nTextureVertexBuffer_1, GL_UNSIGNED_INT, nullptr);
}

void rollerCoaster::drawSkybox() {
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    float m[16];
    matrix.GetMatrix(m);
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    float p[16];
    matrix.GetMatrix(p);

    textureProgram->Bind();
    textureProgram->SetModelViewMatrix(m);
    textureProgram->SetProjectionMatrix(p);
    for (int i = 0; i < 6; i++) {
        //textureProgram->Bind();
        //textureProgram->SetModelViewMatrix(m);
        //textureProgram->SetProjectionMatrix(p);
        setTextureUnit(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyboxHandle[i]);
        glBindVertexArray(skyboxVertexArray[i]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxVertexBuffer_index[i]);
        glDrawElements(GL_TRIANGLES, nSkyboxVertexBuffer[i], GL_UNSIGNED_INT, nullptr);
    }
}

void rollerCoaster::drawShadow() {
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    float p[16];
    matrix.GetMatrix(p);

    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.PushMatrix(); // remember first in last out
    matrix.Translate(lightDirection[0], lightDirection[1], lightDirection[2] + groundHeight); // translate back
    matrix.MultMatrix(shadowMatrix); // project
    matrix.Translate(-lightDirection[0], -lightDirection[1], -lightDirection[2]); // move light to origin
    float ms[16];
    matrix.GetMatrix(ms); // read the shadow matrix
    matrix.PopMatrix();
    shadowProgram->Bind();
    shadowProgram->SetModelViewMatrix(ms);
    shadowProgram->SetProjectionMatrix(p);
    glBindVertexArray(splineTrackVertexArray);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, splineTrackVertexBuffer_1_index);
    glDrawElements(GL_TRIANGLES, nSplineTrackVertexBuffer_1, GL_UNSIGNED_INT, nullptr);
}

void rollerCoaster::displayFunc()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    switch (view_mode) {
    case viewMode::normal:
        matrixMode_normal();
        break;
    case viewMode::ride:
        matrixMode_ride();
        break;
    default:
        matrixMode_normal();
    }
    switch (shading_mode) {
    case shadingMode::None:
        shadingMode_None();
        break;
    default:
        shadingMode_PhoneShading();
    }

    // draw track
    drawTrack();
    // draw skybox
    drawSkybox();

    // 1. Set depth buffer to read-only, draw surface
    glDepthMask(GL_FALSE);
    drawGroundTexture();
    // 2. Set depth buffer to read-write, draw shadow
    glDepthMask(GL_TRUE);
    drawShadow();
    // 3. Set color buffer to read-only, draw surface again
    GLboolean colorMask[4];
    glGetBooleanv(GL_COLOR_WRITEMASK, colorMask); // save current color mask
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    drawGroundTexture();
    // 4. Set color buffer to read-write
    glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    // finally draw on the screen
    glutSwapBuffers();
}

void rollerCoaster::idleFunc()
{
    // do some stuff... 
    // for example, here, you can save the screenshots to disk (to make the animation)
    // make the screen update 
    glutPostRedisplay();
}

void rollerCoaster::reshapeFunc(int w, int h)
{
    glViewport(0, 0, w, h);
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.LoadIdentity();
    matrix.Perspective(54.0f, (float)w / (float)h, 0.01f, 2000.0f);
}

void rollerCoaster::mouseMotionDragFunc(int x, int y)
{
    // mouse has moved and one of the mouse buttons is pressed (dragging)
    int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };
    // the change in mouse position since the last invocation of this function
    if (view_mode != viewMode::normal) {
        // mouse has moved
        // store the new mouse position
        mousePosDelta[0] = 0;
        mousePosDelta[1] = 0;
    }

    switch (controlState)
    {
        // translate the landscape
    case TRANSLATE:
        if (mouseButtonState.leftMouseButton)
        {
            // control x,y translation via the left mouse button
            landTranslate[0] += mousePosDelta[0] * 0.01f;
            landTranslate[1] -= mousePosDelta[1] * 0.01f;
        }
        if (mouseButtonState.middleMouseButton)
        {
            // control z translation via the middle mouse button
            landTranslate[2] -= mousePosDelta[1] * 0.01f;
        }
        break;

        // rotate the landscape
    case ROTATE:
        if (mouseButtonState.leftMouseButton)
        {
            // control x,y rotation via the left mouse button
            landRotate[0] += mousePosDelta[1];
            landRotate[1] += mousePosDelta[0];
        }
        if (mouseButtonState.middleMouseButton)
        {
            // control z rotation via the middle mouse button
            landRotate[2] += mousePosDelta[1];
        }
        break;

        // scale the landscape
    case SCALE:
        if (mouseButtonState.leftMouseButton)
        {
            // control x,y scaling via the left mouse button
            landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
            landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
        }
        if (mouseButtonState.middleMouseButton)
        {
            // control z scaling via the middle mouse button
            landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
        }
        break;
    }
    // store the new mouse position
    mousePos[0] = x;
    mousePos[1] = y;
}

void rollerCoaster::mouseMotionFunc(int x, int y)
{
    // mouse has moved
    // store the new mouse position
    mousePos[0] = x;
    mousePos[1] = y;
}

void rollerCoaster::mouseButtonFunc(int button, int state, int x, int y)
{
    // a mouse button has has been pressed or depressed

    // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
    switch (button)
    {
    case GLUT_LEFT_BUTTON:
        mouseButtonState.leftMouseButton = (state == GLUT_DOWN);
        break;
    case GLUT_MIDDLE_BUTTON:
        mouseButtonState.middleMouseButton = (state == GLUT_DOWN);
        break;
    case GLUT_RIGHT_BUTTON:
        mouseButtonState.rightMouseButton = (state == GLUT_DOWN);
        break;
    }

    // keep track of whether CTRL and SHIFT keys are pressed
    switch (glutGetModifiers())
    {
    case GLUT_ACTIVE_CTRL:
        controlState = TRANSLATE;
        break;
    case GLUT_ACTIVE_SHIFT:
        controlState = SCALE;
        break;
        // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
        controlState = ROTATE;
        break;
    }
    // store the new mouse position
    mousePos[0] = x;
    mousePos[1] = y;
}

void rollerCoaster::keyboardFunc(unsigned char key, int x, int y)
{

    switch (key)
    {
    case 27: // ESC key
        exit(0); // exit the program
        break;
    case '1':
        std::cout << "switch to view mode normal" << std::endl;
        view_mode = viewMode::normal;
        break;
    case '2':
        std::cout << "switch to view mode ride" << std::endl;
        view_mode = viewMode::ride;
        ride_index = 0;
        current_position = spline_position[0];
        current_tangent = spline_tangent[0];
        current_normal = spline_normal[0];
        break;
    case '3':
        pipelineProgram->Bind();
        if (shading_mode == shadingMode::None) {
            std::cout << "switch to Phone shading mode" << std::endl;
            shading_mode = shadingMode::PhoneShading;
        }
        else if (shading_mode == shadingMode::PhoneShading) {
            std::cout << "switch to Phone plus texture shading mode" << std::endl;
            shading_mode = shadingMode::PhoneShadingPlusTexture;
        }
        else {
            std::cout << "switch to None shading mode" << std::endl;
            shading_mode = shadingMode::None;
        }
        break;
    case 'x':
        // take a screenshot
        saveScreenshot("screenshot.jpg");
        break;
    }
}

int rollerCoaster::loadSplines(char* argv)
{
    char* cName = (char*)malloc(128 * sizeof(char));
    FILE* fileList;
    FILE* fileSpline;
    int iType, i = 0, j, iLength;

    // load the track file 
    fileList = fopen(argv, "r");
    if (fileList == NULL)
    {
        printf("can't open file\n");
        exit(1);
    }

    // stores the number of splines in a global variable 
    fscanf(fileList, "%d", &numSplines);

    splines = (Spline*)malloc(numSplines * sizeof(Spline));

    // reads through the spline files 
    for (j = 0; j < numSplines; j++)
    {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");

        if (fileSpline == NULL)
        {
            printf("can't open file\n");
            exit(1);
        }

        // gets length for spline file
        fscanf(fileSpline, "%d %d", &iLength, &iType);

        // allocate memory for all the points
        splines[j].points = (Point*)malloc(iLength * sizeof(Point));
        splines[j].numControlPoints = iLength;

        // saves the data to the struct
        while (fscanf(fileSpline, "%lf %lf %lf",
            &splines[j].points[i].x,
            &splines[j].points[i].y,
            &splines[j].points[i].z) != EOF)
        {
            i++;
        }
    }
    free(cName);
    return 0;
}

int rollerCoaster::initTexture(const char* imageFilename, GLuint textureHandle,
    GLuint boarderStyle)
{
    // read the texture image
    LoadJPG img;
    LoadJPG::errorType err = img.load(imageFilename);

    if (err != LoadJPG::OK) {
        printf("Loading texture from %s failed.\n", imageFilename);
        return -1;
    }

    // check that the number of bytes is a multiple of 4
    if (img.getWidth() * img.getBytesPerPixel() % 4) {
        printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
        return -1;
    }

    // allocate space for an array of pixels
    int width = img.getWidth();
    int height = img.getHeight();
    unsigned char* pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

    // fill the pixelsRGBA array with the image pixels
    memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
    for (int h = 0; h < height; h++)
        for (int w = 0; w < width; w++)
        {
            // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
            pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
            pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
            pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
            pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

            // set the RGBA channels, based on the loaded image
            int numChannels = img.getBytesPerPixel();
            for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
                pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
        }

    // bind the texture
    glBindTexture(GL_TEXTURE_2D, textureHandle);

    // initialize the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

    // generate the mipmaps for this texture
    glGenerateMipmap(GL_TEXTURE_2D);

    // set the texture parameters
    // set when the image is rander smaller, use mipmap linear interpolation.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // set when image is rander larger, use linear interpolation
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // set when the coordinate is outside [0,1] in S, then repeat the image
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, boarderStyle);
    // set when the coordinate is outside [0,1] in T, then repeat the image
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, boarderStyle);
    // query support for anisotropic texture filtering
    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    printf("Max available anisotropic samples: %f\n", fLargest);
    // set anisotropic texture filtering
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

    // query for any errors
    GLenum errCode = glGetError();
    if (errCode != 0)
    {
        printf("Texture initialization error. Error code: %d.\n", errCode);
        return -1;
    }

    // de-allocate the pixel array -- it is no longer needed
    delete[] pixelsRGBA;

    return 0;
}

void rollerCoaster::initSceneTrack() {
    // mind that the uncomment part is used to draw spline
   // create spline
    std::vector<std::vector<Point>> position_list, tangent_list, normal_list, binormal_list;
    createSplinesArray(position_list, tangent_list, normal_list, binormal_list);
    flatten_vector(position_list, spline_position);
    flatten_vector(tangent_list, spline_tangent);
    flatten_vector(normal_list, spline_normal);
    flatten_vector(binormal_list, spline_binormal);

    std::vector<float> track_vertex;
    std::vector<float> track_vertex_normal;
    std::vector<float> track_color_vertex;
    std::vector<float> track_texture_coordinate;
    std::vector<unsigned int> track_index;
    generate_T_track_new(position_list, tangent_list, normal_list, binormal_list,
        track_vertex, track_vertex_normal, track_color_vertex, track_index,
        track_texture_coordinate);

    nSplineTrackVertexBuffer_1 = track_index.size();
    generate_gl_buffer(splineTrackVertexBuffer_1, track_vertex.size() * 4, track_vertex.data());
    generate_gl_buffer(splineTrackNormalVectorBuffer, track_vertex_normal.size() * 4, track_vertex_normal.data());
    generate_gl_buffer(splineTrackColorVertexBuffer_1, track_color_vertex.size() * 4, track_color_vertex.data());    generate_gl_buffer(splineTrackColorVertexBuffer_1, track_color_vertex.size() * 4, track_color_vertex.data());
    generate_gl_buffer(splineTrackTextureCoordinateBuffer_1,
        track_texture_coordinate.size() * 4, track_color_vertex.data());

    generate_gl_elem_buffer(splineTrackVertexBuffer_1_index, track_index.size() * 4, track_index.data());
    // prapare for OpenGL program
    pipelineProgram = new Shader(shaderBasePath, "basic.vertexShader.glsl", "basic.fragmentShader.glsl");
    pipelineProgram->Bind();

    glGenVertexArrays(1, &splineTrackVertexArray);
    glBindVertexArray(splineTrackVertexArray);
    gl_layout(splineTrackVertexBuffer_1, "position", 3, pipelineProgram->GetProgramHandle());
    gl_layout(splineTrackColorVertexBuffer_1, "color", 4, pipelineProgram->GetProgramHandle());
    gl_layout(splineTrackTextureCoordinateBuffer_1, "texCoord", 2,
        pipelineProgram->GetProgramHandle());

    // track texture
    glGenTextures(1, &splineTextureHandle);
    int code = initTexture("metal.jpg", splineTextureHandle);
    if (code != 0) {
        printf("Error loading the texture image.\n");
        exit(EXIT_FAILURE);
    }

    glEnable(GL_DEPTH_TEST);
    std::cout << "GL error: " << glGetError() << std::endl;
}

void rollerCoaster::initSceneTexture() {
    std::vector<float> grid_mesh;
    std::vector<float> texture_coordinate;
    std::vector<unsigned int> index;

    generate_ground_mesh(10000.0f, 1000.0f, grid_mesh, texture_coordinate, index);
    nTextureVertexBuffer_1 = index.size();
    generate_gl_buffer(textureVertexBuffer_1, grid_mesh.size() * 4, grid_mesh.data());
    generate_gl_buffer(textureVertexBuffer_1_coordinate,
        texture_coordinate.size() * 4, texture_coordinate.data());
    generate_gl_elem_buffer(textureVertexBuffer_1_index, index.size() * 4, index.data());

    // prapare for OpenGL program
    textureProgram = new Shader(shaderBasePath, "texture.vertexShader.glsl",
        "texture.fragmentShader.glsl");
    textureProgram->Bind();
    glGenVertexArrays(1, &textureVertexArray);
    glBindVertexArray(textureVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, textureVertexBuffer_1);
    gl_layout(textureVertexBuffer_1, "position", 3, textureProgram->GetProgramHandle());
    glBindBuffer(GL_ARRAY_BUFFER, textureVertexBuffer_1_coordinate);
    gl_layout(textureVertexBuffer_1_coordinate, "texCoord", 2, textureProgram->GetProgramHandle());

    // texture part
    glGenTextures(1, &textureHandle);
    int code = initTexture("ground.jpg", textureHandle);
    if (code != 0) {
        printf("Error loading the texture image.\n");
        exit(EXIT_FAILURE);
    }
}

void rollerCoaster::initPhoneShading() {
    pipelineProgram->Bind();
    // init the mode
    GLuint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode_v");
    glUniform1i(loc, 0);
    loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode_f");
    glUniform1i(loc, 0);
    // init normal
    glBindVertexArray(splineTrackVertexArray);
    gl_layout(splineTrackNormalVectorBuffer, "normal", 3, pipelineProgram->GetProgramHandle());
    // init variable in vertex shader

    float ka = 0.3f;
    float kd = 0.4f;
    float ks = 0.6f;
    float alpha = 3.0f;

    loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "La");
    glUniform4f(loc, 1.0f, 1.0f, 1.0f, 1.0f);
    loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ld");
    glUniform4f(loc, 1.0f, 1.0f, 1.0f, 1.0f);
    loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ls");
    glUniform4f(loc, 1.0f, 1.0f, 1.0f, 1.0f);
    loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ka");
    glUniform4f(loc, ka, ka, ka, 1.0f);
    loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "kd");
    glUniform4f(loc, kd, kd, kd, 1.0f);
    loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ks");
    glUniform4f(loc, ks, ks, ks, 1.0f);
    loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "alpha");
    glUniform1f(loc, alpha);
    std::cout << "GL error: " << glGetError() << std::endl;
}

void rollerCoaster::initSkyBox() {
    float& d = sky_box_distance;
    Point sky_box_vertices[8] = { // 8 vertices of the sky box
        {d, d, d}, {d, d, -d}, {d, -d, d}, {d, -d, -d},
        {-d, d, d}, {-d, d, -d}, {-d, -d, d}, {-d, -d, -d},
    };

    //Point sky_box_vertices[8] = { // 8 vertices of the sky box
    //    {d, d, d}, {d, d, 0}, {d, -d, d}, {d, -d, 0},
    //    {-d, d, d}, {-d, d, 0}, {-d, -d, d}, {-d, -d, 0},
    //};

    const char sky_box_img_path_list[6][100] = { // 6 imgs of sky box
        "skybox\\sky0.jpg", "skybox\\sky1.jpg", "skybox\\sky2.jpg",
        "skybox\\sky3.jpg", "skybox\\sky4.jpg", "skybox\\sky5.jpg",
    };
    struct sky_box {
        unsigned int up[4] = { 0, 2, 6, 4 };
        unsigned int down[4] = { 3, 1, 5, 7 };
        unsigned int left[4] = { 6, 7, 5, 4 };
        unsigned int right[4] = { 0, 1, 3, 2 };
        unsigned int front[4] = { 4, 5, 1, 0 };
        unsigned int back[4] = { 2, 3, 7, 6 };
        unsigned int sky_box_order[6] = { 1, 3, 0, 2, 4, 5 };
    };

    sky_box sky_box_obj;
    unsigned int* for_iteration[6] = {
        sky_box_obj.up, sky_box_obj.down, sky_box_obj.back,
        sky_box_obj.front, sky_box_obj.left, sky_box_obj.right
    };
    textureProgram->Bind();
    for (int i = 0; i < 6; i++) {
        std::vector<float> grid_mesh;
        std::vector<float> texture_coordinate;
        std::vector<unsigned int> index;
        generate_skybox_mesh(for_iteration[i], sky_box_vertices, grid_mesh,
            texture_coordinate, index);

        nSkyboxVertexBuffer[i] = index.size();
        generate_gl_buffer(skyboxVertexBuffer[i], grid_mesh.size() * 4, grid_mesh.data());
        generate_gl_buffer(skyboxVertexBuffer_coordinate[i],
            texture_coordinate.size() * 4, texture_coordinate.data());
        generate_gl_elem_buffer(skyboxVertexBuffer_index[i], index.size() * 4, index.data());

        glGenVertexArrays(1, &skyboxVertexArray[i]);
        glBindVertexArray(skyboxVertexArray[i]);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVertexBuffer[i]);
        gl_layout(skyboxVertexBuffer[i], "position", 3, textureProgram->GetProgramHandle());
        glBindBuffer(GL_ARRAY_BUFFER, textureVertexBuffer_1_coordinate);
        gl_layout(skyboxVertexBuffer_coordinate[i], "texCoord", 2, textureProgram->GetProgramHandle());

        // texture part
        glGenTextures(1, &skyboxHandle[i]);
        int code = initTexture(sky_box_img_path_list[sky_box_obj.sky_box_order[i]],
            skyboxHandle[i], GL_CLAMP_TO_EDGE);
        if (code != 0) {
            printf("Error loading the texture image.\n");
            exit(EXIT_FAILURE);
        }
    }
}

void rollerCoaster::initPhysicalEffect() {
    // 1/2 mv^2 = mgh  ==>  v = sqrt(2gh)
    // find the maximum height and the minimum height
    float minimul_height = 1000.0f;
    float maximum_height = -1000.0f;
    for (Point& p : spline_position) {
        maximum_height = std::max(maximum_height, (float)p.z);
        minimul_height = std::min(minimul_height, (float)p.z);
    }
    base_height = minimul_height;
    energy = (maximum_height - minimul_height) * 9.8f + 1;
}

void rollerCoaster::initShadowProgram() {
    glEnable(GL_BLEND); // enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // set blend function

    shadowProgram = new Shader(shaderBasePath, "shadow.vertexShader.glsl",
        "shadow.fragmentShader.glsl");
    
    float tempShadowMatrix[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, -1.0f / lightDirection[2],
        0.0f, 0.0f, 0.0f, 0.0f
    };
    memcpy(shadowMatrix, tempShadowMatrix, sizeof(float) * 16);

    // shadowProgram->Bind();
    // glBindVertexArray(splineTrackVertexArray);
    // gl_layout(splineTrackVertexBuffer_1, "position", 3, pipelineProgram->GetProgramHandle());
}

void rollerCoaster::initScene()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    initSceneTrack();
    // physical effect must be implemented after initialize the scene track.
    initPhysicalEffect();
    // shadow program mst be implemented after initialize the scene track.
    initShadowProgram();
    initPhoneShading();
    initSceneTexture();
    // mind that the sky box initialization need some variable inherit from texture part
    // so, you need to init scene texture first.
    initSkyBox();
}

void rollerCoaster::timerFunc(int t)
{
    glutPostRedisplay();
    // save frame
    if (frame_save_path != nullptr) {
        char path[128];
        strcpy(path, frame_save_path);
        strcat(path, "/");
        char number[4] = { '0' ,'0', '0', 0 };
        int loc = 2;
        unsigned int frame_save_count_ = frame_save_count;
        while (loc >= 0) {
            number[loc] = frame_save_count_ % 10 + '0';
            frame_save_count_ = frame_save_count_ / 10;
            loc--;
        }
        strcat(path, number);
        strcat(path, ".jpg");
        frame_save_count++;
        saveScreenshot(path);
    }
    glutTimerFunc(1000.0f / (float)FPS, timerFunc, 0);
}

void rollerCoaster::render(const char* frame_save_path_, unsigned int FPS_)
{
    frame_save_path = frame_save_path_;
    FPS = FPS_;
    std::cout << "Initializing GLUT..." << std::endl;
    int argc = 1;
    glutInit(&argc, nullptr);
    std::cout << "Initializing OpenGL..." << std::endl;

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(windowTitle);
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // tells glut to use a particular display function to redraw 
    glutDisplayFunc(displayFunc);
    // perform animation inside idleFunc
    // glutIdleFunc(idleFunc);
    // callback for mouse drags
    glutMotionFunc(mouseMotionDragFunc);
    // callback for idle mouse movement
    glutPassiveMotionFunc(mouseMotionFunc);
    // callback for mouse button changes
    glutMouseFunc(mouseButtonFunc);
    // callback for resizing the window
    glutReshapeFunc(reshapeFunc);
    // callback for pressing the keys on the keyboard
    glutKeyboardFunc(keyboardFunc);
    // control output FPS
    glutTimerFunc(1000.0f / (float)FPS_, timerFunc, 0);

    // init glew
    GLint result = glewInit();
    if (result != GLEW_OK) {
        std::cout << "error: " << glewGetErrorString(result) << std::endl;
        exit(EXIT_FAILURE);
    }
    // do initialization
    initScene();
    // sink forever into the glut loop
    glutMainLoop();
}

void rollerCoaster::generate_gl_buffer(GLuint& glBuffer, unsigned int size,
    float* vertices_mesh)
{
    glGenBuffers(1, &glBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, glBuffer);
    glBufferData(GL_ARRAY_BUFFER, size, vertices_mesh, GL_STATIC_DRAW);
}

void rollerCoaster::generate_gl_elem_buffer(GLuint& indicesBuffer, unsigned int size,
    unsigned int* indices_mesh)
{
    glGenBuffers(1, &indicesBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices_mesh
        , GL_STATIC_DRAW);
}

void rollerCoaster::gl_layout(GLuint& vertexBuffer, const char* variable_name,
    unsigned int step, GLuint program)
{
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    GLuint loc = glGetAttribLocation(program, variable_name);
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, step, GL_FLOAT, GL_FALSE, 0, (const void*)0);
}

// convert std::vector<Point> into float vertex array, and index element array
void rollerCoaster::convertListToGLData(IN std::vector<std::vector<Point>>& input_point_list,
    OUT float*& vertex, OUT unsigned int*& index, OUT unsigned int& nVertex,
    OUT unsigned int& nIndex) {
    unsigned int nVertex_temp = 0;
    unsigned int nIndex_temp = 0;
    for (int i = 0; i < input_point_list.size(); i++) {
        nVertex_temp += input_point_list[i].size();
        nIndex_temp += input_point_list[i].size() - 1;
    }

    vertex = new float[nVertex_temp * 3];
    index = new unsigned int[nIndex_temp * 2];
    int vertex_pos = 0, line_pos = 0;
    for (int i = 0; i < input_point_list.size(); i++) {
        for (int j = 0; j < input_point_list[i].size(); j++) {
            int v = vertex_pos * 3;
            vertex_pos++;
            vertex[v++] = input_point_list[i][j].x;
            vertex[v++] = input_point_list[i][j].y;
            vertex[v++] = input_point_list[i][j].z;
            if (j != input_point_list[i].size() - 1) {
                index[line_pos++] = vertex_pos - 1;
                index[line_pos++] = vertex_pos;
            }
        }
    }
    nVertex = nVertex_temp * 3;
    nIndex = nIndex_temp * 2;
}

void rollerCoaster::flatten_vector(IN const std::vector<std::vector<Point>> vec_in,
    OUT std::vector<Point>& vec_out) {
    for (auto& vec : vec_in) {
        for (auto item : vec) {
            vec_out.push_back(item);
        }
    }
}

// generate the whole spline by splines
void rollerCoaster::createSplinesArray(
    OUT std::vector<std::vector<Point>>& position_list,
    OUT std::vector<std::vector<Point>>& tangent_list,
    OUT std::vector<std::vector<Point>>& normal_list,
    OUT std::vector<std::vector<Point>>& binormal_list) {
    for (int i = 0; i < numSplines; i++) {
        position_list.push_back(std::vector<Point>());
        tangent_list.push_back(std::vector<Point>());
        normal_list.push_back(std::vector<Point>());
        binormal_list.push_back(std::vector<Point>());
        for (int j = 0; j < splines[i].numControlPoints - 3; j++) {
            auto points = splines[i].points;
            std::vector<Point> temp_position_list;
            std::vector<Point> temp_tangent_list;
            if (spline_create_type == splineCreateType::brute_force) {
                splineInterpolationBF(points[j], points[j + 1], points[j + 2],
                    points[j + 3], temp_position_list, temp_tangent_list);
            }
            else if (spline_create_type == splineCreateType::recursive_subdivision) {
                splineInterpolationRS(points[j], points[j + 1], points[j + 2],
                    points[j + 3], temp_position_list, temp_tangent_list);
            }
            if (j == 0) {
                position_list.back().push_back(temp_position_list[0]);
                tangent_list.back().push_back(temp_tangent_list[0]);
            }
            for (int k = 1; k < temp_position_list.size(); k++) {
                position_list.back().push_back(temp_position_list[k]);
                tangent_list.back().push_back(temp_tangent_list[k]);
            }
        }

        // calculate normal by Sloan's method
        //srand(time(NULL));
        //glm::vec3 binormal((float)rand() / (RAND_MAX),
        //    (float)rand() / (RAND_MAX), (float)rand() / (RAND_MAX));
        //for (int j = 0; j < tangent_list.back().size(); j++) {
        //    glm::vec3 tengent(tangent_list.back()[j].x, tangent_list.back()[j].y,
        //        tangent_list.back()[j].z);
        //    glm::vec3 normal;
        //    normal = glm::normalize(glm::cross(binormal, tengent));
        //    binormal = glm::normalize(glm::cross(tengent, normal));
        //    normal_list.back().push_back({ normal[0], normal[1], normal[2] });
        //    binormal_list.back().push_back({ binormal[0], binormal[1], binormal[2] });
        //}

        for (int j = 0; j < tangent_list.back().size() - 1; j++) {
            // pick two contiguous tangent, the binormal must be on the direction
            // of the cross product of the two tangent
            glm::vec3 tangent(tangent_list.back()[j].x, tangent_list.back()[j].y,
                tangent_list.back()[j].z);
            glm::vec3 tangent2(tangent_list.back()[j + 1].x, tangent_list.back()[j + 1].y,
                tangent_list.back()[j + 1].z);
            glm::vec3 binormal = glm::normalize(glm::cross(tangent, tangent2));
            glm::vec3 normal = glm::normalize(glm::cross(tangent, binormal));
            // if the normal is on the opposite side of the last normal, then flip it
            // in another word, the normal can't be change abruptly
            if (!normal_list.back().empty()) {
                if (normal_list.back().back().x * normal.x +
                    normal_list.back().back().y * normal.y +
                    normal_list.back().back().z * normal.z < 0) {
                    normal = -normal;
                    binormal = -binormal;
                }
            }
            normal_list.back().push_back({ normal[0], normal[1], normal[2] });
            binormal_list.back().push_back({ binormal[0], binormal[1], binormal[2] });
            if (j == tangent_list.back().size() - 2) {
                normal = glm::normalize(glm::cross(tangent2, binormal));
                normal_list.back().push_back({ normal[0], normal[1], normal[2] });
                binormal_list.back().push_back({ binormal[0], binormal[1], binormal[2] });
            }
        }

    }
    spline_statistic(position_list);
}

// generate one part of the spline by brute force
void rollerCoaster::splineInterpolationBF(
    IN Point p0, IN Point p1, IN Point p2, IN Point p3,
    OUT std::vector<Point>& interpolation_list,
    OUT std::vector<Point>& interpolation_tangent) {

    glm::mat4 basis_mat;
    if (spline_type == splineType::Catmull_Rom) {
        float s = 0.5f;
        basis_mat = {
            glm::vec4(-s, 2 * s, -s, 0),
            glm::vec4(2 - s, s - 3, 0, 1),
            glm::vec4(s - 2, 3 - 2 * s, s, 0),
            glm::vec4(s, -s, 0, 0)
        };
    }
    else if (spline_type == splineType::B_Splines) {
        basis_mat = {
            glm::vec4(-1, 3, -3, 1),
            glm::vec4(3, -6, 0, 4),
            glm::vec4(-3, 3, 3, 1),
            glm::vec4(1, 0, 0, 0)
        };
        basis_mat = basis_mat * (1.0f / 6.0f);
    }

    glm::mat3x4 control_mat = {
        glm::vec4(p0.x, p1.x, p2.x, p3.x),
        glm::vec4(p0.y, p1.y, p2.y, p3.y),
        glm::vec4(p0.z, p1.z, p2.z, p3.z)
    };

    //glm::mat3x4 transform_mat = basis_B_mat * control_mat;

    glm::mat3x4 transform_mat = basis_mat * control_mat;

    for (int i = 0; i < interpolate_point + 1; i++) {
        float u = (float)i / interpolate_point;
        glm::vec4 u_vec = { u * u * u, u * u, u, 1 };
        glm::vec3 position = u_vec * transform_mat;
        glm::vec4 n_vec = { 3 * u * u, 2 * u, 1, 0 };
        glm::vec3 tangent = glm::normalize(n_vec * transform_mat);
        interpolation_list.push_back({ position[0], position[1], position[2] });
        interpolation_tangent.push_back({ tangent[0], tangent[1], tangent[2] });
    }
}

// generate one part of the spline by recursive subdivision
void rollerCoaster::splineInterpolationRS(
    IN Point p0, IN Point p1, IN Point p2, IN Point p3,
    OUT std::vector<Point>& interpolation_list,
    OUT std::vector<Point>& interpolation_tangent) {
    glm::mat4 basis_mat;
    if (spline_type == splineType::Catmull_Rom) {
        float s = 0.5f;
        basis_mat = {
            glm::vec4(-s, 2 * s, -s, 0),
            glm::vec4(2 - s, s - 3, 0, 1),
            glm::vec4(s - 2, 3 - 2 * s, s, 0),
            glm::vec4(s, -s, 0, 0)
        };
    }
    else if (spline_type == splineType::B_Splines) {
        basis_mat = {
            glm::vec4(-1, 3, -3, 1),
            glm::vec4(3, -6, 0, 4),
            glm::vec4(-3, 3, 3, 1),
            glm::vec4(1, 0, 0, 0)
        };
        basis_mat = basis_mat * (1.0f / 6.0f);
    }
    glm::mat3x4 control_mat = {
        glm::vec4(p0.x, p1.x, p2.x, p3.x),
        glm::vec4(p0.y, p1.y, p2.y, p3.y),
        glm::vec4(p0.z, p1.z, p2.z, p3.z)
    };
    //glm::mat3x4 transform_mat = basis_B_mat * control_mat;
    glm::mat3x4 transform_mat = basis_mat * control_mat;

    // before subdivision, push the first position into the list
    glm::vec4 start_vec = { 0.0f, 0.0f, 0.0f, 1.0f };
    glm::vec3 start_position = start_vec * transform_mat;
    glm::vec4 n_vec = { 0, 0, 1, 0 };
    glm::vec3 tangent = glm::normalize(n_vec * transform_mat);
    interpolation_list.push_back({ start_position[0], start_position[1], start_position[2] });
    interpolation_tangent.push_back({ tangent[0], tangent[1], tangent[2] });

    // interpolation with uniform length
    std::stack<std::pair<float, float>> subdivision_tree;
    // threshold
    float length_threshold_square = length_threshold * length_threshold;
    subdivision_tree.push({ 0.0f, 1.0f });
    while (!subdivision_tree.empty()) {
        auto item = subdivision_tree.top();
        subdivision_tree.pop();
        // test the distance between this two position
        float start = item.first;
        float end = item.second;
        // map from u space to the spline space
        glm::vec4 start_vec = { start * start * start, start * start, start, 1.0f };
        glm::vec3 start_position = start_vec * transform_mat;
        glm::vec4 end_vec = { end * end * end, end * end, end, 1.0f };
        glm::vec3 end_position = end_vec * transform_mat;
        glm::vec3 distance_vec = end_position - start_position;
        float distance_square = distance_vec[0] * distance_vec[0] +
            distance_vec[1] * distance_vec[1] + distance_vec[2] * distance_vec[2];
        // when distance below the threshold, then end the division
        if (distance_square < length_threshold_square) {
            glm::vec4 n_vec = { 3 * end * end, 2 * end, 1.0f, 0.0f };
            glm::vec3 tangent = glm::normalize(n_vec * transform_mat);
            interpolation_list.push_back({ end_position[0], end_position[1], end_position[2] });
            interpolation_tangent.push_back({ tangent[0], tangent[1], tangent[2] });
        }
        else {
            // subdivide the two position into two part
            float middle = (start + end) / 2;
            subdivision_tree.push({ middle, end });
            subdivision_tree.push({ start, middle });
        }
    }
}

void rollerCoaster::generate_T_track_new(
    IN const std::vector<std::vector<Point>>& position_list,
    IN const std::vector<std::vector<Point>>& tangent_list,
    IN const std::vector<std::vector<Point>>& normal_list,
    IN const std::vector<std::vector<Point>>& binormal_list,
    OUT std::vector<float>& track_vertex_stream,
    OUT std::vector<float>& track_normal_stream,
    OUT std::vector<float>& track_color_vertex_stream,
    OUT std::vector<unsigned int>& track_index_stream,
    OUT std::vector<float>& track_texture_coordinate) {
    generate_T_track_one_strip(position_list, tangent_list, normal_list, binormal_list,
        -0.05f, -0.1f, -0.05f, 0.1f,
        track_vertex_stream, track_normal_stream,
        track_color_vertex_stream, track_index_stream, track_texture_coordinate);
    generate_T_track_one_strip(position_list, tangent_list, normal_list, binormal_list,
        -0.1f, -0.1f, -0.1f, 0.1f,
        track_vertex_stream, track_normal_stream,
        track_color_vertex_stream, track_index_stream, track_texture_coordinate);
    generate_T_track_one_strip(position_list, tangent_list, normal_list, binormal_list,
        -0.05f, -0.1f, -0.1f, -0.1f,
        track_vertex_stream, track_normal_stream,
        track_color_vertex_stream, track_index_stream, track_texture_coordinate);
    generate_T_track_one_strip(position_list, tangent_list, normal_list, binormal_list,
        -0.05f, 0.1f, -0.1f, 0.1f,
        track_vertex_stream, track_normal_stream,
        track_color_vertex_stream, track_index_stream, track_texture_coordinate);
    // second triangle
    generate_T_track_one_strip(position_list, tangent_list, normal_list, binormal_list,
        -0.25f, -0.02f, -0.25f, 0.02f,
        track_vertex_stream, track_normal_stream,
        track_color_vertex_stream, track_index_stream, track_texture_coordinate);
    generate_T_track_one_strip(position_list, tangent_list, normal_list, binormal_list,
        -0.1f, -0.02f, -0.25f, -0.02f,
        track_vertex_stream, track_normal_stream,
        track_color_vertex_stream, track_index_stream, track_texture_coordinate);
    generate_T_track_one_strip(position_list, tangent_list, normal_list, binormal_list,
        -0.1f, 0.02f, -0.25f, 0.02f,
        track_vertex_stream, track_normal_stream,
        track_color_vertex_stream, track_index_stream, track_texture_coordinate);
}

void rollerCoaster::generate_T_track_one_strip(IN const std::vector<std::vector<Point>>& position_list,
    IN const std::vector<std::vector<Point>>& tangent_list,
    IN const std::vector<std::vector<Point>>& normal_list,
    IN const std::vector<std::vector<Point>>& binormal_list,
    IN const float strip1_alpha, IN const float strip1_beta,
    IN const float strip2_alpha, IN const float strip2_beta,
    OUT std::vector<float>& track_vertex_stream,
    OUT std::vector<float>& track_normal_stream,
    OUT std::vector<float>& track_color_vertex_stream,
    OUT std::vector<unsigned int>& track_index_stream,
    OUT std::vector<float>& track_texture_coordinate) {
    int index = track_vertex_stream.size() / 3;
    float distance = 0.0f;
    for (int i = 0; i < normal_list.size(); i++) {
        for (int j = 0; j < normal_list[i].size(); j++) {
            glm::vec3 position(position_list[i][j].x, position_list[i][j].y, position_list[i][j].z);
            glm::vec3 tangent(tangent_list[i][j].x, tangent_list[i][j].y, tangent_list[i][j].z);
            glm::vec3 normal(normal_list[i][j].x, normal_list[i][j].y, normal_list[i][j].z);
            glm::vec3 binormal(binormal_list[i][j].x, binormal_list[i][j].y, binormal_list[i][j].z);
            glm::vec3 vertex1 = position + normal * strip1_alpha + binormal * strip1_beta;
            glm::vec3 vertex2 = position + normal * strip2_alpha + binormal * strip2_beta;
            track_vertex_stream.push_back(vertex1.x);
            track_vertex_stream.push_back(vertex1.y);
            track_vertex_stream.push_back(vertex1.z);
            track_vertex_stream.push_back(vertex2.x);
            track_vertex_stream.push_back(vertex2.y);
            track_vertex_stream.push_back(vertex2.z);

            glm::vec3 plane_normal = glm::normalize(
                glm::cross(tangent, vertex2 - vertex1)
            );

            if (j != 0) {
                glm::vec3 position2(position_list[i][j - 1].x, position_list[i][j - 1].y,
                    position_list[i][j - 1].z);
                glm::vec3 dist = position2 - position;
                distance += std::sqrt(dist[0] * dist[0] + dist[1] * dist[1] +
                    dist[2] * dist[2]) / 5;
            }
            track_texture_coordinate.push_back(strip1_alpha + strip1_beta);
            track_texture_coordinate.push_back(distance);
            track_texture_coordinate.push_back(strip2_alpha + strip2_beta);
            track_texture_coordinate.push_back(distance);

            track_normal_stream.push_back(plane_normal.x);
            track_normal_stream.push_back(plane_normal.y);
            track_normal_stream.push_back(plane_normal.z);
            track_normal_stream.push_back(plane_normal.x);
            track_normal_stream.push_back(plane_normal.y);
            track_normal_stream.push_back(plane_normal.z);

            glm::vec3 direction1 = glm::normalize(vertex1 - position) * 0.5f + 0.5f;
            glm::vec3 direction2 = glm::normalize(vertex2 - position) * 0.5f + 0.5f;
            track_color_vertex_stream.push_back(direction1.x);
            track_color_vertex_stream.push_back(direction1.y);
            track_color_vertex_stream.push_back(direction1.z);
            track_color_vertex_stream.push_back(1.0f);
            track_color_vertex_stream.push_back(direction2.x);
            track_color_vertex_stream.push_back(direction2.y);
            track_color_vertex_stream.push_back(direction2.z);
            track_color_vertex_stream.push_back(1.0f);
        }
        for (int j = 0; j < normal_list[i].size() - 1; j++) {
            // triangle 1 index
            track_index_stream.push_back(index);
            track_index_stream.push_back(index + 1);
            track_index_stream.push_back(index + 2);
            track_index_stream.push_back(index + 1);
            track_index_stream.push_back(index + 2);
            track_index_stream.push_back(index + 3);
            index += 2;
        }
        index += 2;
    }
}

void rollerCoaster::generate_ground_mesh(IN const float boarder,
    IN const float boarder_coordinate, OUT std::vector<float>& vertex,
    OUT std::vector<float>& texture_coordinate, OUT std::vector<unsigned int>& index) {
    float position[4][2] = {
        {boarder, boarder}, {-boarder, boarder}, {boarder, -boarder}, {-boarder, -boarder}
    };
    float coordinate[4][2] = {
        {boarder_coordinate, boarder_coordinate}, {-boarder_coordinate, boarder_coordinate},
        {boarder_coordinate, -boarder_coordinate}, {-boarder_coordinate, -boarder_coordinate}
    };
    for (int i = 0; i < 4; i++) {
        vertex.push_back(position[i][0]);
        vertex.push_back(position[i][1]);
        vertex.push_back(-1.0f);
        texture_coordinate.push_back(coordinate[i][0]);
        texture_coordinate.push_back(coordinate[i][1]);
    }
    index.push_back(0); index.push_back(1); index.push_back(2);
    index.push_back(1); index.push_back(2); index.push_back(3);
}

void rollerCoaster::generate_skybox_mesh(IN unsigned int* vertex_index,
    IN Point* sky_box_vertices,
    OUT std::vector<float>& vertex, OUT std::vector<float>& texture_coordinate,
    OUT std::vector<unsigned int>& index) {
    float coordinate[4][2] = {
        {1, 1}, {1, 0}, {0, 0}, {0, 1}
    };
    for (int i = 0; i < 4; i++) {
        vertex.push_back(sky_box_vertices[vertex_index[i]].x);
        vertex.push_back(sky_box_vertices[vertex_index[i]].y);
        vertex.push_back(sky_box_vertices[vertex_index[i]].z);
        texture_coordinate.push_back(coordinate[i][0]);
        texture_coordinate.push_back(coordinate[i][1]);
    }
    index.push_back(0); index.push_back(1); index.push_back(2);
    index.push_back(0); index.push_back(2); index.push_back(3);
}

void rollerCoaster::setTextureUnit(GLint unit) {
    // select texture unit affected by subsequent texture calls
    glActiveTexture(unit);
    // get a handle to the "textureImage" shader variable
    GLint h_textureImage = glGetUniformLocation(textureProgram->GetProgramHandle(),
        "textureImage");
    // deem the shader variable "textureImage" to read from texture unit "unit"
    glUniform1i(h_textureImage, unit - GL_TEXTURE0);
}

void rollerCoaster::spline_statistic(std::vector<std::vector<Point>>& position_list) {
    int nSegment = 0;
    float total_length = 0.0f;
    float maximum_length = 0.0f;
    float minimum_length = 1000.0f;
    for (auto spline : position_list) {
        nSegment += spline.size() - 1;
        for (int i = 1; i < spline.size(); i++) {
            Point vertex_1 = spline[i - 1];
            Point vertex_2 = spline[i];
            float length = (vertex_2.x - vertex_1.x) * (vertex_2.x - vertex_1.x) +
                (vertex_2.y - vertex_1.y) * (vertex_2.y - vertex_1.y) +
                (vertex_2.z - vertex_1.z) * (vertex_2.z - vertex_1.z);
            length = std::sqrt(length);
            maximum_length = std::max(maximum_length, length);
            minimum_length = std::min(minimum_length, length);
            total_length += length;
        }
    }
    float length_average = total_length / nSegment;
    float length_variance = 0.0f;
    for (auto spline : position_list) {
        nSegment += spline.size() - 1;
        for (int i = 1; i < spline.size(); i++) {
            Point vertex_1 = spline[i - 1];
            Point vertex_2 = spline[i];
            float length = (vertex_2.x - vertex_1.x) * (vertex_2.x - vertex_1.x) +
                (vertex_2.y - vertex_1.y) * (vertex_2.y - vertex_1.y) +
                (vertex_2.z - vertex_1.z) * (vertex_2.z - vertex_1.z);
            length = std::sqrt(length);
            length_variance += (length - length_average) * (length - length_average);
        }
    }
    length_variance = length_variance / nSegment;
    float length_std = std::sqrt(length_variance);

    std::cout << "In spline, total " << nSegment << " segment. length: "
        << total_length << ". average length: " << length_average
        << ". length variance: " << length_variance << ". length std: "
        << length_std << ". maximum length: " << maximum_length
        << ". minimum length: " << minimum_length << std::endl;

}
