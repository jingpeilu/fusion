//
// Created by LiYang on 2018-12-04.
//

#ifndef FUSION_RENDERENGINE_H
#define FUSION_RENDERENGINE_H

#include "utilities.h"
#include <pangolin/pangolin.h>
#include "Utilities/image_utilities.h"

#include "GPUBuffer.h"


enum ShaderID { ICP_PROJECT_CL=0, DISPLAY,DISPLAY_CL,FUSION_PROJECT_CL };

class renderEngine {
public:
    using ShaderInputFn = std::function<void()>;

    static renderEngine& engine(){
        static renderEngine instance;
        return instance;
    };

    void renderPoints(cPoints&, int num,ShaderInputFn&& fn=0);
    void renderPoints(gpuPoints&, ShaderInputFn&& fn=0);
    void renderPoints(gpuPoints&, GPUBuffer& index, ShaderInputFn&& fn=0);
    void renderPoints(cPoints& data, int num, GPUBuffer& indexBuffer, ShaderInputFn&& fn=0);
    void renderPoints(ShaderID id, cPoints& data, std::vector<pangolin::TypedImage>& out,ShaderInputFn&& fn, bool DEBUG = false);
    void renderPoints(ShaderID id, cPoints& data, pangolin::GlTexture* outputs, int numOutput, ShaderInputFn&& fn);
    void renderPoints(ShaderID id, std::vector<GPUBuffer>& inputBuffers, pangolin::GlTexture* outputs, int numOutput, ShaderInputFn&& fn);
    void renderPoints(ShaderID id, gpuPoints& points, GPUBuffer& index, pangolin::GlTexture* outputs, int numOutput, ShaderInputFn&& fn);
    void renderPoints(ShaderID id, gpuPoints& points, pangolin::GlTexture* outputs, int numOutput, ShaderInputFn&& fn);


        //  void renderPoints(ShaderID id, cPoints&, std::vector<pangolin::TypedImage>& out,ShaderInputFn&& fn, bool DEBUG = false);

    void render2MRT(ShaderID id, std::vector<pangolin::TypedImage>& in, std::vector<pangolin::TypedImage>& out,ShaderInputFn&& fn);
    void render2MRT(ShaderID id, pangolin::GlTexture* in, int numInput, std::vector<pangolin::TypedImage>& out,ShaderInputFn&& fn);
    void render2MRT(ShaderID id, pangolin::GlTexture* in, int numInput, pangolin::GlTexture* out, int numOutput,ShaderInputFn&& fn);


    void deBugTest(cVec3* points, int num);

    static pangolin::GlSlProgram& getShader(ShaderID id)
    {
        return engine().progs[id];
    }

    static pangolin::GlSlProgram& getShader()
    {
        return engine().progs[engine().currentID];
    }

    inline static void convert(pangolin::OpenGlMatrix& a, glm::mat4& b){
        for (int i =0; i<4; ++i){
            for (int j =0; j<4; ++j){
                b[i][j] = a.m[i*4 +j];
            }
        }
    }

    inline static void convert2GL( glm::mat4& b, pangolin::OpenGlMatrix& a){
        for (int i =0; i<4; ++i){
            for (int j =0; j<4; ++j){
                a.m[i*4 +j] = b[i][j];
            }
        }
    }

    ShaderID currentID;

    GLint getInternalFormat(pangolin::PixelFormat& fmt);
    GLint getGLFormat(pangolin::PixelFormat& fmt);
    GLint getGLType(pangolin::PixelFormat& fmt);

private:
    renderEngine(){
        //loading all shaders

        std::string basePath = "/Users/jingpeilu/Desktop/fusion-master/";


        progs.push_back(pangolin::GlSlProgram());
        progs[ICP_PROJECT_CL].AddShaderFromFile(pangolin::GlSlVertexShader,basePath+"Shaders/ICPprojectionCL.vert");
        progs[ICP_PROJECT_CL].AddShaderFromFile(pangolin::GlSlFragmentShader,basePath+"Shaders/ICPprojectionCL.frag");
        progs[ICP_PROJECT_CL].Link();

        progs.push_back(pangolin::GlSlProgram());
        progs[DISPLAY].AddShaderFromFile(pangolin::GlSlVertexShader,basePath+"Shaders/display.vert");
        progs[DISPLAY].AddShaderFromFile(pangolin::GlSlFragmentShader,basePath+"Shaders/display.frag");
        progs[DISPLAY].Link();


        progs.push_back(pangolin::GlSlProgram());
        progs[DISPLAY_CL].AddShaderFromFile(pangolin::GlSlVertexShader,basePath+"Shaders/displayCL.vert");
        progs[DISPLAY_CL].AddShaderFromFile(pangolin::GlSlFragmentShader,basePath+"Shaders/display.frag");
        progs[DISPLAY_CL].Link();


        progs.push_back(pangolin::GlSlProgram());
        progs[FUSION_PROJECT_CL].AddShaderFromFile(pangolin::GlSlVertexShader,basePath+"Shaders/fusionProjCL.vert");
        progs[FUSION_PROJECT_CL].AddShaderFromFile(pangolin::GlSlFragmentShader,basePath+"Shaders/fusionProjCL.frag");
        progs[FUSION_PROJECT_CL].Link();



//        // probably should not do in here, but ...


        currentID = DISPLAY_CL;

        // Save locations of uniforms
        //u_color = prog_fixed.GetUniformHandle("u_color");
        //u_modelViewMatrix = prog_fixed.GetUniformHandle("u_modelViewMatrix");
        //u_modelViewProjectionMatrix = prog_fixed.GetUniformHandle("u_projView");
        //u_texture = prog_fixed.GetUniformHandle("u_texture");
        //u_textureEnable = prog_fixed.GetUniformHandle("u_textureEnable");

//        glGenFramebuffersEXT(1, &(point_fbo.fbid)); // deleted in deconstructer



//
    }
    renderEngine(renderEngine const&);
    void operator=(renderEngine const&);



    std::vector<pangolin::GlSlProgram>  progs;



    GLint u_color;
    GLint u_modelViewMatrix;
    GLint u_modelViewProjectionMatrix;
    GLint u_texture;
    GLint u_textureEnable;


//    pangolin::GlRenderBuffer depthB;
//    pangolin::GlFramebuffer point_fbo;

};




#endif //FUSION_RENDERENGINE_H
