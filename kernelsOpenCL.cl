__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;



__kernel void test(__global float4* output1,
                    __global float4* output2,
                    __read_only image2d_t model_pos,
                    __read_only image2d_t model_norm,
                   int range)
{
        //get our index in the array
        const int2 cid = {get_global_id(0), get_global_id(1)};
        const int2 pos = cid *4;

        //printf("test: %d, %d \n", cid.x,cid.y);
        int currentID = 0;
        float result[8] = {-1.0f};
        float conf[8] = {-1.0f};

        for (int j=0; j<4; ++j) {
            for (int i = 0; i<4; ++i){
                float4 posM = read_imagef(model_pos, sampler, pos + (int2)(j,i));
                float4 normM = read_imagef(model_norm, sampler, pos + (int2)(j,i));


                // not valid flag
                if (posM.w <= 0.0) continue;

                // back facing
                if (dot(posM.xyz,normM.xyz) >=0.0) continue;


                // finding correspondence
                if (currentID<8) {
                    result[currentID] = posM.w;
                    conf[currentID++] = normM.w;
                }
                else
                {
                    for (int j=0;j<8;++j)
                    {
                        if (normM.w > conf[j])
                        {
                            result[currentID] = posM.w;
                            conf[currentID++] = normM.w;
                            break;
                        }
                    }
                }

            }
        }


        output1[cid.x+cid.y*get_global_size(0)] = (float4)(result[0],result[1],result[2],result[3]);
        output2[cid.x+cid.y*get_global_size(0)] = (float4)(result[4],result[5],result[6],result[7]);

}

//////////////////////////////////////////////////////////////////////////////////////////
//// fuse part kernel
//////////////////////////////////////////////////////////////////////////////


//float4 multipleMatrix(float16 m, float4 v){
//
//    return (float4)(dot(m.s0123,v),dot(m.s4567,v),dot(m.s89ab,v),dot(m.scdef,v));
//}

static float4 multipleMatrix(float16 m, float4 v){

    return (float4)(dot(m.s048c,v),dot(m.s159d,v),dot(m.s26ae,v),dot(m.s37bf,v));
}


static void mergeWithPoint(__global float4* vertexPos,
__global float4* vertexNorm,
__global float4* vertexAtt,int idx, float4 pos, float4 norm, float4 att)
{
// fusing the position and normal

    float4 p = vertexPos[idx];
    float4 n = vertexNorm[idx];
    float4 a = vertexAtt[idx];

    float w = a.y;

    p = (w*p + att.y * pos);
    n = (w*n + att.y * norm);
    a.x =  (w*a.x + att.y * att.x);

    w +=  att.y;

    vertexPos[idx] = p / w;
    vertexNorm[idx] = n/ w;
    vertexAtt[idx] = (float4)(a.x / w, w, idx, att.w > a.w ? att.w : a.w);


}

//static void deletePoint(__global float4* vertexPos,
//                __global float4* vertexNorm,
//                __global float4* vertexAtt,
//                int idx,
//                __global int* numP)
//{
//// decrease the num of point
//
//// add the new point into global vertex
//if (idx <0)
//    printf("ERROR!!!!!!!!!HERE!!!!! ZERO!!!\n");
//
//int newid = atomic_dec(numP) - 1;
//
//if (idx >=*numP)
//printf("ERROR!!!!%d   %d\n",idx,*numP);
//
//if (newid <1)
//printf("ERROR!!!!!!!!!HERE!!!!! negative!!!\n");
//
//vertexPos[idx] = vertexPos[newid];
//vertexNorm[idx] = vertexNorm[newid];
//vertexAtt[idx] = vertexAtt[newid];
//vertexAtt[idx].z = idx;
//
//}


static void swap(__global int *xp,__global int *yp)
{
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

static void bubbleSort(__global int* arr, int n)
{
    int i, j;
    bool swapped;
    for (i = 0; i < n-1; i++)
    {
        swapped = false;
        for (j = 0; j < n-i-1; j++)
        {
            if (arr[j] < arr[j+1])
            {
                swap(&arr[j], &arr[j+1]);
                swapped = true;
            }
        }

        // IF no two elements were swapped by inner loop, then break
        if (swapped == false)
            break;
    }
}

__kernel void fuse(__global float4* vertexPos,
                    __global float4* vertexNorm,
                    __global float4* vertexAtt,
                    __read_only image2d_t model_index,
        __global float4* input_pos,
        __global float4* input_norm,
        __global float4* input_att,
                    __global float16* toG,
                    __global float16* toL,
                    __global int* numP,
int confidence,
int time)
{
    //get our index in the array
    const int2 cid = {get_global_id(0), get_global_id(1)};
    const int2 mid = cid *4;

    int id = cid.x+cid.y*get_global_size(0);
//int id = cid.x+(sz.y - cid.y - 1 )*get_global_size(0);

    int neigID = 0;
    int neigPoints[16] = {0};
    float4 neigPointsDebug[16];

    float4 iPos = input_pos[id];//read_imagef(input_pos, sampler, cid);
    float4 iNorm = input_norm[id];
    float4 iAtt = input_att[id];
    iAtt.w = time;



    if (iPos.w <= 0.5) // if input is not valid. maybe need to check whether need to merge other
        return;

    //convert into global coordinates
    float4 gp = multipleMatrix(*toG,(float4)(iPos.xyz,1.0f));
    float4 gn = multipleMatrix(*toG,(float4)(iNorm.xyz,0.0f));

    float confThs = -1000000.0f;
    float disThs = 100000.0f;

    int3 correspondence = (int3)(-1,-1,-1);

    for (int j=0; j<4; ++j) {
        for (int i = 0; i<4; ++i){
            float4 M = read_imagef(model_index, sampler, mid + (int2)(j,i));

            if (length(M.xyz) < 0.1f)
                continue;
            int idxM = convert_int(M.w + 0.4f);

            float4 posM = vertexPos[idxM];
            float4 normM = vertexNorm[idxM];
            float4 attM = vertexAtt[idxM];
            float confM = attM.y;

            // not valid flag
            if (posM.w <= 0.5)
                continue;

            // back facing
            //if (dot(posM.xyz,normM.xyz) >=0.0) continue;

            float dis = distance(posM.xyz, gp.xyz);
            float ang = dot(normM.xyz,gn.xyz);

            if ( (dis < 1.0f)
            && (ang > 0.85)
            && ((confM > confThs) || (confM == confThs && dis < disThs)))
            {
                confThs = confM;
                disThs = dis;
                correspondence = (int3)(j,i,idxM);
            }

            //get all neighbour points
            neigPointsDebug[neigID] = M;
            neigPoints[neigID++] = idxM;

        }
    }



    if (correspondence.x > -1) // find correspondence
    {
//        // merge the point with new comer
        if (iAtt.x > 1.5*vertexAtt[correspondence.z].x)
            vertexAtt[correspondence.z].y += iAtt.y;
        else
            mergeWithPoint(vertexPos,vertexNorm,vertexAtt,correspondence.z, gp, gn, iAtt);
        vertexAtt[correspondence.z].w = time;
    }
    else
    {
        // add the new point into global vertex
        int newid = atomic_inc(numP);
        vertexPos[newid] = gp;
        vertexNorm[newid] = gn;
        vertexAtt[newid] = (float4)(iAtt.x,iAtt.y,convert_float(newid),convert_float(time));
    }


    bool flag = true; // not hit in merge logic
    // merge neighbor points
    for (int i=0;i<neigID;++i){
        flag = true;
        int idx = neigPoints[i];
        float4 pos = vertexPos[idx];
        float4 norm = vertexNorm[idx];
        float4 att = vertexAtt[idx];
        for (int j=i+1; j<neigID; ++j)
        {
            int jdx = neigPoints[j];

            float4 p = vertexPos[jdx];
            float4 n = vertexNorm[jdx];
            float4 a = vertexAtt[jdx];
            if (distance(pos, p) < 0.5
                                   && dot(n, norm) > 0.85
                                   && (att.y > confidence || a.y > confidence)) // merge conditions
            {
                mergeWithPoint(vertexPos,
                        vertexNorm,vertexAtt,jdx,
                        pos,norm,att);

                vertexPos[neigPoints[i]] = (float4)(0.0f); // fake delete
                vertexNorm[neigPoints[i]] = (float4)(0.0f);
                vertexAtt[neigPoints[i]] = (float4)(0.0f);
                flag = false;
                break;

            }
        }
//        if (flag && (
//        att.y > 10 &&
//        multipleMatrix(*toL,pos).z < iPos.z)) // delete stable points in font of current observation
//        {
//            vertexPos[neigPoints[i]] = (float4)(0.0f); // fake delete
//            vertexNorm[neigPoints[i]] = (float4)(0.0f);
//            vertexAtt[neigPoints[i]] = (float4)(0.0f);
//        }
    }

}


//////////////////////////////////////////////////////////////////////////////////////////
//// depth map input converting kernel
//////////////////////////////////////////////////////////////////////////////
#define SQRT2 1.414213562373095f
#define DIVTERM 1.0f/(2.0f*0.6f*0.6f)
#define SIGMA 4.0f
#define BSIGMA 0.1f
#define MSIZE 5

#define CONSTANT_NUM 255.0f

constant const int kSize = (MSIZE-1)/2;

static float normpdf(float x, float sigma)
{
    return 0.39894f*exp(-0.5f*x*x/(sigma*sigma))/sigma;
}



static float2 filteredPos( int2 cid, float* gauss, __read_only image2d_t depthMap)
{
    float c = read_imagef(depthMap, sampler, cid).x;
    float2 final_colour = (float2)(0.0,1.0);
    float Z = 0.0;
    float cc;
    float factor;
    float bZ = 1.0/normpdf(0.0, BSIGMA);
    //read out the texels
    for (int i=-kSize; i <= kSize; ++i)
    {
        for (int j=-kSize; j <= kSize; ++j)
        {
            cc = read_imagef(depthMap, sampler, cid + (int2)(i,j)).x;
            factor = normpdf(cc-c, BSIGMA)*bZ*gauss[kSize+j]*gauss[kSize+i];
            Z += factor;
            final_colour.x += factor*cc;
            final_colour.y *= CONSTANT_NUM*cc;
        }
    }
    final_colour.x =final_colour.x / Z;
    return final_colour;
}

static float4 getPF(int2 ind, float* gauss, __read_only image2d_t depthMap,__global float16* inv_K) {
    float2 depth = CONSTANT_NUM*filteredPos(ind, gauss, depthMap);
    // calculate the pos in camera space
    float4 P = depth.x * multipleMatrix(*inv_K, (float4)(convert_float2(ind), 1.0,0.0));
    return (float4)(P.x,-P.y,-P.z,depth.y); // please pay attention to the coordinates
}

static float4 getP(int2 ind, __read_only image2d_t depthMap, __global float16* inv_K) {
    float depth = CONSTANT_NUM*read_imagef( depthMap, sampler, ind ).x;
    // calculate the pos in camera space
    float4 P = depth * multipleMatrix(*inv_K, (float4)(convert_float2(ind), 1.0,0.0));
    return (float4)(P.x,-P.y,-P.z,depth);// please pay attention to the coordinates(float4)(depth, inv_K[0].s012);// please pay attention to the coordinatesreturn
}

static float4 getN(int2 ind, __read_only image2d_t depthMap, __global float16* inv_K){
// calculate the normal in camera space, maybe use another render pass for efficiency
    int2 offx = (int2)(1, 0);
    int2 offy = (int2)(0, 1);
    float4 hL = getP( ind - offx, depthMap,inv_K);
    float4 hR = getP( ind + offx, depthMap,inv_K);
    float4 hD = getP( ind - offy, depthMap,inv_K);
    float4 hU = getP( ind + offy, depthMap,inv_K);

    float3 N = cross((hL.xyz - hR.xyz),( hU.xyz- hD.xyz));
    bool isvalid = hL.w>0.0f && hR.w>0.0f && hD.w >0.0f && hU.w>0.0f;
    N = normalize(N);

    return (float4)(N,convert_float(isvalid));
}


static float4 getNF(int2 ind, float* gauss,__read_only image2d_t depthMap,__global float16* inv_K){
// calculate the normal in camera space, maybe use another render pass for efficiency
    int2 offx = (int2)(1, 0);
    int2 offy = (int2)(0, 1);
    float4 hL = getPF( ind - offx, gauss, depthMap,inv_K);
    float4 hR = getPF( ind + offx, gauss, depthMap,inv_K);
    float4 hD = getPF( ind - offy, gauss, depthMap,inv_K);
    float4 hU = getPF( ind + offy, gauss, depthMap,inv_K);

    float3 N = cross((hL.xyz - hR.xyz),( hU.xyz - hD.xyz));
    bool isvalid = hL.w>0.0f && hR.w>0.0f && hD.w >0.0f && hU.w>0.0f;
    N = normalize(N);

    return (float4)(N,convert_float(isvalid));

}


__kernel void depthProcessing(__global float4* output1,
        __global float4* output2,
        __global float4* output3,
        __global float4* output4,
        __global float4* output5,
        __read_only image2d_t depthMap,
        float invF,
        __global float16* inv_K,
        __global int* numPoints)
{
//get our index in the array
const int2 sz = (int2)(get_global_size(0),get_global_size(1));
const int2 cid = {get_global_id(0), get_global_id(1)};


float gauss[MSIZE];
//create the 1-D kernel
for (int j = 0; j <= kSize; ++j)
{
    float tmp = normpdf((float)(j), SIGMA);;
    gauss[kSize+j] = tmp;
    gauss[kSize-j] = tmp;
}

// get depth
float depth = CONSTANT_NUM * read_imagef(depthMap, sampler, cid).x;
//vec2 indI = vec2(gl_TexCoord[0].x * WH.x,gl_TexCoord[0].y * WH.y);
float4 P = getPF(cid,gauss,depthMap,inv_K);
float4 PP = getP(cid,depthMap,inv_K);

float4 N = getNF(cid,gauss,depthMap,inv_K);
float4 NN = getN(cid,depthMap,inv_K);


int outID = cid.x+(sz.y - cid.y - 1 )*get_global_size(0);

if (
N.w <0.5
|| dot(N.xyz,NN.xyz) < 0.8
|| NN.w < 0.5
|| depth < 1.0
    || distance(P.xyz,PP.xyz) > 1.0
        )
{
output1[outID] = (float4)(0.0f);
output2[outID] = (float4)(0.0f);
output3[outID] = (float4)(0.0f,0.0f,convert_float(outID), 0.0f);
output4[outID] = (float4)(0.0f);
output5[outID] = (float4)(0.0f);
return;
}

// calculate the radius, confidence and the time stamp
float2 dis = 2.0f*convert_float2(cid)/convert_float2(sz) - (float2)(1.0,1.0);
float t =SQRT2*depth*invF/clamp(N.z,0.26f,1.0f);
float ldis = length(dis);
float tt = exp(-(ldis*ldis)*DIVTERM);
float4 D = (float4)(t, tt, depth,1.0f);



if (tt > 1.0f)
printf("Bigger!!! %f\n",tt);
//printf("%d\n",newid);

output1[outID] = (float4)(P.xyz,1.0);
output2[outID] = N;//(float4)(N.xyz,1.0);
output3[outID] = (float4)(D.xy,convert_float(outID), 0.0f);
output4[outID] = (float4)(PP.xyz,1.0);
output5[outID] = NN;

}
//////////////////////////////////////////////////////////////////////////////////////////
//// filter points to get stable index kernel
//////////////////////////////////////////////////////////////////////////////


__kernel void filterStable(__global float4* vertexPos,
        __global float4* vertexNorm,
        __global float4* vertexAtt,
        __global float4* vertexPos2,
        __global float4* vertexNorm2,
        __global float4* vertexAtt2,
        __global uint* stableIndex,
        __global int* numStablePoints,
        __global int* numNew,
        int confidence,
        int time){

const int id = get_global_id(0);

int nid = 0;
// remove unstable and deleted points
bool needDelete = ((vertexPos[id].w < 0.5f) || ((time - vertexAtt[id].w >= 30) && (vertexAtt[id].y < confidence)));//
//bool need = (vertexPos[id].w > 0.5f) && ((time - vertexAtt[id].w < 30)|| (vertexAtt[id].y > confidence));
if (!needDelete){
    nid = atomic_inc(numNew);
    vertexPos2[nid]  = vertexPos[id];
    vertexNorm2[nid] = vertexNorm[id];
    vertexAtt2[nid]  = vertexAtt[id];
    vertexAtt2[nid].z = convert_float(nid);

    if ( (time < 30) || (vertexAtt[id].y > confidence)) {
        int newid = atomic_inc(numStablePoints);
        stableIndex[newid] = nid;
    }

}


}


//////////////////////////////////////////////////////////////////////////////////////////
//// InnerLoop for ICP kernel
//////////////////////////////////////////////////////////////////////////////

static void computeLocalResult(float* lr, float3 A1, float3 A2, float b){
    // might use vector operation to speedup
//    int iter = 0;
//    float3 tmp = A1.x*A1;
//    lr[0] = tmp.x;
//    lr[1] = tmp.y;
//    lr[2] = tmp.z;
//    tmp = A1.x * A2;
//    lr[3] = tmp.x;
//    lr[4] = tmp.y;
//    lr[5] = tmp.z;
//
//    tmp = A1.y*A1;
//    lr[6] = tmp.y;
//    lr[7] = tmp.z;
//    tmp = A1.y*A2;
//    lr[8] = tmp.x;
//    lr[9] = tmp.y;
//    lr[10] = tmp.z;
//
//    lr[11] = A1.z * A1.z;
//    lr[12] = A1.z * A2.x;
//    lr[13] = A1.z * A2.y;
//    lr[14] = A1.z * A2.z;
//
//    lr[15] = A2.x * A2.x;
//    lr[16] = A2.y * A2.x;
//    lr[17] = A2.z * A2.x;
//
//    lr[18] = A2.y * A2.y;
//    lr[19] = A2.z * A2.y;
//
//    lr[20] = A2.z * A2.z;
//
//    tmp = b*A1;
//    lr[21] = tmp.x;
//    lr[22] = tmp.y;
//    lr[23] = tmp.z;
//    tmp = b*A2;
//    lr[24] = tmp.x;
//    lr[25] = tmp.y;
//    lr[26] = tmp.z;


    lr[0] = A1.x * A1.x;
    lr[1] = A1.x * A1.y;
    lr[2] = A1.x * A1.z;
    lr[3] = A1.x * A2.x;
    lr[4] = A1.x * A2.y;
    lr[5] = A1.x * A2.z;

    lr[6] = A1.y * A1.y;
    lr[7] = A1.y * A1.z;
    lr[8] = A1.y * A2.x;
    lr[9] = A1.y * A2.y;
    lr[10] = A1.y * A2.z;

    lr[11] = A1.z * A1.z;
    lr[12] = A1.z * A2.x;
    lr[13] = A1.z * A2.y;
    lr[14] = A1.z * A2.z;

    lr[15] = A2.x * A2.x;
    lr[16] = A2.y * A2.x;
    lr[17] = A2.z * A2.x;

    lr[18] = A2.y * A2.y;
    lr[19] = A2.z * A2.y;

    lr[20] = A2.z * A2.z;

    lr[21] = b * A1.x;
    lr[22] = b * A1.y;
    lr[23] = b * A1.z;
    lr[24] = b * A2.x;
    lr[25] = b * A2.y;
    lr[26] = b * A2.z;
}


static void computeLocalResult2(float* lr, float4* J, float4 b){
    // might use vector operation to speedup

    lr[0] = dot(J[0], J[0]);
    lr[1] = dot(J[0], J[1]);
    lr[2] = dot(J[0], J[2]);
    lr[3] = dot(J[0], J[3]);
    lr[4] = dot(J[0], J[4]);
    lr[5] = dot(J[0], J[5]);

    lr[6] = dot(J[1], J[1]);
    lr[7] = dot(J[1], J[2]);
    lr[8] = dot(J[1], J[3]);
    lr[9] = dot(J[1], J[4]);
    lr[10] = dot(J[1], J[5]);

    lr[11] = dot(J[2], J[2]);
    lr[12] = dot(J[2], J[3]);
    lr[13] = dot(J[2], J[4]);
    lr[14] = dot(J[2], J[5]);

    lr[15] = dot(J[3], J[3]);
    lr[16] = dot(J[3], J[4]);
    lr[17] = dot(J[3], J[5]);

    lr[18] = dot(J[4], J[4]);
    lr[19] = dot(J[4], J[5]);

    lr[20] = dot(J[5], J[5]);

    lr[21] = dot(b, J[0]);
    lr[22] = dot(b, J[1]);
    lr[23] = dot(b, J[2]);
    lr[24] = dot(b, J[3]);
    lr[25] = dot(b, J[4]);
    lr[26] = dot(b, J[5]);
}


__kernel void icpInner(__read_only image2d_t posM,
                           __read_only image2d_t normM,
                           __read_only image2d_t posI,
                           __read_only image2d_t normI,
                            __global float* results,
                            int num,
                       __local float* sdata
                        ){
const int2 cid = {get_global_id(0), get_global_id(1)};
int id = cid.x+cid.y*get_global_size(0);

float4 pM = read_imagef(posM, sampler, cid);
float4 pI = read_imagef(posI, sampler, cid);

float lr[27];
for (int i=0;i<27;++i) lr[i]=0.0f;
for(int forbreak = 0; forbreak<1;++forbreak) {
    if (pM.w < 0.5f || pI.w < 0.5f) break;

    float4
    D = (pM - pI);
    if (length(D.xyz) > 5.0f) break;

    float4
    nM = read_imagef(normM, sampler, cid);
    float4
    nI = read_imagef(normI, sampler, cid);

    if (dot(nI, nM) < 0.7f) break;

    float b = dot(nM, D);

    float4 F = -(float4)(b,D.x,D.y,D.z);

    float3 A1 = nM.xyz;
    float3 A2 = cross(pI.xyz, nM.xyz);


    float4 J[6];
    float lambda = 0.0; float4 ppI = lambda*pM;
//    J[0] = (float4)(A1.x, lambda, 0.0, 0.0);
//    J[1] = (float4)(A1.y, 0.0, lambda, 0.0);
//    J[2] = (float4)(A1.z, 0.0, 0.0, lambda);
//    J[3] = (float4)(A2.x, 0.0, -ppI.z, ppI.y);
//    J[4] = (float4)(A2.y, ppI.z, 0.0, -ppI.x);
//    J[5] = (float4)(A2.z, -ppI.y, ppI.x, 0.0);

// try quaternion
    J[0] = (float4)(A1.x, lambda, 0.0, 0.0);
    J[1] = (float4)(A1.y, 0.0, lambda, 0.0);
    J[2] = (float4)(A1.z, 0.0, 0.0, lambda);
    J[3] = (float4)(A2.x, 0.0, ppI.z, -ppI.y);
    J[4] = (float4)(A2.y, -ppI.z, 0.0, ppI.x);
    J[5] = (float4)(A2.z, ppI.y, -ppI.x, 0.0);

       computeLocalResult2(lr,J,F);
//    computeLocalResult(lr,A1,A2,b);
}

unsigned int tid = get_local_id(1)*get_local_size(0) + get_local_id(0);

int lid = tid*27;
for (int i = 0; i <27; i+=3){
    sdata[lid + i ] = lr[i];
    sdata[lid + i +1] = lr[i+1];
    sdata[lid + i +2] = lr[i+2];
}


barrier(CLK_LOCAL_MEM_FENCE);

// do reduction in shared mem
for(unsigned int s=get_local_size(0)*get_local_size(1)/2; s>0; s>>=1)
{
if (tid < s)
{
    for (int i=0; i < 27; i+=3)
    {
        sdata[lid + i] += sdata[lid + s *27 + i];
        sdata[lid + i+1] += sdata[lid + s *27 + i+1];
        sdata[lid + i+2] += sdata[lid + s *27 + i+2];
    }

}
barrier(CLK_LOCAL_MEM_FENCE);
}

// write result for this block to global mem
if (tid == 0) {
    int iid = (get_group_id(0)+get_group_id(1)*get_num_groups(0))*27;
    for (int i=0;i<27;i+=3) {
        results[iid + i] = sdata[i];
        results[iid + i+1] = sdata[i+1];
        results[iid + i+2] = sdata[i+2];
   }

}


}

__kernel void procrustes(__global float4* posL,
                        __global float4* posC,
                       __global float4* matchIdx,
        __global float4* dataA,
        __global float4* dataB
) {
    const int cid = get_global_id(0);

    float4 indexMatch = matchIdx[cid];

    int4 index = (int4)(indexMatch.x +0.5,indexMatch.y+0.5,indexMatch.z+0.5,indexMatch.w+0.5);

    int indexy = 479 - index.y;
    int indexw = 479 - index.w;

    int id = (int)(index.x+indexy*640);
    int id2 = (int)(index.z+indexw*640);
//    id2=id+1;
    float4 pL = posL[id];
    float4 pC = posC[id2];

    float4 d = pC - pL;
    //printf("Index: %d,%d,%d,%d\nPL: %f,%f,%f,%f\nPC: %f,%f,%f,%f\n", index.x, index.y, index.z, index.w, pL.x, pL.y, pL.z, pL.w, pC.x, pC.y, pC.z, pC.w);
    // || length(d) >2.0
    if (pL.w<0.5f || pC.w<0.5f){
    d = (float4)(0.0, 0.0, 0.0, -1.0);
    }
    else
    {

        d.w = 1.0;
    }
    matchIdx[cid] = d;
    dataA[cid] = pL;
    dataB[cid] = pC;

}