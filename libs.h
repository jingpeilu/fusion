//
// Created by yang on 3/18/19.
//

#ifndef FUSION_LIBS_H
#define FUSION_LIBS_H

#include <ctime>
#include <vector>
#include <array>
#include <functional>
#include <glm/glm.hpp>
#include <iostream>
#include <iomanip>

#include <Eigen/Dense>


#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/ext.hpp>

#include <pangolin/pangolin.h>
#include <pangolin/image/pixel_format.h>


#if defined __APPLE__ || defined(MACOSX)
#include <OpenCL/opencl.h>
#include <OpenGL/OpenGL.h>
#include <sys/time.h>
#else
#if defined WIN32
#else
//needed for context sharing functions
#include <GL/glx.h>
#include <sys/time.h>
#endif
#endif

#include <CL/cl.hpp>

#endif //FUSION_LIBS_H
