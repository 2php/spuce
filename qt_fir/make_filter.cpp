// Copyright (c) 2015 Tony Kirke. License MIT  (http://www.opensource.org/licenses/mit-license.php)
#include "make_filter.h"
#include <cmath>
#include <complex>
#include <spuce/filters/design_window.h>
#include <spuce/filters/design_fir.h>
namespace spuce {

void fir_freq(std::vector<double>& MF, int pts, double* w, double inc) {
  double t;
  double w_inc = inc*M_PI/(float)pts; 
  std::complex<double> z_inc, nom;
	
  for (int i=0;i<pts;i++) {
    double wf = w_inc*i;
    std::complex<double> z(1,0);
		z_inc = std::complex<double>(cos(wf),sin(wf));
		nom = 0;
		for (int j=0;j<MF.size();j++) {
			nom += z*(std::complex<double>(MF[j]));
			z *= z_inc;
		}
		t = sqrt(norm(nom));
    if (t==0) t = 0.00001;
    t = 20.0*log(t)/log(10.0);
    w[i] = t;
  }
}
	
void make_filter::sel_filter(const char *c_sel) {
  std::string sel(c_sel);
  if (sel == "Hamming")    change_filter(Hamming);
  else if (sel == "Hanning")    change_filter(Hanning);
  else if (sel == "Blackman")    change_filter(Blackman);
  else if (sel == "Bartlett")    change_filter(Bartlett);
  else if (sel == "Kaiser")    change_filter(Kaiser);
  else if (sel == "Maxflat FIR")    change_filter(MaxflatFIR);
  else if (sel == "Gaussian FIR")    change_filter(GaussianFIR);
  else if (sel == "Remez FIR")    change_filter(RemezFIR);
  else if (sel == "Raised Cosine")    change_filter(RaisedCosine);
  else if (sel == "Root Raised Cosine")    change_filter(RootRaisedCosine);
  else if (sel == "None")    change_filter(None);
  else
    std::cout << "Invalid filter selection\n";
}

void make_filter::change_filter(fil_enum f) {
  last_shape = shape;
  shape = f;
}

void make_filter::set_filter_type(int t) {
	pass_type = low;
}

void make_filter::init(int points) { pts = points; }

make_filter::~make_filter() {}
make_filter::make_filter() {
  reset();
}

void make_filter::reset() {
  pass_type = low;

  maxflat_fc = 0.16;
  gauss_fc = 0.06;
  rc_fc = rrc_fc = 0.5;

  remez_pass_edge = 0.4;
  remez_trans = 0.2;
  remez_stop_edge = remez_pass_edge + remez_trans;
  remez_stop_weight = 50;

  rc_alpha = rrc_alpha = 0.25;

	hamming_taps = 23;
	hanning_taps = 23;
	bartlett_taps =23 ;
	blackman_taps =23;
	kaiser_taps=23;
	kaiser_tw = 0.1;
	kaiser_ripple = 0.04;
	
  gauss_taps = 21;
  remez_taps = 33;
  maxflat_taps = 45;
  rc_taps = rrc_taps = 33;
  shape = RemezFIR;
  last_shape = shape;
}

double make_filter::limit(double x, double mx, double mn) {
  if (x > mx)
    x = mx;
  else if (x < mn)
    x = mn;
  return x;
}

int make_filter::get_order() {
  switch (shape) {
	case Hamming:    return (hamming_taps);      break;
	case Hanning:    return (hanning_taps);      break;
	case Blackman:    return (blackman_taps);      break;
	case Bartlett:    return (bartlett_taps);      break;
	case MaxflatFIR:    return (maxflat_taps);      break;
	case GaussianFIR:   return (gauss_taps);      break;
	case RemezFIR:      return (remez_taps);      break;
	case RaisedCosine:  return (rc_taps);      break;
	case RootRaisedCosine:      return (rrc_taps);      break;
	case Kaiser:
	case None:      return (0);
  }
  return (0);
}
	bool make_filter::is_bpf() { return (false);}

double make_filter::horiz_swipe(int len, bool in_passband) {
  // Convert swipe to dB inc/decrease
  const double min_fc = 0.0;
  double gain = pow(2, 0.002 * len);

  switch (shape) {
	case MaxflatFIR:
		maxflat_fc = limit(gain * maxflat_fc, 0.4, 0.001);
		break;
	case GaussianFIR:
		gauss_fc = limit(gain * gauss_fc, 0.4, min_fc);
		break;
	case RemezFIR:
		if (in_passband) {
			remez_pass_edge = limit(gain * remez_pass_edge, 0.95 - remez_trans, 0.001);
		} else {
			remez_trans = limit(gain * remez_trans, 0.95 - remez_pass_edge, 0.001);
		}
		remez_stop_edge = remez_pass_edge + remez_trans;
		break;
	case RaisedCosine:
		if (in_passband) {
			rc_fc = limit(gain * rc_fc, 0.5, 0.001);
		} else {
			rc_alpha = limit(gain * rc_alpha, 1, 0.01);
		}
		break;
	case RootRaisedCosine:
		rrc_alpha = limit(gain * rrc_alpha, 1, 0.01);
		break;
	case Kaiser:
		kaiser_tw = limit(gain * kaiser_tw, 0.95, 0.001);
		break;
	case Hanning:
	case Hamming:
	case Blackman:
	case Bartlett:
	case None:
      break;
  }
  return (0.0);
}
void make_filter::vertical_swipe(int len, bool in_passband, bool above_stop) {
  const int MAX_FIR = 99;
  const int MIN_FIR = 15;
  int inc;

  if (len < 0)
    inc = 1;
  else
    inc = -1;

  // Convert swipe to dB inc/decrease
  double gain = pow(2, 0.002 * len);
  double ogain = 1.0 / gain;

  switch (shape) {
	case MaxflatFIR:
		maxflat_taps = limit(maxflat_taps + 8 * inc, MAX_FIR, MIN_FIR);
		break;
	case GaussianFIR:
		gauss_taps = limit(gauss_taps + inc, MAX_FIR, MIN_FIR);
		break;
    // FIX ME - should change to passband ripple while in passband
	case RemezFIR:
		if (in_passband) {
			remez_stop_weight = limit(ogain * remez_stop_weight, 100, 0.01);
		} else {
			if (above_stop) {
				remez_stop_weight = limit(ogain * remez_stop_weight, 100, 0.01);
			} else {
				remez_taps = limit(remez_taps + inc, MAX_FIR, MIN_FIR);
			}
		}
		break;
	case RootRaisedCosine:
		rrc_taps = limit(rrc_taps + 2 * inc, MAX_FIR, MIN_FIR);
		break;
	case RaisedCosine:
		rc_taps = limit(rc_taps + 2 * inc, MAX_FIR, MIN_FIR);
		break;
	case Kaiser:
		kaiser_ripple = limit(gain * kaiser_ripple, 0.1, 0.01);
		break;
	default:
		break;
  }
}
double make_filter::update(double *w) {
  switch (shape) {
	case RemezFIR:         taps = design_fir("remez", remez_taps, remez_pass_edge, remez_stop_edge, remez_stop_weight);		break;
	case MaxflatFIR:       taps = design_fir("butterworth", maxflat_taps, 0, maxflat_fc, 0);		break;
	case GaussianFIR:      taps = design_fir("gaussian", gauss_taps, 0, 0.4, gauss_fc);		break;
	case RaisedCosine:     taps = design_fir("rootraisedcosine", rc_taps, rc_alpha, 1.0/rc_fc, 0);		break;
	case RootRaisedCosine: taps = design_fir("rootraisedcosine", rrc_taps, rrc_alpha, 2, 0); break;
	//
	case Hanning:		       taps = design_window("hanning", hanning_taps); break;
	case Hamming:		       taps = design_window("hamming", hamming_taps); break;
	case Blackman:	       taps = design_window("blackman", blackman_taps); break;
	case Bartlett:	       taps = design_window("bartlett", bartlett_taps); break;
	case Kaiser:		       taps = design_window("kaiser", kaiser_taps, kaiser_tw, kaiser_ripple); break;
	case None:			for (int i = 0; i < pts; i++) w[i] = 1.0;			break;
	}
	fir_freq(taps, pts, w, 1.0);
  return (0);
}
double make_filter::get_mag(double w) {
	std::complex<double> z_inc,z;
	z_inc = std::complex<double>(cos(w),sin(w));
	std::complex<double> nom = 0;
	for (size_t j=0;j<taps.size();j++) {
		nom += taps[j]*z;
		z *= z_inc;
	}
	double t = sqrt(std::norm(nom));
	if (t==0) t = 0.00001;
	t = 20.0*log(t)/log(10.0);
  return t;
}



}  // namespace spuce