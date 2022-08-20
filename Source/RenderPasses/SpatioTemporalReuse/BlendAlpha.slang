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


float interpAlpha(float dv, float ddv, float alpha, float beta, float kdv = 1.0f, float kddv = 1.0f)
{
    float amb = alpha - beta;
    // xx xy yy
    float x = saturate(kdv * dv * dv);
    float xy = saturate(kdv * kddv * dv * ddv);
    float y = saturate(kddv * ddv * ddv);

    // xx xy y
    // more lagging
    //float x = saturate(kdv * dv);
    //float xy = saturate(kdv * kddv * dv * ddv);
    //float y = saturate(kddv * ddv);

    // beta alpha alpha beta
    // this is better
    float cx = amb;
    float cxy = -2. * amb;
    float cy = amb;
    float d = beta;

    // 1 a a b
    //float cx = alpha - 1.;
    //float cxy = beta-2.*alpha +1.;
    //float cy = cx;
    //float d = 1.;
    
    return saturate(cx * x + cxy * xy + cy * y + d);
    //return lerp(baseAlpha, 1, f);
}

float calAlpha(float dv, float ddv, float baseAlpha, float dvRatio = 1.0f, float ddvRatio = 1.0f)
{
    float lam1 = saturate(dvRatio * (1.0 - saturate(dvRatio * (dv - ddv))));
    
    float lam2 = saturate(exp((1.0 / (ddvRatio + 0.001) * (-1.0 / (ddv + 0.0001) + 1.0))));
    float res = lerp(baseAlpha, 1.0, lam2);
    //return res;
    return lerp(baseAlpha, res, lam1);

}
