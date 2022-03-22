#include "Utils/Math/MathHelpers.slang"

//import Utils.Math.MathHelpers;

struct EnvMapLighting
{
    Texture2D dfgTexture;       ///< Texture containing shared pre-integrated (DFG) term. The u-axis is NdotV=cos(theta) and the v-axis is GGX alpha, both in [0,1].
    SamplerState dfgSampler;    ///< Sampler for the DFG texture: point filtering with clamp wrap mode.

    Texture2D diffuseTexture;   ///< Texture containing pre-integrated diffuse (LD) term
    Texture2D specularTexture;  ///< Texture containing pre-integrated specular (LD) term
    SamplerState sampler;

    float gIntensity;
    float3x4 gInvTransform;

    float linearRoughnessToLod(float linearRoughness, float mipCount)
    {
        return sqrt(linearRoughness) * (mipCount - 1);
    }

    float3 getDiffuseDominantDir(float3 N, float3 V, float ggxAlpha)
    {
        float a = 1.02341 * ggxAlpha - 1.51174;
        float b = -0.511705 * ggxAlpha + 0.755868;
        float factor = saturate((saturate(dot(N, V)) * a + b) * ggxAlpha);
        return normalize(lerp(N, V, factor));
    }

    float3 getSpecularDominantDir(float3 N, float3 R, float ggxAlpha)
    {
        float smoothness = 1 - ggxAlpha;
        float factor = smoothness * (sqrt(smoothness) + ggxAlpha);
        return normalize(lerp(N, R, factor));
    }

    float3 evalDiffuse(float3 N,float3 V,float ggxAlpha ,float NdotV,float3 diffuse)
    {
        float3 dominantDir = getDiffuseDominantDir(N, V, ggxAlpha);
        //float2 uv = world_to_latlong_map(dominantDir);
        float2 uv = world_to_latlong_map(mul(dominantDir, (float3x3) gInvTransform));

        float width, height, mipCount;
        diffuseTexture.GetDimensions(0, width, height, mipCount);

        float3 diffuseLighting = diffuseTexture.SampleLevel(sampler, uv, 0).rgb;
        float preintegratedDisneyBRDF = dfgTexture.SampleLevel(dfgSampler, float2(NdotV, ggxAlpha), 0).z;

        return diffuseLighting * preintegratedDisneyBRDF * diffuse.rgb * gIntensity;
    }

    float3 evalSpecular(float3 N, float3 V, float ggxAlpha, float NdotV, float3 specular, float3 L)
    {
        float dfgWidth, dfgHeight;
        dfgTexture.GetDimensions(dfgWidth, dfgHeight);

        float width, height, mipCount;
        specularTexture.GetDimensions(0, width, height, mipCount);

        float3 dominantDir = getSpecularDominantDir(N, L, ggxAlpha);
        //float2 uv = world_to_latlong_map(dominantDir);
        float2 uv = world_to_latlong_map(mul(dominantDir, (float3x3) gInvTransform));

        float mipLevel = linearRoughnessToLod(ggxAlpha, mipCount);

        float3 ld = specularTexture.SampleLevel(sampler, uv, mipLevel).rgb;

        float2 dfg = dfgTexture.SampleLevel(dfgSampler, float2(NdotV, ggxAlpha), 0).xy;

        // ld * (f0 * Gv * (1 - Fc)) + (f90 * Gv * Fc)
        return ld * (specular * dfg.x + dfg.y) * gIntensity;
    }
};
