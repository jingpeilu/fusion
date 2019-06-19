clear all;
clc
%% Quaternion version of our energy function
n_m = sym('n_model%d', [3 1],'real'); % Normal of the point
v_i = sym('v_input%d', [3 1],'real'); % position of the point
v_m = sym('v_model%d', [3 1],'real'); % position of the point

q = sym('q%d', [4 1],'real'); % quaternion of the rotation v = {q1,q2,q3}, theta = q4
t = sym('t%d', [3 1],'real'); % translational parameters

qx = skewMat(q); % for cross product and skew in derivative

R = eye(3) + 2*q(4)*qx + 2*qx*qx; % Rodriguesz's formula for rotation matrix


L_1 = n_m'*(R*v_i + t - v_m); % point-plane data term

L_2 = norm(R*v_i +t -v_m);

J1_p = jacobian(L_1,q)'

J1_t = jacobian(L_1,t)'

J2_p = jacobian(L_2,q)'

J2_t = jacobian(L_2,t)'

%% Rotation matrix version of our energy function
R = sym('R', [3 3],'real'); % directly use Rotation matrix

L_1 = n_m'*(R*v_i + t - v_m); % point-plane data term

L_2 = norm(R*v_i +t -v_m,2); % the directly difference term

%L3 and L4 are rigid transform regularization term
L_3 = norm( R'*R - eye(3),'fro'); % encourage to have R'R = I

L_4 = det(R) - 1; % encourage det(R) = 1 which is rotation matrix property

J1_p = jacobian(L_1,R(:))';

J1_t = jacobian(L_1,t)';

J2_p = jacobian(L_2,R(:))';

J2_t = jacobian(L_2,t)';

J3_p = jacobian(L_3,R(:))';

J3_p = simplify(J3_p);

J3_t = jacobian(L_3,t)';

J4_p = jacobian(L_4,R(:))';

J4_t = jacobian(L_4,t)';


function [mat] = skewMat(q)
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here
mat = [0 -q(3) q(2);q(3) 0 -q(1);-q(2) q(1) 0];
end

