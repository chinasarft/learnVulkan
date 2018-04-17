#version 450

layout(set=0, binding=0) uniform sampler2D u_Texture;

layout(location = 0) in vec2 v_Texcoord;

layout(location = 0) out vec4 o_Color;
const vec2 TexSize=vec2(400, 400);
  
void main()                 
{                             
  vec2 tex =v_Texcoord;   
  vec2 upLeftUV = vec2(tex.x-1.0/TexSize.x,tex.y-1.0/TexSize.y);           
  vec4 curColor = texture(u_Texture,v_Texcoord);                           
  vec4 upLeftColor = texture(u_Texture,upLeftUV);                  
  vec4 delColor = curColor - upLeftColor;                           
  float h = 0.3*delColor.x + 0.59*delColor.y + 0.11*delColor.z;                  
  vec4 bkColor = vec4(0.5, 0.5, 0.5, 1.0);                   
  o_Color = vec4(h,h,h,0.0) +bkColor;                             
}  
