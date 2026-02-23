/// Lunar longitude (DE404 Moshier theory)
///
/// Implementation of Steve Moshier's DE404-fitted analytical lunar ephemeris.
/// Only the longitude pipeline is retained.

use std::f64::consts::PI;
use super::sun;

const STR: f64 = 4.8481368110953599359e-6;
const J2000: f64 = 2451545.0;

fn mods3600(x: f64) -> f64 {
    x - 1296000.0 * (x / 1296000.0).floor()
}

// DE404 least-squares corrections
const Z: [f64; 25] = [
    -1.312045233711e+01, -1.138215912580e-03, -9.646018347184e-06,
    3.146734198839e+01, 4.768357585780e-02, -3.421689790404e-04,
    -6.847070905410e+00, -5.834100476561e-03, -2.905334122698e-04,
    -5.663161722088e+00, 5.722859298199e-03, -8.466472828815e-05,
    -8.429817796435e+01, -2.072552484689e+02,
    7.876842214863e+00, 1.836463749022e+00,
    -1.557471855361e+01, -2.006969124724e+01,
    2.152670284757e+01, -6.179946916139e+00,
    -9.070028191196e-01, -1.270848233038e+01,
    -2.145589319058e+00, 1.381936399935e+01,
    -1.999840061168e+00,
];

// Main longitude + radius perturbation table (118 terms)
const NLR: usize = 118;
#[rustfmt::skip]
const LR: [i16; 8 * NLR] = [
 0, 0, 1, 0, 22639, 5858,-20905,-3550,
 2, 0,-1, 0,  4586, 4383, -3699,-1109,
 2, 0, 0, 0,  2369, 9139, -2955,-9676,
 0, 0, 2, 0,   769,  257,  -569,-9251,
 0, 1, 0, 0,  -666,-4171,    48, 8883,
 0, 0, 0, 2,  -411,-5957,    -3,-1483,
 2, 0,-2, 0,   211, 6556,   246, 1585,
 2,-1,-1, 0,   205, 4358,  -152,-1377,
 2, 0, 1, 0,   191, 9562,  -170,-7331,
 2,-1, 0, 0,   164, 7285,  -204,-5860,
 0, 1,-1, 0,  -147,-3213,  -129,-6201,
 1, 0, 0, 0,  -124,-9881,   108, 7427,
 0, 1, 1, 0,  -109,-3803,   104, 7552,
 2, 0, 0,-2,    55, 1771,    10, 3211,
 0, 0, 1, 2,   -45, -996,     0,    0,
 0, 0, 1,-2,    39, 5333,    79, 6606,
 4, 0,-1, 0,    38, 4298,   -34,-7825,
 0, 0, 3, 0,    36, 1238,   -23,-2104,
 4, 0,-2, 0,    30, 7726,   -21,-6363,
 2, 1,-1, 0,   -28,-3971,    24, 2085,
 2, 1, 0, 0,   -24,-3582,    30, 8238,
 1, 0,-1, 0,   -18,-5847,    -8,-3791,
 1, 1, 0, 0,    17, 9545,   -16,-6747,
 2,-1, 1, 0,    14, 5303,   -12,-8314,
 2, 0, 2, 0,    14, 3797,   -10,-4448,
 4, 0, 0, 0,    13, 8991,   -11,-6500,
 2, 0,-3, 0,    13, 1941,    14, 4027,
 0, 1,-2, 0,    -9,-6791,    -7,  -27,
 2, 0,-1, 2,    -9,-3659,     0, 7740,
 2,-1,-2, 0,     8, 6055,    10,  562,
 1, 0, 1, 0,    -8,-4531,     6, 3220,
 2,-2, 0, 0,     8,  502,    -9,-8845,
 0, 1, 2, 0,    -7,-6302,     5, 7509,
 0, 2, 0, 0,    -7,-4475,     1,  657,
 2,-2,-1, 0,     7, 3712,    -4,-9501,
 2, 0, 1,-2,    -6,-3832,     4, 1311,
 2, 0, 0, 2,    -5,-7416,     0,    0,
 4,-1,-1, 0,     4, 3740,    -3,-9580,
 0, 0, 2, 2,    -3,-9976,     0,    0,
 3, 0,-1, 0,    -3,-2097,     3, 2582,
 2, 1, 1, 0,    -2,-9145,     2, 6164,
 4,-1,-2, 0,     2, 7319,    -1,-8970,
 0, 2,-1, 0,    -2,-5679,    -2,-1171,
 2, 2,-1, 0,    -2,-5212,     2, 3536,
 2, 1,-2, 0,     2, 4889,     0, 1437,
 2,-1, 0,-2,     2, 1461,     0, 6571,
 4, 0, 1, 0,     1, 9777,    -1,-4226,
 0, 0, 4, 0,     1, 9337,    -1,-1169,
 4,-1, 0, 0,     1, 8708,    -1,-5714,
 1, 0,-2, 0,    -1,-7530,    -1,-7385,
 2, 1, 0,-2,    -1,-4372,     0,-1357,
 0, 0, 2,-2,    -1,-3726,    -4,-4212,
 1, 1, 1, 0,     1, 2618,     0,-9333,
 3, 0,-2, 0,    -1,-2241,     0, 8624,
 4, 0,-3, 0,     1, 1868,     0,-5142,
 2,-1, 2, 0,     1, 1770,     0,-8488,
 0, 2, 1, 0,    -1,-1617,     1, 1655,
 1, 1,-1, 0,     1,  777,     0, 8512,
 2, 0, 3, 0,     1,  595,     0,-6697,
 2, 0, 1, 2,     0,-9902,     0,    0,
 2, 0,-4, 0,     0, 9483,     0, 7785,
 2,-2, 1, 0,     0, 7517,     0,-6575,
 0, 1,-3, 0,     0,-6694,     0,-4224,
 4, 1,-1, 0,     0,-6352,     0, 5788,
 1, 0, 2, 0,     0,-5840,     0, 3785,
 1, 0, 0,-2,     0,-5833,     0,-7956,
 6, 0,-2, 0,     0, 5716,     0,-4225,
 2, 0,-2,-2,     0,-5606,     0, 4726,
 1,-1, 0, 0,     0,-5569,     0, 4976,
 0, 1, 3, 0,     0,-5459,     0, 3551,
 2, 0,-2, 2,     0,-5357,     0, 7740,
 2, 0,-1,-2,     0, 1790,     8, 7516,
 3, 0, 0, 0,     0, 4042,    -1,-4189,
 2,-1,-3, 0,     0, 4784,     0, 4950,
 2,-1, 3, 0,     0,  932,     0, -585,
 2, 0, 2,-2,     0,-4538,     0, 2840,
 2,-1,-1, 2,     0,-4262,     0,  373,
 0, 0, 0, 4,     0, 4203,     0,    0,
 0, 1, 0, 2,     0, 4134,     0,-1580,
 6, 0,-1, 0,     0, 3945,     0,-2866,
 2,-1, 0, 2,     0,-3821,     0,    0,
 2,-1, 1,-2,     0,-3745,     0, 2094,
 4, 1,-2, 0,     0,-3576,     0, 2370,
 1, 1,-2, 0,     0, 3497,     0, 3323,
 2,-3, 0, 0,     0, 3398,     0,-4107,
 0, 0, 3, 2,     0,-3286,     0,    0,
 4,-2,-1, 0,     0,-3087,     0,-2790,
 0, 1,-1,-2,     0, 3015,     0,    0,
 4, 0,-1,-2,     0, 3009,     0,-3218,
 2,-2,-2, 0,     0, 2942,     0, 3430,
 6, 0,-3, 0,     0, 2925,     0,-1832,
 2, 1, 2, 0,     0,-2902,     0, 2125,
 4, 1, 0, 0,     0,-2891,     0, 2445,
 4,-1, 1, 0,     0, 2825,     0,-2029,
 3, 1,-1, 0,     0, 2737,     0,-2126,
 0, 1, 1, 2,     0, 2634,     0,    0,
 1, 0, 0, 2,     0, 2543,     0,    0,
 3, 0, 0,-2,     0,-2530,     0, 2010,
 2, 2,-2, 0,     0,-2499,     0,-1089,
 2,-3,-1, 0,     0, 2469,     0,-1481,
 3,-1,-1, 0,     0,-2314,     0, 2556,
 4, 0, 2, 0,     0, 2185,     0,-1392,
 4, 0,-1, 2,     0,-2013,     0,    0,
 0, 2,-2, 0,     0,-1931,     0,    0,
 2, 2, 0, 0,     0,-1858,     0,    0,
 2, 1,-3, 0,     0, 1762,     0,    0,
 4, 0,-2, 2,     0,-1698,     0,    0,
 4,-2,-2, 0,     0, 1578,     0,-1083,
 4,-2, 0, 0,     0, 1522,     0,-1281,
 3, 1, 0, 0,     0, 1499,     0,-1077,
 1,-1,-1, 0,     0,-1364,     0, 1141,
 1,-3, 0, 0,     0,-1281,     0,    0,
 6, 0, 0, 0,     0, 1261,     0, -859,
 2, 0, 2, 2,     0,-1239,     0,    0,
 1,-1, 1, 0,     0,-1207,     0, 1100,
 0, 0, 5, 0,     0, 1110,     0, -589,
 0, 3, 0, 0,     0,-1013,     0,  213,
 4,-1,-3, 0,     0,  998,     0,    0,
];

// T^1 corrections (38 terms)
const NLRT: usize = 38;
#[rustfmt::skip]
const LRT: [i16; 8 * NLRT] = [
 0, 1, 0, 0,    16, 7680,    -1,-2302,
 2,-1,-1, 0,    -5,-1642,     3, 8245,
 2,-1, 0, 0,    -4,-1383,     5, 1395,
 0, 1,-1, 0,     3, 7115,     3, 2654,
 0, 1, 1, 0,     2, 7560,    -2,-6396,
 2, 1,-1, 0,     0, 7118,     0,-6068,
 2, 1, 0, 0,     0, 6128,     0,-7754,
 1, 1, 0, 0,     0,-4516,     0, 4194,
 2,-2, 0, 0,     0,-4048,     0, 4970,
 0, 2, 0, 0,     0, 3747,     0, -540,
 2,-2,-1, 0,     0,-3707,     0, 2490,
 2,-1, 1, 0,     0,-3649,     0, 3222,
 0, 1,-2, 0,     0, 2438,     0, 1760,
 2,-1,-2, 0,     0,-2165,     0,-2530,
 0, 1, 2, 0,     0, 1923,     0,-1450,
 0, 2,-1, 0,     0, 1292,     0, 1070,
 2, 2,-1, 0,     0, 1271,     0,-6070,
 4,-1,-1, 0,     0,-1098,     0,  990,
 2, 0, 0, 0,     0, 1073,     0,-1360,
 2, 0,-1, 0,     0,  839,     0, -630,
 2, 1, 1, 0,     0,  734,     0, -660,
 4,-1,-2, 0,     0, -688,     0,  480,
 2, 1,-2, 0,     0, -630,     0,    0,
 0, 2, 1, 0,     0,  587,     0, -590,
 2,-1, 0,-2,     0, -540,     0, -170,
 4,-1, 0, 0,     0, -468,     0,  390,
 2,-2, 1, 0,     0, -378,     0,  330,
 2, 1, 0,-2,     0,  364,     0,    0,
 1, 1, 1, 0,     0, -317,     0,  240,
 2,-1, 2, 0,     0, -295,     0,  210,
 1, 1,-1, 0,     0, -270,     0, -210,
 2,-3, 0, 0,     0, -256,     0,  310,
 2,-3,-1, 0,     0, -187,     0,  110,
 0, 1,-3, 0,     0,  169,     0,  110,
 4, 1,-1, 0,     0,  158,     0, -150,
 4,-2,-1, 0,     0, -155,     0,  140,
 0, 0, 1, 0,     0,  155,     0, -250,
 2,-2,-2, 0,     0, -148,     0, -170,
];

// T^2 corrections (25 terms)
const NLRT2: usize = 25;
#[rustfmt::skip]
const LRT2: [i16; 6 * NLRT2] = [
 0, 1, 0, 0,  487,   -36,
 2,-1,-1, 0, -150,   111,
 2,-1, 0, 0, -120,   149,
 0, 1,-1, 0,  108,    95,
 0, 1, 1, 0,   80,   -77,
 2, 1,-1, 0,   21,   -18,
 2, 1, 0, 0,   20,   -23,
 1, 1, 0, 0,  -13,    12,
 2,-2, 0, 0,  -12,    14,
 2,-1, 1, 0,  -11,     9,
 2,-2,-1, 0,  -11,     7,
 0, 2, 0, 0,   11,     0,
 2,-1,-2, 0,   -6,    -7,
 0, 1,-2, 0,    7,     5,
 0, 1, 2, 0,    6,    -4,
 2, 2,-1, 0,    5,    -3,
 0, 2,-1, 0,    5,     3,
 4,-1,-1, 0,   -3,     3,
 2, 0, 0, 0,    3,    -4,
 4,-1,-2, 0,   -2,     0,
 2, 1,-2, 0,   -2,     0,
 2,-1, 0,-2,   -2,     0,
 2, 1, 1, 0,    2,    -2,
 2, 0,-1, 0,    2,     0,
 0, 2, 1, 0,    2,     0,
];

/// Mutable state for moon computation
pub struct MoonState {
    ss: [[f64; 8]; 5],
    cc: [[f64; 8]; 5],
    swelp: f64,
    m_sun: f64,
    mp: f64,
    d: f64,
    nf: f64,
    t: f64,
    t2: f64,
    ve: f64,
    ea: f64,
    ma: f64,
    ju: f64,
    sa: f64,
}

impl MoonState {
    pub fn new() -> Self {
        MoonState {
            ss: [[0.0; 8]; 5],
            cc: [[0.0; 8]; 5],
            swelp: 0.0, m_sun: 0.0, mp: 0.0, d: 0.0, nf: 0.0,
            t: 0.0, t2: 0.0,
            ve: 0.0, ea: 0.0, ma: 0.0, ju: 0.0, sa: 0.0,
        }
    }

    fn sscc(&mut self, k: usize, arg: f64, n: usize) {
        let cu = arg.cos();
        let su = arg.sin();
        self.ss[k][0] = su;
        self.cc[k][0] = cu;
        let mut sv = 2.0 * su * cu;
        let mut cv = cu * cu - su * su;
        self.ss[k][1] = sv;
        self.cc[k][1] = cv;
        for i in 2..n {
            let s = su * cv + cu * sv;
            cv = cu * cv - su * sv;
            sv = s;
            self.ss[k][i] = sv;
            self.cc[k][i] = cv;
        }
    }

    fn chewm(&self, pt: &[i16], nlines: usize, nangles: usize, typflg: i32) -> f64 {
        let mut ans = 0.0f64;
        let mut idx = 0;
        for _ in 0..nlines {
            let mut k1 = 0i32;
            let mut sv = 0.0f64;
            let mut cv = 0.0f64;
            for m in 0..nangles {
                let j = pt[idx] as i32;
                idx += 1;
                if j != 0 {
                    let k = (if j < 0 { -j } else { j }) as usize;
                    let mut su = self.ss[m][k - 1];
                    let cu = self.cc[m][k - 1];
                    if j < 0 { su = -su; }
                    if k1 == 0 {
                        sv = su;
                        cv = cu;
                        k1 = 1;
                    } else {
                        let ff = su * cv + cu * sv;
                        cv = cu * cv - su * sv;
                        sv = ff;
                    }
                }
            }
            match typflg {
                1 => {
                    let j = pt[idx] as f64;
                    idx += 1;
                    let k = pt[idx] as f64;
                    idx += 1;
                    ans += (10000.0 * j + k) * sv;
                    idx += 2; // skip radius
                }
                2 => {
                    let j = pt[idx] as f64;
                    idx += 1;
                    ans += j * sv;
                    idx += 1; // skip radius
                }
                _ => {}
            }
        }
        ans
    }

    fn mean_elements(&mut self) {
        let t = self.t;
        let t2 = self.t2;
        let frac_t = t % 1.0;

        self.m_sun = mods3600(129600000.0 * frac_t - 3418.961646 * t + 1287104.76154);
        self.m_sun += ((((((((
            1.62e-20 * t
            - 1.0390e-17) * t
            - 3.83508e-15) * t
            + 4.237343e-13) * t
            + 8.8555011e-11) * t
            - 4.77258489e-8) * t
            - 1.1297037031e-5) * t
            + 1.4732069041e-4) * t
            - 0.552891801772) * t2;

        self.nf = mods3600(1739232000.0 * frac_t + 295263.0983 * t
            - 2.079419901760e-01 * t + 335779.55755);

        self.mp = mods3600(1717200000.0 * frac_t + 715923.4728 * t
            - 2.035946368532e-01 * t + 485868.28096);

        self.d = mods3600(1601856000.0 * frac_t + 1105601.4603 * t
            + 3.962893294503e-01 * t + 1072260.73512);

        self.swelp = mods3600(1731456000.0 * frac_t + 1108372.83264 * t
            - 6.784914260953e-01 * t + 785939.95571);

        self.nf += ((Z[2] * t + Z[1]) * t + Z[0]) * t2;
        self.mp += ((Z[5] * t + Z[4]) * t + Z[3]) * t2;
        self.d += ((Z[8] * t + Z[7]) * t + Z[6]) * t2;
        self.swelp += ((Z[11] * t + Z[10]) * t + Z[9]) * t2;
    }

    fn mean_elements_pl(&mut self) {
        let t = self.t;
        let t2 = self.t2;

        self.ve = mods3600(210664136.4335482 * t + 655127.283046);
        self.ve += ((((((((
            -9.36e-023 * t
            - 1.95e-20) * t
            + 6.097e-18) * t
            + 4.43201e-15) * t
            + 2.509418e-13) * t
            - 3.0622898e-10) * t
            - 2.26602516e-9) * t
            - 1.4244812531e-5) * t
            + 0.005871373088) * t2;

        self.ea = mods3600(129597742.26669231 * t + 361679.214649);
        self.ea += ((((((((-1.16e-22 * t
            + 2.976e-19) * t
            + 2.8460e-17) * t
            - 1.08402e-14) * t
            - 1.226182e-12) * t
            + 1.7228268e-10) * t
            + 1.515912254e-7) * t
            + 8.863982531e-6) * t
            - 2.0199859001e-2) * t2;

        self.ma = mods3600(68905077.59284 * t + 1279559.78866);
        self.ma += (-1.043e-5 * t + 9.38012e-3) * t2;

        self.ju = mods3600(10925660.428608 * t + 123665.342120);
        self.ju += (1.543273e-5 * t - 3.06037836351e-1) * t2;

        self.sa = mods3600(4399609.65932 * t + 180278.89694);
        self.sa += ((4.475946e-8 * t - 6.874806e-5) * t + 7.56161437443e-1) * t2;
    }

    /// Compute all perturbation corrections to mean lunar longitude.
    /// Returns lunar longitude in radians (arcseconds-based).
    fn lunar_perturbations(&mut self) -> f64 {
        let g = |x: f64| -> f64 { (STR * x).sin() };

        // Zero ss/cc arrays
        for i in 0..5 {
            for j in 0..8 {
                self.ss[i][j] = 0.0;
                self.cc[i][j] = 0.0;
            }
        }

        self.sscc(0, STR * self.d, 6);
        self.sscc(1, STR * self.m_sun, 4);
        self.sscc(2, STR * self.mp, 4);
        self.sscc(3, STR * self.nf, 4);

        // T^2 table corrections
        let mut moonpol0 = self.chewm(&LRT2, NLRT2, 4, 2);

        // Explicit planetary perturbations
        let f_ve = 18.0 * self.ve - 16.0 * self.ea;

        let g_arg = STR * (f_ve - self.mp);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        let mut l_acc = 6.367278 * cg + 12.747036 * sg;
        let mut l1 = 23123.70 * cg - 10570.02 * sg;
        let mut l2 = Z[12] * cg + Z[13] * sg;

        let g_arg = STR * (10.0 * self.ve - 3.0 * self.ea - self.mp);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += -0.253102 * cg + 0.503359 * sg;
        l1 += 1258.46 * cg + 707.29 * sg;
        l2 += Z[14] * cg + Z[15] * sg;

        let g_arg = STR * (8.0 * self.ve - 13.0 * self.ea);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += -0.187231 * cg - 0.127481 * sg;
        l1 += -319.87 * cg - 18.34 * sg;
        l2 += Z[16] * cg + Z[17] * sg;

        let a = 4.0 * self.ea - 8.0 * self.ma + 3.0 * self.ju;
        let g_arg = STR * a;
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += -0.866287 * cg + 0.248192 * sg;
        l1 += 41.87 * cg + 1053.97 * sg;
        l2 += Z[18] * cg + Z[19] * sg;

        let g_arg = STR * (a - self.mp);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += -0.165009 * cg + 0.044176 * sg;
        l1 += 4.67 * cg + 201.55 * sg;

        let g_arg = STR * f_ve;
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += 0.330401 * cg + 0.661362 * sg;
        l1 += 1202.67 * cg - 555.59 * sg;
        l2 += Z[20] * cg + Z[21] * sg;

        let g_arg = STR * (f_ve - 2.0 * self.mp);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += 0.352185 * cg + 0.705041 * sg;
        l1 += 1283.59 * cg - 586.43 * sg;

        let g_arg = STR * (2.0 * self.ju - 5.0 * self.sa);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += -0.034700 * cg + 0.160041 * sg;
        l2 += Z[22] * cg + Z[23] * sg;

        let g_arg = STR * (self.swelp - self.nf);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += 0.000116 * cg + 7.063040 * sg;
        l1 += 298.8 * sg;

        // T^3 terms
        let sg = (STR * self.m_sun).sin();
        let l3 = Z[24] * sg;
        let l4 = 0.0f64;

        l2 += moonpol0;

        // T^1 table corrections
        moonpol0 = self.chewm(&LRT, NLRT, 4, 1);

        let g_arg = STR * (2.0 * self.ve - 3.0 * self.ea);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += -0.343550 * cg - 0.000276 * sg;
        l1 += 105.90 * cg + 336.53 * sg;

        let g_arg = STR * (f_ve - 2.0 * self.d);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += 0.074668 * cg + 0.149501 * sg;
        l1 += 271.77 * cg - 124.20 * sg;

        let g_arg = STR * (f_ve - 2.0 * self.d - self.mp);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += 0.073444 * cg + 0.147094 * sg;
        l1 += 265.24 * cg - 121.16 * sg;

        let g_arg = STR * (f_ve + 2.0 * self.d - self.mp);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += 0.072844 * cg + 0.145829 * sg;
        l1 += 265.18 * cg - 121.29 * sg;

        let g_arg = STR * (f_ve + 2.0 * (self.d - self.mp));
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += 0.070201 * cg + 0.140542 * sg;
        l1 += 255.36 * cg - 116.79 * sg;

        let g_arg = STR * (self.ea + self.d - self.nf);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += 0.288209 * cg - 0.025901 * sg;
        l1 += -63.51 * cg - 240.14 * sg;

        let g_arg = STR * (2.0 * self.ea - 3.0 * self.ju + 2.0 * self.d - self.mp);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += 0.077865 * cg + 0.438460 * sg;
        l1 += 210.57 * cg + 124.84 * sg;

        let g_arg = STR * (self.ea - 2.0 * self.ma);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += -0.216579 * cg + 0.241702 * sg;
        l1 += 197.67 * cg + 125.23 * sg;

        let g_arg = STR * (a + self.mp);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += -0.165009 * cg + 0.044176 * sg;
        l1 += 4.67 * cg + 201.55 * sg;

        let g_arg = STR * (a + 2.0 * self.d - self.mp);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += -0.133533 * cg + 0.041116 * sg;
        l1 += 6.95 * cg + 187.07 * sg;

        let g_arg = STR * (a - 2.0 * self.d + self.mp);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += -0.133430 * cg + 0.041079 * sg;
        l1 += 6.28 * cg + 169.08 * sg;

        let g_arg = STR * (3.0 * self.ve - 4.0 * self.ea);
        let cg = g_arg.cos();
        let sg = g_arg.sin();
        l_acc += -0.175074 * cg + 0.003035 * sg;
        l1 += 49.17 * cg + 150.57 * sg;

        let g_arg = STR * (2.0 * (self.ea + self.d - self.mp) - 3.0 * self.ju + 213534.0);
        l1 += 158.4 * g_arg.sin();

        l1 += moonpol0;

        // Additional DE404-fitted explicit terms
        l_acc += 1.14307 * g(2.0 * (self.ea - self.ju + self.d) - self.mp + 648431.172);
        l_acc += 0.82155 * g(self.ve - self.ea + 648035.568);
        l_acc += 0.64371 * g(3.0 * (self.ve - self.ea) + 2.0 * self.d - self.mp + 647933.184);
        l_acc += 0.63880 * g(self.ea - self.ju + 4424.04);
        l_acc += 0.49331 * g(self.swelp + self.mp - self.nf + 4.68);
        l_acc += 0.4914 * g(self.swelp - self.mp - self.nf + 4.68);
        l_acc += 0.36061 * g(self.swelp + self.nf + 2.52);
        l_acc += 0.30154 * g(2.0 * self.ve - 2.0 * self.ea + 736.2);
        l_acc += 0.28282 * g(2.0 * self.ea - 3.0 * self.ju + 2.0 * self.d - 2.0 * self.mp + 36138.2);
        l_acc += 0.24516 * g(2.0 * self.ea - 2.0 * self.ju + 2.0 * self.d - 2.0 * self.mp + 311.0);
        l_acc += 0.21117 * g(self.ea - self.ju - 2.0 * self.d + self.mp + 6275.88);
        l_acc += 0.19444 * g(2.0 * (self.ea - self.ma) - 846.36);
        l_acc -= 0.18457 * g(2.0 * (self.ea - self.ju) + 1569.96);
        l_acc += 0.18256 * g(2.0 * (self.ea - self.ju) - self.mp - 55.8);
        l_acc += 0.16499 * g(self.ea - self.ju - 2.0 * self.d + 6490.08);
        l_acc += 0.16427 * g(self.ea - 2.0 * self.ju - 212378.4);
        l_acc += 0.16088 * g(2.0 * (self.ve - self.ea - self.d) + self.mp + 1122.48);
        l_acc -= 0.15350 * g(self.ve - self.ea - self.mp + 32.04);
        l_acc += 0.14346 * g(self.ea - self.ju - self.mp + 4488.88);
        l_acc += 0.13594 * g(2.0 * (self.ve - self.ea + self.d) - self.mp - 8.64);
        l_acc += 0.13432 * g(2.0 * (self.ve - self.ea - self.d) + 1319.76);
        l_acc -= 0.13122 * g(self.ve - self.ea - 2.0 * self.d + self.mp - 56.16);
        l_acc -= 0.12722 * g(self.ve - self.ea + self.mp + 54.36);
        l_acc += 0.12539 * g(3.0 * (self.ve - self.ea) - self.mp + 433.8);
        l_acc += 0.10994 * g(self.ea - self.ju + self.mp + 4002.12);
        l_acc += 0.10652 * g(20.0 * self.ve - 21.0 * self.ea - 2.0 * self.d + self.mp - 317511.72);
        l_acc += 0.10490 * g(26.0 * self.ve - 29.0 * self.ea - self.mp + 270002.52);
        l_acc += 0.10386 * g(3.0 * self.ve - 4.0 * self.ea + self.d - self.mp - 322765.56);

        // Main perturbation table (118 terms) + final assembly
        moonpol0 = self.chewm(&LR, NLR, 4, 1);
        let t = self.t;
        l_acc += (((l4 * t + l3) * t + l2) * t + l1) * t * 1.0e-5;
        let moonpol0 = self.swelp + l_acc + 1.0e-4 * moonpol0;

        STR * mods3600(moonpol0)
    }

    /// Compute tropical lunar longitude in degrees [0, 360)
    pub fn lunar_longitude(&mut self, jd_ut: f64) -> f64 {
        let jd_tt = jd_ut + sun::delta_t_days(jd_ut);
        self.t = (jd_tt - J2000) / 36525.0;
        self.t2 = self.t * self.t;

        self.mean_elements();
        self.mean_elements_pl();
        let lon_rad = self.lunar_perturbations();

        let mut lon_deg = lon_rad * (180.0 / PI);

        // Distance-dependent light-time correction
        {
            let cos_l = (STR * self.mp).cos();
            let cos_2d_l = (STR * (2.0 * self.d - self.mp)).cos();
            let cos_2d = (STR * (2.0 * self.d)).cos();
            let cos_2l = (STR * (2.0 * self.mp)).cos();
            let cos_lp = (STR * self.m_sun).cos();

            let r_mean = 385000.529;
            let delta_r = -20905.355 * cos_l
                - 3699.111 * cos_2d_l
                - 2955.968 * cos_2d
                - 569.925 * cos_2l
                + 48.888 * cos_lp;

            lon_deg -= 0.000196 * (r_mean / (r_mean + delta_r));
        }

        // Apply nutation
        lon_deg += sun::nutation_longitude(jd_ut);

        // Normalize to [0, 360)
        lon_deg = lon_deg % 360.0;
        if lon_deg < 0.0 { lon_deg += 360.0; }

        lon_deg
    }
}
