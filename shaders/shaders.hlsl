cbuffer ConstantBuffer : register(b0)
{
	float4x4 mwpMatrix;
}

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
	float3 bary : COLOR1;
};

PSInput VSMain(float4 position
			   : POSITION, float4 normal
			   : NORMAL, float4 ambient
			   : COLOR0, float4 diffuse
			   : COLOR1, float4 emissive
			   : COLOR2, float3 bary
			   : COLOR3)
{
	PSInput result;

	result.position = mul(position, mwpMatrix);
	result.color = ambient;
	result.bary = bary;

	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float bary_factor = (1.0f - dot(input.bary, input.bary));
	return saturate(input.color * (0.4f + 1.0f * bary_factor));
}