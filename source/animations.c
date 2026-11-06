#include "animations.h"

static const AnimSprite BlackSludge_loop_frame000[] = {
    { 0, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 1, 7.250000f, 6.875000f, 1.016660f, 1.325000f, 0, 0, 0.000000f, 1 },
    { 2, -7.250000f, 2.425000f, 1.000000f, 0.296880f, 0, 0, 0.000000f, 2 },
};

static const AnimSprite BlackSludge_loop_frame001[] = {
    { 0, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 1, 7.275000f, 6.575000f, 1.015660f, 1.263580f, 0, 0, 0.000000f, 1 },
    { 2, -7.250000f, 2.725000f, 1.000000f, 0.365830f, 0, 0, 0.000000f, 2 },
};

static const AnimSprite BlackSludge_loop_frame002[] = {
    { 0, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 1, 7.250000f, 5.850000f, 1.012790f, 1.089220f, 0, 0, 0.000000f, 1 },
    { 2, -7.250000f, 3.525000f, 1.000000f, 0.553470f, 0, 0, 0.000000f, 2 },
};

static const AnimSprite BlackSludge_loop_frame003[] = {
    { 0, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 1, 7.275000f, 4.750000f, 1.008670f, 0.838820f, 0, 0, 0.000000f, 1 },
    { 2, -7.250000f, 4.550000f, 1.000000f, 0.794300f, 0, 0, 0.000000f, 2 },
};

static const AnimSprite BlackSludge_loop_frame004[] = {
    { 0, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 1, 7.250000f, 3.650000f, 1.004380f, 0.578450f, 0, 0, 0.000000f, 1 },
    { 2, -7.250000f, 5.400000f, 1.000000f, 0.991270f, 0, 0, 0.000000f, 2 },
};

static const AnimSprite BlackSludge_loop_frame005[] = {
    { 0, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 1, 7.275000f, 2.800000f, 1.001170f, 0.384200f, 0, 0, 0.000000f, 1 },
    { 2, -7.250000f, 5.725000f, 1.000000f, 1.066120f, 0, 0, 0.000000f, 2 },
};

static const AnimSprite BlackSludge_loop_frame006[] = {
    { 0, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 1, 7.250000f, 2.500000f, 1.000000f, 0.312500f, 0, 0, 0.000000f, 1 },
    { 2, -7.250000f, 5.525000f, 1.000000f, 1.017640f, 0, 0, 0.000000f, 2 },
};

static const AnimSprite BlackSludge_loop_frame007[] = {
    { 0, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 1, 7.250000f, 2.875000f, 1.001450f, 0.400480f, 0, 0, 0.000000f, 1 },
    { 2, -7.250000f, 4.925000f, 1.000000f, 0.881070f, 0, 0, 0.000000f, 2 },
};

static const AnimSprite BlackSludge_loop_frame008[] = {
    { 0, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 1, 7.250000f, 3.925000f, 1.005430f, 0.642460f, 0, 0, 0.000000f, 1 },
    { 2, -7.250000f, 4.100000f, 1.000000f, 0.687560f, 0, 0, 0.000000f, 2 },
};

static const AnimSprite BlackSludge_loop_frame009[] = {
    { 0, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 1, 7.250000f, 5.275000f, 1.010640f, 0.958690f, 0, 0, 0.000000f, 1 },
    { 2, -7.250000f, 3.250000f, 1.000000f, 0.489810f, 0, 0, 0.000000f, 2 },
};

static const AnimSprite BlackSludge_loop_frame010[] = {
    { 0, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 1, 7.250000f, 6.400000f, 1.014980f, 1.222900f, 0, 0, 0.000000f, 1 },
    { 2, -7.250000f, 2.625000f, 1.000000f, 0.344830f, 0, 0, 0.000000f, 2 },
};

static const AnimSprite GJBeast01_attack01_frame000[] = {
    { 0, 4.725000f, -17.625000f, 1.000030f, 1.000030f, 0, 0, -17.649841f, 0 },
    { 1, 2.950000f, 17.875000f, 1.000000f, 1.000000f, 0, 0, 16.926117f, 1 },
};

static const AnimSprite GJBeast01_attack01_frame001[] = {
    { 0, 6.775000f, -19.800000f, 0.998790f, 0.998790f, 0, 0, -23.781830f, 0 },
    { 1, 4.125000f, 19.750000f, 0.998870f, 0.998870f, 0, 0, 22.066650f, 1 },
};

static const AnimSprite GJBeast01_attack01_frame002[] = {
    { 0, 8.875000f, -21.925000f, 1.000010f, 1.000010f, 0, 0, -29.851456f, 0 },
    { 1, 5.450000f, 21.725000f, 1.000010f, 1.000010f, 0, 0, 27.391296f, 1 },
};

static const AnimSprite GJBeast01_attack01_end_frame000[] = {
    { 0, 11.900000f, -23.625000f, 1.000000f, 1.000000f, 0, 0, -37.289948f, 0 },
    { 1, 9.625000f, 24.175000f, 0.999980f, 0.999980f, 0, 0, 40.147202f, 1 },
};

static const AnimSprite GJBeast01_attack01_end_frame001[] = {
    { 0, 8.925000f, -21.550000f, 0.997960f, 0.997960f, 0, 0, -30.344223f, 0 },
    { 1, 6.500000f, 21.925000f, 0.997950f, 0.997950f, 0, 0, 31.553055f, 1 },
};

static const AnimSprite GJBeast01_attack01_end_frame002[] = {
    { 0, 4.750000f, -18.350000f, 0.998530f, 0.998530f, 0, 0, -20.575195f, 0 },
    { 1, 2.275000f, 18.325000f, 0.998620f, 0.998620f, 0, 0, 19.077454f, 1 },
};

static const AnimSprite GJBeast01_attack01_end_frame003[] = {
    { 0, 2.650000f, -15.525000f, 0.998230f, 0.998230f, 0, 0, -11.560059f, 0 },
    { 1, 1.400000f, 15.550000f, 0.998330f, 0.998330f, 0, 0, 10.812943f, 1 },
};

static const AnimSprite GJBeast01_attack01_end_frame004[] = {
    { 0, 0.900000f, -12.450000f, 0.999210f, 0.999210f, 0, 0, -2.842514f, 0 },
    { 1, 0.725000f, 12.750000f, 0.999240f, 0.999240f, 0, 0, 2.866943f, 1 },
};

static const AnimSprite GJBeast01_attack01_end_frame005[] = {
    { 0, -0.025000f, -10.725000f, 0.999590f, 0.999590f, 0, 0, -0.298996f, 0 },
    { 1, 0.125000f, 11.100000f, 0.999560f, 0.999560f, 0, 0, -1.009674f, 1 },
};

static const AnimSprite GJBeast01_attack01_loop_frame000[] = {
    { 0, 9.625000f, -22.375000f, 0.998540f, 0.998540f, 0, 0, -31.605087f, 0 },
    { 1, 6.475000f, 22.350000f, 0.998530f, 0.998530f, 0, 0, 30.568344f, 1 },
};

static const AnimSprite GJBeast01_attack01_loop_frame001[] = {
    { 0, 11.000000f, -23.550000f, 0.997470f, 0.997470f, 0, 0, -35.331375f, 0 },
    { 1, 8.350000f, 23.625000f, 0.997440f, 0.997440f, 0, 0, 36.088165f, 1 },
};

static const AnimSprite GJBeast01_attack01_loop_frame002[] = {
    { 0, 12.350000f, -24.675000f, 0.997900f, 0.997900f, 0, 0, -39.086899f, 0 },
    { 1, 10.300000f, 24.825000f, 0.997860f, 0.997860f, 0, 0, 41.610428f, 1 },
};

static const AnimSprite GJBeast01_attack01_loop_frame003[] = {
    { 0, 13.825000f, -25.750000f, 1.000010f, 1.000010f, 0, 0, -42.765244f, 0 },
    { 1, 12.350000f, 26.000000f, 0.999990f, 0.999990f, 0, 0, 47.128906f, 1 },
};

static const AnimSprite GJBeast01_attack01_loop_frame004[] = {
    { 0, 12.500000f, -24.725000f, 0.997480f, 0.997480f, 0, 0, -39.332016f, 0 },
    { 1, 10.425000f, 24.925000f, 0.997410f, 0.997410f, 0, 0, 41.882507f, 1 },
};

static const AnimSprite GJBeast01_attack01_loop_frame005[] = {
    { 0, 11.175000f, -23.700000f, 0.996560f, 0.996560f, 0, 0, -35.826965f, 0 },
    { 1, 8.600000f, 23.825000f, 0.996510f, 0.996510f, 0, 0, 36.820007f, 1 },
};

static const AnimSprite GJBeast01_attack01_loop_frame006[] = {
    { 0, 9.900000f, -22.575000f, 0.997170f, 0.997170f, 0, 0, -32.318054f, 0 },
    { 1, 6.825000f, 22.600000f, 0.997170f, 0.997170f, 0, 0, 31.577820f, 1 },
};

static const AnimSprite GJBeast01_bite_frame000[] = {
    { 0, 0.225000f, -10.450000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 1, 0.775000f, 11.500000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast01_bite_frame001[] = {
    { 0, 0.700000f, -11.600000f, 0.999870f, 0.999870f, 0, 0, -2.253571f, 0 },
    { 1, 1.025000f, 12.500000f, 0.999860f, 0.999860f, 0, 0, 2.529358f, 1 },
};

static const AnimSprite GJBeast01_bite_frame002[] = {
    { 0, 1.200000f, -12.750000f, 1.000010f, 1.000010f, 0, 0, -4.531464f, 0 },
    { 1, 1.275000f, 13.600000f, 1.000000f, 1.000000f, 0, 0, 5.231003f, 1 },
};

static const AnimSprite GJBeast01_bite_frame003[] = {
    { 0, 2.900000f, -15.225000f, 0.999410f, 0.999410f, 0, 0, -11.030396f, 0 },
    { 1, 2.075000f, 15.775000f, 0.999390f, 0.999390f, 0, 0, 11.026184f, 1 },
};

static const AnimSprite GJBeast01_bite_frame004[] = {
    { 0, 4.725000f, -17.625000f, 1.000030f, 1.000030f, 0, 0, -17.649841f, 0 },
    { 1, 2.950000f, 17.875000f, 1.000000f, 1.000000f, 0, 0, 16.926117f, 1 },
};

static const AnimSprite GJBeast01_bite_frame005[] = {
    { 0, 6.775000f, -19.800000f, 0.998790f, 0.998790f, 0, 0, -23.781830f, 0 },
    { 1, 4.125000f, 19.750000f, 0.998870f, 0.998870f, 0, 0, 22.066650f, 1 },
};

static const AnimSprite GJBeast01_bite_frame006[] = {
    { 0, 8.875000f, -21.925000f, 1.000010f, 1.000010f, 0, 0, -29.851456f, 0 },
    { 1, 5.450000f, 21.725000f, 1.000010f, 1.000010f, 0, 0, 27.391296f, 1 },
};

static const AnimSprite GJBeast01_bite_frame007[] = {
    { 0, 9.625000f, -22.375000f, 0.998540f, 0.998540f, 0, 0, -31.605087f, 0 },
    { 1, 6.475000f, 22.350000f, 0.998530f, 0.998530f, 0, 0, 30.568344f, 1 },
};

static const AnimSprite GJBeast01_bite_frame008[] = {
    { 0, 10.400000f, -22.850000f, 0.998510f, 0.998510f, 0, 0, -33.577560f, 0 },
    { 1, 7.500000f, 22.950000f, 0.998450f, 0.998450f, 0, 0, 33.812973f, 1 },
};

static const AnimSprite GJBeast01_bite_frame009[] = {
    { 0, 11.150000f, -23.275000f, 0.998440f, 0.998440f, 0, 0, -35.358719f, 0 },
    { 1, 8.550000f, 23.525000f, 0.998390f, 0.998390f, 0, 0, 36.868729f, 1 },
};

static const AnimSprite GJBeast01_bite_frame010[] = {
    { 0, 11.900000f, -23.625000f, 1.000000f, 1.000000f, 0, 0, -37.289948f, 0 },
    { 1, 9.625000f, 24.175000f, 0.999980f, 0.999980f, 0, 0, 40.147202f, 1 },
};

static const AnimSprite GJBeast01_bite_frame011[] = {
    { 0, 8.925000f, -21.550000f, 0.997960f, 0.997960f, 0, 0, -30.344223f, 0 },
    { 1, 6.500000f, 21.925000f, 0.997950f, 0.997950f, 0, 0, 31.553055f, 1 },
};

static const AnimSprite GJBeast01_bite_frame012[] = {
    { 0, 4.750000f, -18.350000f, 0.998530f, 0.998530f, 0, 0, -20.575195f, 0 },
    { 1, 2.275000f, 18.325000f, 0.998620f, 0.998620f, 0, 0, 19.077454f, 1 },
};

static const AnimSprite GJBeast01_bite_frame013[] = {
    { 0, 2.650000f, -15.525000f, 0.998230f, 0.998230f, 0, 0, -11.560059f, 0 },
    { 1, 1.400000f, 15.550000f, 0.998330f, 0.998330f, 0, 0, 10.812943f, 1 },
};

static const AnimSprite GJBeast01_bite_frame014[] = {
    { 0, 0.900000f, -12.450000f, 0.999210f, 0.999210f, 0, 0, -2.842514f, 0 },
    { 1, 0.725000f, 12.750000f, 0.999240f, 0.999240f, 0, 0, 2.866943f, 1 },
};

static const AnimSprite GJBeast01_bite_frame015[] = {
    { 0, -0.025000f, -10.725000f, 0.999590f, 0.999590f, 0, 0, -0.298996f, 0 },
    { 1, 0.125000f, 11.100000f, 0.999560f, 0.999560f, 0, 0, -1.009674f, 1 },
};

static const AnimSprite GJBeast01_idle01_frame000[] = {
    { 0, 0.225000f, -10.450000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 1, 0.775000f, 11.500000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast01_idle01_frame001[] = {
    { 0, 0.150000f, -10.250000f, 0.999980f, 0.999980f, 0, 0, 0.302490f, 0 },
    { 1, 0.700000f, 11.275000f, 0.999980f, 0.999980f, 0, 0, -0.300751f, 1 },
};

static const AnimSprite GJBeast01_idle01_frame002[] = {
    { 0, 0.000000f, -9.800000f, 0.999920f, 0.999920f, 0, 0, 1.304184f, 0 },
    { 1, 0.575000f, 10.750000f, 0.999920f, 0.999920f, 0, 0, -1.298935f, 1 },
};

static const AnimSprite GJBeast01_idle01_frame003[] = {
    { 0, -0.150000f, -9.325000f, 0.999870f, 0.999870f, 0, 0, 2.501434f, 0 },
    { 1, 0.425000f, 10.225000f, 0.999880f, 0.999880f, 0, 0, -2.305939f, 1 },
};

static const AnimSprite GJBeast01_idle01_frame004[] = {
    { 0, -0.225000f, -9.100000f, 1.000000f, 1.000000f, 0, 0, 2.986404f, 0 },
    { 1, 0.350000f, 9.975000f, 1.000000f, 1.000000f, 0, 0, -2.951523f, 1 },
};

static const AnimSprite GJBeast01_idle01_frame005[] = {
    { 0, -0.175000f, -9.250000f, 0.999840f, 0.999840f, 0, 0, 2.545059f, 0 },
    { 1, 0.400000f, 10.125000f, 0.999840f, 0.999840f, 0, 0, -2.537201f, 1 },
};

static const AnimSprite GJBeast01_idle01_frame006[] = {
    { 0, -0.050000f, -9.600000f, 0.999870f, 0.999870f, 0, 0, 1.780304f, 0 },
    { 1, 0.475000f, 10.575000f, 0.999870f, 0.999870f, 0, 0, -1.775055f, 1 },
};

static const AnimSprite GJBeast01_idle01_frame007[] = {
    { 0, 0.100000f, -10.100000f, 0.999950f, 0.999950f, 0, 0, 0.753571f, 0 },
    { 1, 0.650000f, 11.125000f, 0.999950f, 0.999950f, 0, 0, -0.750946f, 1 },
};

static const AnimSprite GJBeast02_idle01_frame000[] = {
    { 1, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -2.500000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle01_frame001[] = {
    { 1, 0.000000f, -0.200000f, 1.005040f, 0.984940f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -2.700000f, 1.000000f, 0.973950f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle01_frame002[] = {
    { 1, 0.000000f, -0.650000f, 1.017110f, 0.948880f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -3.200000f, 1.000000f, 0.911610f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle01_frame003[] = {
    { 1, 0.000000f, -1.075000f, 1.028730f, 0.914140f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -3.675000f, 1.000000f, 0.851530f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle01_frame004[] = {
    { 1, 0.000000f, -1.250000f, 1.033330f, 0.900410f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -3.875000f, 1.000000f, 0.827790f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle01_frame005[] = {
    { 1, 0.000000f, -0.875000f, 1.024780f, 0.929150f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -3.625000f, 1.000000f, 0.915250f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle01_frame006[] = {
    { 1, 0.000000f, -0.525000f, 1.016660f, 0.956420f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -3.225000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle01_frame007[] = {
    { 1, 0.000000f, -0.400000f, 1.011020f, 0.967650f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -3.025000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle01_frame008[] = {
    { 1, 0.000000f, -0.125000f, 1.000000f, 0.989620f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -2.650000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle02_frame000[] = {
    { 1, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -2.500000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle02_frame001[] = {
    { 1, 0.000000f, -0.200000f, 1.005050f, 0.984940f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -2.700000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle02_frame002[] = {
    { 1, 0.000000f, -0.650000f, 1.017120f, 0.948880f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -3.200000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle02_frame003[] = {
    { 1, 0.000000f, -1.075000f, 1.028750f, 0.914140f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -3.675000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle02_frame004[] = {
    { 1, 0.000000f, -1.250000f, 1.033340f, 0.900410f, 0, 0, 0.000000f, 0 },
};

static const AnimSprite GJBeast02_idle02_frame005[] = {
    { 1, 0.000000f, -0.875000f, 1.024780f, 0.929150f, 0, 0, 0.000000f, 0 },
};

static const AnimSprite GJBeast02_idle02_frame006[] = {
    { 1, 0.000000f, -0.525000f, 1.016660f, 0.956420f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -3.225000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle02_frame007[] = {
    { 1, 0.000000f, -0.400000f, 1.011020f, 0.967650f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -3.025000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast02_idle02_frame008[] = {
    { 1, 0.000000f, -0.125000f, 1.000000f, 0.989620f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -2.650000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle01_frame000[] = {
    { 1, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 0, 0.025000f, 0.500000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle01_frame001[] = {
    { 1, 0.000000f, -0.150000f, 1.006040f, 0.992070f, 0, 0, 0.000000f, 0 },
    { 0, 0.025000f, 0.300000f, 1.001880f, 0.983570f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle01_frame002[] = {
    { 1, 0.000000f, -0.525000f, 1.022490f, 0.970440f, 0, 0, 0.000000f, 0 },
    { 0, 0.025000f, -0.225000f, 1.006990f, 0.938830f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle01_frame003[] = {
    { 1, 0.000000f, -0.950000f, 1.041260f, 0.945770f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -0.850000f, 1.012820f, 0.887740f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle01_frame004[] = {
    { 1, 0.000000f, -1.175000f, 1.049670f, 0.934720f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -1.125000f, 1.015430f, 0.864870f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle01_frame005[] = {
    { 1, 0.000000f, -1.075000f, 1.045790f, 0.939830f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -1.000000f, 1.014070f, 0.876740f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle01_frame006[] = {
    { 1, 0.000000f, -0.825000f, 1.034650f, 0.954440f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, -0.625000f, 1.010190f, 0.910780f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle01_frame007[] = {
    { 1, 0.000000f, -0.475000f, 1.019240f, 0.974700f, 0, 0, 0.000000f, 0 },
    { 0, 0.025000f, -0.125000f, 1.004780f, 0.958010f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle01_frame008[] = {
    { 1, 0.000000f, -0.150000f, 1.005520f, 0.992740f, 0, 0, 0.000000f, 0 },
    { 0, 0.025000f, 0.325000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle02_frame000[] = {
    { 1, 0.000000f, 0.000000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 0 },
    { 0, 0.025000f, 0.500000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle02_frame001[] = {
    { 1, 0.000000f, -0.150000f, 1.006040f, 0.992070f, 0, 0, 0.000000f, 0 },
    { 0, 0.000000f, 0.325000f, 1.002410f, 0.882860f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle02_frame002[] = {
    { 1, 0.000000f, -0.525000f, 1.022490f, 0.970440f, 0, 0, 0.000000f, 0 },
    { 0, -0.025000f, -0.225000f, 1.009320f, 0.547200f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle02_frame003[] = {
    { 1, 0.000000f, -0.950000f, 1.041260f, 0.945770f, 0, 0, 0.000000f, 0 },
    { 0, -0.075000f, -0.950000f, 1.018910f, 0.081330f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle02_frame004[] = {
    { 1, 0.000000f, -1.175000f, 1.049670f, 0.934720f, 0, 0, 0.000000f, 0 },
};

static const AnimSprite GJBeast03_idle02_frame005[] = {
    { 1, 0.000000f, -1.075000f, 1.045790f, 0.939830f, 0, 0, 0.000000f, 0 },
    { 0, -0.075000f, -0.950000f, 1.018910f, 0.218190f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle02_frame006[] = {
    { 1, 0.000000f, -0.825000f, 1.034650f, 0.954440f, 0, 0, 0.000000f, 0 },
    { 0, -0.050000f, -0.750000f, 1.015980f, 0.339650f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle02_frame007[] = {
    { 1, 0.000000f, -0.475000f, 1.019240f, 0.974700f, 0, 0, 0.000000f, 0 },
    { 0, -0.025000f, -0.225000f, 1.008150f, 0.663160f, 0, 0, 0.000000f, 1 },
};

static const AnimSprite GJBeast03_idle02_frame008[] = {
    { 1, 0.000000f, -0.150000f, 1.005520f, 0.992740f, 0, 0, 0.000000f, 0 },
    { 0, 0.025000f, 0.325000f, 1.000000f, 1.000000f, 0, 0, 0.000000f, 1 },
};

static const AnimFrame BlackSludge_loop_frames[] = {
    { BlackSludge_loop_frame000, 3 },
    { BlackSludge_loop_frame001, 3 },
    { BlackSludge_loop_frame002, 3 },
    { BlackSludge_loop_frame003, 3 },
    { BlackSludge_loop_frame004, 3 },
    { BlackSludge_loop_frame005, 3 },
    { BlackSludge_loop_frame006, 3 },
    { BlackSludge_loop_frame007, 3 },
    { BlackSludge_loop_frame008, 3 },
    { BlackSludge_loop_frame009, 3 },
    { BlackSludge_loop_frame010, 3 },
};

static const AnimFrame GJBeast01_attack01_frames[] = {
    { GJBeast01_attack01_frame000, 2 },
    { GJBeast01_attack01_frame001, 2 },
    { GJBeast01_attack01_frame002, 2 },
};

static const AnimFrame GJBeast01_attack01_end_frames[] = {
    { GJBeast01_attack01_end_frame000, 2 },
    { GJBeast01_attack01_end_frame001, 2 },
    { GJBeast01_attack01_end_frame002, 2 },
    { GJBeast01_attack01_end_frame003, 2 },
    { GJBeast01_attack01_end_frame004, 2 },
    { GJBeast01_attack01_end_frame005, 2 },
};

static const AnimFrame GJBeast01_attack01_loop_frames[] = {
    { GJBeast01_attack01_loop_frame000, 2 },
    { GJBeast01_attack01_loop_frame001, 2 },
    { GJBeast01_attack01_loop_frame002, 2 },
    { GJBeast01_attack01_loop_frame003, 2 },
    { GJBeast01_attack01_loop_frame004, 2 },
    { GJBeast01_attack01_loop_frame005, 2 },
    { GJBeast01_attack01_loop_frame006, 2 },
};

static const AnimFrame GJBeast01_bite_frames[] = {
    { GJBeast01_bite_frame000, 2 },
    { GJBeast01_bite_frame001, 2 },
    { GJBeast01_bite_frame002, 2 },
    { GJBeast01_bite_frame003, 2 },
    { GJBeast01_bite_frame004, 2 },
    { GJBeast01_bite_frame005, 2 },
    { GJBeast01_bite_frame006, 2 },
    { GJBeast01_bite_frame007, 2 },
    { GJBeast01_bite_frame008, 2 },
    { GJBeast01_bite_frame009, 2 },
    { GJBeast01_bite_frame010, 2 },
    { GJBeast01_bite_frame011, 2 },
    { GJBeast01_bite_frame012, 2 },
    { GJBeast01_bite_frame013, 2 },
    { GJBeast01_bite_frame014, 2 },
    { GJBeast01_bite_frame015, 2 },
};

static const AnimFrame GJBeast01_idle01_frames[] = {
    { GJBeast01_idle01_frame000, 2 },
    { GJBeast01_idle01_frame001, 2 },
    { GJBeast01_idle01_frame002, 2 },
    { GJBeast01_idle01_frame003, 2 },
    { GJBeast01_idle01_frame004, 2 },
    { GJBeast01_idle01_frame005, 2 },
    { GJBeast01_idle01_frame006, 2 },
    { GJBeast01_idle01_frame007, 2 },
};

static const AnimFrame GJBeast02_idle01_frames[] = {
    { GJBeast02_idle01_frame000, 2 },
    { GJBeast02_idle01_frame001, 2 },
    { GJBeast02_idle01_frame002, 2 },
    { GJBeast02_idle01_frame003, 2 },
    { GJBeast02_idle01_frame004, 2 },
    { GJBeast02_idle01_frame005, 2 },
    { GJBeast02_idle01_frame006, 2 },
    { GJBeast02_idle01_frame007, 2 },
    { GJBeast02_idle01_frame008, 2 },
};

static const AnimFrame GJBeast02_idle02_frames[] = {
    { GJBeast02_idle02_frame000, 2 },
    { GJBeast02_idle02_frame001, 2 },
    { GJBeast02_idle02_frame002, 2 },
    { GJBeast02_idle02_frame003, 2 },
    { GJBeast02_idle02_frame004, 1 },
    { GJBeast02_idle02_frame005, 1 },
    { GJBeast02_idle02_frame006, 2 },
    { GJBeast02_idle02_frame007, 2 },
    { GJBeast02_idle02_frame008, 2 },
};

static const AnimFrame GJBeast03_idle01_frames[] = {
    { GJBeast03_idle01_frame000, 2 },
    { GJBeast03_idle01_frame001, 2 },
    { GJBeast03_idle01_frame002, 2 },
    { GJBeast03_idle01_frame003, 2 },
    { GJBeast03_idle01_frame004, 2 },
    { GJBeast03_idle01_frame005, 2 },
    { GJBeast03_idle01_frame006, 2 },
    { GJBeast03_idle01_frame007, 2 },
    { GJBeast03_idle01_frame008, 2 },
};

static const AnimFrame GJBeast03_idle02_frames[] = {
    { GJBeast03_idle02_frame000, 2 },
    { GJBeast03_idle02_frame001, 2 },
    { GJBeast03_idle02_frame002, 2 },
    { GJBeast03_idle02_frame003, 2 },
    { GJBeast03_idle02_frame004, 1 },
    { GJBeast03_idle02_frame005, 2 },
    { GJBeast03_idle02_frame006, 2 },
    { GJBeast03_idle02_frame007, 2 },
    { GJBeast03_idle02_frame008, 2 },
};

const Animation animations[ANIMATION_COUNT] = {
    { BlackSludge_loop_frames, 11, 18 },  /* ANIM_BLACKSLUDGE_LOOP */
    { GJBeast01_attack01_frames, 3, 18 },  /* ANIM_GJBEAST01_ATTACK01 */
    { GJBeast01_attack01_end_frames, 6, 18 },  /* ANIM_GJBEAST01_ATTACK01_END */
    { GJBeast01_attack01_loop_frames, 7, 18 },  /* ANIM_GJBEAST01_ATTACK01_LOOP */
    { GJBeast01_bite_frames, 16, 38 },  /* ANIM_GJBEAST01_BITE */
    { GJBeast01_idle01_frames, 8, 18 },  /* ANIM_GJBEAST01_IDLE01 */
    { GJBeast02_idle01_frames, 9, 18 },  /* ANIM_GJBEAST02_IDLE01 */
    { GJBeast02_idle02_frames, 9, 18 },  /* ANIM_GJBEAST02_IDLE02 */
    { GJBeast03_idle01_frames, 9, 18 },  /* ANIM_GJBEAST03_IDLE01 */
    { GJBeast03_idle02_frames, 9, 18 },  /* ANIM_GJBEAST03_IDLE02 */
};
