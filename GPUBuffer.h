//
// Created by yang on 3/11/19.
//

#ifndef FUSION_GPUBUFFER_H
#define FUSION_GPUBUFFER_H

#include "libs.h"


class GPUBuffer {
public:
    GPUBuffer(): data_cpu(NULL), cl_buffer(NULL), gl_buffer(NULL), size(0),inUsedSize(0),perPointSize(4* sizeof(float)){}

    GPUBuffer( pangolin::GlTexture&& gltex);

    GPUBuffer( pangolin::GlTexture& gltex):GPUBuffer(std::move(gltex)){}

    GPUBuffer( pangolin::TypedImage& img):GPUBuffer(pangolin::GlTexture(img,false)){}

    GPUBuffer( int w, int h,int perElement=4);

    GPUBuffer(const glm::mat4& mat );

    GPUBuffer(const int& value );

    GPUBuffer(size_t sizeInByte, void* data);

    GPUBuffer( size_t sizeInByte, pangolin::GlBufferType buffer_type = pangolin::GlArrayBuffer);

    GPUBuffer(GPUBuffer&& buffer)
    {
        *this = std::move(buffer);
    }

    GPUBuffer& operator=(GPUBuffer&& buffer)
    {
        if (&buffer != this) {
            cl_buffer = buffer.cl_buffer;
            gl_buffer = buffer.gl_buffer;
            data_cpu = buffer.data_cpu;
            gl_tex = std::move(buffer.gl_tex);
            size = buffer.size;
            inUsedSize = buffer.inUsedSize;
            perPointSize = buffer.perPointSize;

            buffer.cl_buffer = 0;
            buffer.data_cpu = 0;
            buffer.size = 0;
            buffer.gl_buffer=0;
        }
        return *this;
    }


     void free(){
         if (data_cpu)
             delete[] data_cpu;

         if (cl_buffer)
             delete cl_buffer;

         if (gl_buffer)
             delete gl_buffer;

         if (gl_tex.IsValid())
             gl_tex.Delete();
    }

    ~GPUBuffer()
    {
        free();
    }

    bool isGL(){
        return gl_tex.IsValid() || gl_buffer!=0;
    }


    void* begin(){return data_cpu;}

    void download();

    void upload();

    int sizeInPoints(){
        return inUsedSize / perPointSize;
    }

    void bind()
    {
        if (gl_buffer!=0)
            gl_buffer->Bind();
    }

    void unbind(){
        if (gl_buffer!=0)
            gl_buffer->Unbind();
    }

    cl::Memory* getMemoryCL() const {return cl_buffer;}


    bool copyGPU(GPUBuffer& buffer);
    pangolin::GlTexture gl_tex;
    size_t inUsedSize;
    size_t perPointSize;
private:
    char* data_cpu;
    cl::Memory* cl_buffer;


    pangolin::GlBufferData* gl_buffer;
    size_t size;






};


#endif //FUSION_GPUBUFFER_H
