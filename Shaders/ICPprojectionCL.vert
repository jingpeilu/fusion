#version 120
uniform float ProjParam;
attribute vec4 p_attri;
attribute vec4 p_normal;
uniform mat4 u_projViewModel;
uniform mat4 u_cameraPos;
varying vec4 vNormal;
varying vec4 vPos;
varying vec3 vAtt;

void main() {

    gl_Position = u_projViewModel * vec4(gl_Vertex.xyz,1.0f);
    gl_FrontColor = u_cameraPos*vec4(gl_Vertex.xyz,1.0f);
    vPos = u_cameraPos*vec4(gl_Vertex.xyz,1.0f);
    vNormal = u_cameraPos* vec4(p_normal.xyz,0.0);

    vAtt = p_attri.xyz;
    gl_PointSize = max(1,vAtt.x * ProjParam / (length(gl_FrontColor)));
}