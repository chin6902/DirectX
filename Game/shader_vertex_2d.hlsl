// 2D vertex shader
float4x4 mtx; 

float4 main( float4 posL : POSITION0 ) : SV_POSITION
{
	return mul(posL, mtx);
}