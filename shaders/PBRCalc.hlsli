static const float PI = 3.14159265359;
static const float InvPI = 0.31830988618379067153776752674503f;
static const float Inv2PI = 0.15915494309189533576888376337251f;
static const float2 InvAtan = float2(Inv2PI, InvPI);
// ----------------------------------------------------------------------------
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
float3 UvToVec(float u, float v)
{
	float x = cos(2.0 * PI * (0.5 - u)) * sin(PI * v);
	float y = sin(2.0 * PI * (0.5 - u)) * sin(PI * v);
	float z = cos(PI * v);
	return float3(x, y, z);
}
// ----------------------------------------------------------------------------

float2 VecToUv(float3 dir)
{
	float u = 0.5 - atan2(-dir.x, dir.z) / (2 * PI);
	float v = acos(dir.y) / PI;
	return float2(u, v);
}
// ----------------------------------------------------------------------------
float3 SampleRandomVectorGGX(float u, float v, float roughness)
{
	float nom = roughness * sqrt(v);
	float dnom = sqrt(1 - v);
	float theta = atan(nom/dnom);
	float2 newUV = float2(u, theta);
	return normalize(UvToVec(newUV.x, newUV.y / PI));
}
// ----------------------------------------------------------------------------
float3 RotateZaxisToLightAxis(float3 zSampled, float3 reflected)
{
	reflected = normalize(reflected);
	float3 A = normalize(float3(reflected.z, 0, -reflected.x));
	float3 B = normalize(cross(reflected, A));
	return normalize(zSampled.x * A + zSampled.y * B + zSampled.z * reflected);
}
// ----------------------------------------------------------------------------

float3 ToneMapping(float3 HDRColor, float exposure)
{
    const float gamma = 2.2;
    float3 mapped = float3(1.0, 1.0, 1.0) - exp(-HDRColor * exposure);
    float inverseGamma = 1.0 / gamma;
    mapped = pow(mapped, float3(inverseGamma, inverseGamma, inverseGamma));
    return mapped;
}
// ----------------------------------------------------------------------------
float3 LightEquation(float3 viewDir, float3 normal, float3 fragmentPos, 
    float3 lightPos, float3 lightColor, 
    float3 albedo, float roughness, float metalic)
{
    float3 F0 = float3(0.04, 0.04, 0.04); 
    F0 = lerp(F0, albedo, metalic);

    float3 lightDir = normalize(lightPos - fragmentPos);
    float3 halfVector = normalize(viewDir + lightDir);
    float distance = length(lightPos - fragmentPos);
    float attenuation = 1.0 / (distance * distance);
    float3 radiance = lightColor * attenuation;

    float NDF = DistributionGGX(normal, halfVector, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    float3 F = fresnelSchlick(clamp(dot(halfVector, viewDir), 0.0, 1.0), F0);

    float3 KS = F;
    float3 KD = float3(1.0, 1.0, 1.0) - KS;
    KD *= 1.0 - metalic;

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
    float3 specular = numerator / denominator;

    float NdotL = max(dot(normal, lightDir), 0.0);

    return (KD * albedo / PI + specular) * radiance * NdotL;
}

float3 IBLSpecular(float3 viewDir, float3 normal, float3 lightDir,
    float3 lightColor, float3 albedo, float roughness, float metalic)
{
    float3 F0 = float3(0.04, 0.04, 0.04); 
    F0 = lerp(F0, albedo, metalic);

    float3 H = normalize(viewDir + lightDir);
	float NdotL = max(dot(normal, lightDir), 0.0);

	float G = GeometrySmith(normal, viewDir, lightDir, roughness);
	float3 F = fresnelSchlick(clamp(dot(H, viewDir), 0.0, 1.0), F0);

	float3 nom = G * F;
	float denom = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
		
	return lightColor * NdotL * (nom / denom);
}