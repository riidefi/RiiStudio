#include "analysis.h"
#include "palette.h"

#include <math.h>
#include <stdlib.h>

#ifndef _WIN32

#define min(a, b) ((a < b) ? a : b)
#define max(a, b) ((a > b) ? a : b)

#endif

DWORD getAverageColor(DWORD *colors, int nColors) {
	int totalR = 0, totalG = 0, totalB = 0, nCounted = 0;
	for (int i = 0; i < nColors; i++) {
		DWORD col = colors[i];
		if ((col >> 24) >= 0x80) {
			totalR += col & 0xFF;
			totalG += (col >> 8) & 0xFF;
			totalB += (col >> 16) & 0xFF;
			nCounted++;
		}
	}
	//round halfway
	if (nCounted > 0) {
		totalR = (totalR + (nCounted >> 1)) / nCounted;
		totalG = (totalG + (nCounted >> 1)) / nCounted;
		totalB = (totalB + (nCounted >> 1)) / nCounted;
	}
	return totalR | (totalG << 8) | (totalB << 16);
}

float computeCovariance(DWORD *colors, int nColors, DWORD avg, int i, int j) {
	//covariance between ith and jth components of the set
	int total = 0, nCounted = 0;
	int avgI = (avg >> (i * 8)) & 0xFF;
	int avgJ = (avg >> (j * 8)) & 0xFF;
	for (int k = 0; k < nColors; k++) {
		DWORD col = colors[k];
		if ((col >> 24) >= 0x80) {
			int compI = ((col >> (i * 8)) & 0xFF) - avgI;
			int compJ = ((col >> (j * 8)) & 0xFF) - avgJ;
			total += compI * compJ;
			nCounted++;
		}
	}
	if (!nCounted) return 0.0f;
	return ((float) total) / nCounted;
}

void computeCovarianceMatrix(DWORD *colors, int nColors, float cov[3][3]) {
	DWORD avg = getAverageColor(colors, nColors);

	int avgR = avg & 0xFF;
	int avgG = (avg >> 8) & 0xFF;
	int avgB = (avg >> 16) & 0xFF;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			cov[i][j] = computeCovariance(colors, nColors, avg, i, j);
		}
	}
}

#ifndef PI
#	define PI 3.141592653f
#endif

//
// Charles-Alban Deledalle, Loic Denis, Sonia Tabti, Florence Tupin. Closed-
// form expressions of the eigen decomposition of 2 x 2 and 3 x 3 Hermitian
// matrices. [Research Report] Universite de Lyon. 2017. ffhal-01501221
//

void findGreatestEigenvectorSymmetric3x3(float mtx[3][3], float *vec) {
	float a = mtx[0][0], b = mtx[1][1], c = mtx[2][2], d = mtx[0][1], e = mtx[1][2], f = mtx[0][2];

	//do some tests on the covariance matrix. If columns/rows are missing, do a lesser computation.
	int use2x2 = 0, missing = 0;
	float mtx2x2[2][2] = { {0, 0}, {0, 0} };
	
	if (a == 0.0f && d == 0.0f && f == 0.0f) {
		mtx2x2[0][0] = b;
		mtx2x2[0][1] = e;
		mtx2x2[1][0] = e;
		mtx2x2[1][1] = c;
		use2x2 = 1;
		missing = 0;
	} else if (b == 0.0f && d == 0.0f && e == 0.0f) {
		mtx2x2[0][0] = a;
		mtx2x2[0][1] = f;
		mtx2x2[1][0] = f;
		mtx2x2[1][1] = c;
		use2x2 = 1;
		missing = 1;
	} else if (c == 0.0f && e == 0.0f && f == 0.0f) {
		mtx2x2[0][0] = a;
		mtx2x2[0][1] = d;
		mtx2x2[1][0] = d;
		mtx2x2[1][1] = b;
		use2x2 = 1;
		missing = 2;
	}

	if (use2x2) {
		a = mtx2x2[0][0];
		b = mtx2x2[1][1];
		c = mtx2x2[0][1];
		float delta = (float) sqrt(4.0f * c * c + (a - b) * (a - b));
		float l1 = (a + b - delta) * 0.5f;
		float l2 = (a + b + delta) * 0.5f;
		float v[2] = { 0, 0 };
		if (l1 >= l2) {
			v[0] = -c;
			v[1] = a - l1;
		} else {
			v[0] = -c;
			v[1] = a - l2;
		}
		if (missing == 0) {
			vec[1] = v[0];
			vec[2] = v[1];
		} else if (missing == 1) {
			vec[0] = v[0];
			vec[2] = v[1];
		} else if (missing == 2) {
			vec[1] = v[0];
			vec[2] = v[1];
		}
		return;
	}

	float x1 = (a * a + b * b + c * c - a * b - a * c - b * c + 3.0f * (d * d + f * f + e * e));
	float x2 = -(2 * a - b - c) * (2 * b - a - c) * (2 * c - a - b) 
		+ 9.0f * (d * d * (2 * c - a - b) + f * f * (2 * b - a - c) + e * e * (2 * a - b - c)) 
		- 54.0f * d * e * f;
	float phi = 0.5f * PI;
	if (x2 != 0.0f) {
		phi = (float) atan(sqrt(4.0f * x1 * x1 * x1 - x2 * x2) / x2);
		if (x2 < 0.0f) {
			phi += PI;
		}
	}

	//find eigenvalues
	float sqrtX1 = 2.0f * (float) sqrt(x1);
	float l1 = (a + b + c - sqrtX1 * (float) cos(phi / 3.0f)) / 3.0f;
	float l2 = (a + b + c + sqrtX1 * (float) cos((phi - PI) / 3.0f)) / 3.0f;
	float l3 = (a + b + c + sqrtX1 * (float) cos((phi + PI) / 3.0f)) / 3.0f;

	float l = max(max(l1, l2), l3);
	float m = (d * (c - l) - e * f) / (f * (b - l) - d * e);
	vec[0] = l - c - e * m;
	vec[1] = m * f;
	vec[2] = f;
}

void getPrincipalComponent(DWORD *colors, int nColors, float *vec) {
	//compute covariance matrix
	float cov[3][3];
	computeCovarianceMatrix(colors, nColors, cov);

	//find eigenvector corresponding to the greatest eigenvalue
	findGreatestEigenvectorSymmetric3x3(cov, vec);

	//make the vector unit length
	float length = (float) sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
	if (length > 0.0f) {
		vec[0] /= length;
		vec[1] /= length;
		vec[2] /= length;
	}
}

void getColorEndPoints(DWORD *colors, int nColors, DWORD *points) {
	//find principal component
	float principal[3];
	getPrincipalComponent(colors, nColors, principal);

	DWORD avg = getAverageColor(colors, nColors);
	int avgR = avg & 0xFF, avgG = (avg >> 8) & 0xFF, avgB = (avg >> 16) & 0xFF;
	float means[3];
	means[0] = (float) avgR;
	means[1] = (float) avgG;
	means[2] = (float) avgB;

	//find endpoints on this line that do not go past the projections onto it
	float minClampComponent = 0.0f, maxClampComponent = 0.0f;
	{
		int foundClampMin = 0, foundClampMax = 0;
		for (int i = 0; i < 3; i++) {
			float t = -means[i] / principal[i];
			if ((!foundClampMin || t > minClampComponent) && t < 0) {
				minClampComponent = t;
				foundClampMin = 1;
			}
			if ((!foundClampMax || t < maxClampComponent) && t > 0) {
				maxClampComponent = t;
				foundClampMax = 1;
			}
			t = (255.0f - means[i]) / principal[i];
			if ((!foundClampMax || t < maxClampComponent) && t > 0) {
				maxClampComponent = t;
				foundClampMax = 1;
			}
			if ((!foundClampMin || t > minClampComponent) && t < 0) {
				minClampComponent = t;
				foundClampMax = 1;
			}
		}
	}

	float minComponent = 255.0f, maxComponent = -1.0f;
	for (int i = 0; i < nColors; i++) {
		DWORD col = colors[i];
		if ((col >> 24) < 0x80) continue;

		int r = (col & 0xFF) - avgR;
		int g = ((col >> 8) & 0xFF) - avgG;
		int b = ((col >> 16) & 0xFF) - avgB;
		float component = r * principal[0] + g * principal[1] + b * principal[2];
		if (component > maxComponent || i == 0) {
			maxComponent = component;
		}
		if (component < minComponent || i == 0) {
			minComponent = component;
		}
	}

	//clamp the component into the correct range so the colors don't fall out of the RGB cube!
	if (minComponent < minClampComponent) minComponent = minClampComponent;
	if (minComponent > maxClampComponent) minComponent = maxClampComponent;
	if (maxComponent < minClampComponent) maxComponent = minClampComponent;
	if (maxComponent > maxClampComponent) maxComponent = maxClampComponent;

	int r1 = (int) (means[0] + minComponent * principal[0] + 0.5f);
	int g1 = (int) (means[1] + minComponent * principal[1] + 0.5f);
	int b1 = (int) (means[2] + minComponent * principal[2] + 0.5f);
	int r2 = (int) (means[0] + maxComponent * principal[0] + 0.5f);
	int g2 = (int) (means[1] + maxComponent * principal[1] + 0.5f);
	int b2 = (int) (means[2] + maxComponent * principal[2] + 0.5f);

	points[0] = r1 | (g1 << 8) | (b1 << 16);
	points[1] = r2 | (g2 << 8) | (b2 << 16);
	qsort(points, 2, 4, lightnessCompare);
}
