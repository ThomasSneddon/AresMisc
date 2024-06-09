#include <YRPP.h>

#include <vector>
#include <cmath>

namespace YRRasterizer
{
	using vec2 = Vector2D<double>;

	enum class rasterize_dir :int
	{
		vertical = 0,
		horizontal = 1
	};

	//How to: see GScreenClass_DrawOnTop_RasterizerTest
	struct range {
		union {
			struct {
				//x coord the range
				int x = 0, 
					//min Y of the range, in screen space it's on the top.
					bottomy = 0, 
					// max Y of the range, in screen space it's on the bottom.
					topy = 0;
				//weights of p1, p2 and p3, respectively, for the pixel at bottomY
				double bottom_weight[3] = { 0.0,0.0,0.0 }, 
					//weights of p1, p2 and p3, respectively, for the pixel at topY
					top_weight[3] = { 0.0,0.0,0.0 }; 
			}v;

			struct {
				//Y coord the range
				int y = 0, 
					//min X of the range
					leftx = 0, 
					// max X of the range.
					rightx = 0; 

				//weights of p1, p2 and p3, respectively, for the pixel at leftX
				double left_weight[3] = { 0.0,0.0,0.0 }, 
					//weights of p1, p2 and p3, respectively, for the pixel at rightX
					right_weight[3] = { 0.0,0.0,0.0 }; 
			}h;
		};
	};

	struct rasterize_result {
		//points from the parameters
		vec2 p1 = { 0.0,0.0 }, p2 = { 0.0,0.0 }, p3 = { 0.0,0.0 };
		rasterize_dir direction = rasterize_dir::vertical;
		std::vector<range> ranges = {};
	};
	//core function
	std::vector<range> rasterize(const vec2& p1, const vec2& p2, const vec2& p3);

	//
	rasterize_result rasterize_triangle(const vec2& p1, const vec2& p2, const vec2& p3);
	//needs a cache system

}