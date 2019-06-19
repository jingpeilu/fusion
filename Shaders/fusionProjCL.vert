#version 120
attribute vec4 p_attri;
attribute vec4 p_normal;
uniform mat4 u_projViewModel;
uniform mat4 u_cameraPos;
varying vec4 vNormal;
varying vec4 vPos;
varying vec3 vAtt;
void main() {
    gl_Position = u_projViewModel * gl_Vertex;
    gl_FrontColor = u_cameraPos*gl_Vertex;
    vPos = u_cameraPos*gl_Vertex;
    vNormal = u_cameraPos* vec4(p_normal.xyz,0.0);
    vAtt = p_attri.xyz;
    gl_PointSize = 1;
}