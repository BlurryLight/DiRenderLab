#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D ssaoTexInput;
uniform int uBlurSize = 4; // use size of noise texture

out float fResult;

void main() {
   vec2 texelSize = 1.0 / vec2(textureSize(ssaoTexInput, 0));
   float result = 0.0;
   vec2 hlim = vec2(float(-uBlurSize) * 0.5 + 0.5);
   for (int i = 0; i < uBlurSize; ++i) {
      for (int j = 0; j < uBlurSize; ++j) {
         vec2 offset = (hlim + vec2(float(i), float(j))) * texelSize;
         result += texture(ssaoTexInput, TexCoords + offset).r;
      }
   }

   fResult = result / float(uBlurSize * uBlurSize);
}
