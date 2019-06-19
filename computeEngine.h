//
// Created by yang on 3/8/19.
//
// This class should wrap the GPU computing component.
// Currently, it should wrap the OpenCL 1.2 to do the job

#ifndef FUSION_COMPUTEENGINE_H
#define FUSION_COMPUTEENGINE_H

#include "GPUBuffer.h"
#include "utilities.h"


#define CL_CHECK_ERROR(cmd) try {cmd} catch (cl::Error er) {printf("ERROR: %s(%d) in Ln.%d of file: %s\n", er.what(), er.err(),__LINE__, __FILE__);}


enum KernelID { CL_DEPTH_PROCESSING=0, CL_TEST,CL_FUSE, CL_FILTER_STABLE, CL_ICP_INNER, CL_PROCRUSTES_FIT };


class computeEngine {
public:
    using KernelLambdaFn = std::function<void()>;

    static computeEngine &engine() {
        static computeEngine instance;

        return instance;
    };

    static cl::Context& Context()
    {
        return engine().context;
    }

    static cl::CommandQueue& Queue()
    {
        return engine().queue;
    }


    ~computeEngine(){};



    //setup the data for the kernel
    //void loadData(std::vector<Vec4> pos, std::vector<Vec4> vel, std::vector<Vec4> color);
    //these are implemented in part1.cpp (in the future we will make these more general)
    void popCorn();

    //execute the kernel
    void runKernel(KernelID id, std::vector<GPUBuffer>& args,cl::NDRange num,
            KernelLambdaFn&& prelambda = [](){},
                   cl::NDRange local = cl::NullRange);

    void runKernel(KernelID id, std::vector<GPUBuffer*>& args, cl::NDRange num,
                   KernelLambdaFn&& prelambda = [](){},
                   KernelLambdaFn&& postlambda = [](){});

    //execute the kernel
    void runKernel(KernelID id, std::vector<GPUBuffer>& args,int numData,
                   KernelLambdaFn&& prelambda = [](){},
                   KernelLambdaFn&& postlambda = [](){})
    {
        currentID = id;
        prelambda();
        runKernel(id, args, cl::NDRange(numData));
        postlambda();
    }

    static cl::Kernel& getKernel()
    {
        return engine().kernels[engine().currentID];
    }

    static void readDataFromGPU(cl::Buffer* cl_buf, void* dst, int size){
        engine().err = engine().queue.enqueueReadBuffer(*cl_buf, CL_TRUE, 0, size, dst, NULL, &(engine().event));
        //printf("clEnqueueReadBuffer: %s\n", oclErrorString(err));
        //clReleaseEvent(event);
    }

    static void writeDataToGPU(cl::Buffer* cl_buf, void* src, int size){
        engine().err = engine().queue.enqueueWriteBuffer(*cl_buf, CL_TRUE, 0, size, src, NULL, &(engine().event));
        //printf("clEnqueueReadBuffer: %s\n", oclErrorString(err));
        //clReleaseEvent(event);
    }


//    real coders bare all
//    private:

private:
    computeEngine() {
        initOpenCL();
        std::string path(CL_SOURCE_DIR);
        path += "/kernelsOpenCL.cl";
        loadProgram(path);

        // Order matters!!!

        CL_CHECK_ERROR(kernels.push_back(cl::Kernel());)
        CL_CHECK_ERROR(kernels[CL_DEPTH_PROCESSING] = cl::Kernel(program, "depthProcessing", &err);)
        CheckError();
        CL_CHECK_ERROR(kernels.push_back(cl::Kernel());)
        CL_CHECK_ERROR(kernels[CL_TEST] = cl::Kernel(program, "test", &err);)
        CheckError ();
        CL_CHECK_ERROR(kernels.push_back(cl::Kernel());)
        CL_CHECK_ERROR(kernels[CL_FUSE] = cl::Kernel(program, "fuse", &err);)
        CheckError ();
        CL_CHECK_ERROR(kernels.push_back(cl::Kernel());)
        CL_CHECK_ERROR(kernels[CL_FILTER_STABLE] = cl::Kernel(program, "filterStable", &err);)
        CheckError ();
        CL_CHECK_ERROR(kernels.push_back(cl::Kernel());)
        CL_CHECK_ERROR(kernels[CL_ICP_INNER] = cl::Kernel(program, "icpInner", &err);)
        CheckError ();
        CL_CHECK_ERROR(kernels.push_back(cl::Kernel());)
        CL_CHECK_ERROR(kernels[CL_PROCRUSTES_FIT] = cl::Kernel(program, "procrustes", &err);)
        CheckError ();

    }

    void CheckError ()
    {
        if (err != CL_SUCCESS) {
            std::cout << "OpenCL call failed with error " << err << std::endl;
        }
    }

    //load an OpenCL program from a string
    void loadProgram(std::string kernel_source);

    void initOpenCL();

    const char *oclErrorString(cl_int error);

    computeEngine(computeEngine const &);

    void operator=(computeEngine const &);

    KernelID currentID ;

    unsigned int deviceUsed;
    std::vector<cl::Device> devices;

    cl::Context context;
    cl::CommandQueue queue;
    cl::Program program;
    std::vector<cl::Kernel> kernels;

//debugging variables
    cl_int err;
///cl_event event;
    cl::Event event;
};


#endif //FUSION_COMPUTEENGINE_H
