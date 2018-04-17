#version 450

layout(set=0, binding=0) uniform sampler2D u_Texture;
layout(location = 0) in vec2 v_Texcoord;
layout(location = 0) out vec4 o_Color;

const vec2 iResolution = vec2(1024, 683);
const bool flip = false;
const vec2 direction = vec2(0, 5.9);

vec4 blur(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
  vec4 color = vec4(0.0);
  vec2 off1 = vec2(1.3846153846) * direction;
  vec2 off2 = vec2(3.2307692308) * direction;
  color += texture(image, uv) * 0.2270270270;
  color += texture(image, uv + (off1 / resolution)) * 0.3162162162;
  color += texture(image, uv - (off1 / resolution)) * 0.3162162162;
  color += texture(image, uv + (off2 / resolution)) * 0.0702702703;
  color += texture(image, uv - (off2 / resolution)) * 0.0702702703;
  return color;
}

void main() {
  vec2 uv = vec2(v_Texcoord.xy);
  if (flip) {
    uv.y = 1.0 - uv.y;
  }

  o_Color = blur(u_Texture, uv, iResolution.xy, direction);
}
