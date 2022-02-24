float calcBlendAlphaCommon(float dv, float ddv, float baseAlpha, float dvRatio = 1.0f, float ddvRatio = 1.0f)
{
    // 1. full bright or full dark: small dv, small ddv, big alpha to avoid ghosting
    // 2. half bright/dark: large dv, small ddv, small alpha to hasten covergence
    // 3. edge of 1 and 2: dont care dv, big ddv, big alpha to avoid ghosting
    // 4. edge of object: big dv, big ddv, big alpha to avoid ghosting (discard by pos/normal can handle part of this)
    // have to design a 2d function that satisfy above conditions

    //return log(x * (exp(1) - 1) + 1);
    //return x;
    //return lerp(baseAlpha, 1., saturate(ddv * ddvRatio - dv * dvRatio));
    //return lerp(gAlpha, 1., saturate(ddv * 3.));

    // old
    float2 base = float2(-1, 1);
    float2 target = float2(dv * dvRatio, ddv * ddvRatio) - float2(1, 0);
    float x = saturate(dot(base, target) / sqrt(2));
    //x = max(x, saturate(1 - dv + ddv * 8));
    //return log(x * (exp(1) - 1) + 1);
    //return x;
    //return lerp(gAlpha, 1., saturate(ddv * 3.));
    return lerp(baseAlpha, 1., x);
}
