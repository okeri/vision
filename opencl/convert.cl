inline void yuyv2rgb_local(const int y, const int u, const int v,
                           global uchar *result) {
    result[0] = clamp((int)(y + (1.402 * (v - 128))), 0, 255);
    result[1] = clamp((int)(y - 0.344 * (u - 128) -  0.714 *
                            (v - 128)), 0, 255);
    result[2] = clamp((int)(y + (1.772 * (u - 128))), 0, 255);
}

void kernel yuyv2rgb_flip(global const uchar* yuyv,
                     global uchar* rgb) {
    int start = get_global_id(0) << 2;
    int dst = 6 * mad24(get_group_id(0), get_local_size(0),
                        (int)(get_local_size(0) - get_local_id(0) - 1));

    yuyv2rgb_local(yuyv[start], yuyv[start + 1], yuyv[start + 3], &rgb[dst + 3]);
    yuyv2rgb_local(yuyv[start + 2], yuyv[start + 1], yuyv[start + 3], &rgb[dst]);
}

void kernel yuyv2rgb(global const uchar* yuyv,
                     global uchar* rgb) {
    int start = get_global_id(0) << 2;
    int dst = mul24(get_global_id(0), 3) << 1;
    yuyv2rgb_local(yuyv[start], yuyv[start + 1], yuyv[start + 3], &rgb[dst]);
    yuyv2rgb_local(yuyv[start + 2], yuyv[start + 1], yuyv[start + 3], &rgb[dst + 3]);
}


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

void kernel rgb2gs(global const uchar* rgb,
                   global uchar* grayscale) {
    int pixel = get_global_id(0);
    int src = mul24(pixel, 3);
    grayscale[pixel] = (rgb[src] + rgb[src + 1] * 5 + (rgb[src + 2] << 1)) >> 3;
}

void kernel yuyv2gs_flip(global const uchar* yuyv,
                    global uchar* grayscale) {
    int src = get_global_id(0) << 2;
    int dst = mad24(get_group_id(0), get_local_size(0),
                    (int)(get_local_size(0) - get_local_id(0) - 1)) << 1;
    grayscale[dst] = yuyv[src];
    grayscale[dst + 1] = yuyv[src + 2];
}

void kernel yuyv2gs(global const uchar* yuyv,
                    global uchar* grayscale) {
    int src = get_global_id(0) << 2;
    int dst = get_global_id(0) << 1;
    grayscale[dst] = yuyv[src];
    grayscale[dst + 1] = yuyv[src + 2];
}
