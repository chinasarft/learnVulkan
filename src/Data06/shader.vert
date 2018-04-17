// Copyright 2016 Intel Corporation All Rights Reserved
// 
// Intel makes no representations about the suitability of this software for any purpose.
// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,
// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY
// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// Intel does not assume any responsibility for any errors which may appear in this software
// nor any responsibility to update it.

#version 450

layout(location = 0) in vec2 i_Position;
layout(location = 1) in vec2 i_Texcoord;

//layout(binding = 0) uniform MVP{
//    mat4 model;
//    mat4 view;
//    mat4 proj;
//} mvp;

out gl_PerVertex
{
  vec4 gl_Position;
};

layout(location = 0) out vec2 v_Texcoord;

void main() {
    //gl_Position = mvp.proj * mvp.view * mvp.model * vec4(i_Position, 0.0, 1.0);
    gl_Position = vec4(i_Position, 0.0, 1.0);
    v_Texcoord = i_Texcoord;
}
