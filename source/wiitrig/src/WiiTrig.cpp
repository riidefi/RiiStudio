#include "WiiTrig.hpp"

#include <cmath>
#include <math.h>
#include <stdlib.h>

namespace rlibrii::g3d {

struct SinCosLUTEntry {
  f32 sin;
  f32 cos;
  f32 sin_prime;
  f32 cos_prime;
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
static const SinCosLUTEntry SinCosLUT[] = {
    0.f,        1.f,        0.024541f,  -0.000301f, 0.024541f,  0.999699f,
    0.024526f,  -0.000903f, 0.049068f,  0.998795f,  0.024497f,  -0.001505f,
    0.073565f,  0.99729f,   0.024453f,  -0.002106f, 0.098017f,  0.995185f,
    0.024394f,  -0.002705f, 0.122411f,  0.99248f,   0.02432f,   -0.003303f,
    0.14673f,   0.989177f,  0.024231f,  -0.003899f, 0.170962f,  0.985278f,
    0.024128f,  -0.004492f, 0.19509f,   0.980785f,  0.024011f,  -0.005083f,
    0.219101f,  0.975702f,  0.023879f,  -0.005671f, 0.24298f,   0.970031f,
    0.023733f,  -0.006255f, 0.266713f,  0.963776f,  0.023572f,  -0.006836f,
    0.290285f,  0.95694f,   0.023397f,  -0.007412f, 0.313682f,  0.949528f,
    0.023208f,  -0.007984f, 0.33689f,   0.941544f,  0.023005f,  -0.008551f,
    0.359895f,  0.932993f,  0.022788f,  -0.009113f, 0.382683f,  0.92388f,
    0.022558f,  -0.00967f,  0.405241f,  0.91421f,   0.022314f,  -0.01022f,
    0.427555f,  0.903989f,  0.022056f,  -0.010765f, 0.449611f,  0.893224f,
    0.021785f,  -0.011303f, 0.471397f,  0.881921f,  0.021501f,  -0.011834f,
    0.492898f,  0.870087f,  0.021205f,  -0.012358f, 0.514103f,  0.857729f,
    0.020895f,  -0.012875f, 0.534998f,  0.844854f,  0.020573f,  -0.013384f,
    0.55557f,   0.83147f,   0.020238f,  -0.013885f, 0.575808f,  0.817585f,
    0.019891f,  -0.014377f, 0.595699f,  0.803208f,  0.019532f,  -0.014861f,
    0.615232f,  0.788346f,  0.019162f,  -0.015336f, 0.634393f,  0.77301f,
    0.01878f,   -0.015802f, 0.653173f,  0.757209f,  0.018386f,  -0.016258f,
    0.671559f,  0.740951f,  0.017982f,  -0.016704f, 0.689541f,  0.724247f,
    0.017566f,  -0.01714f,  0.707107f,  0.707107f,  0.01714f,   -0.017566f,
    0.724247f,  0.689541f,  0.016704f,  -0.017982f, 0.740951f,  0.671559f,
    0.016258f,  -0.018386f, 0.757209f,  0.653173f,  0.015802f,  -0.01878f,
    0.77301f,   0.634393f,  0.015336f,  -0.019162f, 0.788346f,  0.615232f,
    0.014861f,  -0.019532f, 0.803208f,  0.595699f,  0.014377f,  -0.019891f,
    0.817585f,  0.575808f,  0.013885f,  -0.020238f, 0.83147f,   0.55557f,
    0.013384f,  -0.020573f, 0.844854f,  0.534998f,  0.012875f,  -0.020895f,
    0.857729f,  0.514103f,  0.012358f,  -0.021205f, 0.870087f,  0.492898f,
    0.011834f,  -0.021501f, 0.881921f,  0.471397f,  0.011303f,  -0.021785f,
    0.893224f,  0.449611f,  0.010765f,  -0.022056f, 0.903989f,  0.427555f,
    0.01022f,   -0.022314f, 0.91421f,   0.405241f,  0.00967f,   -0.022558f,
    0.92388f,   0.382683f,  0.009113f,  -0.022788f, 0.932993f,  0.359895f,
    0.008551f,  -0.023005f, 0.941544f,  0.33689f,   0.007984f,  -0.023208f,
    0.949528f,  0.313682f,  0.007412f,  -0.023397f, 0.95694f,   0.290285f,
    0.006836f,  -0.023572f, 0.963776f,  0.266713f,  0.006255f,  -0.023733f,
    0.970031f,  0.24298f,   0.005671f,  -0.023879f, 0.975702f,  0.219101f,
    0.005083f,  -0.024011f, 0.980785f,  0.19509f,   0.004492f,  -0.024128f,
    0.985278f,  0.170962f,  0.003899f,  -0.024231f, 0.989177f,  0.14673f,
    0.003303f,  -0.02432f,  0.99248f,   0.122411f,  0.002705f,  -0.024394f,
    0.995185f,  0.098017f,  0.002106f,  -0.024453f, 0.99729f,   0.073565f,
    0.001505f,  -0.024497f, 0.998795f,  0.049068f,  0.000903f,  -0.024526f,
    0.999699f,  0.024541f,  0.000301f,  -0.024541f, 1.f,        0.f,
    -0.000301f, -0.024541f, 0.999699f,  -0.024541f, -0.000903f, -0.024526f,
    0.998795f,  -0.049068f, -0.001505f, -0.024497f, 0.99729f,   -0.073565f,
    -0.002106f, -0.024453f, 0.995185f,  -0.098017f, -0.002705f, -0.024394f,
    0.99248f,   -0.122411f, -0.003303f, -0.02432f,  0.989177f,  -0.14673f,
    -0.003899f, -0.024231f, 0.985278f,  -0.170962f, -0.004492f, -0.024128f,
    0.980785f,  -0.19509f,  -0.005083f, -0.024011f, 0.975702f,  -0.219101f,
    -0.005671f, -0.023879f, 0.970031f,  -0.24298f,  -0.006255f, -0.023733f,
    0.963776f,  -0.266713f, -0.006836f, -0.023572f, 0.95694f,   -0.290285f,
    -0.007412f, -0.023397f, 0.949528f,  -0.313682f, -0.007984f, -0.023208f,
    0.941544f,  -0.33689f,  -0.008551f, -0.023005f, 0.932993f,  -0.359895f,
    -0.009113f, -0.022788f, 0.92388f,   -0.382683f, -0.00967f,  -0.022558f,
    0.91421f,   -0.405241f, -0.01022f,  -0.022314f, 0.903989f,  -0.427555f,
    -0.010765f, -0.022056f, 0.893224f,  -0.449611f, -0.011303f, -0.021785f,
    0.881921f,  -0.471397f, -0.011834f, -0.021501f, 0.870087f,  -0.492898f,
    -0.012358f, -0.021205f, 0.857729f,  -0.514103f, -0.012875f, -0.020895f,
    0.844854f,  -0.534998f, -0.013384f, -0.020573f, 0.83147f,   -0.55557f,
    -0.013885f, -0.020238f, 0.817585f,  -0.575808f, -0.014377f, -0.019891f,
    0.803208f,  -0.595699f, -0.014861f, -0.019532f, 0.788346f,  -0.615232f,
    -0.015336f, -0.019162f, 0.77301f,   -0.634393f, -0.015802f, -0.01878f,
    0.757209f,  -0.653173f, -0.016258f, -0.018386f, 0.740951f,  -0.671559f,
    -0.016704f, -0.017982f, 0.724247f,  -0.689541f, -0.01714f,  -0.017566f,
    0.707107f,  -0.707107f, -0.017566f, -0.01714f,  0.689541f,  -0.724247f,
    -0.017982f, -0.016704f, 0.671559f,  -0.740951f, -0.018386f, -0.016258f,
    0.653173f,  -0.757209f, -0.01878f,  -0.015802f, 0.634393f,  -0.77301f,
    -0.019162f, -0.015336f, 0.615232f,  -0.788346f, -0.019532f, -0.014861f,
    0.595699f,  -0.803208f, -0.019891f, -0.014377f, 0.575808f,  -0.817585f,
    -0.020238f, -0.013885f, 0.55557f,   -0.83147f,  -0.020573f, -0.013384f,
    0.534998f,  -0.844854f, -0.020895f, -0.012875f, 0.514103f,  -0.857729f,
    -0.021205f, -0.012358f, 0.492898f,  -0.870087f, -0.021501f, -0.011834f,
    0.471397f,  -0.881921f, -0.021785f, -0.011303f, 0.449611f,  -0.893224f,
    -0.022056f, -0.010765f, 0.427555f,  -0.903989f, -0.022314f, -0.01022f,
    0.405241f,  -0.91421f,  -0.022558f, -0.00967f,  0.382683f,  -0.92388f,
    -0.022788f, -0.009113f, 0.359895f,  -0.932993f, -0.023005f, -0.008551f,
    0.33689f,   -0.941544f, -0.023208f, -0.007984f, 0.313682f,  -0.949528f,
    -0.023397f, -0.007412f, 0.290285f,  -0.95694f,  -0.023572f, -0.006836f,
    0.266713f,  -0.963776f, -0.023733f, -0.006255f, 0.24298f,   -0.970031f,
    -0.023879f, -0.005671f, 0.219101f,  -0.975702f, -0.024011f, -0.005083f,
    0.19509f,   -0.980785f, -0.024128f, -0.004492f, 0.170962f,  -0.985278f,
    -0.024231f, -0.003899f, 0.14673f,   -0.989177f, -0.02432f,  -0.003303f,
    0.122411f,  -0.99248f,  -0.024394f, -0.002705f, 0.098017f,  -0.995185f,
    -0.024453f, -0.002106f, 0.073565f,  -0.99729f,  -0.024497f, -0.001505f,
    0.049068f,  -0.998795f, -0.024526f, -0.000903f, 0.024541f,  -0.999699f,
    -0.024541f, -0.000301f, 0.f,        -1.f,       -0.024541f, 0.000301f,
    -0.024541f, -0.999699f, -0.024526f, 0.000903f,  -0.049068f, -0.998795f,
    -0.024497f, 0.001505f,  -0.073565f, -0.99729f,  -0.024453f, 0.002106f,
    -0.098017f, -0.995185f, -0.024394f, 0.002705f,  -0.122411f, -0.99248f,
    -0.02432f,  0.003303f,  -0.14673f,  -0.989177f, -0.024231f, 0.003899f,
    -0.170962f, -0.985278f, -0.024128f, 0.004492f,  -0.19509f,  -0.980785f,
    -0.024011f, 0.005083f,  -0.219101f, -0.975702f, -0.023879f, 0.005671f,
    -0.24298f,  -0.970031f, -0.023733f, 0.006255f,  -0.266713f, -0.963776f,
    -0.023572f, 0.006836f,  -0.290285f, -0.95694f,  -0.023397f, 0.007412f,
    -0.313682f, -0.949528f, -0.023208f, 0.007984f,  -0.33689f,  -0.941544f,
    -0.023005f, 0.008551f,  -0.359895f, -0.932993f, -0.022788f, 0.009113f,
    -0.382683f, -0.92388f,  -0.022558f, 0.00967f,   -0.405241f, -0.91421f,
    -0.022314f, 0.01022f,   -0.427555f, -0.903989f, -0.022056f, 0.010765f,
    -0.449611f, -0.893224f, -0.021785f, 0.011303f,  -0.471397f, -0.881921f,
    -0.021501f, 0.011834f,  -0.492898f, -0.870087f, -0.021205f, 0.012358f,
    -0.514103f, -0.857729f, -0.020895f, 0.012875f,  -0.534998f, -0.844854f,
    -0.020573f, 0.013384f,  -0.55557f,  -0.83147f,  -0.020238f, 0.013885f,
    -0.575808f, -0.817585f, -0.019891f, 0.014377f,  -0.595699f, -0.803208f,
    -0.019532f, 0.014861f,  -0.615232f, -0.788346f, -0.019162f, 0.015336f,
    -0.634393f, -0.77301f,  -0.01878f,  0.015802f,  -0.653173f, -0.757209f,
    -0.018386f, 0.016258f,  -0.671559f, -0.740951f, -0.017982f, 0.016704f,
    -0.689541f, -0.724247f, -0.017566f, 0.01714f,   -0.707107f, -0.707107f,
    -0.01714f,  0.017566f,  -0.724247f, -0.689541f, -0.016704f, 0.017982f,
    -0.740951f, -0.671559f, -0.016258f, 0.018386f,  -0.757209f, -0.653173f,
    -0.015802f, 0.01878f,   -0.77301f,  -0.634393f, -0.015336f, 0.019162f,
    -0.788346f, -0.615232f, -0.014861f, 0.019532f,  -0.803208f, -0.595699f,
    -0.014377f, 0.019891f,  -0.817585f, -0.575808f, -0.013885f, 0.020238f,
    -0.83147f,  -0.55557f,  -0.013384f, 0.020573f,  -0.844854f, -0.534998f,
    -0.012875f, 0.020895f,  -0.857729f, -0.514103f, -0.012358f, 0.021205f,
    -0.870087f, -0.492898f, -0.011834f, 0.021501f,  -0.881921f, -0.471397f,
    -0.011303f, 0.021785f,  -0.893224f, -0.449611f, -0.010765f, 0.022056f,
    -0.903989f, -0.427555f, -0.01022f,  0.022314f,  -0.91421f,  -0.405241f,
    -0.00967f,  0.022558f,  -0.92388f,  -0.382683f, -0.009113f, 0.022788f,
    -0.932993f, -0.359895f, -0.008551f, 0.023005f,  -0.941544f, -0.33689f,
    -0.007984f, 0.023208f,  -0.949528f, -0.313682f, -0.007412f, 0.023397f,
    -0.95694f,  -0.290285f, -0.006836f, 0.023572f,  -0.963776f, -0.266713f,
    -0.006255f, 0.023733f,  -0.970031f, -0.24298f,  -0.005671f, 0.023879f,
    -0.975702f, -0.219101f, -0.005083f, 0.024011f,  -0.980785f, -0.19509f,
    -0.004492f, 0.024128f,  -0.985278f, -0.170962f, -0.003899f, 0.024231f,
    -0.989177f, -0.14673f,  -0.003303f, 0.02432f,   -0.99248f,  -0.122411f,
    -0.002705f, 0.024394f,  -0.995185f, -0.098017f, -0.002106f, 0.024453f,
    -0.99729f,  -0.073565f, -0.001505f, 0.024497f,  -0.998795f, -0.049068f,
    -0.000903f, 0.024526f,  -0.999699f, -0.024541f, -0.000301f, 0.024541f,
    -1.f,       -0.f,       0.000301f,  0.024541f,  -0.999699f, 0.024541f,
    0.000903f,  0.024526f,  -0.998795f, 0.049068f,  0.001505f,  0.024497f,
    -0.99729f,  0.073565f,  0.002106f,  0.024453f,  -0.995185f, 0.098017f,
    0.002705f,  0.024394f,  -0.99248f,  0.122411f,  0.003303f,  0.02432f,
    -0.989177f, 0.14673f,   0.003899f,  0.024231f,  -0.985278f, 0.170962f,
    0.004492f,  0.024128f,  -0.980785f, 0.19509f,   0.005083f,  0.024011f,
    -0.975702f, 0.219101f,  0.005671f,  0.023879f,  -0.970031f, 0.24298f,
    0.006255f,  0.023733f,  -0.963776f, 0.266713f,  0.006836f,  0.023572f,
    -0.95694f,  0.290285f,  0.007412f,  0.023397f,  -0.949528f, 0.313682f,
    0.007984f,  0.023208f,  -0.941544f, 0.33689f,   0.008551f,  0.023005f,
    -0.932993f, 0.359895f,  0.009113f,  0.022788f,  -0.92388f,  0.382683f,
    0.00967f,   0.022558f,  -0.91421f,  0.405241f,  0.01022f,   0.022314f,
    -0.903989f, 0.427555f,  0.010765f,  0.022056f,  -0.893224f, 0.449611f,
    0.011303f,  0.021785f,  -0.881921f, 0.471397f,  0.011834f,  0.021501f,
    -0.870087f, 0.492898f,  0.012358f,  0.021205f,  -0.857729f, 0.514103f,
    0.012875f,  0.020895f,  -0.844854f, 0.534998f,  0.013384f,  0.020573f,
    -0.83147f,  0.55557f,   0.013885f,  0.020238f,  -0.817585f, 0.575808f,
    0.014377f,  0.019891f,  -0.803208f, 0.595699f,  0.014861f,  0.019532f,
    -0.788346f, 0.615232f,  0.015336f,  0.019162f,  -0.77301f,  0.634393f,
    0.015802f,  0.01878f,   -0.757209f, 0.653173f,  0.016258f,  0.018386f,
    -0.740951f, 0.671559f,  0.016704f,  0.017982f,  -0.724247f, 0.689541f,
    0.01714f,   0.017566f,  -0.707107f, 0.707107f,  0.017566f,  0.01714f,
    -0.689541f, 0.724247f,  0.017982f,  0.016704f,  -0.671559f, 0.740951f,
    0.018386f,  0.016258f,  -0.653173f, 0.757209f,  0.01878f,   0.015802f,
    -0.634393f, 0.77301f,   0.019162f,  0.015336f,  -0.615232f, 0.788346f,
    0.019532f,  0.014861f,  -0.595699f, 0.803208f,  0.019891f,  0.014377f,
    -0.575808f, 0.817585f,  0.020238f,  0.013885f,  -0.55557f,  0.83147f,
    0.020573f,  0.013384f,  -0.534998f, 0.844854f,  0.020895f,  0.012875f,
    -0.514103f, 0.857729f,  0.021205f,  0.012358f,  -0.492898f, 0.870087f,
    0.021501f,  0.011834f,  -0.471397f, 0.881921f,  0.021785f,  0.011303f,
    -0.449611f, 0.893224f,  0.022056f,  0.010765f,  -0.427555f, 0.903989f,
    0.022314f,  0.01022f,   -0.405241f, 0.91421f,   0.022558f,  0.00967f,
    -0.382683f, 0.92388f,   0.022788f,  0.009113f,  -0.359895f, 0.932993f,
    0.023005f,  0.008551f,  -0.33689f,  0.941544f,  0.023208f,  0.007984f,
    -0.313682f, 0.949528f,  0.023397f,  0.007412f,  -0.290285f, 0.95694f,
    0.023572f,  0.006836f,  -0.266713f, 0.963776f,  0.023733f,  0.006255f,
    -0.24298f,  0.970031f,  0.023879f,  0.005671f,  -0.219101f, 0.975702f,
    0.024011f,  0.005083f,  -0.19509f,  0.980785f,  0.024128f,  0.004492f,
    -0.170962f, 0.985278f,  0.024231f,  0.003899f,  -0.14673f,  0.989177f,
    0.02432f,   0.003303f,  -0.122411f, 0.99248f,   0.024394f,  0.002705f,
    -0.098017f, 0.995185f,  0.024453f,  0.002106f,  -0.073565f, 0.99729f,
    0.024497f,  0.001505f,  -0.049068f, 0.998795f,  0.024526f,  0.000903f,
    -0.024541f, 0.999699f,  0.024541f,  0.000301f,  -0.f,       1.f,
    0.024541f,  -0.000301f,
};
#pragma clang diagnostic pop

using WiiFloat = f64;

f32 WiiSin(f32 x) {
  // [0, 2pi * 256]
  WiiFloat x_mod = static_cast<WiiFloat>(std::fmod(std::abs(x), 65536.0f));
  // Integer component
  u16 k = static_cast<u16>(x_mod);
  // Fractional component. Angles inside 2pi/256th (~1.4 degree) intervals.
  WiiFloat frac = x_mod -
                  /* This *must* be single precision */
                  static_cast<f32>(k);
  // Take advantage of the symmetry of sin
  u8 circle_index = static_cast<u8>(k % 0xFF);
  WiiFloat sin_of_abs_x =
      static_cast<WiiFloat>(SinCosLUT[circle_index].sin) +
      frac * static_cast<WiiFloat>(SinCosLUT[circle_index].sin_prime);
  // sin(x) is an odd function
  return static_cast<f32>(x < 0.0f ? -sin_of_abs_x : sin_of_abs_x);
}

f32 WiiCos(f32 x) {
  // [0, 2pi * 256]
  WiiFloat x_mod = static_cast<WiiFloat>(std::fmod(std::abs(x), 65536.0f));
  // Integer component
  u16 k = static_cast<u16>(x_mod);
  // Fractional component. Angles inside 2pi/256th (~1.4 degree) intervals.
  WiiFloat frac = x_mod -
                  /* This *must* be single precision */
                  static_cast<f32>(k);
  // Take advantage of the symmetry of cos
  u8 circle_index = static_cast<u8>(k % 0xFF);
  WiiFloat cos_of_abs_x =
      static_cast<WiiFloat>(SinCosLUT[circle_index].cos) +
      frac * static_cast<WiiFloat>(SinCosLUT[circle_index].cos_prime);
  // cos(x) is an even function: no correction needed
  return static_cast<f32>(cos_of_abs_x);
}

#if 0
// COLUMN-MAJOR IMPLEMENTATION
glm::mat4x3 MTXConcat(const glm::mat4x3& a, const glm::mat4x3& b) {
  glm::mat4x3 v3;
  // clang-format off
  v3[0][0] = ((WiiFloat)a[0][0] * (WiiFloat)b[0][0] + (WiiFloat)a[1][0] * (WiiFloat)b[0][1] + (WiiFloat)a[2][0] * (WiiFloat)b[0][2]);
  v3[1][0] = ((WiiFloat)a[0][0] * (WiiFloat)b[1][0] + (WiiFloat)a[1][0] * (WiiFloat)b[1][1] + (WiiFloat)a[2][0] * (WiiFloat)b[1][2]);
  v3[2][0] = ((WiiFloat)a[0][0] * (WiiFloat)b[2][0] + (WiiFloat)a[1][0] * (WiiFloat)b[2][1] + (WiiFloat)a[2][0] * (WiiFloat)b[2][2]);
  v3[3][0] = ((WiiFloat)a[0][0] * (WiiFloat)b[3][0] + (WiiFloat)a[1][0] * (WiiFloat)b[3][1] + (WiiFloat)a[2][0] * (WiiFloat)b[3][2] + (WiiFloat)a[3][0]);
  v3[0][1] = ((WiiFloat)a[0][1] * (WiiFloat)b[0][0] + (WiiFloat)a[1][1] * (WiiFloat)b[0][1] + (WiiFloat)a[2][1] * (WiiFloat)b[0][2]);
  v3[1][1] = ((WiiFloat)a[0][1] * (WiiFloat)b[1][0] + (WiiFloat)a[1][1] * (WiiFloat)b[1][1] + (WiiFloat)a[2][1] * (WiiFloat)b[1][2]);
  v3[2][1] = ((WiiFloat)a[0][1] * (WiiFloat)b[2][0] + (WiiFloat)a[1][1] * (WiiFloat)b[2][1] + (WiiFloat)a[2][1] * (WiiFloat)b[2][2]);
  v3[3][1] = ((WiiFloat)a[0][1] * (WiiFloat)b[3][0] + (WiiFloat)a[1][1] * (WiiFloat)b[3][1] + (WiiFloat)a[2][1] * (WiiFloat)b[3][2] + (WiiFloat)a[3][1]);
  v3[0][2] = ((WiiFloat)a[0][2] * (WiiFloat)b[0][0] + (WiiFloat)a[1][2] * (WiiFloat)b[0][1] + (WiiFloat)a[2][2] * (WiiFloat)b[0][2]);
  v3[1][2] = ((WiiFloat)a[0][2] * (WiiFloat)b[1][0] + (WiiFloat)a[1][2] * (WiiFloat)b[1][1] + (WiiFloat)a[2][2] * (WiiFloat)b[1][2]);
  v3[2][2] = ((WiiFloat)a[0][2] * (WiiFloat)b[2][0] + (WiiFloat)a[1][2] * (WiiFloat)b[2][1] + (WiiFloat)a[2][2] * (WiiFloat)b[2][2]);
  v3[3][2] = ((WiiFloat)a[0][2] * (WiiFloat)b[3][0] + (WiiFloat)a[1][2] * (WiiFloat)b[3][1] + (WiiFloat)a[2][2] * (WiiFloat)b[3][2] + (WiiFloat)a[3][2]);
  // clang-format on
  return v3;
}

// COLUMN-MAJOR IMPLEMENTATION
void Mtx_makeRotateFIdx(glm::mat4x3& mtx, f64 rx, f64 ry, f64 rz) {
  f32 sinxf32 = WiiSin(rx);
  f32 cosxf32 = WiiCos(rx);
  f32 sinyf32 = WiiSin(ry);
  f32 cosyf32 = WiiCos(ry);
  f32 sinzf32 = WiiSin(rz);
  f32 coszf32 = WiiCos(rz);
  WiiFloat sinx = static_cast<WiiFloat>(sinxf32);
  WiiFloat cosx = static_cast<WiiFloat>(cosxf32);
  WiiFloat siny = static_cast<WiiFloat>(sinyf32);
  WiiFloat cosy = static_cast<WiiFloat>(cosyf32);
  WiiFloat sinz = static_cast<WiiFloat>(sinzf32);
  WiiFloat cosz = static_cast<WiiFloat>(coszf32);
  mtx[0][0] = cosz * cosy;
  mtx[0][1] = sinz * cosy;
  mtx[0][2] = -siny;
  mtx[1][0] = (sinx * cosz) * siny - (cosx * sinz);
  mtx[1][1] = (sinx * sinz) * siny + (cosx * cosz);
  mtx[1][2] = cosy * sinx;
  mtx[2][0] = (cosx * cosz) * siny + (sinx * sinz);
  mtx[2][1] = (cosx * sinz) * siny - (sinx * cosz);
  mtx[2][2] = cosy * cosx;
  mtx[3][0] = 0.0f;
  mtx[3][1] = 0.0f;
  mtx[3][2] = 0.0f;
}

void Mtx_makeRotateDegrees(glm::mat4x3& out, f64 rx, f64 ry, f64 rz) {
  Mtx_makeRotateFIdx(out, DegreesToFIDX(rx), DegreesToFIDX(ry),
                     DegreesToFIDX(rz));
}

void Mtx_scale(glm::mat4x3& out, const glm::mat4x3& mtx, const glm::vec3& scl) {
  out[0][0] = (WiiFloat)mtx[0][0] * (WiiFloat)scl.x;
  out[0][1] = (WiiFloat)mtx[0][1] * (WiiFloat)scl.x;
  out[0][2] = (WiiFloat)mtx[0][2] * (WiiFloat)scl.x;
  out[1][0] = (WiiFloat)mtx[1][0] * (WiiFloat)scl.y;
  out[1][1] = (WiiFloat)mtx[1][1] * (WiiFloat)scl.y;
  out[1][2] = (WiiFloat)mtx[1][2] * (WiiFloat)scl.y;
  out[2][0] = (WiiFloat)mtx[2][0] * (WiiFloat)scl.z;
  out[2][1] = (WiiFloat)mtx[2][1] * (WiiFloat)scl.z;
  out[2][2] = (WiiFloat)mtx[2][2] * (WiiFloat)scl.z;
  if (&out != &mtx) {
    out[3][0] = mtx[3][0];
    out[3][1] = mtx[3][1];
    out[3][2] = mtx[3][2];
  }
}

// COLUMN-MAJOR IMPLEMENTATION
std::optional<glm::mat4x3> MTXInverse(const glm::mat4x3& mtx) {
  glm::mat4x3 out;
  f32 determinant =
      (WiiFloat)mtx[0][0] * (WiiFloat)mtx[1][1] * (WiiFloat)mtx[2][2] +
      (WiiFloat)mtx[1][0] * (WiiFloat)mtx[2][1] * (WiiFloat)mtx[0][2] +
      (WiiFloat)mtx[2][0] * (WiiFloat)mtx[0][1] * (WiiFloat)mtx[1][2] -
      (WiiFloat)mtx[0][2] * (WiiFloat)mtx[1][1] * (WiiFloat)mtx[2][0] -
      (WiiFloat)mtx[0][1] * (WiiFloat)mtx[1][0] * (WiiFloat)mtx[2][2] -
      (WiiFloat)mtx[0][0] * (WiiFloat)mtx[1][2] * (WiiFloat)mtx[2][1];
  if (determinant == 0.0f) {
    return std::nullopt;
  }
  f32 coeff = 1.0f / determinant;
  // clang-format off

  out[0][0] =  ((WiiFloat)mtx[1][1] * (WiiFloat)mtx[2][2] - (WiiFloat)mtx[1][2] * (WiiFloat)mtx[2][1]) * coeff;
#ifdef __APPLE__
  {
    WiiFloat a = (WiiFloat)mtx[1][0] * (WiiFloat)mtx[2][2];
    WiiFloat b = (WiiFloat)mtx[1][2] * (WiiFloat)mtx[2][0];
    WiiFloat c = a-b;
    WiiFloat d = -c;
    WiiFloat e = d * coeff;
    out[1][0] = e;
  }
#else
  out[1][0] = -((WiiFloat)mtx[1][0] * (WiiFloat)mtx[2][2] - (WiiFloat)mtx[1][2] * (WiiFloat)mtx[2][0]) * coeff;
#endif
  out[2][0] =  ((WiiFloat)mtx[1][0] * (WiiFloat)mtx[2][1] - (WiiFloat)mtx[1][1] * (WiiFloat)mtx[2][0]) * coeff;
#ifdef __APPLE__
  {
    WiiFloat a = (WiiFloat)mtx[0][1] * (WiiFloat)mtx[2][2];
    WiiFloat b = (WiiFloat)mtx[0][2] * (WiiFloat)mtx[2][1];
    WiiFloat c = a-b;
    WiiFloat d = -c;
    WiiFloat e = d * coeff;
    out[0][1] = e;
  }
#else
  out[0][1] = -((WiiFloat)mtx[0][1] * (WiiFloat)mtx[2][2] - (WiiFloat)mtx[0][2] * (WiiFloat)mtx[2][1]) * coeff;
#endif
  out[1][1] =  ((WiiFloat)mtx[0][0] * (WiiFloat)mtx[2][2] - (WiiFloat)mtx[0][2] * (WiiFloat)mtx[2][0]) * coeff;
#ifdef __APPLE__
  {
    WiiFloat a = (WiiFloat)mtx[0][0] * (WiiFloat)mtx[2][1];
    WiiFloat b = (WiiFloat)mtx[0][1] * (WiiFloat)mtx[2][0];
    WiiFloat c = a-b;
    WiiFloat d = -c;
    WiiFloat e = d * coeff;
    out[2][1] = e;
  }
#else
  out[2][1] = -((WiiFloat)mtx[0][0] * (WiiFloat)mtx[2][1] - (WiiFloat)mtx[0][1] * (WiiFloat)mtx[2][0]) * coeff;
#endif
  out[0][2] =  ((WiiFloat)mtx[0][1] * (WiiFloat)mtx[1][2] - (WiiFloat)mtx[0][2] * (WiiFloat)mtx[1][1]) * coeff;
#ifdef __APPLE__
  {
    WiiFloat a = (WiiFloat)mtx[0][0] * (WiiFloat)mtx[1][2];
    WiiFloat b = (WiiFloat)mtx[0][2] * (WiiFloat)mtx[1][0];
    WiiFloat c = a-b;
    WiiFloat d = -c;
    WiiFloat e = d * coeff;
    out[1][2] = e;
  }
#else
  out[1][2] = -((WiiFloat)mtx[0][0] * (WiiFloat)mtx[1][2] - (WiiFloat)mtx[0][2] * (WiiFloat)mtx[1][0]) * coeff;
#endif
  out[2][2] =  ((WiiFloat)mtx[0][0] * (WiiFloat)mtx[1][1] - (WiiFloat)mtx[0][1] * (WiiFloat)mtx[1][0]) * coeff;
  out[3][0] = -out[0][0] * (WiiFloat)mtx[3][0] - (WiiFloat)out[1][0] * (WiiFloat)mtx[3][1] - (WiiFloat)out[2][0] * (WiiFloat)mtx[3][2];
  out[3][1] = -out[0][1] * (WiiFloat)mtx[3][0] - (WiiFloat)out[1][1] * (WiiFloat)mtx[3][1] - (WiiFloat)out[2][1] * (WiiFloat)mtx[3][2];
  out[3][2] = -out[0][2] * (WiiFloat)mtx[3][0] - (WiiFloat)out[1][2] * (WiiFloat)mtx[3][1] - (WiiFloat)out[2][2] * (WiiFloat)mtx[3][2];
  // clang-format on
  return out;
}

// COLUMN-MAJOR IMPLEMENTATION
void CalcEnvelopeContribution(glm::mat4& thisMatrix, glm::vec3& thisScale,
                              const librii::math::SRT3& srt, bool ssc,
                              const glm::mat4& parentMtx,
                              const glm::vec3& parentScale,
                              librii::g3d::ScalingRule scalingRule) {
  if (ssc) {
    librii::g3d::Mtx_makeRotateDegrees(/*out*/ thisMatrix, srt.rotation.x,
                                       srt.rotation.y, srt.rotation.z);
    thisMatrix[3][0] = (WiiFloat)srt.translation.x * (WiiFloat)parentScale.x;
    thisMatrix[3][1] = (WiiFloat)srt.translation.y * (WiiFloat)parentScale.y;
    thisMatrix[3][2] = (WiiFloat)srt.translation.z * (WiiFloat)parentScale.z;
    // PSMTXConcat(parentMtx, &thisMatrix, /*out*/ &thisMatrix);
    // thisMatrix = parentMtx * thisMatrix;
    thisMatrix = librii::g3d::MTXConcat(parentMtx, thisMatrix);
    thisScale.x = srt.scale.x;
    thisScale.y = srt.scale.y;
    thisScale.z = srt.scale.z;
  } else if (scalingRule ==
             librii::g3d::ScalingRule::XSI) { // CLASSIC_SCALE_OFF
    librii::g3d::Mtx_makeRotateDegrees(/*out*/ thisMatrix, srt.rotation.x,
                                       srt.rotation.y, srt.rotation.z);
    thisMatrix[3][0] = (WiiFloat)srt.translation.x * (WiiFloat)parentScale.x;
    thisMatrix[3][1] = (WiiFloat)srt.translation.y * (WiiFloat)parentScale.y;
    thisMatrix[3][2] = (WiiFloat)srt.translation.z * (WiiFloat)parentScale.z;
    // PSMTXConcat(parentMtx, &thisMatrix, /*out*/ &thisMatrix);
    // thisMatrix = parentMtx * thisMatrix;
    thisMatrix = librii::g3d::MTXConcat(parentMtx, thisMatrix);
    thisScale.x = (WiiFloat)srt.scale.x * (WiiFloat)parentScale.x;
    thisScale.y = (WiiFloat)srt.scale.y * (WiiFloat)parentScale.y;
    thisScale.z = (WiiFloat)srt.scale.z * (WiiFloat)parentScale.z;
  } else {
    glm::mat4x3 scratch;
    // Mtx_scale(/*out*/ &scratch, parentMtx, parentScale);
    librii::g3d::Mtx_scale(/*out*/ scratch, parentMtx, parentScale);
    // scratch = glm::scale(parentMtx, parentScale);
    librii::g3d::Mtx_makeRotateDegrees(/*out*/ thisMatrix, srt.rotation.x,
                                       srt.rotation.y, srt.rotation.z);
    thisMatrix[3][0] = srt.translation.x;
    thisMatrix[3][1] = srt.translation.y;
    thisMatrix[3][2] = srt.translation.z;
    // PSMTXConcat(scratch, &thisMatrix, /*out*/ &thisMatrix);
    // thisMatrix = scratch * thisMatrix;
    thisMatrix = librii::g3d::MTXConcat(scratch, thisMatrix);
    thisScale.x = srt.scale.x;
    thisScale.y = srt.scale.y;
    thisScale.z = srt.scale.z;
  }
}
#endif

} // namespace rlibrii::g3d
