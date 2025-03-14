# Order Independent Transparency

![OIT](screenshot.jpg)

Transparency is a tricky thing in rasterization.  Because the order in which the transparent triangles are drawn matters.  You can sort your triangles by depth and using the painter's algorithm, but this doesn't solve the problem of overlapping/intersecting triangles.

A-buffer OIT is the oldest technique that I am aware exists.  It stores a per pixel linked list of transparent fragments/pixels that can be sorted and drawn in the correct order.

Depth peeling was developed in the early 2000's.  It worked by using two different depth buffers with multiple render passes to properly draw the transparent triangles.

Stochastic OIT is screendoor transparency that uses the alpha channel of the fragment/pixel and a random number to determine if the fragment/pixel should be drawn.  This is the technique used in this sample.  The technique is described in the paper "Stochastic Transparency" and can be found here: https://research.nvidia.com/publication/2011-08_stochastic-transparency

This version differs from the author's original paper, as it doesn't use an msaa buffer to improve sampling, but instead relies on TAA to converge the fragments over time.


https://github.com/GameTechDev/AOIT-Update

https://interplayoflight.wordpress.com/2022/06/25/order-independent-transparency-part-1/
https://interplayoflight.wordpress.com/2022/07/02/order-independent-transparency-part-2/

https://developer.apple.com/documentation/metal/metal_sample_code_library/implementing_order-independent_transparency_with_image_blocks

https://ubm-twvideo01.s3.amazonaws.com/o1/vault/GDC2014/Presentations/Gareth_Thomas_Compute-based_GPU_Particle.pdf

tiled order independent transparency

https://en.wikipedia.org/wiki/Order-independent_transparency