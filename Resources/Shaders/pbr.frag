#version 450
#pragma shader_stage(fragment)

#define SHADOW_MAP_CASCADE_COUNT 4

layout (location = 0) in vec2 in_uv;
layout (location = 1) in vec3 in_pos;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_mv_pos;

layout (location = 0) out vec4 out_color;

// material parameters
layout (set = 2, binding = 0) uniform sampler2D albedo_map;
layout (set = 2, binding = 1) uniform sampler2D normal_map;
layout (set = 2, binding = 2) uniform sampler2D metallic_map;
layout (set = 2, binding = 3) uniform sampler2D roughness_map;
layout (set = 2, binding = 4) uniform sampler2D ao_map;

// IBL
layout (set = 0, binding = 1) uniform samplerCube irradiance_map;
layout (set = 0, binding = 2) uniform samplerCube prefilter_map;
layout (set = 0, binding = 3) uniform sampler2D brdf_lut;
layout (set = 0, binding = 4) uniform sampler2DArray shadow_map;

// cool FX
layout (set = 0, binding = 5) uniform sampler2D g_ssao;

layout(set = 0, binding = 0) uniform Uniforms {
    mat4 projection;
    mat4 view;
} uniforms;

layout (set = 0, binding = 6) uniform Cascades {
    // i can get away with vec4 because float[SHADOW_MAP_CASCADE_COUNT] -> float[4] -> vec4 and thus i don't have to deal with alignment
    vec4 cascade_splits;
    mat4 cascade_view_proj_mats[SHADOW_MAP_CASCADE_COUNT];
    vec3 light_direction;
};

// the sun :O
const vec3 lightColors[1] = vec3[1](
    vec3(1000, 1000, 1000)
);

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

layout (push_constant) uniform PushConstants {
    vec3 cam_pos;
    float _pad;
    vec2 screen_size;
};

// the rest of this is a slightly modified version of LearnOpenGL's PBR shader

const float PI = 3.14159265359;

vec3 get_normal_from_map() {
    vec3 tangent_normal = texture(normal_map, in_uv).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(in_pos);
    vec3 Q2  = dFdy(in_pos);
    vec2 st1 = dFdx(in_uv);
    vec2 st2 = dFdy(in_uv);

    vec3 N   = normalize(in_normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangent_normal);
}

float distribution_ggx(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnel_schlick_roughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float texture_proj(vec4 shadowCoord, vec2 offset, uint cascadeIndex, vec3 N) {
	float shadow = 1.0;
    float bias = 0.005 * tan(acos(clamp(dot(N, normalize(-light_direction)), 0, 1)));

	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) {
		float dist = texture(shadow_map, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
			shadow = 0; // ambient light
		}
	}

	return shadow;
}

// shadows :)
float shadow_calculation(vec3 N) {
    uint cascadeIndex = 0;
    for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i) {
        if (in_mv_pos.z < cascade_splits[i]) {
            cascadeIndex = i + 1;
        }
    }

    vec4 sc = (biasMat * cascade_view_proj_mats[cascadeIndex]) * vec4(in_pos + in_normal * 0.1, 1.0);
    sc = sc / sc.w;

	ivec2 texDim = textureSize(shadow_map, 0).xy;
	float scale = 0.75;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 8;
	
	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			shadowFactor += texture_proj(sc, vec2(dx*x, dy*y), cascadeIndex, N);
			count++;
		}
	}

	return shadowFactor / count;
}

// thanks vinc

vec3 uncharted2_tonemap_partial(vec3 x) {
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 uncharted2_filmic(vec3 v) {
    float exposure_bias = 2.0f;
    vec3 curr = uncharted2_tonemap_partial(v * exposure_bias);

    vec3 W = vec3(11.2f);
    vec3 white_scale = vec3(1.0f) / uncharted2_tonemap_partial(W);
    return curr * white_scale;
}

void main() {
    // material properties
    vec3 albedo = pow(texture(albedo_map, in_uv).rgb, vec3(2.2));
    float metallic = texture(metallic_map, in_uv).r;
    float roughness = texture(roughness_map, in_uv).r;
    float ao = texture(ao_map, in_uv).r;

    vec2 screen_uv = gl_FragCoord.xy / vec2(screen_size.x, screen_size.y);
    //screen_uv.y = 1 - screen_uv.y;

    float ssao = texture(g_ssao, screen_uv).r;

    // input lighting data
    vec3 N = get_normal_from_map();
    vec3 V = normalize(cam_pos - in_pos);
    vec3 R = reflect(-V, N);

    float shadow = shadow_calculation(N);
    shadow = clamp(shadow, 0, 1);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);

    // for a single directional light:
    {
        vec3 L = normalize(-light_direction); // light direction
        vec3 H = normalize(V + L);
        float distance = length(light_direction - in_pos);
        vec3 radiance = lightColors[0]; // no attenuation for directional lights

        // Cook-Torrance BRDF
        float NDF = distribution_ggx(N, H, roughness);
        float G   = geometry_smith(N, V, L, roughness);
        vec3 F    = fresnel_schlick(max(dot(H, V), 0.0), F0);
            
        vec3 nominator    = NDF * G * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // 0.001 to prevent divide by zero.
        vec3 specular = nominator / denominator;
            
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	                
                
        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }
    
    // ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnel_schlick_roughness(max(dot(N, V), 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    
    vec3 irradiance = texture(irradiance_map, N).rgb;
    vec3 diffuse = irradiance * albedo;
    
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilter_map, R, roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdf_lut, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    
    vec3 color = ambient * ssao + Lo * shadow;

    shadow = clamp(shadow, 0.2, 1);

    color.rgb *= shadow;

    // HDR tonemapping
    color = uncharted2_filmic(color);
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    out_color = vec4(color, 1.0);

}