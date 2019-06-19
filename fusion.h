//
// Created by LiYang on 2018-11-30.
//

#ifndef FUSION_FUSION_H
#define FUSION_FUSION_H



#include "utilities.h"
#include "Utilities/image_utilities.h"
#include "GPUBuffer.h"



struct cameraStatus{
    cVec3 p;
    cQuat r;

    glm::mat4 projMat;

    glm::mat4 projMatFuse; // used for fusing

    glm::mat4 poseMat;

    //float projParam;

    CameraParameters instrinsic;

    inline float getProjParam(int height){
        return projMat[1][1] * height/2;//mCamera.instrinsic.image_height

    }

    inline glm::mat4 K(){

        //TODO get internal camera K matrix

        return glm::transpose(glm::mat4(instrinsic.focal_x, 0.0, instrinsic.principal_x, 0.0,
                     0.0, instrinsic.focal_y, instrinsic.principal_y, 0.0,
                     0.0, 0.0, 1.0, 0.0,
                     0.0, 0.0, 0.0, 1.0));
    }


    inline glm::mat4 T(){

        //TODO get internal camera SE_3 T matrix
        glm::mat4 tmp = glm::toMat4(r);
        tmp[3] = glm::vec4(p,1.0f);

        return poseMat;
    }

    inline bool setT(glm::mat4&& in){

//        r = glm::toQuat(in);
//        p = glm::vec3(in[3]);

        poseMat = in;
        return true;
    }

};

class Fusion {

public:
    Fusion():time(0){}
    ~Fusion(){
        destroy();
    }

    bool init(pangolin::TypedImage& firstImage, CameraParameters& cp,int modelSize=100);

    void destroy();

    void inputDepthMapCL(pangolin::TypedImage& input);

    void displayGlobalModel(pangolin::OpenGlRenderState& s_cam, bool renderAll = false, int window_height = 480);

    void calculateICP_GPU(std::vector<GPUBuffer>& inputFrame, glm::mat4& modelView, glm::mat4& TPos);

    void update(pangolin::TypedImage& imgA,std::vector<cl_float4>& match);//

    void setCameraParam(CameraParameters& cp){
        mCamera.instrinsic = cp;
    };



    CameraParameters getCameraParam(){
        return mCamera.instrinsic ;
    };

    cVec3 getCameraPos()
    {
        glm::mat4 tmp = mCamera.T();
        return cVec3(tmp[3]);
    }

    glm::mat4 getModelViewMatrix()
    {

        return mCamera.T();
    }

    glm::mat4 getKMatrix()
    {

        return mCamera.K();
    }



    gpuPoints allModel;
    gpuPoints allModel2;
    GPUBuffer stableIndex;

    std::string getMessage(){
        return message.str();
    }

private:

    float innerLoopICP_CL(std::vector<GPUBuffer>& inputTex, int resample, Eigen::MatrixXd& ATA, Eigen::VectorXd& ATb);

    void prepareStableIndexNSwapAllModel();

    void procrustes(std::vector<cl_float4>&, glm::mat4&);

    void debug_saveGLTexture(pangolin::GlTexture* tex, std::string baseName = "DebugImage");
    void debug_saveGLTexture(GPUBuffer* tex, std::string baseName = "DebugImage");

    float * get(float* cur, int dx, int dy);

    bool isValidPoint(cVec3& v)
    {
        if (v.x != v.x || v.y != v.y || v.z != v.z) return false;
        if (v.x > EPSILON || v.x < -EPSILON || v.y > EPSILON || v.y < -EPSILON || v.z > EPSILON || v.z < -EPSILON) return true;
        return false;
    }

    cameraStatus mCamera;

    std::vector<glm::mat4> trajs;


    unsigned input_width;
    unsigned input_height;

    unsigned int time;

    int mConfidence;

    std::vector<GPUBuffer> CLbuffersForInput;

    GPUBuffer CLbufferLastInput;

    const static int ICP_LEVEL = 3; // hierarchical level number

    std::vector<GPUBuffer>  CLbuffersForICP[ICP_LEVEL];

    pangolin::GlTexture outputModelGL;
    GPUBuffer modelIndexCL;

    std::ostringstream message;

};


#endif //FUSION_FUSION_H
