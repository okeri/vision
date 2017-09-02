void insertionSort(uchar *window, int size) {
    int i , j;
    uchar temp;
    for(i = 0; i < size; i++) {
        temp = window[i];
        for(j = i-1; j >= 0 && temp < window[j]; j--) {
            window[j+1] = window[j];
        }
        window[j+1] = temp;
    }
}


void kernel median(global const uchar *input, global uchar *output,
                   uint stride) {
    const uint kernelOfs = 2;
    const uint windowSize = 25;
    const uint medianPos = 12;

    uchar window[windowSize];
    uint x = get_global_id(0);
    uint y = get_global_id(1);

    uint pixel = mad24(y, stride, mul24(x, 3u));


    if (x >= get_local_size(0) - kernelOfs || x < kernelOfs ||
        y >= get_local_size(1) - kernelOfs || y < kernelOfs) {
        for (uint channel = 0; channel < 3; ++ channel) {
            output[pixel + channel] = input[pixel + channel];
        }
    } else {
        for (uint channel = 0; channel < 3; ++ channel) {
            for (uint wi = 0, fy = y - kernelOfs; fy <= y + kernelOfs; ++fy) {
                uint linestart = mul24(fy,  stride);
                for (uint fx = x - kernelOfs; fx <= x + kernelOfs; ++fx) {
                    window[wi++] = input[linestart + mad24(fx, 3u, channel)];
                }
            }
            insertionSort(window, windowSize);
            output[pixel + channel] = window[medianPos];
        }
    }
}
