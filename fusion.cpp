//
// Created by LiYang on 2018-11-30.
//

#include "fusion.h"
#include "renderEngine.h"
#include "computeEngine.h"



#include <vector>

static int num_debug = 0;
static int num_debug2 = 0;
static int num_input = 0;


void Fusion::debug_saveGLTexture(pangolin::GlTexture *tex, std::string baseName) {
    //DEBUG CODE
    int w = tex->width;
    int h = tex->height;

    const pangolin::PixelFormat fmt = pangolin::PixelFormatFromString("RGBA128F");
    pangolin::TypedImage out = pangolin::TypedImage(w, h, fmt);

    GLint ouputIType = renderEngine::engine().getInternalFormat(out.fmt);
    GLenum outputType = renderEngine::engine().getGLType(out.fmt);
    GLenum outputFMT = renderEngine::engine().getGLFormat(out.fmt);

    tex->Download(out.ptr, outputFMT, outputType);

    std::string prefix = baseName + std::to_string(num_debug++);
    const pangolin::PixelFormat fmt2 = pangolin::PixelFormatFromString("RGBA32");


    pangolin::TypedImage buffer(w, h, fmt2);
    float *in = (float *) out.begin();
    char *o = (char *) buffer.begin();

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            o[i * w * 4 + j * 4] = (char) ((in[i * w * 4 + j * 4] + 1) * 127.5f);
            o[i * w * 4 + j * 4 + 1] = (char) ((in[i * w * 4 + j * 4 + 1] + 1) * 127.5f);
            o[i * w * 4 + j * 4 + 2] = (char) ((in[i * w * 4 + j * 4 + 2] + 1) * 127.5f);
            o[i * w * 4 + j * 4 + 3] = 255;
        }

        //std::cout<<in[i*w*4 + 300*4 + 2]<<std::endl;
    }
    SaveImage(buffer, fmt2, prefix + ".png", false);
    // END DEBUG CODE
}


void Fusion::debug_saveGLTexture(GPUBuffer *tex, std::string baseName) {
    //DEBUG CODE
    const pangolin::PixelFormat fmt = pangolin::PixelFormatFromString("RGBA128F");

    tex->download();

    std::string prefix = baseName + std::to_string(num_debug++);
    const pangolin::PixelFormat fmt2 = pangolin::PixelFormatFromString("RGBA32");

    int w = input_width;
    int h = input_height;
    pangolin::TypedImage buffer(w, h, fmt2);
    float *in = (float *) tex->begin();
    char *o = (char *) buffer.begin();

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            o[i * w * 4 + j * 4] = (char) ((in[i * w * 4 + j * 4] + 1) * 127.5f);
            o[i * w * 4 + j * 4 + 1] = (char) ((in[i * w * 4 + j * 4 + 1] + 1) * 127.5f);
            o[i * w * 4 + j * 4 + 2] = (char) ((in[i * w * 4 + j * 4 + 2] + 1) * 127.5f);
            o[i * w * 4 + j * 4 + 3] = 255;
        }

        //std::cout<<in[i*w*4 + 300*4 + 2]<<std::endl;
    }
    SaveImage(buffer, fmt2, prefix + ".png", false);
    // END DEBUG CODE
}


bool Fusion::init(pangolin::TypedImage &img, CameraParameters &cp, int modelSize) {


    mConfidence = 10;

    modelSize = modelSize * img.h * img.w;

    // init camera
    setCameraParam(cp);


    pangolin::OpenGlMatrixSpec p = pangolin::ProjectionMatrix(mCamera.instrinsic.image_width,
                                                              mCamera.instrinsic.image_height,
                                                              mCamera.instrinsic.focal_x,
                                                              mCamera.instrinsic.focal_y,
                                                              mCamera.instrinsic.principal_x,
                                                              mCamera.instrinsic.principal_y,
                                                              CAMERA_MODEL_N,
                                                              CAMERA_MODEL_F);

    mCamera.p = cVec3(0);
    mCamera.r = cQuat();

    renderEngine::convert(p, mCamera.projMat);

    // mCamera.projParam = mCamera.projMat[1][1] * mCamera.instrinsic.image_height/2;//mCamera.instrinsic.image_height

    pangolin::OpenGlMatrixSpec projectionMatrix = pangolin::ProjectionMatrix(mCamera.instrinsic.image_width * 4,
                                                                             mCamera.instrinsic.image_height * 4,
                                                                             mCamera.instrinsic.focal_x * 4,
                                                                             mCamera.instrinsic.focal_y * 4,
                                                                             mCamera.instrinsic.principal_x * 4,
                                                                             mCamera.instrinsic.principal_y * 4,
                                                                             CAMERA_MODEL_N,
                                                                             CAMERA_MODEL_F);


    renderEngine::convert(projectionMatrix, mCamera.projMatFuse);

    mCamera.poseMat = glm::mat4(1.0f);

    input_width = img.w;
    input_height = img.h;

    time = 0;

    // init the model on GPU
    size_t preAllocatedSize = 500 * 10000 * 4 * sizeof(float);
    allModel.mPos = GPUBuffer(preAllocatedSize);
    allModel.mNorm = GPUBuffer(preAllocatedSize);
    allModel.mAttri = GPUBuffer(preAllocatedSize);

    allModel2.mPos = GPUBuffer(preAllocatedSize);
    allModel2.mNorm = GPUBuffer(preAllocatedSize);
    allModel2.mAttri = GPUBuffer(preAllocatedSize);

    stableIndex = GPUBuffer(preAllocatedSize / 4, pangolin::GlElementArrayBuffer);

    stableIndex.inUsedSize = 0;


    // pre allocate buffers
    size_t sz = input_width * input_height * 4 * sizeof(float);
    CLbuffersForInput.push_back(GPUBuffer(sz));
    CLbuffersForInput.push_back(GPUBuffer(sz));
    CLbuffersForInput.push_back(GPUBuffer(sz));
    CLbuffersForInput.push_back(GPUBuffer(sz));
    CLbuffersForInput.push_back(GPUBuffer(sz));


    // init the first image as global model
    inputDepthMapCL(img);
    // preprocessing the input data into global model.
    allModel.mPos.copyGPU(CLbuffersForInput[0]);
    allModel.mNorm.copyGPU(CLbuffersForInput[1]);
    allModel.mAttri.copyGPU(CLbuffersForInput[2]);

    int tmp3 = CLbuffersForInput[0].inUsedSize / (4 * sizeof(float));
    allModel.setNumPoints(tmp3);

    CLbufferLastInput = GPUBuffer(sz);


    prepareStableIndexNSwapAllModel();



    // prepare buffers for ICP
    int fineTime[ICP_LEVEL] = {4, 5, 10};
    int resample[ICP_LEVEL] = {4, 2, 1};

    pangolin::PixelFormat fmt = pangolin::PixelFormatFromString("RGBA128F");

    for (int l = 0; l < ICP_LEVEL; ++l) {
        CLbuffersForICP[l].clear();
        int sizeY = input_height / resample[l];
        int sizeX = input_width / resample[l];


        int num = sizeX * sizeY;

        for (int i = 0; i < 4; ++i) {
            //outputs[i].Delete(); // clean last time texture
            CLbuffersForICP[l].push_back(
                    pangolin::GlTexture((int) input_width / resample[l], (int) input_height / resample[l],
                                        renderEngine::engine().getInternalFormat(fmt), false, 0,
                                        renderEngine::engine().getGLFormat(fmt),
                                        renderEngine::engine().getGLType(fmt)));
        }
        CLbuffersForICP[l].push_back(GPUBuffer(sizeX / 32, sizeY / 8, 27)); // check here make sure mod ==0
    }

    // for model
    pangolin::GlTexture gltmp = pangolin::GlTexture((int) input_width * 4, (int) input_height * 4,
                                                    renderEngine::engine().getInternalFormat(fmt), false, 0,
                                                    renderEngine::engine().getGLFormat(fmt),
                                                    renderEngine::engine().getGLType(fmt));

    modelIndexCL = GPUBuffer(gltmp);

    // dirty code
    outputModelGL.internal_format = modelIndexCL.gl_tex.internal_format;
    outputModelGL.tid = modelIndexCL.gl_tex.tid;
    outputModelGL.width = modelIndexCL.gl_tex.width;
    outputModelGL.height = modelIndexCL.gl_tex.height;

    return true;
}


void Fusion::destroy() {

    // dirty code clean up
    outputModelGL.internal_format = 0;
    outputModelGL.tid = 0;
    outputModelGL.width = 0;
    outputModelGL.height = 0;

    CLbuffersForInput.clear();
    for (int l = 0; l < ICP_LEVEL; ++l) {
        CLbuffersForICP[l].clear();
    }

}


void Fusion::inputDepthMapCL(pangolin::TypedImage &input) {

    CLbuffersForInput.push_back(GPUBuffer(input));

    glm::mat4 inv_K = (glm::inverse(mCamera.K()));
    GPUBuffer tmp(inv_K);
    GPUBuffer *ptr = &tmp;
    int numOfPoints = 0; // buffer return back
    GPUBuffer tmp2(numOfPoints);
    GPUBuffer *ptr2 = &tmp2;
    int w = input.w;
    int h = input.h;
    float invf = 1.0 / mCamera.instrinsic.focal_x;// assuming focal is same;

    computeEngine::engine().runKernel(CL_DEPTH_PROCESSING, CLbuffersForInput, cl::NDRange(input_width, input_height),
                                      [ptr, ptr2, invf]() {
                                          CL_CHECK_ERROR(computeEngine::getKernel().setArg(6, invf);)
                                          CL_CHECK_ERROR(computeEngine::getKernel().setArg(7, *(ptr->getMemoryCL()));)
                                          CL_CHECK_ERROR(computeEngine::getKernel().setArg(8, *(ptr2->getMemoryCL()));)
                                      });

    CLbuffersForInput.pop_back();

}


void Fusion::displayGlobalModel(pangolin::OpenGlRenderState &s_cam, bool renderAll ,int window_height) {

    float proj = mCamera.getProjParam(window_height);
    if (renderAll) {
        glColor3f(0.0, 0.7, 0.0);
        renderEngine::engine().renderPoints(allModel,
                                            [proj]() {
                                                renderEngine::getShader(DISPLAY).SetUniform("ProjParam", proj);
                                            });
    } else {
//        glColor3f(1.0,0.0,0.0);
//        renderEngine::engine().renderPoints(stablePoints, stablePoints.mPos.size(),
//                [proj](){
//            renderEngine::getShader(DISPLAY).SetUniform("ProjParam", proj);
//        });
        glColor3f(0.7, 0.7, 0.7);
        renderEngine::engine().renderPoints(allModel, stableIndex,
                                            [proj]() {
                                                renderEngine::getShader(DISPLAY).SetUniform("ProjParam", proj);
                                            });
    }


//    glColor3f(0.0,1.0,0.0);
//    renderEngine::engine().renderPoints(inputModel, inputModel.mPos.size());
}


float
Fusion::innerLoopICP_CL(std::vector<GPUBuffer> &CLbuffers, int resample, Eigen::MatrixXd &ATA, Eigen::VectorXd &ATb) {

    float er = 0.0f;

    int sizeY = input_height / resample;
    int sizeX = input_width / resample;


    int num = sizeX * sizeY;
    //CLbuffers.push_back(GPUBuffer(sizeX/32,sizeY/8,27)); // check here make sure mod ==0
    cl::LocalSpaceArg local = cl::Local(32 * 8 * 27 * sizeof(float));


    computeEngine::engine().runKernel(CL_ICP_INNER, CLbuffers, cl::NDRange(sizeX, sizeY),
                                      [num, local]() {
                                          CL_CHECK_ERROR(computeEngine::getKernel().setArg(5, num);)
                                          CL_CHECK_ERROR(computeEngine::getKernel().setArg(6, local);)
                                      },
                                      cl::NDRange(32, 8));

    CLbuffers[4].download();
    float *r = (float *) CLbuffers[4].begin();

    for (int i = 1; i < num / 256; ++i) {
        int id = i * 27;
        for (int j = 0; j < 27; ++j) {
            r[j] += r[id + j];
        }
    }

    int iter = 0;
    ATA(0, 0) = r[0];
    ATA(0, 1) = r[1];
    ATA(0, 2) = r[2];
    ATA(0, 3) = r[3];
    ATA(0, 4) = r[4];
    ATA(0, 5) = r[5];

    ATA(1, 0) = r[1];
    ATA(1, 1) = r[6];
    ATA(1, 2) = r[7];
    ATA(1, 3) = r[8];
    ATA(1, 4) = r[9];
    ATA(1, 5) = r[10];

    ATA(2, 0) = r[2];
    ATA(2, 1) = r[7];
    ATA(2, 2) = r[11];
    ATA(2, 3) = r[12];
    ATA(2, 4) = r[13];
    ATA(2, 5) = r[14];

    ATA(3, 0) = r[3];
    ATA(3, 1) = r[8];
    ATA(3, 2) = r[12];
    ATA(3, 3) = r[15];
    ATA(3, 4) = r[16];
    ATA(3, 5) = r[17];

    ATA(4, 0) = r[4];
    ATA(4, 1) = r[9];
    ATA(4, 2) = r[13];
    ATA(4, 3) = r[16];
    ATA(4, 4) = r[18];
    ATA(4, 5) = r[19];

    ATA(5, 0) = r[5];
    ATA(5, 1) = r[10];
    ATA(5, 2) = r[14];
    ATA(5, 3) = r[17];
    ATA(5, 4) = r[19];
    ATA(5, 5) = r[20];

    ATb(0) = r[21];
    ATb(1) = r[22];
    ATb(2) = r[23];
    ATb(3) = r[24];
    ATb(4) = r[25];
    ATb(5) = r[26];

    //CLbuffers.pop_back();
    return er;
}


void Fusion::calculateICP_GPU(std::vector<GPUBuffer> &inputFrame, glm::mat4 &modelView, glm::mat4 &TPos) {

    // parameters for the ICP
    int fineTime[ICP_LEVEL] = {0, 0, 19};
    int resample[ICP_LEVEL] = {4, 2, 1};

    int iter = 0;

    double et = 0.0;

    pangolin::GlTexture outputs[4];

    float er;
    //pangolin::PixelFormat fmt = pangolin::PixelFormatFromString("RGBA128F");
    for (int l = 0; l < ICP_LEVEL; ++l) {

        // rendering the global model

        for (int i = 0; i < 4; ++i) {

            // dirty work here!!
            outputs[i].internal_format = CLbuffersForICP[l][i].gl_tex.internal_format;
            outputs[i].tid = CLbuffersForICP[l][i].gl_tex.tid;
            outputs[i].width = CLbuffersForICP[l][i].gl_tex.width;
            outputs[i].height = CLbuffersForICP[l][i].gl_tex.height;
            //outputs[i].tid = i;
        }

        glm::mat4 lastT(1.0f);
        float proj = mCamera.getProjParam(input_height / resample[l]);

        glm::mat4 projView = mCamera.projMat * lastT;
        glm::mat4 cameraMat = lastT;
        //renderEngine::engine().deBugTest(inputModel.mPos.data(), inputModel.mPos.size());

        glViewport(0, 0, input_width / resample[l], input_height / resample[l]);
        renderEngine::engine().renderPoints(ICP_PROJECT_CL,
                                            inputFrame, outputs+ 2, 2, [projView, cameraMat, proj]() {
                    renderEngine::getShader(ICP_PROJECT_CL).SetUniform("ProjParam", proj);
                    glUniformMatrix4fv(glGetUniformLocation(renderEngine::getShader(ICP_PROJECT_CL).ProgramId(),
                                                            "u_projViewModel"), 1,
                                       GL_FALSE, glm::value_ptr(projView));
                    glUniformMatrix4fv(glGetUniformLocation(renderEngine::getShader(ICP_PROJECT_CL).ProgramId(),
                                                            "u_cameraPos"), 1,
                                       GL_FALSE, glm::value_ptr(cameraMat));
                });
        glViewport(0, 0, input_width, input_height);



        for (int rf = 0; rf < fineTime[l]; ++rf) {
            // prepare for new correspondence


            cameraMat = TPos * modelView ;
            projView = mCamera.projMat * cameraMat;

            //renderEngine::engine().deBugTest(inputModel.mPos.data(), inputModel.mPos.size());

            glViewport(0, 0, input_width / resample[l], input_height / resample[l]);
            renderEngine::engine().renderPoints(ICP_PROJECT_CL,
                                                allModel, stableIndex, outputs , 2, [projView, cameraMat, proj]() {
                        renderEngine::getShader(ICP_PROJECT_CL).SetUniform("ProjParam", proj);
                        glUniformMatrix4fv(
                                glGetUniformLocation(renderEngine::getShader(ICP_PROJECT_CL).ProgramId(), "u_projViewModel"),
                                1,
                                GL_FALSE, glm::value_ptr(projView));
                        glUniformMatrix4fv(
                                glGetUniformLocation(renderEngine::getShader(ICP_PROJECT_CL).ProgramId(), "u_cameraPos"), 1,
                                GL_FALSE, glm::value_ptr(cameraMat));
                    });
            glViewport(0, 0, input_width, input_height);

            // here are the prepared inputs for the cholesky decomposition
            Eigen::MatrixXd ATA = Eigen::MatrixXd::Zero(6, 6);    // directly put into Eigen
            Eigen::VectorXd ATb = Eigen::VectorXd::Zero(6);
            shrDeltaT(2);

            //           er = innerLoopICP_GPU(outputs, resample[l], ATA,  ATb);
            er = innerLoopICP_CL(CLbuffersForICP[l], resample[l], ATA, ATb);

            // Cholesky decomposition and solver for the system.
            Eigen::VectorXd c = ATA.ldlt().solve(ATb);
            et += shrDeltaT(2);
            //DEBUG CODE
//            std::cout << c << std::endl << std::endl<< ATA<< std::endl<< std::endl <<ATb<< std::endl<< std::endl;
            //END DEBUG CODE


            glm::mat4 delta(
                    1.0f); // setting back into glm https://lists.nongnu.org/archive/html/toon-members/2009-06/msg00029.html
            delta[0][0] = 1.0f;
            delta[0][1] = c(5);
            delta[0][2] = -c(4);

            delta[1][0] = -c(5);
            delta[1][1] = 1.0f;
            delta[1][2] = c(3);

            delta[2][0] = c(4);
            delta[2][1] = -c(3);
            delta[2][2] = 1.0f;

//            std::cout<<"Approxi Rotation::"<<std::endl<<glm::to_string(delta)<<std::endl;

            // try quaternion

            glm::quat qq(1.0f,c(3),c(4),c(5));

//            std::cout<<"Quater ::"<<std::endl<<glm::to_string(qq)<<std::endl;
            glm::quat q = glm::normalize(qq);
//            std::cout<<"Normal Quater::"<<std::endl<<glm::to_string(q)<<std::endl;
            glm::mat3 R = glm::toMat3(q);

//            delta[0][0] = R[0][0];
//            delta[0][1] = R[0][1];
//            delta[0][2] = R[0][2];
//
//            delta[1][0] = R[1][0];
//            delta[1][1] = R[1][1];
//            delta[1][2] = R[1][2];
//
//            delta[2][0] = R[2][0];
//            delta[2][1] = R[2][1];
//            delta[2][2] = R[2][2];

//            std::cout<<"Quaternion matrix:"<<std::endl<<glm::to_string(delta)<<std::endl;



            // translation

            delta[3][0] = c(0);
            delta[3][1] = c(1);
            delta[3][2] = c(2);

            iter += 1;

            TPos = delta * TPos;


            std::cout<<"ICP c norm = "<<c.norm()<<std::endl;
            if (c.norm() < 1e-7) break;


        }
    }


    //std::cout << "error: " << er << std::endl;
    message << "iter: " << iter << std::endl;
    message << "InnerLoop Time: " << et << std::endl;

// DEBUG CODE
    if (0) {
        debug_saveGLTexture(&outputs[1]);
        debug_saveGLTexture(&outputs[3]);
    }

    // dirty code here to prevent the GPU buffers deleting.
    for (int i = 0; i < 4; ++i) {

        // dirty work here!!
        outputs[i].internal_format = 0;
        outputs[i].tid = 0;
        outputs[i].width = 0;
        outputs[i].height = 0;
        //outputs[i].tid = i;
    }

}


float *Fusion::get(float *cur, int dx = 0, int dy = 0) {

    dy = dy < input_height ? dy : input_height;
    dy = dy >= 0 ? dy : 0;
    dx = dx < input_width ? dx : input_width;
    dx = dx >= 0 ? dx : 0;
    return cur + dy * input_width * 4 + dx * 4;
}


void Fusion::prepareStableIndexNSwapAllModel() {

    // run the kernel
    int conf = mConfidence;
    int t = time;
    GPUBuffer numStable(0);
    GPUBuffer numNew(0);
    std::vector<GPUBuffer *> CLbuffers;
    CLbuffers.push_back(&(allModel.mPos));
    CLbuffers.push_back(&(allModel.mNorm));
    CLbuffers.push_back(&(allModel.mAttri));
    CLbuffers.push_back(&(allModel2.mPos));
    CLbuffers.push_back(&(allModel2.mNorm));
    CLbuffers.push_back(&(allModel2.mAttri));
    CLbuffers.push_back(&(stableIndex));
    CLbuffers.push_back(&numStable);
    CLbuffers.push_back(&numNew);


    computeEngine::engine().runKernel(CL_FILTER_STABLE, CLbuffers, cl::NDRange(allModel.mPos.sizeInPoints()),
                                      [conf, t]() {
                                          CL_CHECK_ERROR(computeEngine::getKernel().setArg(9, conf);)
                                          CL_CHECK_ERROR(computeEngine::getKernel().setArg(10, t);)
                                      });

    numStable.download();
    int tmp2 = *((int *) numStable.begin());
    stableIndex.inUsedSize = tmp2 * sizeof(GLuint);
    stableIndex.perPointSize = sizeof(GLuint);

    numNew.download();
    int tmp3 = *((int *) numNew.begin());

    allModel2.setNumPoints(tmp3);

    // swap(allModel,allModel2);
    GPUBuffer gtmp = std::move(allModel.mPos);
    allModel.mPos = std::move(allModel2.mPos);
    allModel2.mPos = std::move(gtmp);

    gtmp = std::move(allModel.mNorm);
    allModel.mNorm = std::move(allModel2.mNorm);
    allModel2.mNorm = std::move(gtmp);

    gtmp = std::move(allModel.mAttri);
    allModel.mAttri = std::move(allModel2.mAttri);
    allModel2.mAttri = std::move(gtmp);

    gtmp = std::move(CLbufferLastInput);
    CLbufferLastInput = std::move(CLbuffersForInput[0]);
    CLbuffersForInput[0] = std::move(gtmp);

    allModel.setNumPoints(allModel2.numPoints);

    message << "Model points: " << tmp3 << std::endl
              << "Stable points: " << tmp2 << std::endl;
}



void Fusion::procrustes(std::vector<cl_float4>& match, glm::mat4& lastT)
{

    // calculate the default initialization for ICP
    std::vector<GPUBuffer *> CLbuffersFeature;
    GPUBuffer matchData = GPUBuffer(match.size()*sizeof(float)*4, (void*)match.data());
    GPUBuffer DataA = GPUBuffer(match.size()*sizeof(float)*4, (void*)match.data());
    GPUBuffer DataB = GPUBuffer(match.size()*sizeof(float)*4, (void*)match.data());
    CLbuffersFeature.push_back(&(CLbufferLastInput));
    CLbuffersFeature.push_back(&(CLbuffersForInput[0]));
    CLbuffersFeature.push_back(&(matchData));
    CLbuffersFeature.push_back(&(DataA));
    CLbuffersFeature.push_back(&(DataB));
    computeEngine::engine().runKernel(CL_PROCRUSTES_FIT, CLbuffersFeature, cl::NDRange(match.size()),
                                      [](){});

    matchData.download();
    DataA.download();
    DataB.download();

    cl_float4* trans = ((cl_float4 *) matchData.begin());
    cl_float4* dA = ((cl_float4 *) DataA.begin());
    cl_float4* dB = ((cl_float4 *) DataB.begin());

    glm::vec4 transT;
    transT.x = 0.0f;transT.y = 0.0f;transT.z = 0.0f;transT.w = 1.0f;

    glm::vec4 transTB;
    transTB.x = 0.0f;transTB.y = 0.0f;transTB.z = 0.0f;transTB.w = 1.0f;
    int count = 0;

// Procrustes analysis
    for (int i=0;i<match.size();++i)
    {
        if (trans[i].w > 0.0) {
            transT.x += dA[i].x;
            transT.y += dA[i].y;
            transT.z += dA[i].z;

            transTB.x += dB[i].x;
            transTB.y += dB[i].y;
            transTB.z += dB[i].z;
            count++;
        }
    }

    transT.x = transT.x/ count;
    transT.y = transT.y/count;
    transT.z = transT.z/ count;

    transTB.x = transTB.x/ count;
    transTB.y = transTB.y/count;
    transTB.z = transTB.z/ count;

    Eigen::Matrix3f BAT = Eigen::MatrixXf::Zero(3,3);
    // Procrustes analysis
    for (int i=0;i<match.size();++i)
    {
        if (trans[i].w > 0.0) {
            cl_float4 a,b;
            a.x= dA[i].x - transT.x;a.y= dA[i].y - transT.y;a.z= dA[i].z - transT.z;
            b.x= dB[i].x - transTB.x;b.y= dB[i].y - transTB.y;b.z= dB[i].z - transTB.z;
            BAT(0,0) += a.x * b.x;BAT(0,1) += a.x * b.y;BAT(0,2) += a.x * b.z;
            BAT(1,0) += a.y * b.x;BAT(1,1) += a.y * b.y;BAT(1,2) += a.y * b.z;
            BAT(2,0) += a.z * b.x;BAT(2,1) += a.z * b.y;BAT(2,2) += a.z * b.z;
        }
    }

    Eigen::JacobiSVD<Eigen::MatrixXf> svd( BAT, Eigen::ComputeThinU | Eigen::ComputeThinV);

    float det = (svd.matrixU()*svd.matrixV().transpose()).determinant();

    Eigen::Matrix3f s = Eigen::MatrixXf::Identity(3,3);
    s(2,2) = det;

    Eigen::Matrix3f R = svd.matrixU()*s*svd.matrixV().transpose();//

//    R.transposeInPlace();

    shrDeltaT(1);

    glm::mat4 T1(1.0f);
    glm::mat4 T2(1.0f);
    glm::mat4 TR(1.0f);

    T1[3][0] = - transT.x;
    T1[3][1] = - transT.y;
    T1[3][2] = - transT.z;

    T2[3][0] = -transTB.x ;
    T2[3][1] = -transTB.y ;
    T2[3][2] = -transTB.z ;

    TR[0][0] = R(0,0);
    TR[0][1] = R(0,1);
    TR[0][2] = R(0,2);

    TR[1][0] = R(1,0);
    TR[1][1] = R(1,1);
    TR[1][2] = R(1,2);

    TR[2][0] = R(2,0);
    TR[2][1] = R(2,1);
    TR[2][2] = R(2,2);

    T2 = glm::inverse(T2);

    glm::vec4 tmp = transTB - TR*transT;

    lastT = T2*TR*T1;


    printf("Final PT pos: %f, %f,%f; Matched points: %d\n",lastT[3][0],lastT[3][1],lastT[3][2], count);

}


void Fusion::update(pangolin::TypedImage &depthImage,std::vector<cl_float4>& match) {//
    ++time;
    message.str("");
    message << "current frame number is " << time << std::endl;

    // preprocessing the input data into global model.
    inputDepthMapCL(depthImage);


    // DEBUG CODE
    if (0) {
        debug_saveGLTexture(&CLbuffersForInput[0], std::string("InputNorm"));
    }

    glm::mat4 lastT(1.0f);
    procrustes(match,lastT);
    glm::mat4 lastT2(1.0);

    calculateICP_GPU(CLbuffersForInput, mCamera.poseMat, lastT2);//stablePoints

    printf("Final ICP pos: %f, %f,%f; \n", lastT2[3][0],lastT2[3][1],lastT2[3][2]);

    message << "ICP time elipsed: " << shrDeltaT(1) << std::endl;

    // set current mat
    mCamera.setT(lastT2 * mCamera.T());

    message << "Camera position:" << mCamera.T()[3][0] << " " << mCamera.T()[3][1] << " " << mCamera.T()[3][2] << " "
              << std::endl;

    // rendering the global model for fusion
    pangolin::PixelFormat fmt = pangolin::PixelFormatFromString("RGBA128F");

    glm::mat4 projView = mCamera.projMatFuse * mCamera.T();
    glm::mat4 cameraMat = mCamera.T();


    //set viewport size

    glViewport(0, 0, input_width * 4, input_height * 4);
    //renderEngine::engine().deBugTest(inputModel.mPos.data(), inputModel.mPos.size());
    renderEngine::engine().renderPoints(FUSION_PROJECT_CL,
                                        allModel, &outputModelGL, 1, [projView, cameraMat]() {
                glUniformMatrix4fv(glGetUniformLocation(renderEngine::getShader().ProgramId(), "u_projViewModel"), 1,
                                   GL_FALSE, glm::value_ptr(projView));
                glUniformMatrix4fv(glGetUniformLocation(renderEngine::getShader().ProgramId(), "u_cameraPos"), 1,
                                   GL_FALSE, glm::value_ptr(cameraMat));
            });

    // reset viewport
    glViewport(0, 0, input_width, input_height);


    if (0) {
        debug_saveGLTexture(&outputModelGL, std::string("Model"));
    }

    // Fusing all the data
    int conf = mConfidence;
    int t = time;
    glm::mat4 toG = glm::inverse(mCamera.T());


    GPUBuffer numPP(allModel.mPos.sizeInPoints());

    GPUBuffer toGB(toG);
    GPUBuffer toL(mCamera.T());
    std::vector<GPUBuffer *> CLbuffers;
    CLbuffers.push_back(&(allModel.mPos));
    CLbuffers.push_back(&(allModel.mNorm));
    CLbuffers.push_back(&(allModel.mAttri));
    CLbuffers.push_back(&(modelIndexCL));
    CLbuffers.push_back(&(CLbuffersForInput[0]));
    CLbuffers.push_back(&(CLbuffersForInput[1]));
    CLbuffers.push_back(&(CLbuffersForInput[2]));
    CLbuffers.push_back(&toGB);
    CLbuffers.push_back(&toL);
    CLbuffers.push_back(&numPP);

    computeEngine::engine().runKernel(CL_FUSE, CLbuffers, cl::NDRange(input_width, input_height),
                                      [conf, t]() {
                                          CL_CHECK_ERROR(computeEngine::getKernel().setArg(10, conf);)
                                          CL_CHECK_ERROR(computeEngine::getKernel().setArg(11, t);)
                                      });

    numPP.download();
    int tmp = *((int *) numPP.begin());
    allModel.setNumPoints(tmp);


    // prepare the stable list
    prepareStableIndexNSwapAllModel();


    message << "Time elipsed: " << shrDeltaT() << std::endl;

}
