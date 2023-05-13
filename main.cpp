#include "rollerCoaster.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("usage: %s <trackfile>\n", argv[0]);
        exit(0);
    }
    // load the splines from the provided filename
    rollerCoaster::loadSplines(argv[1]);
    // use this paramter to save
    // rollerCoaster::render("frame_save_folder", 15);
    rollerCoaster::render(nullptr, 60);
    // enable depth test
    glEnable(GL_DEPTH_TEST);
    return 0;
}

