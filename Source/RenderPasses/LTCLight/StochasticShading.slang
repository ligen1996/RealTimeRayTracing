#include "SData.slangh"

#define PI 3.14159265

struct GPURay
{
    float3 origin;
    float3 direction;
    float minDistance;
    float maxDistance;
};

//static const GPURay degenerateRay = { float3(0), float3(0, 1, 0), 0.0f, 0.00001f };


struct FullScreenPassVsOut
{
    float2 texC : TEXCOORD;
#ifndef _VIEWPORT_MASK
    float4 posH : SV_POSITION;
#else
    float4 posH  : POSITION;
#endif
};




SamplerState gSampler;
/* Precomputed LTC data */
Texture2D gLTC_1;
Texture2D gLTC_2;

Texture2D gPosW;
Texture2D gNormalW;
Texture2D gRoughness;
Texture2D gDiffuseOpacity;
Texture2D gSpecRough;

Texture2D gNoiseTex;


struct LightData
{
    float4 LightCenter;
    float4 LightAxis;
    float2 LightExtent;
};

cbuffer PerFrameCB
{
    SData gData;
    LightData gLightData;
    float4 Halton;
    float4x4 gLightMat;
};

struct PsOut
{
    float4 analyticUnshadowed : SV_Target0;
    float4 stochasticUnshadowed : SV_Target1;
    
};

/** Inverts a 3x3 matrix.
*/
float3x3 inverse(float3x3 M)
{
    float3x3 inv;
    float invdet = 1.0f / determinant(M);
    inv[0][0] = (M[1][1] * M[2][2] - M[2][1] * M[1][2]) * invdet;
    inv[0][1] = (M[0][2] * M[2][1] - M[0][1] * M[2][2]) * invdet;
    inv[0][2] = (M[0][1] * M[1][2] - M[0][2] * M[1][1]) * invdet;
    inv[1][0] = (M[1][2] * M[2][0] - M[1][0] * M[2][2]) * invdet;
    inv[1][1] = (M[0][0] * M[2][2] - M[0][2] * M[2][0]) * invdet;
    inv[1][2] = (M[1][0] * M[0][2] - M[0][0] * M[1][2]) * invdet;
    inv[2][0] = (M[1][0] * M[2][1] - M[2][0] * M[1][1]) * invdet;
    inv[2][1] = (M[2][0] * M[0][1] - M[0][0] * M[2][1]) * invdet;
    inv[2][2] = (M[0][0] * M[1][1] - M[1][0] * M[0][1]) * invdet;
    return inv;
}

/** CIE luminance (Y) of linearRGB value (not sRGB!) */
float RGBtoCIELuminance(float3 linearRGB)
{
    return dot(float3(0.2126, 0.7152, 0.0722), linearRGB);
}
//todo:optimize

//LTC Functions:
void ClipQuadToHorizon(inout float3 L[5], out int n)
{
    // detect clipping config
    int config = 0;
    if (L[0].z > 0.0)
        config += 1;
    if (L[1].z > 0.0)
        config += 2;
    if (L[2].z > 0.0)
        config += 4;
    if (L[3].z > 0.0)
        config += 8;

    // clip
    n = 0;

    if (config == 0)
    {
        // clip all
    }
    else if (config == 1) // V1 clip V2 V3 V4
    {
        n = 3;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 2) // V2 clip V1 V3 V4
    {
        n = 3;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 3) // V1 V2 clip V3 V4
    {
        n = 4;
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
        L[3] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 4) // V3 clip V1 V2 V4
    {
        n = 3;
        L[0] = -L[3].z * L[2] + L[2].z * L[3];
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
    }
    else if (config == 5) // V1 V3 clip V2 V4) impossible
    {
        n = 0;
    }
    else if (config == 6) // V2 V3 clip V1 V4
    {
        n = 4;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 7) // V1 V2 V3 clip V4
    {
        n = 5;
        L[4] = -L[3].z * L[0] + L[0].z * L[3];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 8) // V4 clip V1 V2 V3
    {
        n = 3;
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
        L[1] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = L[3];
    }
    else if (config == 9) // V1 V4 clip V2 V3
    {
        n = 4;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[2].z * L[3] + L[3].z * L[2];
    }
    else if (config == 10) // V2 V4 clip V1 V3) impossible
    {
        n = 0;
    }
    else if (config == 11) // V1 V2 V4 clip V3
    {
        n = 5;
        L[4] = L[3];
        L[3] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 12) // V3 V4 clip V1 V2
    {
        n = 4;
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
    }
    else if (config == 13) // V1 V3 V4 clip V2
    {
        n = 5;
        L[4] = L[3];
        L[3] = L[2];
        L[2] = -L[1].z * L[2] + L[2].z * L[1];
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
    }
    else if (config == 14) // V2 V3 V4 clip V1
    {
        n = 5;
        L[4] = -L[0].z * L[3] + L[3].z * L[0];
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
    }
    else if (config == 15) // V1 V2 V3 V4
    {
        n = 4;
    }

    if (n == 3)
        L[3] = L[0];
    if (n == 4)
        L[4] = L[0];
}

float3 IntegrateEdgeV3(float3 v1, float3 v2)
{
    float x = dot(v1, v2);
    float y = abs(x);

    float a = 0.8543985 + (0.4965155 + 0.0145206 * y) * y;
    float b = 3.4175940 + (4.1616724 + y) * y;
    float v = a / b;

    float theta_sintheta = (x > 0.0) ? v : 0.5 * rsqrt(max(1.0 - x * x, 1e-7)) - v;

    return cross(v1, v2) * theta_sintheta;
}

float3 IntegrateEdge(float3 vVertex1, float3 vVertex2)
{
    // return IntegrateEdgeV3(v1, v2).z;
    float CosTheta = dot(vVertex1, vVertex2);
    float Theta = acos(CosTheta);
    return cross(vVertex1, vVertex2) * ((Theta > 0.001) ? Theta / sin(Theta) : 1.0);
}

float LTC_Evaluate(float3 P, float3x3 Minv, float4 points[4])
{
    // polygon (allocate 5 vertices for clipping)
    float3 L[5];
    L[0] = mul(points[0].rgb - P, Minv);
    L[1] = mul(points[1].rgb - P, Minv);
    L[2] = mul(points[2].rgb - P, Minv);
    L[3] = mul(points[3].rgb - P, Minv);
    
    // L[0] = mul(Minv,points[0].rgb - P);
    // L[1] = mul(Minv,points[1].rgb - P);
    // L[2] = mul(Minv,points[2].rgb - P);
    // L[3] = mul(Minv,points[3].rgb - P);
    

    // integrate
    float sum = 0.0;

    {
        int n;
        ClipQuadToHorizon(L, n);

        if (n == 0)
            return 0.0;

        // project onto sphere
        L[0] = normalize(L[0]);
        L[1] = normalize(L[1]);
        L[2] = normalize(L[2]);
        L[3] = normalize(L[3]);
        L[4] = normalize(L[4]);

        float3 vsum;

        // integrate
        vsum = IntegrateEdgeV3(L[0], L[1]);
        vsum += IntegrateEdgeV3(L[1], L[2]);
        vsum += IntegrateEdgeV3(L[2], L[3]);
        if (n >= 4)
            vsum += IntegrateEdgeV3(L[3], L[4]);
        if (n == 5)
            vsum += IntegrateEdgeV3(L[4], L[0]);

        sum = max(0.0, vsum.z);

    }

    return sum;
}

struct LTCTerms
{
    float3x3 MinvS;
    float3x3 MS;
    float MdetS;

    float3x3 MinvD;
    float3x3 MD;
    float MdetD;

    float2 scaleBias;
};


/** 
 N = shading normal
 V = vector to the eye from the point (outgoing direction)
 */

 //void => float3 DEBUG USE
void GetLTCTerms(float3 N, float3 V, float roughness, out LTCTerms terms)
{
    float ndotv = dot(N, V);
    float u = roughness;
    //float v = sqrt(1.0f - clamp(ndotv, 0.0f, 1.0f));
    float v = acos(ndotv) / (0.5 * PI);

    const float LUT_SIZE  = 64.0;
    const float LUT_SCALE = (LUT_SIZE - 1.0)/LUT_SIZE;
    const float LUT_BIAS  = 0.5/LUT_SIZE;
    float2 uv = float2(u,v);
    uv = uv*LUT_SCALE + LUT_BIAS;

    float4 t1 = gLTC_1.Sample(gSampler,uv);
    float4 t2 = gLTC_2.Sample(gSampler,uv);

    //Matrix3 Minv = Matrix3(t1.x, 0.0f, t1.y,
    //                       0.0f, 1.0f, 0.0f,
    //                       t1.z, 0.0f, t1.w);

    float3x3 Minv = float3x3(t1.x, 0.0f, t1.y,
                           0.0f, 1.0f, 0.0f,
                           t1.z, 0.0f, t1.w);

    // construct orthonormal basis around N
    float3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);

    // World to tangent matrix
    float3x3 w2t = transpose(float3x3(T1, T2, N));

    //terms.MinvS = Minv * w2t;
    //terms.MinvS = mul(Minv, w2t);
    terms.MinvS = mul(w2t, Minv);
    terms.MS = inverse(terms.MinvS);
    terms.MdetS = determinant(terms.MS);

    // diffuse
    terms.MinvD = w2t;
    terms.MD = transpose(terms.MinvD);
    terms.MdetD = 1.0f;
    terms.scaleBias = t2.xy;
}

/** L = vector to the light from the surface */
float LTC_Evaluate(float3x3 Minv, float Mdet, float3 L)
{
    //float3 LCosine = Minv * L;
    float3 LCosine = mul(L , Minv);
    float l2 = dot(LCosine, LCosine);
    float square = l2 * l2;
    float Jacobian = Mdet * square;
    return max(LCosine.z, 0.0f) / (PI * Jacobian);
}

float3 LTC_Sample(float3x3 M, float Mdet, float u1, float u2)
{
    float ct = sqrt(u1);
    float st = sqrt(1.0f - u1);
    float phi = 2.0f * PI * u2;

    float3 dir = float3(st * cos(phi), st * sin(phi), ct);

    //float3 L = M * dir;
    float3 L = mul(dir,M);
    float3 w_i = normalize(L);

    return w_i;
}

float3 areaLight
   (float3 X,
    float3 diffColor,
    float3 specColor,
    LTCTerms ltcTerms,
    float4 lightVertices[4],
    out float diffProb,
    out float brdfProb)
{

    // apply BRDF magnitude and Fresnel
    specColor = specColor * ltcTerms.scaleBias.x + (float3(1.0f,1.0f,1.0f) - specColor) * ltcTerms.scaleBias.y;

    float specLobe = LTC_Evaluate(X, ltcTerms.MinvS, lightVertices);
    float3 specular = specColor * specLobe;

    float diffLobe = LTC_Evaluate(X, ltcTerms.MinvD, lightVertices);
    float3 diffuse = diffColor * diffLobe;

    // These could be computed outside of the function
    float specMag = RGBtoCIELuminance(specular);
    float diffMag = RGBtoCIELuminance(diffuse);

    diffProb = diffMag / max(diffMag + specMag, 1e-5);
    //brdfProb = mix(specLobe, diffLobe, diffProb);
    brdfProb = lerp(specLobe, diffLobe, diffProb);

    return specular + diffuse;

    // if(!any(isnan(ltcTerms.MinvS))) return float3(1.0,0.0,0.0);
    // else return float3(0.0,1.0,0.0);
}


float3 brdfEval
    (float3 diffColor,
     float3 specColor,
     LTCTerms ltcTerms,
     float3 w_i,
     float diffProb,
     out float pdf)
{

    float specPDF = LTC_Evaluate(ltcTerms.MinvS, ltcTerms.MdetS, w_i);
    float diffPDF = LTC_Evaluate(ltcTerms.MinvD, ltcTerms.MdetD, w_i);

    // (Could be precomputed)
    float3 specAlbedo = specColor * ltcTerms.scaleBias.x + (float3(1) - specColor) * ltcTerms.scaleBias.y;

    pdf = lerp(specPDF, diffPDF, diffProb);

    return specAlbedo * specPDF + diffColor * diffPDF;
}


float3 brdfSample(bool bSpec, float u1, float u2, LTCTerms ltcTerms)
{
    float3x3 M = bSpec ? ltcTerms.MS : ltcTerms.MD;
    float Mdet = bSpec ? ltcTerms.MdetS : ltcTerms.MdetD;
    return LTC_Sample(M, Mdet, u1, u2);
}

bool rayRectLightIntersection(GPURay ray, float3 lightCenter, float4x4 lightFrame, float2 lightExtent, float3 lightAxis, out float3 hitPoint)
{
    // Ray-plane intersection
    float d = -dot(lightAxis, lightCenter);
    float rate = -dot(ray.direction, lightAxis);


    //float4x4 test = transpose(lightFrame);
    
    if (rate >= 0.0)
    {
        float t = (d + dot(ray.origin, lightAxis)) / rate;
        if ((t >= ray.minDistance) && (t <= ray.maxDistance))
        {
            // Hit the plane, did we hit the light surface?
            hitPoint = ray.origin + ray.direction * t;
            float u = dot(hitPoint - lightCenter, lightFrame[0].xyz); //maybe transpose 
            float v = dot(hitPoint - lightCenter, lightFrame[1].xyz);
            //float u = dot(hitPoint - lightCenter, test[0].xyz); //maybe transpose 
            //float v = dot(hitPoint - lightCenter, test[1].xyz);
            if ((abs(u) < lightExtent.x * 0.5f) &&
                (abs(v) < lightExtent.y * 0.5f))
            {
                return true;
            }
        }
    }

    return false;
}

struct SphQuad
{
    float3 o, x, y, z;
    float z0, z0sq;
    float x0, y0, y0sq;
    float x1, y1, y1sq;
    float b0, b1, b0sq, k;
    float S;
};

SphQuad SphQuadInit(float3 s, float3 ex, float3 ey, float3 o)
{
    SphQuad squad;

    squad.o = o;
    float exl = length(ex);
    float eyl = length(ey);

    // compute local reference system 'R'
    squad.x = ex / exl;
    squad.y = ey / eyl;
    squad.z = cross(squad.x, squad.y);

    // compute rectangle coords in local reference system
    float3 d = s - o;
    squad.z0 = dot(d, squad.z);

    // flip 'z' to make it point against 'Q'
    if (squad.z0 > 0.0f)
    {
        squad.z = -squad.z;
        squad.z0 = -squad.z0;
    }

    squad.z0sq = squad.z0 * squad.z0;
    squad.x0 = dot(d, squad.x);
    squad.y0 = dot(d, squad.y);
    squad.x1 = squad.x0 + exl;
    squad.y1 = squad.y0 + eyl;
    squad.y0sq = squad.y0 * squad.y0;
    squad.y1sq = squad.y1 * squad.y1;

    // create vectors to four vertices
    float3 v00 = float3(squad.x0, squad.y0, squad.z0);
    float3 v01 = float3(squad.x0, squad.y1, squad.z0);
    float3 v10 = float3(squad.x1, squad.y0, squad.z0);
    float3 v11 = float3(squad.x1, squad.y1, squad.z0);

    // compute normals to edges
    float3 n0 = normalize(cross(v00, v10));
    float3 n1 = normalize(cross(v10, v11));
    float3 n2 = normalize(cross(v11, v01));
    float3 n3 = normalize(cross(v01, v00));

    // compute internal angles (gamma_i)
    float g0 = acos(-dot(n0, n1));
    float g1 = acos(-dot(n1, n2));
    float g2 = acos(-dot(n2, n3));
    float g3 = acos(-dot(n3, n0));

    // compute predefined constants
    squad.b0 = n0.z;
    squad.b1 = n2.z;
    squad.b0sq = squad.b0 * squad.b0;
    squad.k = 2.0f * PI - g2 - g3;

    // compute solid angle from internal angles
    squad.S = g0 + g1 - squad.k;

    return squad;
}

float3 SphQuadSample(SphQuad squad, float u, float v)
{
    // 1. compute 'cu'
    float au = u * squad.S + squad.k;
    float fu = (cos(au) * squad.b0 - squad.b1) / sin(au);
    float cu = 1.0f / sqrt(fu * fu + squad.b0sq) * (fu > 0.0f ? 1.0f : -1.0f);
    cu = clamp(cu, -1.0f, 1.0f); // avoid NaNs

    // 2. compute 'xu'
    float xu = -(cu * squad.z0) / sqrt(1.0f - cu * cu);
    xu = clamp(xu, squad.x0, squad.x1); // avoid Infs

    // 3. compute 'yv'
    float d = sqrt(xu * xu + squad.z0sq);
    float h0 = squad.y0 / sqrt(d * d + squad.y0sq);
    float h1 = squad.y1 / sqrt(d * d + squad.y1sq);
    float hv = h0 + v * (h1 - h0), hv2 = hv * hv;
    float yv = (hv2 < 1.0f - 1e-6f) ? (hv * d) / sqrt(1.0f - hv2) : squad.y1;

    // 4. transform (xu, yv, z0) to world coords
    return squad.o + xu * squad.x + yv * squad.y + squad.z0 * squad.z;
}


struct Surfel
{
    float3 position;
    float3 shadingNormal;
    float3 lambertianReflectivity;
    float3 fresnelReflectionAtNormalIncidence;
    float roughness;
};


void initSurfel(in int2 uv,inout Surfel surfel)
{
    surfel.position = gPosW[uv].rgb;
    float3 Normal = gNormalW[uv].rgb;
    surfel.shadingNormal = normalize(Normal);
    surfel.lambertianReflectivity = gDiffuseOpacity[uv].rgb;
    surfel.fresnelReflectionAtNormalIncidence = gSpecRough[uv].rgb;
    surfel.roughness = gRoughness[uv].r;
    
}

//TODO:MRT
float4 main(FullScreenPassVsOut vsOut) : SV_Target0
{
    uint2 texDim;
    uint levels;
    gPosW.GetDimensions(0, texDim.x, texDim.y, levels);
    float2 pixelPos = vsOut.texC * texDim;
    int2 iPos = int2(pixelPos);

    //float2 SamplePos = vsOut.posH.xy;
    //int2 iPos = int2(SamplePos);
    
    float4 anlticUnshadowed = 1.0f;
    float epsilon = 0.005f;
    
    Surfel surfel;
    initSurfel(iPos, surfel);
    
    float3 w_o;
    w_o = normalize(gData.CamPosW.xyz - surfel.position);
    LTCTerms ltcTerms;
    //float roughness = clamp(1.0f-surfel.roughness,0.05f,1.0f);
    float roughness = surfel.roughness;
    GetLTCTerms(surfel.shadingNormal, w_o, roughness, ltcTerms);

    float diffProb, brdfProb;
    
    anlticUnshadowed.rgb = gData.LightRadiance * areaLight(surfel.position, surfel.lambertianReflectivity, surfel.fresnelReflectionAtNormalIncidence,ltcTerms, gData.LightVertices, diffProb, brdfProb) ;


    //test
    float4 stochasticUnshadowed = 0.0f;

    //DEBUG
    float4 Debug_W_i = 1.0f;
    //DEBUG

    
    int2 NoiseTexPos = iPos & int2(127);
    float4 blueNoise = gNoiseTex[NoiseTexPos];
    
    float3 ex = gData.LightVertices[1].rgb - gData.LightVertices[0].rgb;
    float3 ey = gData.LightVertices[3].rgb - gData.LightVertices[0].rgb;

    SphQuad squad = SphQuadInit(gData.LightVertices[0].rgb, ex, ey, surfel.position);

    float u1 = frac(Halton.r + blueNoise.r);
    float u2 = frac(Halton.g + blueNoise.g);
    float u3 = frac(Halton.b + blueNoise.b);
    float u4 = frac(Halton.a + blueNoise.a);

    if (u4 <= brdfProb)
    {
        bool bSpec = u3 > diffProb;
        float3 w_i = brdfSample(bSpec, u1, u2, ltcTerms);
        GPURay Ray = { surfel.position, w_i, 0, 1e6 };


        Debug_W_i.rgb = w_i;
        
        float3 Y;
        if (rayRectLightIntersection(Ray, gLightData.LightCenter.rgb, gLightMat, gLightData.LightExtent, gLightData.LightAxis.rgb, Y))
        {
            float3 delta = Y - surfel.position;
            float len = length(delta);

            float dSolidAngle = gLightData.LightExtent.x * gLightData.LightExtent.y * abs(dot(w_i, gLightData.LightAxis.rgb)) / (len * len);

            float lightPdf = 1.0 / squad.S;

            float brdfPdf;

            float3 f = brdfEval(surfel.lambertianReflectivity, surfel.fresnelReflectionAtNormalIncidence, ltcTerms, w_i, diffProb, brdfPdf);

            float misPdf = lerp(lightPdf, brdfPdf, brdfProb);

            if (brdfPdf > 0.0f)
            {
                float3 L_i = gData.LightRadiance * f / misPdf;

                stochasticUnshadowed.rgb += L_i;

                //todo

            }

        }
    }
    else
    {
        float3 Y = SphQuadSample(squad, u1, u2);

        float3 delta = Y - surfel.position;
        float len = length(delta);

        float3 w_i = delta / max(len, 0.001f);

        Debug_W_i.rgb = w_i;
        
        float lightPdf = 1.0f / squad.S;

        float brdfPdf;

        float3 f = brdfEval(surfel.lambertianReflectivity, surfel.fresnelReflectionAtNormalIncidence, ltcTerms, w_i, diffProb, brdfPdf);

        float mixPdf = lerp(lightPdf, brdfPdf, brdfProb);


        if (brdfPdf > 0.0f)
        {
            float3 L_i = gData.LightRadiance * f / mixPdf;

            if (isnan(L_i.x))
            {
                L_i = float3(0.0f);
            }

            stochasticUnshadowed.rgb += L_i;
            //todo
        }
        
    }

    


    stochasticUnshadowed.a = 1.0f;


    return stochasticUnshadowed;

    
    //return Debug_W_i;
    //return float4(vsOut.posH.xy,0.0f,1.0f);
    //return float4(SamplePos, 0.0f, 1.0f);
    //return float4(vsOut.texC,0.0,1.0);
} 
