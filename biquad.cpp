#include "biquad.h"
#include <cmath>
#include <cfloat>
#include <list>
#include <cstdio>
#include <QDebug>

biquad::biquad()
{
    internalBiquadCoeffs[0] = 0.0;
    internalBiquadCoeffs[1] = 0.0;
    internalBiquadCoeffs[2] = 0.0;
    internalBiquadCoeffs[3] = 0.0;
    a0 = 0.0;
}

void biquad::RefreshFilter(Type type,double dbGain, double centreFreq, double fs, double dBandwidthOrQOrS, bool isBandwidthOrS)
{
    m_dFilterType = type;
    m_dFilterGain = dbGain;
    m_dFilterFreq = centreFreq;
    m_dFilterBQ = dBandwidthOrQOrS;
    m_isBandwidthOrS = isBandwidthOrS;
    double d;
    if (type == PEAKING || type == LOW_SHELF || type == HIGH_SHELF)
        d = pow(10.0, dbGain / 40.0);
    else
        d = pow(10.0, dbGain / 20.0);
    double a = (6.2831853071795862 * centreFreq) / fs; //omega
    double num3 = sin(a); //sn
    double cs = cos(a);

    double alpha;
    if (!isBandwidthOrS) // Q
        alpha = num3 / (2 * dBandwidthOrQOrS);
    else if (type == LOW_SHELF || type == HIGH_SHELF) // S
        alpha = num3 / 2 * sqrt((d + 1 / d) * (1 / dBandwidthOrQOrS - 1) + 2);
    else // BW
        alpha = num3 * sinh(0.693147180559945309417 / 2 * dBandwidthOrQOrS * a / num3);

    double beta = 2 * sqrt(d) * alpha;
    double B0, B1, B2, A0, A1, A2;

    switch (type)
    {
    case LOW_PASS:
        B0 = (1.0 - cs) / 2.0;
        B1 = 1.0 - cs;
        B2 = (1.0 - cs) / 2.0;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case HIGH_PASS:
        B0 = (1.0 + cs) / 2.0;
        B1 = -(1.0 + cs);
        B2 = (1.0 + cs) / 2.0;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case BAND_PASS:
        B0 = alpha;
        B1 = 0.0;
        B2 = -alpha;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case NOTCH:
        B0 = 1.0;
        B1 = -2.0 * cs;
        B2 = 1.0;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case ALL_PASS:
        B0 = 1.0 - alpha;
        B1 = -2.0 * cs;
        B2 = 1.0 + alpha;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case PEAKING:
        B0 = 1.0 + (alpha * d);
        B1 = -2.0 * cs;
        B2 = 1.0 - (alpha * d);
        A0 = 1.0 + (alpha / d);
        A1 = -2.0 * cs;
        A2 = 1.0 - (alpha / d);
        break;
    case LOW_SHELF:
        B0 = d * ((d + 1.0) - (d - 1.0) * cs + beta);
        B1 = 2.0 * d * ((d - 1.0) - (d + 1.0) * cs);
        B2 = d * ((d + 1.0) - (d - 1.0) * cs - beta);
        A0 = (d + 1.0) + (d - 1.0) * cs + beta;
        A1 = -2.0 * ((d - 1.0) + (d + 1.0) * cs);
        A2 = (d + 1.0) + (d - 1.0) * cs - beta;
        break;
    case HIGH_SHELF:
        B0 = d * ((d + 1.0) + (d - 1.0) * cs + beta);
        B1 = -2.0 * d * ((d - 1.0) + (d + 1.0) * cs);
        B2 = d * ((d + 1.0) + (d - 1.0) * cs - beta);
        A0 = (d + 1.0) - (d - 1.0) * cs + beta;
        A1 = 2.0 * ((d - 1.0) - (d + 1.0) * cs);
        A2 = (d + 1.0) - (d - 1.0) * cs - beta;
        break;
    case UNITY_GAIN:
        B0 = d;
        B1 = 0.0;
        B2 = 0.0;
        A0 = 1.0;
        A1 = 0.0;
        A2 = 0.0;
        break;
    }

    //Check if filter is stable/usable
    double res;
    double d1 = pow(A1,2)-4*a0*A2;
    double realpart = -A1 / (2*a0);
    double ipart = sqrt(-d1)/(2*A1);
    double magnitude = sqrt(pow(realpart,2)+pow(ipart,2));
    double realroot1 = (-A1-sqrt(d1))/2*a0;

    if(d1 < 0) res = magnitude;
    else res = realroot1;

    m_isStable = fabs(res)<1;

    internalBiquadCoeffs[0] = B1 / A0;
    internalBiquadCoeffs[1] = B2 / A0;
    internalBiquadCoeffs[2] = -A1 / A0;
    internalBiquadCoeffs[3] = -A2 / A0;
}
std::list<double> biquad::ExportCoeffs(Type type,double dbGain, double centreFreq, double fs, double dBandwidthOrQOrS, bool isBandwidthOrS)
{
    if (centreFreq <= 2.2204460492503131e-016 || fs <= 2.2204460492503131e-016){
        std::list<double> nulllist;
        return nulllist;
    }

    double d;
    if (type == PEAKING || type == LOW_SHELF || type == HIGH_SHELF)
        d = pow(10.0, dbGain / 40.0);
    else
        d = pow(10.0, dbGain / 20.0);
    double a = (6.2831853071795862 * centreFreq) / fs; //omega
    double num3 = sin(a); //sn
    double cs = cos(a);

    double alpha;
    if (!isBandwidthOrS) // Q
        alpha = num3 / (2 * dBandwidthOrQOrS);
    else if (type == LOW_SHELF || type == HIGH_SHELF) // S
        alpha = num3 / 2 * sqrt((d + 1 / d) * (1 / dBandwidthOrQOrS - 1) + 2);
    else // BW
        alpha = num3 * sinh(0.693147180559945309417 / 2 * dBandwidthOrQOrS * a / num3);

    double beta = 2 * sqrt(d) * alpha;
    double B0, B1, B2, A0, A1, A2;

    switch (type)
    {
    case LOW_PASS:
        B0 = (1.0 - cs) / 2.0;
        B1 = 1.0 - cs;
        B2 = (1.0 - cs) / 2.0;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case HIGH_PASS:
        B0 = (1.0 + cs) / 2.0;
        B1 = -(1.0 + cs);
        B2 = (1.0 + cs) / 2.0;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case BAND_PASS:
        B0 = alpha;
        B1 = 0.0;
        B2 = -alpha;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case NOTCH:
        B0 = 1.0;
        B1 = -2.0 * cs;
        B2 = 1.0;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case ALL_PASS:
        B0 = 1.0 - alpha;
        B1 = -2.0 * cs;
        B2 = 1.0 + alpha;
        A0 = 1.0 + alpha;
        A1 = -2.0 * cs;
        A2 = 1.0 - alpha;
        break;
    case PEAKING:
        B0 = 1.0 + (alpha * d);
        B1 = -2.0 * cs;
        B2 = 1.0 - (alpha * d);
        A0 = 1.0 + (alpha / d);
        A1 = -2.0 * cs;
        A2 = 1.0 - (alpha / d);
        break;
    case LOW_SHELF:
        B0 = d * ((d + 1.0) - (d - 1.0) * cs + beta);
        B1 = 2.0 * d * ((d - 1.0) - (d + 1.0) * cs);
        B2 = d * ((d + 1.0) - (d - 1.0) * cs - beta);
        A0 = (d + 1.0) + (d - 1.0) * cs + beta;
        A1 = -2.0 * ((d - 1.0) + (d + 1.0) * cs);
        A2 = (d + 1.0) + (d - 1.0) * cs - beta;
        break;
    case HIGH_SHELF:
        B0 = d * ((d + 1.0) + (d - 1.0) * cs + beta);
        B1 = -2.0 * d * ((d - 1.0) + (d + 1.0) * cs);
        B2 = d * ((d + 1.0) + (d - 1.0) * cs - beta);
        A0 = (d + 1.0) - (d - 1.0) * cs + beta;
        A1 = 2.0 * ((d - 1.0) - (d + 1.0) * cs);
        A2 = (d + 1.0) - (d - 1.0) * cs - beta;
        break;
    case UNITY_GAIN:
        B0 = d;
        B1 = 0.0;
        B2 = 0.0;
        A0 = 1.0;
        A1 = 0.0;
        A2 = 0.0;
        break;
    }   

    std::list<double> result;
    result.push_back(B0 / A0);
    result.push_back(B1 / A0);
    result.push_back(B2 / A0);
    result.push_back(-A1 / A0);
    result.push_back(-A2 / A0);
    return result;
}
std::list<double> biquad::ExportCoeffs(double dSamplingRate)
{
    return ExportCoeffs(m_dFilterType,m_dFilterGain,m_dFilterFreq,dSamplingRate,m_dFilterBQ,m_isBandwidthOrS);
}
bool biquad::IsStable() const{
    //Check if filter is stable/usable
    return m_isStable;
}
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
double biquad::GainAt(double centreFreq, double fs)
{
    double Arg;
    double z1Real, z1Imag, z2Real, z2Imag, HofZReal, HofZImag, DenomReal, DenomImag, tmpReal, tmpImag;
    Arg = M_PI * centreFreq / (fs * 0.5);
    z1Real = cos(Arg), z1Imag = -sin(Arg);  // z = e^(j*omega)
    complexMultiplicationRI(&z2Real, &z2Imag, z1Real, z1Imag, z1Real, z1Imag); // z squared
    HofZReal = 1.0, HofZImag = 0.0;
    tmpReal = a0 + internalBiquadCoeffs[0] * z1Real + internalBiquadCoeffs[1] * z2Real;
    tmpImag = internalBiquadCoeffs[0] * z1Imag + internalBiquadCoeffs[1] * z2Imag;
    complexMultiplicationRI(&HofZReal, &HofZImag, HofZReal, HofZImag, tmpReal, tmpImag);
    DenomReal = 1.0 + -internalBiquadCoeffs[2] * z1Real + -internalBiquadCoeffs[3] * z2Real;
    DenomImag = -internalBiquadCoeffs[2] * z1Imag + -internalBiquadCoeffs[3] * z2Imag;
    double magnitude;
    if (sqrt(DenomReal * DenomReal + DenomImag * DenomImag) < DBL_EPSILON)
        magnitude = 0.0;
    else
    {
        complexDivisionRI(&HofZReal, &HofZImag, HofZReal, HofZImag, DenomReal, DenomImag);
        magnitude = 20.0 * log10(sqrt(HofZReal * HofZReal + HofZImag * HofZImag + DBL_EPSILON));
    }
    return magnitude;
}
// Simplified Shpak group delay algorithm
// The algorithm only valid when first order / second order IIR filters is provided
// You must break down high order transfer function into N-SOS in order apply the Shpak algorithm
// which is out-of-scope here, since break down high order transfer function require find roots of polynomials
// Root finder may often require the computation of eigenvalue of companion matrix of polynomials
// Which will bloat 6000+ lines of code, and perhaps not the main purpose here.
// We need to calculate group delay of a bunch of second order IIR filters, so the following code already do the job
// Provided by: James34602
double biquad::GroupDelayAt(double centreFreq, double fs)
{
	double Arg = M_PI * centreFreq / (fs * 0.5);
	double cw = cos(Arg);
	double cw2 = cos(2.0 * Arg);
	double sw = sin(Arg);
	double sw2 = sin(2.0 * Arg);
	double b1 = a0, b2 = internalBiquadCoeffs[0], b3 = internalBiquadCoeffs[1];
	double u = b1 * sw2 + b2 * sw;
	double v = b1 * cw2 + b2 * cw + b3;
	double du = 2.0 * b1*cw2 + b2 * cw;
	double dv = -(2.0 * b1*sw2 + b2 * sw);
	double u2v2 = (b1*b1) + (b2*b2) + (b3*b3) + 2.0 * (b1*b2 + b2 * b3)*cw + 2.0 * (b1*b3)*cw2;
	double gdB = (2.0 - (v*du - u * dv) / u2v2);
	b2 = -internalBiquadCoeffs[2], b3 = -internalBiquadCoeffs[3];
	u = sw2 + b2 * sw;
	v = cw2 + b2 * cw + b3;
	du = 2.0 * cw2 + b2 * cw;
	dv = -(2.0 * sw2 + b2 * sw);
	u2v2 = 1.0 + (b2*b2) + (b3*b3) + 2.0 * (b2 + b2 * b3)*cw + 2.0 * b3*cw2;
	return gdB - (2.0 - (v*du - u * dv) / u2v2);
}
