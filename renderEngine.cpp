//
// Created by LiYang on 2018-12-04.
//
#include <glm/glm.hpp>
#include "renderEngine.h"
#include <glm/gtc/type_ptr.hpp>


GLint renderEngine::getInternalFormat(pangolin::PixelFormat& fmt)
{
    if (fmt.channels == 1)
    {
        if (fmt.channel_bit_depth ==16)
        {
            return GL_R16;
        }
    }
    else if (fmt.channels == 4)
    {
        if (fmt.channel_bit_depth ==32)
        {
            return GL_RGBA32F;
        }
    } else{
        return GL_RGBA8UI;
    }
}
GLint renderEngine::getGLFormat(pangolin::PixelFormat& fmt){
    if (fmt.channels == 1)
    {
        if (fmt.channel_bit_depth ==16)
        {
            return GL_RED;
        }
    }
    else if (fmt.channels == 4)
    {

            return GL_RGBA;

    }
}
GLint renderEngine::getGLType(pangolin::PixelFormat& fmt){
    if (fmt.channels == 1)
    {
        if (fmt.channel_bit_depth ==16)
        {
            return GL_UNSIGNED_SHORT;
        }
    }
    else if (fmt.channels == 4)
    {
        if (fmt.channel_bit_depth ==32)
        {
            return GL_FLOAT;
        } else if (fmt.channel_bit_depth == 8)
        {
            return GL_UNSIGNED_BYTE;
        }
    }
}



void renderEngine::renderPoints(cPoints& data, int num, GPUBuffer& indexBuffer, ShaderInputFn&& fn){

    ShaderID id = DISPLAY; // thread not safe here
    glEnable(GL_PROGRAM_POINT_SIZE);

    glEnable(GL_POINT_SPRITE);
    //glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    //renderPoints(DISPLAY,  data, NULL, 0, [fn](){fn();});
    currentID = DISPLAY; // thread not safe here


    getShader().SaveBind();

    fn();

    if (9 * num * sizeof(float) >= 60000000 * 3 * pangolin::GlDataTypeBytes(GL_FLOAT)) std::cout<<"ERROR!!!! GPU MEMORY out!"<<std::endl;

    pangolin::GlBuffer glxyz(pangolin::GlArrayBuffer,3*num, GL_FLOAT, 3, GL_DYNAMIC_DRAW);

    glxyz.Bind();
    //// Upload host data to OpenGL Buffer
    glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * num * sizeof(float), (const unsigned char*)&(data.mPos[0].x) );
    glBufferSubData(GL_ARRAY_BUFFER, 3 * num * sizeof(float), 3 * num * sizeof(float), (const unsigned char*)&(data.mNorm[0].x));
    glBufferSubData(GL_ARRAY_BUFFER, 3 * num * sizeof(float) + 3 * num * sizeof(float), 3 * num * sizeof(float), (const unsigned char*)&(data.mAttri[0].x));
    CheckGlDieOnError()

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    int attributesID = getShader(id).GetAttributeHandle("p_attri");
    glEnableVertexAttribArray(attributesID);

    glVertexPointer(glxyz.count_per_element, glxyz.datatype, 0, 0);
    glNormalPointer(glxyz.datatype, 0 , (void*)(3 * num * sizeof(float)));
    glVertexAttribPointer(attributesID, 3, GL_FLOAT, GL_FALSE, 0, (void*)(6 * num * sizeof(float)));
    //glTexCoordPointer(gltex.count_per_element, gltex.datatype, 0, 0);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    CheckGlDieOnError()


    indexBuffer.bind();// please make sure the index buffer is on ARRAY_INDEX binding

    glDrawElements(
            GL_POINTS,      // mode
            indexBuffer.sizeInPoints(),    // count
            GL_UNSIGNED_INT,   // type
            (void*)0           // element array buffer offset
    );

    //glDrawArrays(GL_POINTS, 0, num);
    indexBuffer.unbind();

    CheckGlDieOnError()

    //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableVertexAttribArray(attributesID);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()

    glxyz.Unbind();
    CheckGlDieOnError()

    getShader(id).Unbind();

    glFlush();

    glDisable (GL_BLEND);

    glDisable(GL_PROGRAM_POINT_SIZE);

//    glBindTexture(GL_TEXTURE_2D, objectID);
    glDisable(GL_POINT_SPRITE);
    //glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

    //glFlush();
}


void renderEngine::renderPoints(cPoints& data, int num, ShaderInputFn&& fn){

    ShaderID id = DISPLAY; // thread not safe here
    glEnable(GL_PROGRAM_POINT_SIZE);

    glEnable(GL_POINT_SPRITE);
    //glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    //renderPoints(DISPLAY,  data, NULL, 0, [fn](){fn();});
    currentID = DISPLAY; // thread not safe here


    getShader().SaveBind();

    fn();

    if (9 * num * sizeof(float) >= 60000000 * 3 * pangolin::GlDataTypeBytes(GL_FLOAT)) std::cout<<"ERROR!!!! GPU MEMORY out!"<<std::endl;

    pangolin::GlBuffer glxyz(pangolin::GlArrayBuffer,3*num, GL_FLOAT, 3, GL_DYNAMIC_DRAW);

    glxyz.Bind();
    //// Upload host data to OpenGL Buffer
    glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * num * sizeof(float), (const unsigned char*)&(data.mPos[0].x) );
    glBufferSubData(GL_ARRAY_BUFFER, 3 * num * sizeof(float), 3 * num * sizeof(float), (const unsigned char*)&(data.mNorm[0].x));
    glBufferSubData(GL_ARRAY_BUFFER, 3 * num * sizeof(float) + 3 * num * sizeof(float), 3 * num * sizeof(float), (const unsigned char*)&(data.mAttri[0].x));
    CheckGlDieOnError()

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    int attributesID = getShader(id).GetAttributeHandle("p_attri");
    glEnableVertexAttribArray(attributesID);

    glVertexPointer(glxyz.count_per_element, glxyz.datatype, 0, 0);
    glNormalPointer(glxyz.datatype, 0 , (void*)(3 * num * sizeof(float)));
    glVertexAttribPointer(attributesID, 3, GL_FLOAT, GL_FALSE, 0, (void*)(6 * num * sizeof(float)));
    //glTexCoordPointer(gltex.count_per_element, gltex.datatype, 0, 0);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    CheckGlDieOnError()

    glDrawArrays(GL_POINTS, 0, num);

    CheckGlDieOnError()

    //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableVertexAttribArray(attributesID);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()

    glxyz.Unbind();
    CheckGlDieOnError()

    getShader(id).Unbind();

    glFlush();

    glDisable (GL_BLEND);

    glDisable(GL_PROGRAM_POINT_SIZE);

//    glBindTexture(GL_TEXTURE_2D, objectID);
    glDisable(GL_POINT_SPRITE);
    //glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

    //glFlush();
}

static int num_debug = 0;
static int num_debug2 = 0;

void renderEngine::renderPoints(ShaderID id, std::vector<GPUBuffer>& inputBuffers, pangolin::GlTexture* outputs, int numOutput, ShaderInputFn&& fn){


    currentID = id; // thread not safe here

    glEnable(GL_PROGRAM_POINT_SIZE);

    glEnable(GL_POINT_SPRITE);
//    //glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
//
//    glEnable (GL_BLEND);
//    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//

    getShader().SaveBind();

    fn();


    // pangolin::GlFramebuffer fbo;
    pangolin::GlRenderBuffer depthB(outputs[0].width,outputs[0].height);
    // Create FBO
    pangolin::GlFramebuffer fbo;
    glGenFramebuffersEXT(1, &(fbo.fbid));
    for (int i=0; i < numOutput; ++i) fbo.AttachColour(outputs[i]);// binding to the outputs
    fbo.AttachDepth(depthB);
    CheckGlDieOnError();


    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
        std::cout<<"FATAL ERROR"<<status<<std::endl;

    fbo.Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CheckGlDieOnError()
    // }

//    pangolin::GlBuffer glxyz(pangolin::GlArrayBuffer,3*num, GL_FLOAT, 3, GL_DYNAMIC_DRAW);
//
//    glxyz.Bind();
//    //// Upload host data to OpenGL Buffer
//    glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * num * sizeof(float), (const unsigned char*)&(data.mPos[0].x) );
//    glBufferSubData(GL_ARRAY_BUFFER, 3 * num * sizeof(float), 3 * num * sizeof(float), (const unsigned char*)&(data.mNorm[0].x));
//    glBufferSubData(GL_ARRAY_BUFFER, 3 * num * sizeof(float) + 3 * num * sizeof(float), 3 * num * sizeof(float), (const unsigned char*)&(data.mAttri[0].x));
//    CheckGlDieOnError()

    glEnableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()
    //glEnableClientState(GL_NORMAL_ARRAY);
    //CheckGlDieOnError()

    int attributesID = getShader(id).GetAttributeHandle("p_attri");
    glEnableVertexAttribArray(attributesID);
    CheckGlDieOnError()


    int normalID = getShader(id).GetAttributeHandle("p_normal");
    glEnableVertexAttribArray(normalID);
    CheckGlDieOnError()

    inputBuffers[0].bind();
    glVertexPointer(4, GL_FLOAT, 0, 0);
    CheckGlDieOnError()
    inputBuffers[0].unbind();

    inputBuffers[1].bind();
    glVertexAttribPointer(normalID, 4, GL_FLOAT, GL_FALSE, 0, 0);
    //glTexCoordPointer(gltex.count_per_element, gltex.datatype, 0, 0);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    CheckGlDieOnError()
    inputBuffers[1].unbind();

    inputBuffers[2].bind();
    glVertexAttribPointer(attributesID, 4, GL_FLOAT, GL_FALSE, 0, 0);
    //glTexCoordPointer(gltex.count_per_element, gltex.datatype, 0, 0);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    CheckGlDieOnError()
    inputBuffers[2].unbind();

    glDrawArrays(GL_POINTS, 0, inputBuffers[0].sizeInPoints());

    CheckGlDieOnError()

    //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableVertexAttribArray(attributesID);
    glDisableVertexAttribArray(normalID);
    glDisableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()

    CheckGlDieOnError()


    fbo.Unbind();



    getShader(id).Unbind();

    glFlush();

//    glDisable (GL_BLEND);
//
    glDisable(GL_PROGRAM_POINT_SIZE);
//
////    glBindTexture(GL_TEXTURE_2D, objectID);
    glDisable(GL_POINT_SPRITE);
}


void renderEngine::renderPoints(ShaderID id, cPoints& data, pangolin::GlTexture* outputs, int numOutput, ShaderInputFn&& fn){


    currentID = id; // thread not safe here

    glEnable(GL_PROGRAM_POINT_SIZE);

    glEnable(GL_POINT_SPRITE);
//    //glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
//
//    glEnable (GL_BLEND);
//    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//

    getShader().SaveBind();

    fn();


   // pangolin::GlFramebuffer fbo;
    pangolin::GlRenderBuffer depthB(outputs[0].width,outputs[0].height);
    // Create FBO
    pangolin::GlFramebuffer fbo;
    glGenFramebuffersEXT(1, &(fbo.fbid));
    for (int i=0; i < numOutput; ++i) fbo.AttachColour(outputs[i]);// binding to the outputs
    fbo.AttachDepth(depthB);
    CheckGlDieOnError();


    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
        std::cout<<"FATAL ERROR"<<status<<std::endl;

    fbo.Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CheckGlDieOnError()
   // }

    int num = data.mPos.size();

    if (9 * num * sizeof(float) >= 60000000 * 3 * pangolin::GlDataTypeBytes(GL_FLOAT)) std::cout<<"ERROR!!!! GPU MEMORY out!"<<std::endl;

    pangolin::GlBuffer glxyz(pangolin::GlArrayBuffer,3*num, GL_FLOAT, 3, GL_DYNAMIC_DRAW);

    glxyz.Bind();
    //// Upload host data to OpenGL Buffer
    glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * num * sizeof(float), (const unsigned char*)&(data.mPos[0].x) );
    glBufferSubData(GL_ARRAY_BUFFER, 3 * num * sizeof(float), 3 * num * sizeof(float), (const unsigned char*)&(data.mNorm[0].x));
    glBufferSubData(GL_ARRAY_BUFFER, 3 * num * sizeof(float) + 3 * num * sizeof(float), 3 * num * sizeof(float), (const unsigned char*)&(data.mAttri[0].x));
    CheckGlDieOnError()

    glEnableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()
    glEnableClientState(GL_NORMAL_ARRAY);
    CheckGlDieOnError()

    int attributesID = getShader(id).GetAttributeHandle("p_attri");
    glEnableVertexAttribArray(attributesID);
    CheckGlDieOnError()

    glVertexPointer(glxyz.count_per_element, glxyz.datatype, 0, 0);
    CheckGlDieOnError()
    glNormalPointer(glxyz.datatype, 0 , (void*)(3 * num * sizeof(float)));
    CheckGlDieOnError()
    glVertexAttribPointer(attributesID, 3, GL_FLOAT, GL_FALSE, 0, (void*)(6 * num * sizeof(float)));
    //glTexCoordPointer(gltex.count_per_element, gltex.datatype, 0, 0);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    CheckGlDieOnError()

    glDrawArrays(GL_POINTS, 0, num);

    CheckGlDieOnError()

    //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableVertexAttribArray(attributesID);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()

    glxyz.Unbind();
    CheckGlDieOnError()


    fbo.Unbind();



    getShader(id).Unbind();

    glFlush();

//    glDisable (GL_BLEND);
//
    glDisable(GL_PROGRAM_POINT_SIZE);
//
////    glBindTexture(GL_TEXTURE_2D, objectID);
    glDisable(GL_POINT_SPRITE);
}



void renderEngine::renderPoints(ShaderID id, gpuPoints& points, GPUBuffer& index, pangolin::GlTexture* outputs, int numOutput, ShaderInputFn&& fn){


    currentID = id; // thread not safe here

    glEnable(GL_PROGRAM_POINT_SIZE);

    glEnable(GL_POINT_SPRITE);

    getShader().SaveBind();

    fn();


    // pangolin::GlFramebuffer fbo;
    pangolin::GlRenderBuffer depthB(outputs[0].width,outputs[0].height);
    // Create FBO
    pangolin::GlFramebuffer fbo;
    glGenFramebuffersEXT(1, &(fbo.fbid));
    for (int i=0; i < numOutput; ++i) fbo.AttachColour(outputs[i]);// binding to the outputs
    fbo.AttachDepth(depthB);
    CheckGlDieOnError();


    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
        std::cout<<"FATAL ERROR"<<status<<std::endl;

    fbo.Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CheckGlDieOnError()
    // }



    glEnableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()

    int attributesID = getShader(id).GetAttributeHandle("p_attri");
    glEnableVertexAttribArray(attributesID);
    CheckGlDieOnError()


    int normalID = getShader(id).GetAttributeHandle("p_normal");
    glEnableVertexAttribArray(normalID);
    CheckGlDieOnError()

    points.mPos.bind();
    glVertexPointer(4, GL_FLOAT, 0, 0);
    CheckGlDieOnError()
    points.mPos.unbind();

    points.mNorm.bind();
    glVertexAttribPointer(normalID, 4, GL_FLOAT, GL_FALSE, 0, 0);
    //glTexCoordPointer(gltex.count_per_element, gltex.datatype, 0, 0);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    CheckGlDieOnError()
    points.mNorm.unbind();

    points.mAttri.bind();
    glVertexAttribPointer(attributesID, 4, GL_FLOAT, GL_FALSE, 0, 0);
    //glTexCoordPointer(gltex.count_per_element, gltex.datatype, 0, 0);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    CheckGlDieOnError()
    points.mAttri.unbind();

    index.bind();// please make sure the index buffer is on ARRAY_INDEX binding

    glDrawElements(
            GL_POINTS,      // mode
            index.sizeInPoints(),    // count
            GL_UNSIGNED_INT,   // type
            (void*)0           // element array buffer offset
    );

    //glDrawArrays(GL_POINTS, 0, num);
    index.unbind();

//    glDrawArrays(GL_POINTS, 0, index.sizeInPoints());

    CheckGlDieOnError()

    //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableVertexAttribArray(attributesID);
    glDisableVertexAttribArray(normalID);
    glDisableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()

    CheckGlDieOnError()


    fbo.Unbind();



    getShader(id).Unbind();

    glFlush();

//    glDisable (GL_BLEND);
//
    glDisable(GL_PROGRAM_POINT_SIZE);
//
////    glBindTexture(GL_TEXTURE_2D, objectID);
    glDisable(GL_POINT_SPRITE);
}



void renderEngine::renderPoints(ShaderID id, gpuPoints& points, pangolin::GlTexture* outputs, int numOutput, ShaderInputFn&& fn){


    currentID = id; // thread not safe here

    glEnable(GL_PROGRAM_POINT_SIZE);

    glEnable(GL_POINT_SPRITE);

    getShader().SaveBind();

    fn();


    // pangolin::GlFramebuffer fbo;
    pangolin::GlRenderBuffer depthB(outputs[0].width,outputs[0].height);
    // Create FBO
    pangolin::GlFramebuffer fbo;
    glGenFramebuffersEXT(1, &(fbo.fbid));
    for (int i=0; i < numOutput; ++i) fbo.AttachColour(outputs[i]);// binding to the outputs
    fbo.AttachDepth(depthB);
    CheckGlDieOnError();


    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
        std::cout<<"FATAL ERROR"<<status<<std::endl;

    fbo.Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CheckGlDieOnError()
    // }



    glEnableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()

    int attributesID = getShader(id).GetAttributeHandle("p_attri");
    glEnableVertexAttribArray(attributesID);
    CheckGlDieOnError()


    int normalID = getShader(id).GetAttributeHandle("p_normal");
    glEnableVertexAttribArray(normalID);
    CheckGlDieOnError()

    points.mPos.bind();
    glVertexPointer(4, GL_FLOAT, 0, 0);
    CheckGlDieOnError()
    points.mPos.unbind();

    points.mNorm.bind();
    glVertexAttribPointer(normalID, 4, GL_FLOAT, GL_FALSE, 0, 0);
    //glTexCoordPointer(gltex.count_per_element, gltex.datatype, 0, 0);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    CheckGlDieOnError()
    points.mNorm.unbind();

    points.mAttri.bind();
    glVertexAttribPointer(attributesID, 4, GL_FLOAT, GL_FALSE, 0, 0);
    //glTexCoordPointer(gltex.count_per_element, gltex.datatype, 0, 0);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    CheckGlDieOnError()
    points.mAttri.unbind();

    glDrawArrays(GL_POINTS, 0, points.mPos.sizeInPoints());

    CheckGlDieOnError()

    //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableVertexAttribArray(attributesID);
    glDisableVertexAttribArray(normalID);
    glDisableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()

    CheckGlDieOnError()


    fbo.Unbind();



    getShader(id).Unbind();

    glFlush();

//    glDisable (GL_BLEND);
//
    glDisable(GL_PROGRAM_POINT_SIZE);
//
////    glBindTexture(GL_TEXTURE_2D, objectID);
    glDisable(GL_POINT_SPRITE);
}


void renderEngine::renderPoints(ShaderID id, cPoints& data, std::vector<pangolin::TypedImage>& out,ShaderInputFn&& fn, bool DEBUG){


    GLint ouputIType = getInternalFormat(out[0].fmt);
    GLenum outputType = getGLType(out[0].fmt);
    GLenum outputFMT = getGLFormat(out[0].fmt);

    int numOutput=out.size();

    // Create output buffer
    pangolin::GlTexture *outputs = new pangolin::GlTexture[numOutput];
    for (int i=0; i<numOutput; ++i) outputs[i] = pangolin::GlTexture((int)out[i].w,(int)out[i].h,ouputIType,true,0,outputFMT,outputType);

    renderPoints( id,  data,  outputs,  numOutput, [fn](){fn();});

    for (int i=0; i<numOutput; ++i) outputs[i].Download(out[i].ptr, outputFMT, outputType);

    // DEBUG CODE
    if (DEBUG) {
        std::string prefix = "renderPoints" + std::to_string(num_debug++);
        const pangolin::PixelFormat fmt = pangolin::PixelFormatFromString("RGBA32");
        pangolin::TypedImage buffer(out[0].w, out[0].h, fmt);
        float *in = (float *) out[1].begin();
        char *o = (char *) buffer.begin();
        int w = out[0].w;
        int h = out[0].h;
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                o[i * w * 4 + j * 4] = (char) ((in[i * w * 4 + j * 4] + 1) * 127.5f);
                o[i * w * 4 + j * 4 + 1] = (char) ((in[i * w * 4 + j * 4 + 1] + 1) * 127.5f);
                o[i * w * 4 + j * 4 + 2] = (char) ((in[i * w * 4 + j * 4 + 2] + 1) * 127.5f);
                o[i * w * 4 + j * 4 + 3] = 255;
            }

            //std::cout<<in[i*w*4 + 300*4 + 2]<<std::endl;
        }
        SaveImage(buffer, fmt, prefix + ".png", false);
        // END DEBUG CODE
    }

    delete[] outputs;
}


void renderEngine::render2MRT(ShaderID id, pangolin::GlTexture* inputs, int numInput, std::vector<pangolin::TypedImage>& buffers,ShaderInputFn&& fn)
{

    // Assume all input share one fmt and all output share one fmt
    pangolin::PixelFormat outputFmt = buffers[0].fmt;

    // determine the type of the input and output
    GLint ouputIType = getInternalFormat(outputFmt);

    GLenum outputType = getGLType(outputFmt);

    GLenum outputFMT = getGLFormat(outputFmt);

    int numOutput=buffers.size();

    // Create output buffer
    pangolin::GlTexture *outputs = new pangolin::GlTexture[numOutput];
    for (int i=0; i<numOutput; ++i) outputs[i] = pangolin::GlTexture((int)buffers[i].w,(int)buffers[i].h,ouputIType,true,0,outputFMT,outputType);

    render2MRT( id,  inputs,  numInput,  outputs, numOutput, [fn](){fn();});

        // get back our data
    for (int i=0; i<numOutput; ++i) outputs[i].Download(buffers[i].ptr, outputFMT, outputType);

//    //            // DEBUG CODE
//            if (1) {
//                std::string prefix = "renderPoints" + std::to_string(num_debug++);
//                const pangolin::PixelFormat fmt = pangolin::PixelFormatFromString("RGBA32");
//                pangolin::TypedImage buffer(buffers[0].w, buffers[0].h, fmt);
//                float *in = (float *) buffers[0].begin();
//                char *o = (char *) buffer.begin();
//                int w = buffers[0].w;
//                int h = buffers[0].h;
//                for (int i = 0; i < h; i++) {
//                    for (int j = 0; j < w; j++) {
//                        o[i * w * 4 + j * 4] = (char) ((in[i * w * 4 + j * 4] + 1) * 127.5f);
//                        o[i * w * 4 + j * 4 + 1] = (char) ((in[i * w * 4 + j * 4 + 1] + 1) * 127.5f);
//                        o[i * w * 4 + j * 4 + 2] = (char) ((in[i * w * 4 + j * 4 + 2] + 1) * 127.5f);
//                        o[i * w * 4 + j * 4 + 3] = 255;
//                    }
//
//                    //std::cout<<in[i*w*4 + 300*4 + 2]<<std::endl;
//                }
//                SaveImage(buffer, fmt, prefix + ".png", false);
//            }


    // release the memory
    delete [] outputs;

}

void renderEngine::render2MRT(ShaderID id, std::vector<pangolin::TypedImage>& inputBuffers, std::vector<pangolin::TypedImage>& buffers,
                              ShaderInputFn&& fn){

    // Assume all input share one fmt and all output share one fmt
    pangolin::PixelFormat inputFmt = inputBuffers[0].fmt;
    // determine the type of the input and output
    GLint inputIType = getInternalFormat(inputFmt);
    GLenum inputType = getGLType(inputFmt);
    GLenum inputFMT = getGLFormat(inputFmt);

    // Create input buffer
    int numInput = inputBuffers.size();

    pangolin::GlTexture *inputs = new pangolin::GlTexture[numInput];
    for (int i=0; i<numInput; ++i)
    {
        inputs[i].Reinitialise((int)inputBuffers[i].w,(int)inputBuffers[i].h,inputIType,false,0,inputFMT, inputType,inputBuffers[i].ptr);
        //inputs[i].Upload(,inputFMT, inputType);
    }

    render2MRT(id, inputs, numInput, buffers,[fn](){fn();});



//    // DEBUG CODE
//    std::string prefix = "MRT" + std::to_string(num_debug2++);
//    const pangolin::PixelFormat fmt = pangolin::PixelFormatFromString("RGBA32");
//    pangolin::TypedImage buffer(buffers[0].w, buffers[0].h, fmt );
//    float* in = (float*)buffers[1].begin();
//    char* o = (char*)buffer.begin();
//    int w = buffers[0].w; int h = buffers[0].h;
//    for(int i = 0; i < h; i++) {
//        for(int j = 0; j < w; j++) {
//            o[i*w*4 + j*4] =    (char)((in[i*w*4 + j*4]+1)*127.5f);
//            o[i*w*4 + j*4 +1] = (char)((in[i*w*4 + j*4 +1]+1)*127.5f);
//            o[i*w*4 + j*4 +2] = (char)((in[i*w*4 + j*4 +2]+1)*127.5f);
//            o[i*w*4 + j*4 +3] = 255;
//        }
//
//        //std::cout<<in[i*w*4 + 300*4 + 2]<<std::endl;
//    }
//    SaveImage(buffer, fmt, prefix + ".png", false);


    delete [] inputs;
}


void renderEngine::render2MRT(ShaderID id, pangolin::GlTexture* inputs, int numInput, pangolin::GlTexture* outputs, int numOutput,ShaderInputFn&& fn){


    currentID = id; // thread not safe here

    getShader(id).SaveBind();

    fn();

    // Create depth buffer
    pangolin::GlRenderBuffer depthB(outputs[0].width,outputs[0].height);

    // Create FBO
    pangolin::GlFramebuffer fbo;
    glGenFramebuffersEXT(1, &(fbo.fbid));
    for (int i=0; i < numOutput; ++i) fbo.AttachColour(outputs[i]);// binding to the outputs
    fbo.AttachDepth(depthB);
    CheckGlDieOnError();


    // Render into FBO
    fbo.Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    for (int i=0; i<numInput; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, inputs[i].tid); // binding the textures
    }

    // set camera and projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // construct quad for rendering
    glEnableClientState(GL_VERTEX_ARRAY);
    GLfloat sq_vert[] = { -1,-1,  1,-1,  1, 1,  -1, 1 };
    glVertexPointer(2, GL_FLOAT, 0, sq_vert);


    // setting UV
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    GLfloat sq_tex[]  = { 0,1,  1,1,  1,0,  0,0  };
    glTexCoordPointer(2, GL_FLOAT, 0, sq_tex);


    //NOTE: glEnable seems not needed here and the bind
    //glEnable(GL_TEXTURE_2D);
    //inputs[0].Bind();

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    //glDisable(GL_TEXTURE_2D);

    // unbind FBO
    fbo.Unbind();

    getShader(id).Unbind();

    glFlush();
}

void renderEngine::deBugTest(cVec3* points, int num){

    int depthW = 640;
    int depthH = 480;
    // Create FBO
    pangolin::GlTexture color(depthW,depthH);
    pangolin::GlRenderBuffer depthB(depthW,depthH);
    pangolin::GlFramebuffer fbo(color, depthB);

    // Render into FBO
    fbo.Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


//    pangolin::GlTexture imageTexture(depth.getW(),depth.getH(),GL_RED,true,0,GL_RED,GL_FLOAT);
//    imageTexture.Upload(depth.data(),GL_RED,GL_FLOAT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //prog_fixed.SaveBind();

    //renderPoints(points, num);
    glColor3f(0.0,0.0,1.0);

    GLfloat sq_vert[] = { -1,-1,  1,-1,  1, 1,  -1, 1 };
    glVertexPointer(2, GL_FLOAT, 0, sq_vert);
    glEnableClientState(GL_VERTEX_ARRAY);

    GLfloat sq_tex[]  = { 0,1,  1,1,  1,0,  0,0  };
    glTexCoordPointer(2, GL_FLOAT, 0, sq_tex);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    //glEnable(GL_TEXTURE_2D);
    //imageTexture.Bind();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    //glDisable(GL_TEXTURE_2D);

    //prog_fixed.Unbind();

    glFlush();


    std::string prefix = "point";
    const pangolin::PixelFormat fmt = pangolin::PixelFormatFromString("RGBA32");
    pangolin::TypedImage buffer(depthW,depthH, fmt );
    color.Download(buffer);
    SaveImage(buffer, fmt, prefix + ".png", false);

    // unbind FBO
    fbo.Unbind();

}

void renderEngine::renderPoints(gpuPoints & model, renderEngine::ShaderInputFn &&fn) {
    ShaderID id = DISPLAY_CL; // thread not safe here
    glEnable(GL_PROGRAM_POINT_SIZE);

    glEnable(GL_POINT_SPRITE);
    //glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    //renderPoints(DISPLAY,  data, NULL, 0, [fn](){fn();});
    currentID = DISPLAY_CL; // thread not safe here


    getShader().SaveBind();

    fn();


    glEnableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()
    //glEnableClientState(GL_NORMAL_ARRAY);
    //CheckGlDieOnError()

    int attributesID = getShader(id).GetAttributeHandle("p_attri");
    glEnableVertexAttribArray(attributesID);
    CheckGlDieOnError()


    int normalID = getShader(id).GetAttributeHandle("p_normal");
    glEnableVertexAttribArray(normalID);
    CheckGlDieOnError()

    model.mPos.bind();
    glVertexPointer(4, GL_FLOAT, 0, 0);
    CheckGlDieOnError()
    model.mPos.unbind();

    model.mNorm.bind();
    glVertexAttribPointer(normalID, 4, GL_FLOAT, GL_FALSE, 0, 0);
    //glTexCoordPointer(gltex.count_per_element, gltex.datatype, 0, 0);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    CheckGlDieOnError()
    model.mNorm.unbind();

    model.mAttri.bind();
    glVertexAttribPointer(attributesID, 4, GL_FLOAT, GL_FALSE, 0, 0);
    //glTexCoordPointer(gltex.count_per_element, gltex.datatype, 0, 0);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    CheckGlDieOnError()
    model.mAttri.unbind();

    glDrawArrays(GL_POINTS, 0, model.numPoints);

    CheckGlDieOnError()

    //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableVertexAttribArray(attributesID);
    glDisableVertexAttribArray(normalID);
    glDisableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()



    getShader(id).Unbind();

    glFlush();

    glDisable (GL_BLEND);
//
    glDisable(GL_PROGRAM_POINT_SIZE);
//
////    glBindTexture(GL_TEXTURE_2D, objectID);
    glDisable(GL_POINT_SPRITE);

}

void renderEngine::renderPoints(gpuPoints & model, GPUBuffer &index, renderEngine::ShaderInputFn &&fn) {

    ShaderID id = DISPLAY_CL; // thread not safe here
    glEnable(GL_PROGRAM_POINT_SIZE);

    glEnable(GL_POINT_SPRITE);
    //glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    //renderPoints(DISPLAY,  data, NULL, 0, [fn](){fn();});
    currentID = DISPLAY_CL; // thread not safe here


    getShader().SaveBind();

    fn();


    glEnableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()
    //glEnableClientState(GL_NORMAL_ARRAY);
    //CheckGlDieOnError()

    int attributesID = getShader(id).GetAttributeHandle("p_attri");
    glEnableVertexAttribArray(attributesID);
    CheckGlDieOnError()


    int normalID = getShader(id).GetAttributeHandle("p_normal");
    glEnableVertexAttribArray(normalID);
    CheckGlDieOnError()

    model.mPos.bind();
    glVertexPointer(4, GL_FLOAT, 0, 0);
    CheckGlDieOnError()
    model.mPos.unbind();

    model.mNorm.bind();
    glVertexAttribPointer(normalID, 4, GL_FLOAT, GL_FALSE, 0, 0);
    //glTexCoordPointer(gltex.count_per_element, gltex.datatype, 0, 0);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    CheckGlDieOnError()
    model.mNorm.unbind();

    model.mAttri.bind();
    glVertexAttribPointer(attributesID, 4, GL_FLOAT, GL_FALSE, 0, 0);
    //glTexCoordPointer(gltex.count_per_element, gltex.datatype, 0, 0);
    //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    CheckGlDieOnError()
    model.mAttri.unbind();


    index.bind();// please make sure the index buffer is on ARRAY_INDEX binding
//    unsigned int* indices = new unsigned int[model.numPoints];
//    for (int i =0; i<model.numPoints;++i)
//        indices[i] = i;
//    GLuint elementbuffer;
//    glGenBuffers(1, &elementbuffer);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.numPoints * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
//
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

    glDrawElements(
            GL_POINTS,      // mode
            index.sizeInPoints(),    // count
            GL_UNSIGNED_INT,   // type
            (void*)0           // element array buffer offset
    );

//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //glDrawArrays(GL_POINTS, 0, num);
    index.unbind();

//    int tmp = model.numPoints;
//    int tmp2 = index.sizeInPoints();


//    glDrawArrays(GL_POINTS, 0, model.numPoints);

    CheckGlDieOnError()

    //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableVertexAttribArray(attributesID);
    glDisableVertexAttribArray(normalID);
    glDisableClientState(GL_VERTEX_ARRAY);
    CheckGlDieOnError()



    getShader(id).Unbind();

    glFlush();

    glDisable (GL_BLEND);
//
    glDisable(GL_PROGRAM_POINT_SIZE);
//
////    glBindTexture(GL_TEXTURE_2D, objectID);
    glDisable(GL_POINT_SPRITE);

//    delete []indices;
//    glDeleteBuffers(1, &elementbuffer);

}
