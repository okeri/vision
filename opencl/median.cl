/* a little bit optimized*/
void kernel median_mean(global const uchar *input, global uchar *output,
                        uint channels, uint kernelSize) {
    int kernelOfs = kernelSize  >> 1;
    uint2 min_pos = (uint2)(max((int)get_global_id(0) - kernelOfs, 0),
                            max((int)get_global_id(1) - kernelOfs, 0));
    uint2 max_pos = (uint2)(min(get_global_id(0) + kernelOfs, get_global_size(0)),
                            min(get_global_id(1) + kernelOfs, get_global_size(1)));
    size_t stride = get_global_size(0) * channels;

    for (size_t channel = 0; channel < channels; ++channel) {
        uint average = 0, wi = 0;

        for (uint ypos = mad24(min_pos.y, stride, channel);
             ypos <= mad24(max_pos.y, stride, channel); ypos += stride) {

            for (uint xpos = mad24(min_pos.x, channels, ypos);
                 xpos <= mad24(max_pos.x, channels, ypos); xpos += channels, ++wi) {
                average += input[xpos];
            }
        }
        output[get_global_id(1) * stride + get_global_id(0) * channels +
               channel] = average / wi;
    }
}

/* TODO: replace with nth-element or partial_sort */
void insertionSort(global uchar *window, int size) {
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

void kernel median(global const uchar *input, global uchar *output,
                   uint channels, uint kernelSize, global uchar *window) {
    int kernelOfs = kernelSize  >> 1;

    uint2 min_pos = (uint2)(max((int)get_global_id(0) - kernelOfs, 0),
                            max((int)get_global_id(1) - kernelOfs, 0));
    uint2 max_pos = (uint2)(min(get_global_id(0) + kernelOfs, get_global_size(0)),
                            min(get_global_id(1) + kernelOfs, get_global_size(1)));
    size_t stride = get_global_size(0) * channels;
    uint windowSize = kernelSize * kernelSize;

    for (size_t channel = 0; channel < channels; ++channel) {
        uint wi = 0;
        for (uint ypos = mad24(min_pos.y, stride, channel);
             ypos <= mad24(max_pos.y, stride, channel); ypos += stride) {

            for (uint xpos = mad24(min_pos.x, channels, ypos);
                 xpos <= mad24(max_pos.x, channels, ypos); xpos += channels) {
                window[wi++] = input[xpos];
            }
        }
        insertionSort(window, windowSize);
        output[get_global_id(1) * stride + get_global_id(0) * channels +
               channel] = window[(windowSize >> 1) + 1];
    }
}
