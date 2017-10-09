inline void localpoints16(uint p, uint stride, uint *points) {
    points[4] = p + 3;
    points[3] = points[4] - stride;
    points[5] = points[4] + stride;

    points[6] = points[5] + stride - 1;

    points[7] = points[6] + stride - 1;
    points[8] = points[7] - 1;
    points[9] = points[8] - 1;

    points[10] = points[9] - stride - 1;

    points[12] = p - 3;
    points[11] = points[12] + stride;
    points[13] = points[12] - stride;

    points[14] = points[13] - stride + 1;

    points[15] = points[14] - stride + 1;
    points[0] = points[15] + 1;
    points[1] = points[0] + 1;

    points[2] = points[1] + stride + 1;
}

inline short test(global const uchar* grayscale, int p, int index) {
    short diff = (short)grayscale[index] - (short)grayscale[p];

    if (diff > FAST_THRESHOLD) {
        return diff - FAST_THRESHOLD;
    }

    if (diff < -FAST_THRESHOLD) {
        return diff + FAST_THRESHOLD;
    }
    return 0;
}

void kernel fast(global const uchar* grayscale,  global short *features) {
    uint keytable[16];
    short itable[16];
    size_t stride = get_global_size(0) + 6u;
    size_t p =  get_global_id(1) * stride + get_global_id(0);

    localpoints16(p, stride, keytable);
    features[p] = 0;

    itable[0] = test(grayscale, p, keytable[0]);
    itable[8] = test(grayscale, p, keytable[8]);
    itable[4] = test(grayscale, p, keytable[4]);
    itable[12] = test(grayscale, p, keytable[12]);

    itable[1] = test(grayscale, p, keytable[1]);
    itable[2] = test(grayscale, p, keytable[2]);
    itable[3] = test(grayscale, p, keytable[3]);
    itable[5] = test(grayscale, p, keytable[5]);
    itable[6] = test(grayscale, p, keytable[6]);
    itable[7] = test(grayscale, p, keytable[7]);
    itable[9] = test(grayscale, p, keytable[9]);
    itable[10] = test(grayscale, p, keytable[10]);
    itable[11] = test(grayscale, p, keytable[11]);
    itable[13] = test(grayscale, p, keytable[13]);
    itable[14] = test(grayscale, p, keytable[14]);
    itable[15] = test(grayscale, p, keytable[15]);

    for (uint i, start = 0; start < 16; ++start) {
        if (itable[start]) {
            bool first_pos = itable[start] > 0;
            short feature = 0;
            for (i = start + 1; i < start + FAST_POINTS - 1; ++i) {
                int ri = i > 15 ? i - 16 : i;
                if (itable[ri] == 0 || (itable[ri] > 0) != first_pos) {
                    break;
                }
                feature += itable[ri];
            }

            if (i == start + FAST_POINTS - 1) {
                features[p] = abs(feature);
                break;
            }
        }
    }
}

void kernel fastNonMax(global const short *allfeatures, global short *features) {
    uint x = get_global_id(0);
    uint y = get_global_id(1);
    uint stride = get_global_size(0);
    uint pixel = stride * y + x;
    if (x > 3 && y > 3 && x < stride - 3 && y < get_global_size(1) - 3)  {
        if (features[pixel - stride - stride - 2] > features[pixel] ||
            features[pixel - stride - stride - 1] > features[pixel] ||
            features[pixel - stride - stride] > features[pixel] ||
            features[pixel - stride - stride + 1] > features[pixel] ||
            features[pixel - stride - stride + 2] > features[pixel] ||
            features[pixel - stride - 2] > features[pixel] ||
            features[pixel - stride - 1] > features[pixel] ||
            features[pixel - stride] > features[pixel] ||
            features[pixel - stride + 1] > features[pixel] ||
            features[pixel - stride - 2] > features[pixel] ||
            features[pixel - 1] > features[pixel] ||
            features[pixel + 1] > features[pixel] ||
            features[pixel - 2] > features[pixel] ||
            features[pixel + 2] > features[pixel] ||
            features[pixel + stride - 2] > features[pixel] ||
            features[pixel + stride - 1] > features[pixel] ||
            features[pixel + stride] > features[pixel] ||
            features[pixel + stride + 1] > features[pixel] ||
            features[pixel + stride + 2] > features[pixel] ||
            features[pixel + stride + stride - 2] > features[pixel] ||
            features[pixel + stride + stride - 1] > features[pixel] ||
            features[pixel + stride + stride] > features[pixel] ||
            features[pixel + stride + stride + 1] > features[pixel] ||
            features[pixel + stride + stride + 2] > features[pixel]) {
            features[pixel] = 0;
        } else {
            features[pixel] = allfeatures[pixel];
        }
    } else {
        features[pixel] = 0;
    }
}

void kernel draw(global const short *features, global uchar* rgb) {
    uint stride = 640;
    if ((features[get_global_id(0)] != 0) && get_global_id(0) > stride * 3) {
        uint keytable[16];
        localpoints16(get_global_id(0), stride, keytable);
        for (int i = 0; i < 16; ++i) {
            uint dst = mul24(keytable[i], 3u);
            rgb[dst] = 0;
            rgb[dst + 1] = 0xff;
            rgb[dst + 2] = 0;
        }
    }
}
