#version 120

varying vec4 vNormal;
varying vec4 vPos;
varying vec3 vAtt;

//varying vec3 vValid;

#define A_P 1.000100010001    //f/(f-n)
#define B_P -0.2000200020002  // -2fn/(f-n) in Eq.4 of paper "High quality point based rendering on modern GPU"


void main()
{
//    if (vValid.y < 0.5 ||vValid.x < 0.5 || vValid.x!=vValid.x )//||vValid.x!=vValid.x
//        discard;
    vec3 nn = vNormal.xyz;

    vec2 p = (gl_PointCoord.xy - 0.5)*2.0;
    p.y = -p.y;// y here is minus because the texture coordinates

    float deltaZ = min(0.75,(-p.x*nn.x - p.y*nn.y)/nn.z);
    float l = length(vec3(p.x,p.y,deltaZ));

    float eyeZ=B_P/(A_P-gl_FragCoord.z);
    float z_win = A_P * (eyeZ + deltaZ) + B_P;

    gl_FragDepth = gl_FragCoord.z;

    float backface = dot(nn,vPos.xyz);

    if (backface >= 0.0 || l >1.0)//
    {
        discard;
    }
    else
    {
      z_win = (eyeZ + deltaZ) / z_win ;
      gl_FragDepth = z_win;//gl_FragCoord.z;//min(z_win,0.9);//
    }



    gl_FragData[0] = vec4(vPos.xyz,1.0);
    //gl_FragData[1] = vec4(l,l,l,1.0);
    gl_FragData[1] = vec4(normalize(vNormal.xyz),0.0);


}