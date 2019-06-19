//
// Created by yang on 3/11/19.
//

#include "GPUBuffer.h"
#include "computeEngine.h"

int getPerByte(GLint internal_format)
{
    switch (internal_format)
    {
        case GL_RGBA32F:
            return sizeof(float)*4;
        default:
            printf("ERROR! GPUBuffer.cpp, ln.15\n");
            return 0;
    }


}


GPUBuffer::GPUBuffer( pangolin::GlTexture&& gltex):GPUBuffer(){

    cl_int err = CL_SUCCESS;
    gl_tex = std::move(gltex);

    CL_CHECK_ERROR(cl_buffer = new cl::ImageGL(computeEngine::Context(),
                                    CL_MEM_READ_ONLY,
                                    GL_TEXTURE_2D,
                                    0,
                                    gl_tex.tid,
                                    &err);)
    if (err != CL_SUCCESS)
        std::cout<<"ERROR!!!"<<std::endl;

    cl_mem_object_type info;
    CL_CHECK_ERROR(cl_buffer->getInfo(CL_MEM_TYPE,&info);)//CL_IMAGE_FORMAT
    if (err != CL_SUCCESS)
        std::cout<<"ERROR!!!"<<std::endl;

    size = gl_tex.height * gl_tex.width * getPerByte(gl_tex.internal_format);

}



GPUBuffer::GPUBuffer( int w, int h, int perElement):GPUBuffer(){

    perPointSize = perElement *sizeof(float);
    size = w*h* perPointSize;



    cl_int err = CL_SUCCESS;

    cl_buffer = new cl::Buffer(computeEngine::Context(),
                               CL_MEM_WRITE_ONLY,
                               size,
                               NULL,
                               &err);

    if (err != CL_SUCCESS)
        std::cout<<"ERROR!!!"<<std::endl;
}

GPUBuffer::GPUBuffer(const glm::mat4& mat ):GPUBuffer(){
    size = 4*4* sizeof(float);
    cl_int err = CL_SUCCESS;

    cl_buffer = new cl::Buffer(computeEngine::Context(),
                               CL_MEM_READ_ONLY,
                               size,
                               NULL,
                               &err);

    computeEngine::writeDataToGPU((cl::Buffer *)cl_buffer,(float*)glm::value_ptr(mat),size);

    if (err != CL_SUCCESS)
        std::cout<<"ERROR!!!"<<std::endl;

}


GPUBuffer::GPUBuffer( size_t sizeInByte,pangolin::GlBufferType buffer_type):GPUBuffer(){
    size = sizeInByte;
    inUsedSize = size;

    cl_int err = CL_SUCCESS;

    gl_buffer = new pangolin::GlBufferData(buffer_type, size);

    cl_buffer = new cl::BufferGL(computeEngine::Context(),
                                 CL_MEM_READ_WRITE,
                               gl_buffer->bo,
                               &err);

    if (err != CL_SUCCESS)
        std::cout<<"ERROR!!!"<<std::endl;

}


void GPUBuffer::download(){

    if (data_cpu)
        delete[] data_cpu;

    data_cpu = new char[size];
    cl::Event event;
    std::vector<cl::Memory> glObjs;
    if (isGL())
    {

    // link the arguments
        glObjs.push_back(*(getMemoryCL()));
        glFinish(); // wait OpenGL finish all work

        CL_CHECK_ERROR(computeEngine::Queue().enqueueAcquireGLObjects(&glObjs, NULL, &event);)
    }

    CL_CHECK_ERROR(computeEngine::readDataFromGPU((cl::Buffer *) cl_buffer, data_cpu, size);)

    if (isGL()){
        CL_CHECK_ERROR(computeEngine::Queue().enqueueReleaseGLObjects(&glObjs, NULL, &event);;)

    }

}

bool GPUBuffer::copyGPU(GPUBuffer &buffer) {
    if (buffer.size < size)
    {
        cl::Event event;
        std::vector<cl::Memory> glObjs;
        bool a = isGL();
        bool b = buffer.isGL();
        if (a)
            // link the arguments
            glObjs.push_back(*(getMemoryCL()));


        if (b)
            // link the arguments
            glObjs.push_back(*(buffer.getMemoryCL()));

        if (a||b){
            glFinish(); // wait OpenGL finish all work
            CL_CHECK_ERROR(computeEngine::Queue().enqueueAcquireGLObjects(&glObjs, NULL, &event);)
        }

        CL_CHECK_ERROR(computeEngine::Queue().enqueueCopyBuffer(*((cl::Buffer*)buffer.cl_buffer),*((cl::Buffer*)cl_buffer),0,0,buffer.size);)
        inUsedSize = buffer.size;
        if (a||b){
            CL_CHECK_ERROR(computeEngine::Queue().enqueueReleaseGLObjects(&glObjs, NULL, &event);;)
        }
        return true;
    }

    return false;

}

GPUBuffer::GPUBuffer(const int &value):GPUBuffer() {
    size = sizeof(int);
    cl_int err = CL_SUCCESS;

    cl_buffer = new cl::Buffer(computeEngine::Context(),
                               CL_MEM_READ_WRITE,
                               size,
                               NULL,
                               &err);

    computeEngine::writeDataToGPU((cl::Buffer *)cl_buffer,(void*)&value,size);

    if (err != CL_SUCCESS)
        std::cout<<"ERROR!!!"<<std::endl;
}


GPUBuffer::GPUBuffer(size_t sizeInByte, void* data):GPUBuffer() {
    size = sizeInByte;
    cl_int err = CL_SUCCESS;

    cl_buffer = new cl::Buffer(computeEngine::Context(),
                               CL_MEM_READ_WRITE,
                               size,
                               NULL,
                               &err);

    computeEngine::writeDataToGPU((cl::Buffer *)cl_buffer,(void*)data,size);

    if (err != CL_SUCCESS)
        std::cout<<"ERROR!!!"<<std::endl;
}