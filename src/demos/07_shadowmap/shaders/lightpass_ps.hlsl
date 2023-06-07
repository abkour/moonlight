struct PS_Input 
{
    float4 Position : SV_Position;
};

float main(PS_Input IN) : SV_Target
{
    return IN.Position.z;
}