constant int2 fastpoints[16] = {{0, -3}, {1, -3}, {2, -2}, {3, -1}, {3, 0},
    {3, 1}, {2, 2}, {1, 3}, {0, 3}, {-1, 3}, {-2, 2}, {-3, 1}, {-3, 0},
    {-3, -1}, {-2, -2}, {-1, -3}};

inline short test_pixel(
    read_only image2d_t grayscale, short intensity, int2 pos, int index) {
    int2 pointpos = pos + fastpoints[index];
    short diff = (short)read_imageui(grayscale, pointpos).x - intensity;

    if (diff > FAST_THRESHOLD) {
        return diff - FAST_THRESHOLD;
    }

    if (diff < -FAST_THRESHOLD) {
        return diff + FAST_THRESHOLD;
    }
    return 0;
}

void kernel fast(read_only image2d_t grayscale, write_only image2d_t features) {
    int2 pos = (int2)(get_global_id(0), get_global_id(1));
    short itable[16];
    short intensity = (short)read_imageui(grayscale, pos).x;

    for (uint i = 0; i < 16; ++i) {
        itable[i] = test_pixel(grayscale, intensity, pos, i);
    }

    for (uint i, start = 0; start < 16; ++start) {
        if (itable[start]) {
            bool first_pos = itable[start] > 0;
            uint feature = 0;
            for (i = start + 1; i < start + FAST_POINTS; ++i) {
                int ri = i > 15 ? i - 16 : i;
                if (itable[ri] == 0 || (itable[ri] > 0) != first_pos) {
                    break;
                }
                feature += abs(itable[ri]);
            }

            if (i == start + FAST_POINTS) {
                write_imageui(features, pos, (uint4)(feature, 0, 0, 0));
                return;
            }
        }
    }

    write_imageui(features, pos, (uint4)(0, 0, 0, 0));
}

void kernel draw(read_only image2d_t features, global uchar* rgb) {
    int2 pos = (int2)(get_global_id(0), get_global_id(1));
    if (read_imageui(features, pos).x != 0) {
        int2 ppos;
        for (int i = 0; i < 16; ++i) {
            ppos = pos + fastpoints[i];
            uint dst = mul24(mad24(get_global_size(0), ppos.y, ppos.x), 3);
            rgb[dst] = 0;
            rgb[dst + 1] = 0xff;
            rgb[dst + 2] = 0;
        }
    }
}
