#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec3 in_pos;

layout (location = 0) out vec4 out_color;

layout (binding = 2) uniform samplerCube environment_map;

const float PI = 3.14159265359;

void main() {
    vec3 N = normalize(in_pos);

    vec3 irradiance = vec3(0.0);
    
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, N);
    up = cross(N, right);
    
    float sample_delta = 0.025;
    float num_samples = 0.0f;

    for (float phi = 0.0; phi < 2.0 * PI; phi += sample_delta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sample_delta) {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

            irradiance += texture(environment_map, sampleVec).rgb * cos(theta) * sin(theta);
            num_samples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(num_samples));
    
    out_color = vec4(irradiance, 1.0);
}