#include <YRPP.h>

#include "Rasterizer.h"
#include "..\src\Utilities\Constructs.h"

namespace YRRasterizer
{
	std::vector<range> rasterize(const vec2& p1, const vec2& p2, const vec2& p3) {
		std::vector<range> ranges;
		const auto left_point = [](const vec2& l, const vec2& r) {
			return l.X < r.X;
		};

		std::vector<vec2> points = { p1,p2,p3 };
		const vec2 left = std::min({ p1,p2,p3 }, left_point);
		const vec2 right = std::max({ p1,p2,p3 }, left_point);

		double dis = p1.DistanceFrom(p2), dis2 = p1.DistanceFrom(p3), dis3 = p2.DistanceFrom(p3);
		//prl
		if (dis < 0.5 || dis2 < 0.5 || dis3 < 0.5 || abs(abs((p2 - p1) * (p3 - p1) / dis / dis2) - 1.0) <= 0.0001)
			return ranges;

		const auto is_middle = [left, right](const vec2& t) {
			return t != left && t != right;
		};

		bool xfind_aligned = p1.X == p2.X || p2.X == p3.X || p1.X == p3.X;
		auto findmid = std::find_if(points.begin(), points.end(), is_middle);

		if (findmid == points.end())
			return ranges;

		const vec2 middle = *findmid;
		double k = (right.Y - left.Y) / (right.X - left.X);/*,
														   k1 = (middle.Y - left.Y) / (middle.X - left.X),
														   k2 = (right.Y - middle.Y) / (right.X - middle.X);*/
														   //prepare for weight calc
		const auto assign_idx = [&points](const vec2& p) {
			return std::find(points.begin(), points.end(), p) - points.begin();
		};

		auto leftidx = assign_idx(left);
		auto middleidx = assign_idx(middle);
		auto rightidx = assign_idx(right);

		//double weightl = 0.0, weightm = 0.0, weightr = 0.0;
		double mbase = (middle.Y - left.Y) * (right.X - left.X) - (middle.X - left.X) * (right.Y - left.Y),
			rbase = (right.Y - left.Y) * (middle.X - left.X) - (right.X - left.X) * (middle.Y - left.Y);
		double middle_yterm = (right.X - left.X) / mbase,
			middle_xterm = (right.Y - left.Y) / mbase,
			right_yterm = (middle.X - left.X) / rbase,
			right_xterm = (middle.Y - left.Y) / rbase;

		//phase I
		const auto calc_and_insert_range = [&](const int i, const int bottompix, const int toppix) {
			range val = { i,bottompix,toppix };
			double xdiff = i + 0.5 - left.X, ybottomdiff = bottompix + 0.5 - left.Y, ytopdiff = toppix + 0.5 - left.Y;
			double xmterm = xdiff * middle_xterm, xrterm = xdiff * right_xterm;
			double bmweight = ybottomdiff * middle_yterm - xmterm,
				brweight = ybottomdiff * right_yterm - xrterm,
				blweight = 1.0 - bmweight - brweight;

			double tmweight = ytopdiff * middle_yterm - xmterm,
				trweight = ytopdiff * right_yterm - xrterm,
				tlweight = 1.0 - tmweight - trweight;

			val.v.bottom_weight[leftidx] = blweight;
			val.v.bottom_weight[middleidx] = bmweight;
			val.v.bottom_weight[rightidx] = brweight;

			val.v.top_weight[leftidx] = tlweight;
			val.v.top_weight[middleidx] = tmweight;
			val.v.top_weight[rightidx] = trweight;

			ranges.push_back(val);
		};

		if (!xfind_aligned || middle.X == right.X)
		{
			double k1 = (middle.Y - left.Y) / (middle.X - left.X); /*,
																   k2 = (right.Y - middle.Y) / (right.X - middle.X);*/

			double kb = std::min(k1, k);//btm
			double kt = std::max(k1, k);//top

			int leftbound = static_cast<int>(std::floor(left.X));
			int rightbound = static_cast<int>(std::floor(middle.X));

			if (middle.X == right.X && middle.X - std::floor(middle.X) > 0.5)
				rightbound++;

			for (int i = leftbound; i < rightbound; i++) {
				double delta = i + 0.5 - left.X;
				if (delta < 0)
					continue;

				double tval = left.Y + kt * delta;
				double bval = left.Y + kb * delta;

				double tfloor = std::floor(tval), bceil = std::ceil(bval);
				int toppix = static_cast<int>((tval - tfloor > 0.5) ? tfloor : (tfloor - 1.0));
				int bottompix = static_cast<int>((bceil - bval >= 0.5) ? (bceil - 1.0) : bceil);

				if (toppix >= bottompix)
					calc_and_insert_range(i, bottompix, toppix);
			}
		}

		//border value
		{
			int middlebound = static_cast<int>(std::floor(middle.X));
			if (middle.X != left.X && middle.X != right.X && middle.X != middlebound) {
				double k1 = (middle.Y - left.Y) / (middle.X - left.X),
					k2 = (right.Y - middle.Y) / (right.X - middle.X);

				double cent = middlebound + 0.5;
				double k0 = cent > middle.X ? k2 : k1;
				double cenval = middle.Y + k0 * (cent - middle.X);
				double cenval2 = left.Y + k * (cent - left.X);

				double tval = std::max(cenval, cenval2);
				double bval = std::min(cenval, cenval2);

				double tfloor = std::floor(tval), bceil = std::ceil(bval);
				int toppix = static_cast<int>((tval - tfloor > 0.5) ? tfloor : (tfloor - 1.0));
				int bottompix = static_cast<int>((bceil - bval >= 0.5) ? (bceil - 1.0) : bceil);

				if (toppix >= bottompix)
					calc_and_insert_range(middlebound, bottompix, toppix);
			}
		}

		//phase II
		if (!xfind_aligned || middle.X == left.X)
		{
			double k1 = (right.Y - middle.Y) / (right.X - middle.X);

			double kb = std::max(k1, k);//btm
			double kt = std::min(k1, k);//top

			int leftbound = static_cast<int>(std::ceil(middle.X));
			int rightbound = static_cast<int>(std::ceil(right.X));
			if (middle.X == left.X && middle.X - std::floor(middle.X) <= 0.5)
				leftbound--;

			for (int i = leftbound; i < rightbound; i++)
			{
				double delta = i + 0.5 - right.X;
				if (delta >= 0.0)
					continue;

				double tval = right.Y + kt * delta;
				double bval = right.Y + kb * delta;

				double tfloor = std::floor(tval), bceil = std::ceil(bval);
				int toppix = static_cast<int>((tval - tfloor > 0.5) ? tfloor : (tfloor - 1.0));
				int bottompix = static_cast<int>((bceil - bval >= 0.5) ? (bceil - 1.0) : bceil);

				if (toppix >= bottompix)
					calc_and_insert_range(i, bottompix, toppix);
			}
		}

		return ranges;
	}

	rasterize_result rasterize_triangle(const vec2& p1, const vec2& p2, const vec2& p3)
	{
		rasterize_result result = { p1,p2,p3 };

		const auto xvals = { p1.X,p2.X,p3.X };
		const auto yvals = { p1.Y,p2.Y,p3.Y };

		double minx = std::min(xvals), maxx = std::max(xvals), miny = std::min(yvals), maxy = std::max(yvals);

		if (maxy - miny > maxx - minx) {
			result.direction = rasterize_dir::vertical;
			result.ranges = rasterize(p1, p2, p3);
		}
		else {
			result.direction = rasterize_dir::horizontal;
			result.ranges = rasterize({ p1.Y,p1.X }, { p2.Y,p2.X }, { p3.Y,p3.X });
		}

		return result;
	}

}

DEFINE_HOOK(4F4583, GScreenClass_DrawOnTop_RasterizerTest, 6)
{
	static const YRRasterizer::vec2 points[] = { {15.3,15.6},{153.2,224.6},{331.5,7.6},{771.6,514.5} };
	static const YRRasterizer::vec2 shadeuvs[] = { {0.0,0.0},{0.0,1.0},{1.0,0.0},{1.0,1.0} };
	static const double values[] = { 0.0,1.0,0.5,0.0 };

	static const YRRasterizer::rasterize_result results[] = { YRRasterizer::rasterize_triangle(points[0], points[1], points[2]),
		YRRasterizer::rasterize_triangle(points[1], points[2], points[3]) };

	//let's rasterize and assign an SHP
	static UniqueGamePtr<SHPReference> pImage = {};

	if (!pImage) {
		pImage.reset(new SHPReference("RASTERIZE.SHP"));
	}

	if (!pImage) {
		return 0;
	}

	//auto res2 = rasterize(points[1], points[2], points[3]);
	//res.insert(res.begin(), res2.begin(), res2.end());

	const auto surface = DSurface::Composite;
	if (!surface)
		return 0;

	const auto pitch = surface->GetPitch();
	const auto pixels = pImage->GetPixels(0);
	if (!pixels || pImage->HasCompression(0))
		return 0;

	auto framebound = pImage->GetFrameBounds(0);
	double framewidth = framebound.Width, frameheight = framebound.Height;
	const auto palette = FileSystem::ANIM_PAL;

	if (auto surfacedata = reinterpret_cast<byte*>(surface->Lock(0, 0)))
	{
		const RectangleStruct bound = { 0,0,surface->GetWidth(),surface->GetHeight() };
		const auto shade = [&](const YRRasterizer::rasterize_result& result, const YRRasterizer::vec2 uvs[]) {
			if (result.direction == YRRasterizer::rasterize_dir::vertical)
			{
				for (const auto& ras : result.ranges)
				{
					if (ras.v.x >= bound.X && ras.v.x < bound.X + bound.Width)
					{
						int minpix = std::max(bound.Y, ras.v.bottomy);
						int maxpix = std::min(bound.Y + bound.Height - 1, ras.v.topy);

						double pixdiff = ras.v.topy - ras.v.bottomy;
						double wdiffs[] = { ras.v.top_weight[0] - ras.v.bottom_weight[0],ras.v.top_weight[1] - ras.v.bottom_weight[1],ras.v.top_weight[2] - ras.v.bottom_weight[2] };

						for (int i = minpix; i <= maxpix; i++)
						{
							double wlerp = pixdiff == 0.0 ? 0.0 : (i - ras.v.bottomy) / pixdiff;
							double w1 = ras.v.bottom_weight[0] + wlerp * wdiffs[0],
								w2 = ras.v.bottom_weight[1] + wlerp * wdiffs[1],
								w3 = ras.v.bottom_weight[2] + wlerp * wdiffs[2];

							auto uv = uvs[0] * w1 + uvs[1] * w2 + uvs[2] * w3;
							Point2D pixelcoord = { static_cast<int>(0.5 + uv.X * (framewidth - 1.0)), static_cast<int>(0.5 + uv.Y * (frameheight - 1.0)) };
							auto idx = pixels[framebound.Width * pixelcoord.Y + pixelcoord.X];
							auto color = *reinterpret_cast<PWORD>(&palette->Midpoint[idx * sizeof WORD]);
							*reinterpret_cast<PWORD>(&surfacedata[i * pitch + ras.v.x * 2]) = color;

							/*double value = w1 * values[0] + w2 * values[1] + w3 * values[2];
							static constexpr WORD lowrmask = 0b11111u;

							WORD mask = (static_cast<WORD>(lowrmask * value) << 11u) & 0xF800u;
							*reinterpret_cast<PWORD>(&surfacedata[i * pitch + ras.v.x * 2]) |= mask; */
						}
					}
				}
			}
			else // horizontal
			{
				for (const auto& ras : result.ranges)
				{
					if (ras.h.y >= bound.Y && ras.h.y < bound.Y + bound.Height)
					{
						int minpix = std::max(bound.X, ras.h.leftx);
						int maxpix = std::min(bound.X + bound.Width - 1, ras.h.rightx);

						double pixdiff = ras.h.rightx - ras.h.leftx;
						double wdiffs[] = { ras.h.right_weight[0] - ras.h.left_weight[0],ras.h.right_weight[1] - ras.h.left_weight[1],ras.h.right_weight[2] - ras.h.left_weight[2] };

						int row = ras.h.y;
						for (int i = minpix; i <= maxpix; i++)
						{
							double wlerp = pixdiff == 0.0 ? 0.0 : (i - ras.h.leftx) / pixdiff;
							double w1 = ras.h.left_weight[0] + wlerp * wdiffs[0],
								w2 = ras.h.left_weight[1] + wlerp * wdiffs[1],
								w3 = ras.h.left_weight[2] + wlerp * wdiffs[2];

							auto uv = uvs[0] * w1 + uvs[1] * w2 + uvs[2] * w3;
							Point2D pixelcoord = { static_cast<int>(0.5 + uv.X * (framewidth - 1.0)), static_cast<int>(0.5 + uv.Y * (frameheight - 1.0)) };
							auto idx = pixels[framebound.Width * pixelcoord.Y + pixelcoord.X];
							auto color = *reinterpret_cast<PWORD>(&palette->Midpoint[idx * sizeof WORD]);
							*reinterpret_cast<PWORD>(&surfacedata[row * pitch + i * 2]) = color;

							/*double value = w1 * values[0] + w2 * values[1] + w3 * values[2];
							static constexpr WORD lowrmask = 0b111111u;

							WORD mask = (static_cast<WORD>(lowrmask * value) << 5u) & 0x7E0u;
							*reinterpret_cast<PWORD>(&surfacedata[row * pitch + i * 2]) |= mask; */
						}
					}
				}
			}
		};

		shade(results[0], &shadeuvs[0]);
		shade(results[1], &shadeuvs[1]);

		surface->Unlock();
	}

	return 0;
}


//let's rasterize some lasers
DEFINE_HOOK(550D0F, LaserClass_Draw_Lines, 6)
{
	/*
	    00-lt                       rt-01
	Src ================================ Dst
	    10-lb                       rb-11
	*/
	static UniqueGamePtr<SHPReference> texture = {};

	if (!texture)
		texture.reset(new SHPReference("LASERMASK.SHP"));

	if (!texture || texture->HasCompression(0))
		return 0;

	GET(LaserDrawClass*, laser, EBX);
	GET_STACK(Point2D, screen_src, 0x7C);
	GET_STACK(Point2D, screen_dst, 0x74);
	GET(int, src_zadj, EBP);
	GET_STACK(int, dst_zadj, 0x48);
	
	using vec2 = YRRasterizer::vec2;
	using vec3 = Vector3D<double>;

	static const vec3 up = { 0.0,0.0,1.0 };
	static constexpr double half_thickness = 12.0;
	static const double pixels_to_leptons = 256.0 / 30.0 / sqrt(2.0);

	const auto modeldiff = laser->Target - laser->Source;
	const vec3 fmodeldiff = { modeldiff.X,modeldiff.Y,modeldiff.Z };
	vec3 left = up.CrossProduct(fmodeldiff);
	if (left.MagnitudeSquared() <= 0.0000001)
		left = { 1.0,-1.0,0.0 };

	left *= half_thickness * pixels_to_leptons / left.Magnitude();
	CoordStruct ileft = { static_cast<int>(left.X),static_cast<int>(left.Y),static_cast<int>(left.Z) },
		iltpoint = laser->Source + ileft;

	double deltaz = -TacticalClass::ZCoordsToScreenPixels(ileft.Z);
	Point2D iltscrpoint = { };
	TacticalClass::Instance->CoordsToClient(&iltpoint, &iltscrpoint);
	auto scr_diff = iltscrpoint - screen_src;

	vec2 fscr_diff = { scr_diff.X,scr_diff.Y };
	vec2 scrsrc = { screen_src.X,screen_src.Y }, scrdst = { screen_dst.X,screen_dst.Y };


	vec2 lt = scrsrc + fscr_diff, lb = scrsrc - fscr_diff, rt = scrdst + fscr_diff, rb = scrdst - fscr_diff;
	YRRasterizer::rasterize_result results[] = { YRRasterizer::rasterize_triangle(lt,lb,rt),YRRasterizer::rasterize_triangle(lb,rt,rb) };
	const auto surface = DSurface::Hidden_2;

	RectangleStruct bound = {};
	surface->GetRect(&bound);
	bound = Drawing::Intersect(&bound, &Drawing::SurfaceDimensions_Hidden, nullptr, nullptr);

	const auto zbuffer = ZBufferClass::ZBuffer;
	double zbase = zbuffer->rect.Y - bound.Y + zbuffer->CurrentBaseZ,
		src_zbase = zbase - scrsrc.Y + src_zadj, dst_zbase = zbase - scrdst.Y + dst_zadj;

	double zvalues[] = { src_zbase + deltaz,src_zbase - deltaz,dst_zbase + deltaz,dst_zbase - deltaz };
	static const vec2 texcoords[] = { {0.0,0.0},{1.0,0.0},{0.0,1.0},{1.0,1.0} };

	const auto pixels = texture->GetPixels(0);
	const auto surfacedata = reinterpret_cast<byte*>(surface->Lock(0, 0));

	if (!pixels || !surfacedata) {
		surface->Unlock();
		return 0;
	}

	const auto framebound = texture->GetFrameBounds(0);
	const double framewidth = framebound.Width, frameheight = framebound.Height;
	const int pitch = surface->GetPitch();
	const auto color = laser->InnerColor;
	const vec3 fcolor = { color.R,color.G,color.B };//255

	const auto shade = [&](const YRRasterizer::rasterize_result& result, const vec2 uvs[], const double zvals[]) {

		if (result.direction == YRRasterizer::rasterize_dir::vertical)
		{
			for (const auto& ras : result.ranges)
			{
				if (ras.v.x >= bound.X && ras.v.x < bound.X + bound.Width)
				{
					int minpix = std::max(bound.Y, ras.v.bottomy);
					int maxpix = std::min(bound.Y + bound.Height - 1, ras.v.topy);

					double pixdiff = ras.v.topy - ras.v.bottomy;
					double wdiffs[] = { ras.v.top_weight[0] - ras.v.bottom_weight[0],ras.v.top_weight[1] - ras.v.bottom_weight[1],ras.v.top_weight[2] - ras.v.bottom_weight[2] };
					auto zbuff = reinterpret_cast<PWORD>(zbuffer->AdjustedGetBufferAt({ ras.v.x,minpix }));

					for (int i = minpix; i <= maxpix; i++)
					{
						double wlerp = pixdiff == 0.0 ? 0.0 : (i - ras.v.bottomy) / pixdiff;
						double w1 = ras.v.bottom_weight[0] + wlerp * wdiffs[0],
							w2 = ras.v.bottom_weight[1] + wlerp * wdiffs[1],
							w3 = ras.v.bottom_weight[2] + wlerp * wdiffs[2];

						auto uv = uvs[0] * w1 + uvs[1] * w2 + uvs[2] * w3;
						int zval = static_cast<int>(zvals[0] * w1 + zvals[1] * w2 + zvals[2] * w3);
						if (zval <= *zbuff)
						{
							Point2D pixelcoord = { static_cast<int>(0.5 + uv.X * (framewidth - 1.0)), static_cast<int>(0.5 + uv.Y * (frameheight - 1.0)) };

							auto idx = pixels[framebound.Width * pixelcoord.Y + pixelcoord.X];
							double ratio = idx / 255.0;
							WORD& destclr = *reinterpret_cast<PWORD>(&surfacedata[i * pitch + ras.v.x * 2]);

							double b = (destclr & 0x001Fu) * 255.0 / 31.0, g = ((destclr & 0x07E0u) >> 5u) * 255.0 / 63.0, r = ((destclr & 0xF800u) >> 11u) * 255.0 / 31.0;
							WORD wr = static_cast<WORD>(std::clamp(fcolor.X * ratio + r, 0.0, 255.0)) >> 3u,
								wg = static_cast<WORD>(std::clamp(fcolor.Y * ratio + g, 0.0, 255.0)) >> 2u,
								wb = static_cast<WORD>(std::clamp(fcolor.Z * ratio + b, 0.0, 255.0)) >> 3u;

							destclr = (wr << 11u) | (wg << 5u) | (wb);
						}

						zbuff += zbuffer->W;
						if (reinterpret_cast<byte*>(zbuff) >= zbuffer->BufferEndpoint)
							zbuff -= zbuffer->BufferSize / 2u;
						// *reinterpret_cast<PWORD>(&surfacedata[i * pitch + ras.v.x * 2]) = color;
					}
				}
			}
		}
		else // horizontal
		{
			for (const auto& ras : result.ranges)
			{
				if (ras.h.y >= bound.Y && ras.h.y < bound.Y + bound.Height)
				{
					int minpix = std::max(bound.X, ras.h.leftx);
					int maxpix = std::min(bound.X + bound.Width - 1, ras.h.rightx);

					double pixdiff = ras.h.rightx - ras.h.leftx;
					double wdiffs[] = { ras.h.right_weight[0] - ras.h.left_weight[0],ras.h.right_weight[1] - ras.h.left_weight[1],ras.h.right_weight[2] - ras.h.left_weight[2] };
					int row = ras.h.y;
					auto zbuff = reinterpret_cast<PWORD>(zbuffer->AdjustedGetBufferAt({ minpix,row }));

					for (int i = minpix; i <= maxpix; i++)
					{
						double wlerp = pixdiff == 0.0 ? 0.0 : (i - ras.h.leftx) / pixdiff;
						double w1 = ras.h.left_weight[0] + wlerp * wdiffs[0],
							w2 = ras.h.left_weight[1] + wlerp * wdiffs[1],
							w3 = ras.h.left_weight[2] + wlerp * wdiffs[2];

						auto uv = uvs[0] * w1 + uvs[1] * w2 + uvs[2] * w3;
						int zval = static_cast<int>(zvals[0] * w1 + zvals[1] * w2 + zvals[2] * w3);
						if (zval <= *zbuff)
						{
							Point2D pixelcoord = { static_cast<int>(0.5 + uv.X * (framewidth - 1.0)), static_cast<int>(0.5 + uv.Y * (frameheight - 1.0)) };
							auto idx = pixels[framebound.Width * pixelcoord.Y + pixelcoord.X];
							double ratio = idx / 255.0;

							WORD& destclr = *reinterpret_cast<PWORD>(&surfacedata[row * pitch + i * 2]);

							double b = (destclr & 0x001Fu) * 255.0 / 31.0, g = ((destclr & 0x07E0u) >> 5u) * 255.0 / 63.0, r = ((destclr & 0xF800u) >> 11u) * 255.0 / 31.0;
							WORD wr = static_cast<WORD>(std::clamp(fcolor.X * ratio + r, 0.0, 255.0)) >> 3u,
								wg = static_cast<WORD>(std::clamp(fcolor.Y * ratio + g, 0.0, 255.0)) >> 2u,
								wb = static_cast<WORD>(std::clamp(fcolor.Z * ratio + b, 0.0, 255.0)) >> 3u;

							destclr = (wr << 11u) | (wg << 5u) | (wb);
						}

						zbuff++;
						if (reinterpret_cast<byte*>(zbuff) >= zbuffer->BufferEndpoint)
							zbuff -= zbuffer->BufferSize / 2u;
						/*
							auto color = *reinterpret_cast<PWORD>(&palette->Midpoint[idx * sizeof WORD]);
							*reinterpret_cast<PWORD>(&surfacedata[row * pitch + i * 2]) = color;
						*/
					}
				}
			}
		}
	};

	shade(results[0], &texcoords[0], &zvalues[0]);
	shade(results[1], &texcoords[1], &zvalues[1]);

	surface->Unlock();
	return 0x5511C3;
} 
