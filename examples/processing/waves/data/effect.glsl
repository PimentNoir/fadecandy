#define PROCESSING_COLOR_SHADER

uniform float time, hue;
uniform vec2 resolution;

vec3 hsv2rgb(vec3 c)
{
  // http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main(void)
{
  // Distance-squared to screen center
  vec2 centerDist = gl_FragCoord.xy - resolution.xy * 0.5;
  float d = dot(centerDist, centerDist);

  // Interpolation endpoints in HSV
  vec3 color1 = vec3(hue, 0.25, 0.0);
  vec3 color2 = vec3(hue, 0.25, 0.75);

  // Mixing factor
  float m = 0.5 * (1.0 + sin(d * 1e-4 - time * 1.2));

  // Linear interpolation in HSV space
  gl_FragColor = vec4(hsv2rgb(mix(color1, color2, m)), 1.0);
}