inline void yuyv2rgb(const int y, const int u, const int v,
                     global uchar *result) {
    result[0] = clamp((int)(y + (1.402 * (v - 128))), 0, 255);
    result[1] = clamp((int)(y - 0.344 * (u - 128) -  0.714 *
                            (v - 128)), 0, 255);
    result[2] = clamp((int)(y + (1.772 * (u - 128))), 0, 255);
}

void kernel convert(global const uchar* yuyv,
                    global uchar* rgb) {
    int start = mul24(get_global_id(0), 4);
    int dst = 6 * mad24(get_group_id(0), get_local_size(0),
                        (int)(get_local_size(0) - get_local_id(0) - 1));
    yuyv2rgb(yuyv[start], yuyv[start + 1], yuyv[start + 3], &rgb[dst + 3]);
    yuyv2rgb(yuyv[start + 2], yuyv[start + 1], yuyv[start + 3], &rgb[dst]);
}
