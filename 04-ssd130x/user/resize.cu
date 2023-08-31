#include <cuda.h>
#include <stdio.h>
#include <assert.h>

extern "C" {

__global__ void resize_frame_kernel(unsigned char *oframe, int ow, int oh,
				    unsigned char *nframe, int nw, int nh,
				    int threshold, unsigned int *locks)
{
	for (int i = blockDim.x * blockIdx.x + threadIdx.x; i < nw;
	     i += blockDim.x * gridDim.x) {
		for (int j = blockDim.y * blockIdx.y + threadIdx.y; j < nh;
		     j += blockDim.y * gridDim.y) {
			int oi = i * ow / nw;
			int oj = j * oh / nh;

			unsigned char b = oframe[oj * ow * 3 + oi * 3];
			unsigned char g = oframe[oj * ow * 3 + oi * 3 + 1];
			unsigned char r = oframe[oj * ow * 3 + oi * 3 + 2];

			unsigned char brightness =
				r * 0.3 + g * 0.59 + b * 0.11;
			brightness = brightness >= threshold ? 1 : 0;
			brightness = brightness << (j % 8);

			bool leaveloop = false;
			while (!leaveloop) {
				if (atomicExch(&locks[j / 8 * nw + i], 1u) ==
				    0u) {
					nframe[j / 8 * nw + i] |= brightness;
					leaveloop = true;
					atomicExch(&locks[j / 8 * nw + i], 0u);
				}
			}
		}
	}
}

void resize_frame(unsigned char *oframe, int ow, int oh, unsigned char *nframe,
		  int nw, int nh, int threshold)
{
	unsigned char *oframe_d;
	unsigned char *nframe_d;
	unsigned int *locks;

	cudaMalloc(&oframe_d, ow * oh * 3);
	cudaMalloc(&nframe_d, nw * nh / 8);
	cudaMalloc(&locks, sizeof(unsigned int) * nw * nh / 8);

	cudaMemcpy(oframe_d, oframe, ow * oh * 3, cudaMemcpyHostToDevice);
	cudaMemset(nframe_d, 0, nw * nh / 8);
	cudaMemset(locks, 0, sizeof(unsigned int) * nw * nh / 8);

	resize_frame_kernel<<<dim3(64, 64), dim3(16, 16)>>>(
		oframe_d, ow, oh, nframe_d, nw, nh, threshold, locks);

	cudaMemcpy(nframe, nframe_d, nw * nh / 8, cudaMemcpyDeviceToHost);

	cudaFree(oframe_d);
	cudaFree(nframe_d);
	cudaFree(locks);
}
}