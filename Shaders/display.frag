#version 120

varying vec3 vAtt;
varying vec3 vFragPos_worldspace;
varying vec3 vFragPos_cameraspace;

varying vec3 Normal_cameraspace;
varying vec3 Normal_worldspace;


#define A_P 1.000100010001    //f/(f-n)
#define B_P -0.2000200020002  // -2fn/(f-n) in Eq.4 of paper "High quality point based rendering on modern GPU"

void main()
{
    float   ambientStrength = 0.1f;
    vec3 lightC = vec3(1,1,1);
    vec3 lightPos = vec3(0,10,0);

    vec3    ambient = ambientStrength * lightC * gl_Color.xyz;
    vec3    lightDir = normalize(lightPos - vFragPos_worldspace);
    vec3    normal = normalize(Normal_worldspace);

    float   diffFactor = max(dot(lightDir, normal), 0.0);
    vec3    diffuse = diffFactor * lightC * gl_Color.xyz;

    vec3 result = ambient + diffuse;

    // Normal of the computed fragment, in camera space
    vec3 n = (normalize( Normal_worldspace ) + 1.0)/2.0;
    vec3 nn = (normalize( Normal_cameraspace ) );//+ 1.0)/2.0;


    // should be same as icp project fs

    vec2 p = (gl_PointCoord.xy - 0.5)*2.0;
    p.y = -p.y;// y here is minus because the texture coordinates

    vec3 nnn = nn;
    nnn.z = clamp(nnn.z,0.26f,1.0f);

    float deltaZ = (-p.x*nnn.x - p.y*nnn.y)/nnn.z;
    float l = length(vec3(p.x,p.y,deltaZ));//

    float backface = dot(nn,vFragPos_cameraspace);

    if (backface >= 0.0 || l >1.0)//
    {
        discard;

    }
    else
    {
    float z_win = A_P * (vFragPos_cameraspace.z + deltaZ) + B_P;
    z_win = (vFragPos_cameraspace.z + deltaZ) / z_win ;
    gl_FragDepth = z_win;//gl_FragCoord.z;//
    }

    gl_FragColor = vec4(result.xyz,1.0);

    //gl_FragColor = vec4(vGlobalNormal/2+0.5, 1.0);//a
    //gl_FragColor = vec4((nn+1.0)/2.0,1.0);

}