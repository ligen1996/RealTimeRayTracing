Texture2D<float> gTex1;
Texture2D<float> gTex2;

float main(float2 texC : TEXCOORD, float4 pos : SV_POSITION) : SV_Target0
{
    float V1 = gTex1[pos.xy];
    float V2 = gTex2[pos.xy];
    return abs(V1 - V2);
}
