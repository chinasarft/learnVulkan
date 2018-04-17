#version 450

layout(set=0, binding=0) uniform sampler2D u_Texture;
layout(location = 0) in vec2 v_Texcoord;
layout(location = 0) out vec4 o_Color;

const float PI = 3.14159265;
const float uR = 0.3;

void main(void)
{
   ivec2 ires = textureSize(u_Texture, 0);
   
   float Res = float(ires.s);
   
   //vec2 st = v_Texcoord.xy/ires;
   vec2 st = v_Texcoord.xy;
   float Radius = Res * uR;
   // pixel coordinates from texture coords
   vec2 xy = Res * st;
   
   // twirl center is (Res/2, Res/2)
   vec2 dxy = xy - vec2(Res/2., Res/2.);
   float r = length(dxy);

   vec4 color = vec4(0.);
   if(r<=Radius)
   {
      float angle = atan(dxy.y, dxy.x);
      int num = 40;
      for(int i=0; i<num; i++)
      {
         float tmpR = (r-i)>0 ?(r-i):0;
         
         float newX = tmpR*cos(angle) + Res/2;  
         float newY = tmpR*sin(angle) + Res/2;  
                     
         if(newX<0)
            newX=0;  
         if(newX>Res-1)
            newX=Res-1;  
         if(newY<0)
            newY=0;  
         if(newY>Res-1)
            newY=Res-1;  
         
         color += texture(u_Texture, vec2(newX, newY)/ires);
      }
      
      color /= num;
   }
   else
   {
      color = texture(u_Texture, st);
   }
 
   o_Color = color;
}
