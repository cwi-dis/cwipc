
Shader "cwipc/PointCloud"{
	Properties{
		_Tint("Tint", Color) = (0.5, 0.5, 0.5, 1)
		_PointSize("Point Size", Float) = 0.05
		_PointSizeFactor("Point Size multiply", Float) = 1.0
		_MainTex("Texture", 2D) = "white" {}
		_Cutoff("Alpha cutoff", Range(0,1)) = 0.5
	}
	
	SubShader {
		// This shader uses the geometry engine, and samples off a texture.
		Lighting Off
		LOD 100
		Cull Off
		// xxxjack We could try enabling/disabling alpha blending to see whether it afects performance.
		// xxxjack It doesn't affect performance (at least not on a RTX2080), but enabling alpha blending does
		// make things more ugly.
		//Blend SrcAlpha OneMinusSrcAlpha
		//BlendOp Add
		Tags {
			"Queue" = "AlphaTest" 
			"IgnoreProjector" = "True" 
			"RenderType" = "TransparentCutout"
		}

		Pass {
			Name "PointCloudBufferAsTexture"
			Tags { 
				"LightMode" = "ForwardBase" 
			}
			CGPROGRAM

			#pragma target 5.0
			#pragma vertex Vertex
			#pragma geometry Geometry
			#pragma fragment Fragment

			#include "UnityCG.cginc"

			half3 PcxDecodeColor(uint data) {
				half r = (data >> 0) & 0xff;
				half g = (data >> 8) & 0xff;
				half b = (data >> 16) & 0xff;
				return half3(r, g, b) / 255;
			}

			struct Varyings {
				float4	position : SV_Position;
				half4	color : COLOR;
				half2	uv : TEXCOORD0;
//					UNITY_FOG_COORDS(0)
			};

			half4		_Tint;
			float4x4	_Transform;
			half		_PointSize;
			half		_PointSizeFactor;
			sampler2D	_MainTex;
			fixed		_Cutoff;

			StructuredBuffer<float4> _PointBuffer;

			Varyings Vertex(uint vid : SV_VertexID) {
				float4 pt = _PointBuffer[vid];
				float4 pos = mul(_Transform, float4(pt.xyz, 1));
				half4  col = half4(PcxDecodeColor(asuint(pt.w)), _Tint.a);

#if UNITY_COLORSPACE_GAMMA
				col.rgb *= _Tint.rgb * 2;
#else
				col.rgb *= LinearToGammaSpace(_Tint) * 2;
				col.rgb = GammaToLinearSpace(col);
#endif
				Varyings o;
				o.position = UnityObjectToClipPos(pos);
				o.color = col;
				o.uv = half2(0.5, 0.5);
//					UNITY_TRANSFER_FOG(o, o.position);
				return o;
			}

			[maxvertexcount(4)]
			void Geometry(point Varyings input[1], inout TriangleStream<Varyings> outStream) {
				float4 origin = input[0].position;
				float2 extent = abs(UNITY_MATRIX_P._11_22  * _PointSize * _PointSizeFactor);
#if SHADER_API_GLCORE || SHADER_API_METAL
				extent.x *= -1;
#endif
				// Copy the basic information.
				Varyings o = input[0];
				o.position.xzw = origin.xzw;

				// Bottom-Left side vertex
				o.position.x = origin.x + extent.x;
				o.position.y = origin.y + extent.y;
				o.uv = half2(1, 1);
				outStream.Append(o);

				// Up-Left vertex
				o.position.x = origin.x - extent.x;
				o.position.y = origin.y + extent.y;
				o.uv = half2(0, 1);
				outStream.Append(o);

				// Up-Right side vertex
				o.position.x = origin.x + extent.x;
				o.position.y = origin.y - extent.y;
				o.uv = half2(1, 0);
				outStream.Append(o);

				// Bottom-Right vertex
				o.position.x = origin.x - extent.x;
				o.position.y = origin.y - extent.y;
				o.uv = half2(0, 0);
				outStream.Append(o);

				outStream.RestartStrip();
			}

			half4 Fragment(Varyings input) : SV_Target{
				half4 c = input.color;
				half4 tc = tex2D(_MainTex, input.uv);
				c.a *= tc.a;
				clip(c.a - _Cutoff);
				return c;
			}


			ENDCG
		}
	}
	
	SubShader {
		// This shader does not use the geometry engine, but does two passes.
		Lighting Off
		LOD 100
		Cull Off
		Tags {
			"Queue" = "AlphaTest" 
			"IgnoreProjector" = "True" 
			"RenderType" = "Transparent"
		}

		Pass {
			// Pass two: normal sized points with no transparency
			Name "PointCloudBufferAsPoints"
			Tags { 
				"LightMode" = "ForwardBase" 
			}
			CGPROGRAM

			#pragma vertex Vertex
			#pragma fragment Fragment

			#include "UnityCG.cginc"

			half3 PcxDecodeColor(uint data) {
				half r = (data >> 0) & 0xff;
				half g = (data >> 8) & 0xff;
				half b = (data >> 16) & 0xff;
				return half3(r, g, b) / 255;
			}

			struct Varyings {
				float4	position : SV_Position;
				half4	color : COLOR;
				half  size : PSIZE;
//					UNITY_FOG_COORDS(0)
			};

			half4		_Tint;
			float4x4	_Transform;
			half		_PointSize;
			half		_PointSizeFactor;
			sampler2D	_MainTex;
			fixed		_Cutoff;

			StructuredBuffer<float4> _PointBuffer;

			Varyings Vertex(uint vid : SV_VertexID) {
				float4 pt = _PointBuffer[vid];
				float4 pos = mul(_Transform, float4(pt.xyz, 1));
				half4  col = half4(PcxDecodeColor(asuint(pt.w)), _Tint.a);

#if UNITY_COLORSPACE_GAMMA
				col.rgb *= _Tint.rgb * 2;
#else
				col.rgb *= LinearToGammaSpace(_Tint) * 2;
				col.rgb = GammaToLinearSpace(col);
#endif
				Varyings o;
				o.position = UnityObjectToClipPos(pos);
				o.color = col;
                float pixelsPerMeter = _ScreenParams.y / o.position.w;
                o.size = _PointSize * _PointSizeFactor * pixelsPerMeter;
//					UNITY_TRANSFER_FOG(o, o.position);
				return o;
			}

			half4 Fragment(Varyings input) : SV_Target{
                half4 c = input.color;
                return c;
			}
			ENDCG
		}
/* Second pass does not improve quality. Need to investigate. xxxjack
		Pass {
			// Pass two: double-sized points with transparency
			Tags { 
				"LightMode" = "ForwardBase" 
			}
			CGPROGRAM

			#pragma vertex Vertex
			#pragma fragment Fragment

			#include "UnityCG.cginc"

			half3 PcxDecodeColor(uint data) {
				half r = (data >> 0) & 0xff;
				half g = (data >> 8) & 0xff;
				half b = (data >> 16) & 0xff;
				return half3(r, g, b) / 255;
			}

			struct Varyings {
				float4	position : SV_Position;
				half4	color : COLOR;
				float  size : PSIZE;
//					UNITY_FOG_COORDS(0)
			};

			half4		_Tint;
			float4x4	_Transform;
			half		_PointSize;
			half		_PointSizeFactor;
			sampler2D	_MainTex;
			fixed		_Cutoff;

			StructuredBuffer<float4> _PointBuffer;

			Varyings Vertex(uint vid : SV_VertexID) {
				float4 pt = _PointBuffer[vid];
				float4 pos = mul(_Transform, float4(pt.xyz, 1));
				half4  col = half4(PcxDecodeColor(asuint(pt.w)), _Tint.a*0.5);

#if UNITY_COLORSPACE_GAMMA
				col.rgb *= _Tint.rgb * 2;
#else
				col.rgb *= LinearToGammaSpace(_Tint) * 2;
				col.rgb = GammaToLinearSpace(col);
#endif
				Varyings o;
				o.position = UnityObjectToClipPos(pos);
				o.color = col;
                //
                // xxxjack I think this computation is wrong. Undoutedly I can get the
                // correct information from the various matrices but I don't know how.
                //
                float pixelsPerMeter = _ScreenParams.y / o.position.w;
                o.size = _PointSize * _PointSizeFactor * pixelsPerMeter * 3;
//					UNITY_TRANSFER_FOG(o, o.position);
				return o;
			}

			half4 Fragment(Varyings input) : SV_Target{
                half4 c = input.color;
                return c;
			}
			ENDCG
		}
*/
	}
}
