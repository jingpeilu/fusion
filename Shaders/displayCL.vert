#version 120
attribute vec4 p_attri;
attribute vec4 p_normal;
uniform float ProjParam;
varying vec3 vAtt;
varying vec3 vFragPos_worldspace;
varying vec3 vFragPos_cameraspace;
varying vec3 Normal_cameraspace;
varying vec3 Normal_worldspace;
void main() {
    gl_Position = gl_ModelViewProjectionMatrix * (gl_Vertex);//
    Normal_worldspace = p_normal.xyz;
    Normal_cameraspace = (gl_ModelViewMatrix * vec4(p_normal.xyz,0)).xyz;
    vec4 tmp = gl_ModelViewMatrix *gl_Vertex;
    vFragPos_cameraspace = tmp.xyz;
    vFragPos_worldspace = gl_Vertex.xyz;
    gl_FrontColor = gl_Color;
    vAtt = p_attri.xyz;
    gl_PointSize = floor(max(1.0f,p_attri.x * ProjParam/ (length(tmp))));//240

}