constant int2 fastpoints[16] = {
    {0, -3}, {1, -3},
    {2, -2},
    {3, -1}, {3, 0}, {3, 1},
    {2, 2},
    {1, 3}, {0, 3}, {-1, 3},
    {-2, 2},
    {-3, -1}, {-3, 0}, {-3, 1},
    {-2, -2},
    {1, -3}};

inline short test(read_only image2d_t grayscale, short intensity, int2 pos, int index) {
    int2 pointpos = pos + fastpoints[index];
    short diff = (short)read_imageui(grayscale, pointpos).w - intensity;

    if (diff > FAST_THRESHOLD) {
        return diff - FAST_THRESHOLD;
    }

    if (diff < -FAST_THRESHOLD) {
        return diff + FAST_THRESHOLD;
    }
    return 0;
}

void kernel fast(read_only image2d_t grayscale,  write_only image2d_t features) {
    int2 pos = (int2)(get_global_id(0), get_global_id(1));
    short itable[16];
    short intensity = (short)read_imageui(grayscale, pos).w;

    itable[0] = test(grayscale, intensity, pos, 0);
    itable[8] = test(grayscale, intensity, pos, 8);

    /* if ((FAST_POINTS >= 12 && (itable[0] > 0 != itable[8] > 0)) || itable[0] == 0) { */
    /*     write_imageui(features, pos, (uint4)(0, 0, 0, 0)); */
    /*     return; */
    /* } */

    itable[4] = test(grayscale, intensity, pos, 4);
    itable[12] = test(grayscale, intensity, pos, 12);

    /* if ((FAST_POINTS >= 12 && (itable[4] > 0 != itable[12] > 0)) || itable[4] == 0) { */
    /*     write_imageui(features, pos, (uint4)(0, 0, 0, 0)); */
    /*     return; */
    /* } */
    itable[1] = test(grayscale, intensity, pos, 1);
    itable[2] = test(grayscale, intensity, pos, 2);
    itable[3] = test(grayscale, intensity, pos, 3);

    itable[5] = test(grayscale, intensity, pos, 5);
    itable[6] = test(grayscale, intensity, pos, 6);
    itable[7] = test(grayscale, intensity, pos, 7);

    itable[9] = test(grayscale, intensity, pos, 9);
    itable[10] = test(grayscale, intensity, pos, 10);
    itable[11] = test(grayscale, intensity, pos, 11);

    itable[13] = test(grayscale, intensity, pos, 13);
    itable[14] = test(grayscale, intensity, pos, 14);
    itable[15] = test(grayscale, intensity, pos, 15);

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
                write_imageui(features, pos, (uint4)(0, 0, 0, feature));
                return;
            }
        }
    }
    write_imageui(features, pos, (uint4)(0, 0, 0, 0));
}

void kernel fastNonMax(read_only image2d_t allfeatures, write_only image2d_t features) {
    int2 pos = (int2)(get_global_id(0), get_global_id(1));
    if (pos.x > 3 && pos.y > 3 && pos.x < get_global_size(0) - 3 && pos.y < get_global_size(1) - 3) {
        int2 wpos;
        uint feature = read_imageui(allfeatures, pos).w;
        for (wpos.y = pos.y - 3; wpos.y <= pos.y + 3; ++wpos.y) {
            for (wpos.x = pos.x - 3; wpos.x <= pos.x + 3; ++wpos.x) {
                if (read_imageui(allfeatures, wpos).w > feature) {
                    write_imageui(features, pos, (uint4)(0, 0, 0, 0));
                    return;
                }
            }
        }
        write_imageui(features, pos, (uint4)(0, 0, 0, feature));
    } else {
        write_imageui(features, pos, (uint4)(0, 0, 0, 0));
    }
}

void kernel draw(read_only image2d_t features, global uchar* rgb) {
    int2 pos = (int2)(get_global_id(0), get_global_id(1));
    if (read_imageui(features, pos).w != 0) {
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
