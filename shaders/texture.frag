#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

void main() {
    // Инвертируем Y, так как Vulkan имеет начало координат в левом верхнем углу,
    // а текстуры загружаются с началом в левом нижнем
    outColor = texture(texSampler, vec2(fragTexCoord.x, 1.0 - fragTexCoord.y));
}
