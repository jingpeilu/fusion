#version 120
attribute vec3 p_attri;
uniform float ProjParam;
varying vec3 vAtt;
varying vec3 vFragPos_worldspace;
varying vec3 vFragPos_cameraspace;
varying vec3 Normal_cameraspace;
varying vec3 Normal_worldspace;
void main() {
    gl_Position = gl_ModelViewProjectionMatrix * (gl_Vertex);//
    Normal_worldspace = gl_Normal;
    Normal_cameraspace = (gl_ModelViewMatrix * vec4(gl_Normal,0)).xyz;
    vec4 tmp = gl_ModelViewMatrix *gl_Vertex;
    vFragPos_cameraspace = tmp.xyz;
    vFragPos_worldspace = gl_Vertex.xyz;
    gl_FrontColor = gl_Color;
    vAtt = p_attri.xyz;
    gl_PointSize = floor(max(1,p_attri.x * ProjParam/ (length(tmp))));//240

}