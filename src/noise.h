#ifndef NOISE_H
#define NOISE_H

float perlin2d(float x, float y, unsigned int seed);
float fbm2d(float x, float y, int octaves, float lacunarity, float gain, unsigned int seed);

#endif
