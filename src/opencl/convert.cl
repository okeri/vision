/* simple rgb conversions */
void kernel argb2rgb(global const uchar* argb,
                     global uchar* rgb) {
    int src = mad24(get_global_id(0), 4, 1);
    int dst = mul24(get_global_id(0), 3);
    rgb[dst] = argb[src];
    rgb[dst + 1] = argb[src + 1];
    rgb[dst + 2] = argb[src + 2];
}

void kernel rgba2rgb(global const uchar* argb,
                     global uchar* rgb) {
    int src = mul24(get_global_id(0), 4);
    int dst = mul24(get_global_id(0), 3);
    rgb[dst] = argb[src];
    rgb[dst + 1] = argb[src + 1];
    rgb[dst + 2] = argb[src + 2];
}

void kernel bgr2rgb(global const uchar* bgr,
                    global uchar* rgb) {
    int pixel = mul24(get_global_id(0), 3);
    rgb[pixel] = bgr[pixel + 2];
    rgb[pixel + 1] = bgr[pixel + 1];
    rgb[pixel + 2] = bgr[pixel];
}

/* yuyv to rgb conversions */
inline void yuyv2rgb_local(const int y, const int u, const int v,
                           global uchar *result) {
    result[0] = clamp((int)(y + (1.402 * (v - 128))), 0, 255);
    result[1] = clamp((int)(y - 0.344 * (u - 128) -  0.714 *
                            (v - 128)), 0, 255);
    result[2] = clamp((int)(y + (1.772 * (u - 128))), 0, 255);
}

void kernel yuyv2rgb(global const uchar* yuyv,
                     global uchar* rgb) {
    int start = get_global_id(0) << 2;
    int dst = mul24(get_global_id(0), 3) << 1;
    yuyv2rgb_local(yuyv[start], yuyv[start + 1], yuyv[start + 3], &rgb[dst]);
    yuyv2rgb_local(yuyv[start + 2], yuyv[start + 1], yuyv[start + 3], &rgb[dst + 3]);
}

/* image2d conversions*/
void kernel yuyv2gs(global const uchar* yuyv,
		    write_only image2d_t grayscale) {
    int src = get_global_id(0) << 2;
    int2 dst = (int2)(get_local_id(0) << 1, get_group_id(0));
    write_imageui(grayscale, dst, (uint4)(0,0,0,yuyv[src]));
    dst.x++;
    write_imageui(grayscale, dst, (uint4)(0,0,0,yuyv[src + 2]));
}

void kernel rgb2gs(global const uchar* rgb,
		   write_only image2d_t grayscale) {
	int src = mul24(get_global_id(0), 3);
	int2 dst = (int2)(get_local_id(0), get_group_id(0));
	write_imageui(grayscale, dst,
		      (uint4)(0, 0, 0, (rgb[src] + rgb[src + 1] * 5 + (rgb[src + 2] << 1)) >> 3));
}

/*test*/
void kernel gs2r(read_only image2d_t grayscale, global uchar* rgb) {
	int dst = mul24(get_global_size(0) * get_global_id(1) + get_global_id(0), 3);
	int2 src = (int2)(get_global_id(0), get_global_id(1));
	uint value = read_imageui(grayscale, src).w;
	rgb[dst] = value;
	rgb[dst + 1] = value;
	rgb[dst + 2] = value;
}
