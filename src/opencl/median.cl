void kernel median_mean(read_only image2d_t input, write_only image2d_t output) {
    int2 pos = (int2)(get_global_id(0), get_global_id(1));
    int2 min_pos = (int2)(max(pos.x - MEDIAN_KERNEL_OFFSET, 0),
                          max(pos.y - MEDIAN_KERNEL_OFFSET, 0));
    int2 max_pos = (int2)(min(pos.x + MEDIAN_KERNEL_OFFSET, (int)get_global_size(0)),
                          min(pos.y + MEDIAN_KERNEL_OFFSET, (int)get_global_size(1)));

    uint sum = 0, wi = 0;
    int2 cpos;
    for (cpos.y = min_pos.y; cpos.y <= max_pos.y; ++cpos.y) {
        for (cpos.x = min_pos.x; cpos.x <= max_pos.x; ++cpos.x, ++wi) {
            sum += read_imageui(input, cpos).w;
        }
    }
    write_imageui(output, pos, (uint4) (0, 0, 0, sum / wi));
}

void insertionSort(uchar *window, int size) {
    uchar temp;
    int j;
    for(int i = 0; i < size; ++i) {
        temp = window[i];
        for(j = i - 1; j >= 0 && temp < window[j]; --j) {
            window[j + 1] = window[j];
        }
        window[j + 1] = temp;
    }
}

void kernel median(read_only image2d_t input, write_only image2d_t output) {
    int2 pos = (int2)(get_global_id(0), get_global_id(1));
    int2 min_pos = (int2)(max(pos.x - MEDIAN_KERNEL_OFFSET, 0),
                          max(pos.y - MEDIAN_KERNEL_OFFSET, 0));
    int2 max_pos = (int2)(min(pos.x + MEDIAN_KERNEL_OFFSET, (int)get_global_size(0)),
                          min(pos.y + MEDIAN_KERNEL_OFFSET, (int)get_global_size(1)));

    uchar window[MEDIAN_WINDOW_SIZE];
    uint sum = 0, wi = 0;
    int2 cpos;
    for (cpos.y = min_pos.y; cpos.y <= max_pos.y; ++cpos.y) {
        for (cpos.x = min_pos.x; cpos.x <= max_pos.x; ++cpos.x, ++wi) {
            window[wi] = read_imageui(input, cpos).w;
        }
    }
    insertionSort(window, wi);
    write_imageui(output, pos, (uint4) (0, 0, 0, window[(wi >> 1) + 1]));
}
