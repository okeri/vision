const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE;

/* a little bit optimized*/
void kernel median_mean(read_only image2d_t input, write_only image2d_t output,
                        uint kernelSize) {
    int kernelOfs = kernelSize  >> 1;
    int2 pos = (int2)(get_global_id(0), get_global_id(1));
    int2 min_pos = (int2)(max(pos.x - kernelOfs, 0),
                          max(pos.y - kernelOfs, 0));
    int2 max_pos = (int2)(min(pos.x + kernelOfs, (int)get_global_size(0)),
                          min(pos.y + kernelOfs, (int)get_global_size(1)));

    uint sum = 0, wi = 0;

    for (int ypos = min_pos.y; ypos <= max_pos.y; ++ypos) {
        for (int xpos = min_pos.x; xpos <= max_pos.x; ++xpos, ++wi) {
            sum += read_imageui(input, sampler, (int2)(xpos, ypos)).w;
        }
    }
    uint v = sum / wi;
    write_imageui(output, pos, (uint4) (v, v, v, v));
}
