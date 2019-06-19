//
// Created by yang on 3/8/19.
//

#include "computeEngine.h"


const char* computeEngine::oclErrorString(cl_int error)
{
    static const char* errorString[] = {
            "CL_SUCCESS",
            "CL_DEVICE_NOT_FOUND",
            "CL_DEVICE_NOT_AVAILABLE",
            "CL_COMPILER_NOT_AVAILABLE",
            "CL_MEM_OBJECT_ALLOCATION_FAILURE",
            "CL_OUT_OF_RESOURCES",
            "CL_OUT_OF_HOST_MEMORY",
            "CL_PROFILING_INFO_NOT_AVAILABLE",
            "CL_MEM_COPY_OVERLAP",
            "CL_IMAGE_FORMAT_MISMATCH",
            "CL_IMAGE_FORMAT_NOT_SUPPORTED",
            "CL_BUILD_PROGRAM_FAILURE",
            "CL_MAP_FAILURE",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "CL_INVALID_VALUE",
            "CL_INVALID_DEVICE_TYPE",
            "CL_INVALID_PLATFORM",
            "CL_INVALID_DEVICE",
            "CL_INVALID_CONTEXT",
            "CL_INVALID_QUEUE_PROPERTIES",
            "CL_INVALID_COMMAND_QUEUE",
            "CL_INVALID_HOST_PTR",
            "CL_INVALID_MEM_OBJECT",
            "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
            "CL_INVALID_IMAGE_SIZE",
            "CL_INVALID_SAMPLER",
            "CL_INVALID_BINARY",
            "CL_INVALID_BUILD_OPTIONS",
            "CL_INVALID_PROGRAM",
            "CL_INVALID_PROGRAM_EXECUTABLE",
            "CL_INVALID_KERNEL_NAME",
            "CL_INVALID_KERNEL_DEFINITION",
            "CL_INVALID_KERNEL",
            "CL_INVALID_ARG_INDEX",
            "CL_INVALID_ARG_VALUE",
            "CL_INVALID_ARG_SIZE",
            "CL_INVALID_KERNEL_ARGS",
            "CL_INVALID_WORK_DIMENSION",
            "CL_INVALID_WORK_GROUP_SIZE",
            "CL_INVALID_WORK_ITEM_SIZE",
            "CL_INVALID_GLOBAL_OFFSET",
            "CL_INVALID_EVENT_WAIT_LIST",
            "CL_INVALID_EVENT",
            "CL_INVALID_OPERATION",
            "CL_INVALID_GL_OBJECT",
            "CL_INVALID_BUFFER_SIZE",
            "CL_INVALID_MIP_LEVEL",
            "CL_INVALID_GLOBAL_WORK_SIZE",
    };

    const int errorCount = sizeof(errorString) / sizeof(errorString[0]);

    const int index = -error;

    return (index >= 0 && index < errorCount) ? errorString[index] : "";

}
char *file_contents(const char *filename, int *length)
{
    FILE *f = fopen(filename, "r");
    void *buffer;

    if (!f) {
        fprintf(stderr, "Unable to open %s for reading\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    *length = ftell(f);
    fseek(f, 0, SEEK_SET);

    buffer = malloc(*length+1);
    *length = fread(buffer, 1, *length, f);
    fclose(f);
    ((char*)buffer)[*length] = '\0';

    return (char*)buffer;
}


void computeEngine::initOpenCL(){
        printf("Initialize OpenCL object and context\n");
        //setup devices and context
        std::vector<cl::Platform> platforms;
        err = cl::Platform::get(&platforms);
        printf("cl::Platform::get(): %s\n", oclErrorString(err));
        printf("platforms.size(): %lu\n", platforms.size());

        deviceUsed = 0;
        err = platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
        printf("getDevices: %s\n", oclErrorString(err));
        printf("devices.size(): %lu\n", devices.size());
        int t = devices.front().getInfo<CL_DEVICE_TYPE>();
        printf("type: device: %d CL_DEVICE_TYPE_GPU: %d \n", t, CL_DEVICE_TYPE_GPU);

        std::vector<size_t> itemSize = devices.front().getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
        size_t maxSize = devices.front().getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
        for (int i =0;i<itemSize.size();++i)
            printf("Dim(%d) size: %d ",i,itemSize[i]);
        printf("\nMax work group size: %d\n",maxSize);

        // Define OS-specific context properties and create the OpenCL context
        // We setup OpenGL context sharing slightly differently on each OS
        // this code comes mostly from NVIDIA's SDK examples
        // we could also check to see if the device supports GL sharing
        // but that is just searching through the properties
        // an example is avaible in the NVIDIA code
#if defined (__APPLE__) || defined(MACOSX)
        CGLContextObj kCGLContext = CGLGetCurrentContext();
        CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
        cl_context_properties props[] =
        {
            CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup,
            0
        };

        //Apple's implementation is weird, and the default values assumed by cl.hpp don't work
        //this works
        //cl_context cxGPUContext = clCreateContext(props, 0, 0, NULL, NULL, &err);
        //these dont
        //cl_context cxGPUContext = clCreateContext(props, 1,(cl_device_id*)&devices.front(), NULL, NULL, &err);
        //cl_context cxGPUContext = clCreateContextFromType(props, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);
        //printf("error? %s\n", oclErrorString(err));
        try{
            context = cl::Context(CL_DEVICE_TYPE_GPU, props);   //had to edit line 1448 of cl.hpp to add this constructor
        }
        catch (cl::Error er) {
            printf("ERROR: %s(%s)\n", er.what(), oclErrorString(er.err()));
        }
#else
#if defined WIN32 // Win32
cl_context_properties props[] =
            {
                CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
                CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
                CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(),
                0
            };
            //cl_context cxGPUContext = clCreateContext(props, 1, &cdDevices[uiDeviceUsed], NULL, NULL, &err);
            try{
                context = cl::Context(CL_DEVICE_TYPE_GPU, props);
            }
            catch (cl::Error er) {
                printf("ERROR: %s(%s)\n", er.what(), oclErrorString(er.err()));
            }
#else
        cl_context_properties props[] =
        {
            CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
                    CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
                    CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(),
                    0
        };
        //cl_context cxGPUContext = clCreateContext(props, 1, &cdDevices[uiDeviceUsed], NULL, NULL, &err);
        try{
            context = cl::Context(CL_DEVICE_TYPE_GPU, props);
        }
        catch (cl::Error er) {
            printf("ERROR: %s(%s)\n", er.what(), oclErrorString(er.err()));
        }
#endif
#endif

        //create the command queue we will use to execute OpenCL commands
        try{
            queue = cl::CommandQueue(context, devices[deviceUsed], 0, &err);
        }
        catch (cl::Error er) {
            printf("ERROR: %s(%d)\n", er.what(), er.err());
        }

}


void computeEngine::loadProgram(std::string path)
{

    //Program Setup
    printf("load the program\n");


    int pl;
    char *kernel_source;
    kernel_source = file_contents(path.c_str(), &pl);

    //pl = kernel_source.size();
    printf("kernel size: %d\n", pl);
    //printf("kernel: \n %s\n", kernel_source.c_str());
    CL_CHECK_ERROR(
        cl::Program::Sources source(1,
                                    std::make_pair(kernel_source, pl));
        program = cl::Program(context, source);
    )

    printf("build program\n");
    CL_CHECK_ERROR(
        std::string includes(CL_SOURCE_DIR);
        includes = "-I" + includes;
        err = program.build(devices, includes.c_str());
    )
    printf("done building program\n");
    std::cout << "Build Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]) << std::endl;
    std::cout << "Build Options:\t" << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]) << std::endl;
    std::cout << "Build Log:\t " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;




}


void computeEngine::runKernel(KernelID id, std::vector<GPUBuffer>& args, cl::NDRange num,
                              KernelLambdaFn&& prelambda,
                              cl::NDRange local){

    currentID = id;

    // should write the data to GPU in lambda if needed
    prelambda();

    CheckError();

    std::vector<cl::Memory> glObjs;
    // link the arguments
    for (int i = 0; i < args.size(); ++i)
    {
        CL_CHECK_ERROR(err = kernels[id].setArg(i, *(args[i].getMemoryCL()) );)
        if (args[i].isGL())
            glObjs.push_back(*(args[i].getMemoryCL()));
    }
    //Wait for the command queue to finish these commands before proceeding

    CheckError();

    if (glObjs.size()>0)
    {
        glFinish(); // wait OpenGL finish all work
        CL_CHECK_ERROR(err = queue.enqueueAcquireGLObjects(&glObjs, NULL, &event);)

    }
    CheckError();
    CL_CHECK_ERROR(queue.finish();)// wait CL finish all work
    CheckError();


    // run the kernel on GPU here.
    CL_CHECK_ERROR(err = queue.enqueueNDRangeKernel(kernels[id],
                                         cl::NullRange,
                                         num,
                                         local,
                                         NULL, &event);
    )
    CheckError();
    if (glObjs.size()>0)
        err = queue.enqueueReleaseGLObjects(&glObjs, NULL, &event);

    CheckError();
    CL_CHECK_ERROR(queue.finish();)// wait CL finish all work

    CheckError();

}


void computeEngine::runKernel(KernelID id, std::vector<GPUBuffer*>& args, cl::NDRange num,
                              KernelLambdaFn&& prelambda,
                              KernelLambdaFn&& postlambd){

    currentID = id;

    // should write the data to GPU in lambda if needed
    prelambda();

    CheckError();

    std::vector<cl::Memory> glObjs;
    // link the arguments
    for (int i = 0; i < args.size(); ++i)
    {
        CL_CHECK_ERROR(err = kernels[id].setArg(i, *(args[i]->getMemoryCL()) );)
        if (args[i]->isGL())
            glObjs.push_back(*(args[i]->getMemoryCL()));
    }
    //Wait for the command queue to finish these commands before proceeding

    CheckError();

    if (glObjs.size()>0)
    {
        glFinish(); // wait OpenGL finish all work
        CL_CHECK_ERROR(err = queue.enqueueAcquireGLObjects(&glObjs, NULL, &event);)

    }
    CheckError();
    CL_CHECK_ERROR(queue.flush();)// wait CL finish all work flush or finish
    CheckError();


    // run the kernel on GPU here.
    CL_CHECK_ERROR(err = queue.enqueueNDRangeKernel(kernels[id],
                                                    cl::NullRange,
                                                    num,
                                                    cl::NullRange,
                                                    NULL, &event);
    )
    CheckError();
    if (glObjs.size()>0)
        err = queue.enqueueReleaseGLObjects(&glObjs, NULL, &event);

    CheckError();
    CL_CHECK_ERROR(queue.flush();)// wait CL finish all work flush or finish?

    CheckError();

    // should download the data from GPU in lambda if needed
    postlambd();

}