Texture2D gTexPrevPos;
Texture2D gTexCurPos;
Texture2D gTexPrevNormal;
Texture2D gTexCurNormal;

float isValid(int2 vPos, int2 vPrevPos, int2 vTexDim, float4x4 vViewProjMatrix, float vPosStrength, float vNormalStrength)
{
    // check if is outside of screen
    if (vPrevPos.x < 0 && vPrevPos.y < 0 && vPrevPos.x >= vTexDim.x && vPrevPos.y >= vTexDim.y)
        return false;
    
    // check if position is acceptable
    if (vPosStrength > 0)
    {
        float3 CurPos = gTexCurPos[vPos].xyz;
        float4 CurPosH = mul(float4(CurPos, 1.0), vViewProjMatrix);
        float Depth = CurPosH.z / CurPosH.w;
        float3 PrevPos = gTexPrevPos[vPrevPos].xyz;
        float MaxOffset = (pow(Depth, 2) * 0.10 + 0.05) / vPosStrength; // further => larger acceptable offset
        if (distance(CurPos, PrevPos) > MaxOffset)
            return false;
    }
    
    // check if normal is acceptable
    if (vNormalStrength > 0)
    {
        float3 CurNormal = gTexCurNormal[vPos].xyz;
        float3 PrevNormal = gTexPrevNormal[vPrevPos].xyz;
        if (distance(CurNormal, PrevNormal) > 0.05 / vNormalStrength)
            return false;
    }
    
    return true;
}
