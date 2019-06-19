#version 120

varying vec4 vNormal;
varying vec4 vPos;
varying vec3 vAtt;

void main()
{
    float backface = dot(vNormal,vPos);

    if (backface >= 0.0 )//
    {
        discard;

    }

    gl_FragData[0] = vec4(vPos.xyz,vAtt.z);
//    gl_FragData[1] = vec4(normalize(vNormal).xyz,vAtt.y);

}