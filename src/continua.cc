/**
   \file   continua.cc

   The following continua parameterizations are implemented:
   1) H2O-H2O: P. W. Rosenkranz, Radio Science, Vol. 33, No 4, Pages 919-928, 1998.
   2) H2O-air: P. W. Rosenkranz, Radio Science, Vol. 33, No 4, Pages 919-928, 1998.
                            and  Radio Science, Vol. 34, No 4, Page  1025,    1999.
   3) O2-air : P. W. Rosenkranz Chapter 2, pp 74, in M. A. Janssen, 
                                "Atmospheric Remote Sensing by Microwave Radiometry",
                                John Wiley & Sons, Inc., 1993
   4) N2-N2  : P. W. Rosenkranz Chapter 2, pp 74, in M. A. Janssen, 
                                "Atmospheric Remote Sensing by Microwave Radiometry",
                                John Wiley & Sons, Inc., 1993
   5) CO2-CO2: P. W. Rosenkranz Chapter 2, pp 74, in M. A. Janssen, 
                                "Atmospheric Remote Sensing by Microwave Radiometry",
                                John Wiley & Sons, Inc., 1993
   6) CO2-N2 : P. W. Rosenkranz Chapter 2, pp 74, in M. A. Janssen, 
                                "Atmospheric Remote Sensing by Microwave Radiometry",
                                John Wiley & Sons, Inc., 1993

   A) Suspended water droplet absorption parameterization from MPM93 model
      H. J. Liebe and G. A. Hufford and M. G. Cotton,
      "Propagation modeling of moist air and suspended water/ice
      particles at frequencies below 1000 GHz",
      AGARD 52nd Specialists Meeting of the Electromagnetic Wave
      Propagation Panel, Palma de Mallorca, Spain, 1993, May 17-21 
      
   B) Ice crystal absorption parameterization from MPM93 model
      H. J. Liebe and G. A. Hufford and M. G. Cotton,
      "Propagation modeling of moist air and suspended water/ice
      particles at frequencies below 1000 GHz",
      AGARD 52nd Specialists Meeting of the Electromagnetic Wave
      Propagation Panel, Palma de Mallorca, Spain, 1993, May 17-21 


   The following unit conversions are used:
   (SI units: meter, kilogram, second, ampere, Kelvin, candela) 
   x GHz   = y Hz       <===>      y = x * 1.00e9
   x 1/GHz = y 1/Hz     <===>      y = x * 1.00e-9
   x hPa   = y Pa       <===>      y = x * 1.00e2
   x 1/hPa = y 1/Pa     <===>      y = x * 1.00e-2
   x 1/cm  = y 1/m      <===>      y = x * 1.0e2
   x 1/km  = y 1/m      <===>      y = x * 1.00e-3
   x Np    = y 1        <===>      y = x
   x dB    = y Np       <===>      y = x / (10.0 * log10(e))
   x dB/km = y 1/m      <===>      y = x * 1.00e-3 / (10.0 * log10(e))
   x g/cm3 = y kg/m3    <===>      y = x * 1.00e3
   x g/m3  = y kg/m3    <===>      y = x * 1.00e-3

   \author Thomas Kuhn
   \date   2001-03-27
*/

#include "vecmat.h"
#include "absorption.h"
#include "messages.h"

/**
   Calculates the continuum according to the simple
   empirical function as formulated by Rosenkranz98:

   self broadened continuum:
   xsec += C * (300/t_abs)^(x+3) * f_mono^2 * (300/t_abs)^3 * p_abs^2 * vmr

   foreign broadened continuum:
   pdry = p_abs * (1-vmr)
   xsec += C * (300/t_abs)^(x+3) * f_mono^2 * (300/t_abs)^3 * p_abs * pdry

   See the list of parameters below for the meaning of these
   variables. The equation is slightly modified from Rosenkranz's
   original, because we want to return only the cross section, not the
   absorption coefficient. (Only vmr, not vmr^2 at the end of the
   equation.) 

   This xsec is understood to parameterize the difference between
   observed absorption cross section and the one calculated from the
   pure line spectrum. The continuum is added to the previous content
   of xsec!

   \retval xsec  Absorption cross section, defined such that the
                 absorption coefficient alpha is:<br>
                 alpaha [1/m] = xsec * VMR.<br>
		 The functions adds to xsec, rather than replacing the
		 previous content. 

   \param  C       Continuum coefficient.
   \param  x       Temperature exponenet.
   \param  f_mono  Frequency grid.
   \param  p_abs   Pressure grid.
   \param  t_abs   Temperatures associated with p_abs.
   \param  vmr     Volume mixing ratio of the calculated species (H2O).

   References:
   
   P.W. Rosenkranz, `Water vapor microwave continuum absorption: A
   comparison of measurements and models', Radio Science, Vol. 33, No
   4, Pages 919-928, July-August 1998.


   \author Stefan Buehler
   \date   2001-01-17
*/
//
// #################################################################################
//
// 1) H2O-H2O: P. W. Rosenkranz, Radio Science, Vol. 33, No 4, Pages 919-928, 1998.
void Rosenkranz_h2o_self_continuum( MATRIX&           xsec,
				    Numeric	      C,
				    Numeric	      x,
				    const VECTOR&     f_mono,
				    const VECTOR&     p_abs,
				    const VECTOR&     t_abs,
				    const VECTOR&     vmr	 )
{
  const size_t n_p = p_abs.size();	// Number of pressure levels
  const size_t n_f = f_mono.size();	// Number of frequencies

  // Check that dimensions of p_abs, t_abs, and vmr agree:
  assert ( n_p==t_abs.size() );
  assert ( n_p==vmr.size()   );

  // Check that dimensions of xsec are consistent with n_f
  // and n_p. It should be [n_f,n_p]:
  assert ( n_f==xsec.nrows() );
  assert ( n_p==xsec.ncols() );
  
  // Loop over pressure/temperature grid:
  for ( size_t i=0; i<n_p; ++i )
    {
      // Dummy scalar holds everything except the quadratic frequency dependence.
      // The second vmr of H2O will be multiplied at the stage of absorption 
      // calculation: abs = vmr * xsec.
      Numeric dummy =
	C * pow( 300./t_abs[i], x+3. ) * pow( p_abs[i], 2 ) * vmr[i];

      // Loop over frequency grid:
      for ( size_t s=0; s<n_f; ++s )
	{
	  xsec[s][i] += dummy * pow( f_mono[s], 2 );
	  //	  cout << "xsec[" << s << "][" << i << "]: " << xsec[s][i] << "\n";
	}
    }
}
//
// #################################################################################
//
//   2) H2O-air: P. W. Rosenkranz, Radio Science, Vol. 33, No 4, Pages 919-928, 1998.
//                            and  Radio Science, Vol. 34, No 4, Page  1025,    1999.
void Rosenkranz_h2o_foreign_continuum( MATRIX&           xsec,
				       Numeric	         C,
				       Numeric	         x,
				       const VECTOR&     f_mono,
				       const VECTOR&     p_abs,
				       const VECTOR&     t_abs,
				       const VECTOR&     vmr	 )
{
  const size_t n_p = p_abs.size();	// Number of pressure levels
  const size_t n_f = f_mono.size();	// Number of frequencies

  // Check that dimensions of p_abs, t_abs, and vmr agree:
  assert ( n_p==t_abs.size() );
  assert ( n_p==vmr.size()   );

  // Check that dimensions of xsec are consistent with n_f
  // and n_p. It should be [n_f,n_p]:
  assert ( n_f==xsec.nrows() );
  assert ( n_p==xsec.ncols() );
  
  // Loop pressure/temperature:
  for ( size_t i=0; i<n_p; ++i )
    {
      // Dry air partial pressure: p_dry := p_tot - p_h2o.
      Numeric pdry  = p_abs[i] * (1.000e0-vmr[i]);
      // Dummy scalar holds everything except the quadratic frequency dependence.
      // The vmr of H2O will be multiplied at the stage of absorption 
      // calculation: abs = vmr * xsec.
      Numeric dummy = C * pow( 300./t_abs[i], x+3. ) * p_abs[i] * pdry;

      // Loop frequency:
      for ( size_t s=0; s<n_f; ++s )
	{
	  xsec[s][i] += dummy * pow( f_mono[s], 2 );
	  //	  cout << "xsec[" << s << "][" << i << "]: " << xsec[s][i] << "\n";
	}
    }
}
//
// #################################################################################
//
// 2b) H2O-air: Coefficients are from Liebe et al., AGARD CP-May93, Paper 3/1-10
//
Numeric MPM93ContinuumPseudoLineShapeFunction( Numeric gamma, 
					       Numeric fl, 
					       Numeric f)
{
  /*
    this routine calculates the line shape function of Van Vleck and Weisskopf
    with the factor (f/f_o)^1. for the MPM pseudo continuum line.

    creation  TKS, 4.11.00 

    input:   gamma   [Hz]    line width of line L
             fl      [Hz]    central frequency of line L
             f       [Hz]    frequency position of calculation
             
    output:  value   [1/Hz]  line shape function value at f for the line parameters
                              of line L 
	      
   */

  double f_minus, f_plus ;           /* internal variables */
  double value;                      /* return value       */

  // line at fl
  f_minus = 1.000 / ((f-fl)*(f-fl) + gamma*gamma);

  // mirror line at -fl
  f_plus  = 1.000 / ((f+fl)*(f+fl) + gamma*gamma);

  // VVW line shape function value
  value = fabs(f/fl) * gamma * (f_minus + f_plus);
  
  return value;
}
//
// MPM93 H2O pseudo continuum line parameters:
// see publication side of National Telecommunications and Information Administration
//   http://www.its.bldrdoc.gov/pub/all_pubs/all_pubs.html
// and ftp side for downloading the MPM93 original source code:
//   ftp://ftp.its.bldrdoc.gov/pub/mpm93/
void MPM93_h2o_continuum( MATRIX&           xsec,
			  Numeric	    MPM93fopcl, // default: 1780.0*10^9 Hz
			  Numeric	    MPM93b1pcl, // default: 22300.0 Hz/Pa
			  Numeric	    MPM93b2pcl, // default: 0.952
			  Numeric	    MPM93b3pcl, // default: 17.6*10^4 Hz/Pa
			  Numeric	    MPM93b4pcl, // default: 30.5
			  Numeric	    MPM93b5pcl, // default: 2
			  Numeric	    MPM93b6pcl, // default: 5
			  const VECTOR&     f_mono,
			  const VECTOR&     p_abs,
			  const VECTOR&     t_abs,
			  const VECTOR&     vmr	 )
{
  const size_t n_p = p_abs.size();	// Number of pressure levels
  const size_t n_f = f_mono.size();	// Number of frequencies

  // Check that dimensions of p_abs, t_abs, and vmr agree:
  assert ( n_p==t_abs.size() );
  assert ( n_p==vmr.size()   );

  // Check that dimensions of xsec are consistent with n_f
  // and n_p. It should be [n_f,n_p]:
  assert ( n_f==xsec.nrows() );
  assert ( n_p==xsec.ncols() );
  

  // Loop pressure/temperature:
  for ( size_t i=0; i<n_p; ++i )
    {
      Numeric th = 300.0 / t_abs[i];
      // the vmr of H2O will be multiplied at the stage of absorption calculation:
      // abs / vmr * xsec.
      Numeric strength =  MPM93b1pcl * p_abs[i] * pow( th, 3.5 ) * exp(MPM93b2pcl * (1 - th));
      Numeric gam =  MPM93b3pcl * 0.001 * 
	           ( MPM93b4pcl * p_abs[i] * vmr[i]       * pow( th, MPM93b6pcl ) +  
	                          p_abs[i]*(1.000-vmr[i]) * pow( th, MPM93b5pcl ) );
      // Loop frequency:
      for ( size_t s=0; s<n_f; ++s )
	{
	  xsec[s][i] += 0.182 * 0.001 / (10.000*log10(2.718281828))  * 
	                f_mono[s] * strength * 
	                MPM93ContinuumPseudoLineShapeFunction(gam, MPM93fopcl, f_mono[s]); 
	  //	  cout << "xsec[" << s << "][" << i << "]: " << xsec[s][i] << "\n";
	}
    }
}
//
// #################################################################################
//
//   3) O2-air : P. W. Rosenkranz Chapter 2, pp 74, in M. A. Janssen, 
//               "Atmospheric Remote Sensing by Microwave Radiometry",
//               John Wiley & Sons, Inc., 1993. Also stated in 
//               Liebe et al. JQSRT, Vol 48, Nr 5/6, pp. 629-643, 1992.
//               Default continuum parameters are  C=1.6E-17*10E-9,  x=0.8
void Rosenkranz_o2_continuum( MATRIX&           xsec,
			      Numeric		C, // default: 1.108*10^-14 K^2/(Hz*Pa*m)
			      Numeric		x, // default: 0.8
			      const VECTOR&  	f_mono,
			      const VECTOR&  	p_abs,
			      const VECTOR&  	t_abs,
			      const VECTOR&     h2o_abs,
			      const VECTOR&     vmr	 )
{
  const size_t n_p = p_abs.size();	// Number of pressure levels
  const size_t n_f = f_mono.size();	// Number of frequencies

  // Check that dimensions of p_abs, t_abs, and vmr agree:
  assert ( n_p==t_abs.size() );
  assert ( n_p==vmr.size()   );

  // Check that dimensions of xsec are consistent with n_f
  // and n_p. It should be [n_f,n_p]:
  assert ( n_f==xsec.nrows() );
  assert ( n_p==xsec.ncols() );

  // loop over all pressure levels:
  for ( size_t i=0; i<n_p; ++i )
    {
      Numeric TH = 300.00 / t_abs[i];        // relative temperature  [1]
      
      Numeric ph2o  = p_abs[i] * h2o_abs[i];  // water vapor partial pressure [Pa]
      Numeric pdry  = p_abs[i] - ph2o;       // dry air partial pressure     [Pa]
      // pseudo broadening term [Hz]
      Numeric gamma = 5600.000 * (pdry * pow( TH, x ) + 1.100 * ph2o * TH); 

      // Loop over frequency grid:
      for ( size_t s=0; s<n_f; ++s )
	{
	  // division by vmr of O2 is necessary because of the absorption calculation
          // abs = vmr * xsec.
	  xsec[s][i] += C * p_abs[i] * gamma * pow( f_mono[s], 2 ) / 
                          ( pow( t_abs[i], 2 ) * ( pow( f_mono[s], 2 ) + pow( gamma, 2 ) ) );
	  //	  cout << "xsec[" << s << "][" << i << "]: " << xsec[s][i] << "\n";
	}
    }
}
//
// #################################################################################
//
// 4) N2-N2  : P. W. Rosenkranz Chapter 2, pp 74, in M. A. Janssen, 
//    "Atmospheric Remote Sensing by Microwave Radiometry", John Wiley & Sons, Inc., 1993
//
void Rosenkranz_n2_self_continuum( MATRIX&           xsec,
				   Numeric	     C, // default: 1.05*10^-38 1/(Pa^2*Hz^2*m)
				   Numeric	     x, // default: 3.55
				   const VECTOR&     f_mono,
				   const VECTOR&     p_abs,
				   const VECTOR&     t_abs,
				   const VECTOR&     vmr	 )
{
  const size_t n_p = p_abs.size();	// Number of pressure levels
  const size_t n_f = f_mono.size();	// Number of frequencies

  // Check that dimensions of p_abs, t_abs, and vmr agree:
  assert ( n_p==t_abs.size() );
  assert ( n_p==vmr.size()   );

  // Check that dimensions of xsec are consistent with n_f
  // and n_p. It should be [n_f,n_p]:
  assert ( n_f==xsec.nrows() );
  assert ( n_p==xsec.ncols() );
  
  // Loop over pressure/temperature grid:
  for ( size_t i=0; i<n_p; ++i )
    {
      // Dummy scalar holds everything except the quadratic frequency dependence.
      // The second vmr of N2 will be multiplied at the stage of absorption 
      // calculation: abs = vmr * xsec.
      Numeric dummy =
	C * pow( 300./t_abs[i], x ) * pow( p_abs[i], 2 ) * vmr[i];

      // Loop over frequency grid:
      for ( size_t s=0; s<n_f; ++s )
	{
	  xsec[s][i] += dummy * pow( f_mono[s], 2 );
	  //	  cout << "xsec[" << s << "][" << i << "]: " << xsec[s][i] << "\n";
	}
    }
}
//
// #################################################################################
//
// 5) CO2-CO2: P. W. Rosenkranz Chapter 2, pp 74, in M. A. Janssen, 
// "Atmospheric Remote Sensing by Microwave Radiometry", John Wiley & Sons, Inc., 1993
//
void Rosenkranz_co2_self_continuum( MATRIX&           xsec,
				    Numeric	      C, // default: 7.43*10^-37 1/(Pa^2*Hz^2*m)
				    Numeric	      x, // default: 5.08
				    const VECTOR&     f_mono,
				    const VECTOR&     p_abs,
				    const VECTOR&     t_abs,
				    const VECTOR&     vmr	 )
{
  const size_t n_p = p_abs.size();	// Number of pressure levels
  const size_t n_f = f_mono.size();	// Number of frequencies

  // Check that dimensions of p_abs, t_abs, and vmr agree:
  assert ( n_p==t_abs.size() );
  assert ( n_p==vmr.size()   );

  // Check that dimensions of xsec are consistent with n_f
  // and n_p. It should be [n_f,n_p]:
  assert ( n_f==xsec.nrows() );
  assert ( n_p==xsec.ncols() );
  
  // Loop over pressure/temperature grid:
  for ( size_t i=0; i<n_p; ++i )
    {
      // Dummy scalar holds everything except the quadratic frequency dependence.
      // The second vmr of CO2 will be multiplied at the stage of absorption 
      // calculation: abs = vmr * xsec.
      Numeric dummy =
	C * pow( 300./t_abs[i], x ) * pow( p_abs[i], 2 ) * vmr[i];

      // Loop over frequency grid:
      for ( size_t s=0; s<n_f; ++s )
	{
	  xsec[s][i] += dummy * pow( f_mono[s], 2 );
	  //	  cout << "xsec[" << s << "][" << i << "]: " << xsec[s][i] << "\n";
	}
    }
}
//
// #################################################################################
//
// 6) CO2-N2 : P. W. Rosenkranz Chapter 2, pp 74, in M. A. Janssen, 
//    "Atmospheric Remote Sensing by Microwave Radiometry", John Wiley & Sons, Inc., 1993
//
void Rosenkranz_co2_foreign_continuum( MATRIX&           xsec,
				       Numeric	         C, // default: 2.71*10^-37 1/(Pa^2*Hz^2*m)
				       Numeric	         x, // default: 4.7
				       const VECTOR&     f_mono,
				       const VECTOR&     p_abs,
				       const VECTOR&     t_abs,
				       const VECTOR&     n2_abs,
				       const VECTOR&     vmr	 )
{
  const size_t n_p = p_abs.size();	// Number of pressure levels
  const size_t n_f = f_mono.size();	// Number of frequencies

  // Check that dimensions of p_abs, t_abs, and vmr agree:
  assert ( n_p==t_abs.size() );
  assert ( n_p==vmr.size()   );

  // Check that dimensions of xsec are consistent with n_f
  // and n_p. It should be [n_f,n_p]:
  assert ( n_f==xsec.nrows() );
  assert ( n_p==xsec.ncols() );
  
  // Loop pressure/temperature:
  for ( size_t i=0; i<n_p; ++i )
    {
      // Dummy scalar holds everything except the quadratic frequency dependence.
      // The vmr of CO2 will be multiplied at the stage of absorption 
      // calculation: abs = vmr * xsec.
      Numeric dummy = C * pow( 300./t_abs[i], x ) * p_abs[i] * p_abs[i] * n2_abs[i];

      // Loop frequency:
      for ( size_t s=0; s<n_f; ++s )
	{
	  xsec[s][i] += dummy * pow( f_mono[s], 2 );
	  //	  cout << "xsec[" << s << "][" << i << "]: " << xsec[s][i] << "\n";
	}
    }
}
//
// #################################################################################
//
// saturation water vapor pressure over liquid water,
// calculated according to Goff and Gratch formula.
// The saturation water vapor pressure is in units of Pa
Numeric WVSatPressureLiquidWater(Numeric t)
{
  Numeric theta  = 373.16 / (t + 273.16);
  Numeric es     = 100.000 * 
                   ( -7.90298 * (theta-1.000) +
		     5.02808 * log10(theta) -
		     1.3816e-7 * ( pow( 10.00, (11.344*(1.00-(1.00/theta))) ) - 1.000 ) +
		     8.1328e-3 * ( pow( 10.00, (-3.49149*(theta-1.00))) - 1.000) +
		     log10(1013.246) );
  return es;
}
//
// #################################################################################
//
// saturation water vapor pressure over ice,
// calculated according to Goff and Gratch formula.
// The saturation water vapor pressure is in units of Pa
Numeric WVSatPressureIce(Numeric t)
{
  Numeric theta  = 273.16 / (t + 273.16);
  Numeric es     = 100.000 * 
                  (-9.09718  * (theta-1.000) -
		   3.56654  * log10(theta)  +
		   0.876793 * (1.000-(1.000/theta)) +
		   log10(6.1071) );
  return es;
}
// #################################################################################
//
//   A) cloud and fog absorption parameterization from MPM93 model
//      input parameters:
//      w :  suspended water droplet density, valid range: 0-10.00e-3 kg/m3
//      m :  specific weight of the droplet,  fixed value:     1.00e3 kg/m3
//      The internal numerical values (and units) are the same as in MPM93
//
void MPM93WaterDropletAbs( MATRIX&           xsec,
			   Numeric	        w, // suspended water droplet density
			   Numeric	        m, // specific droplet weight 
			   const VECTOR&   f_mono,
			   const VECTOR&    p_abs,
			   const VECTOR&    t_abs,
			   const VECTOR&      vmr	 )
{
  const size_t n_p = p_abs.size();	// Number of pressure levels
  const size_t n_f = f_mono.size();	// Number of frequencies

  // Check that dimensions of p_abs, t_abs, and vmr agree:
  assert ( n_p==t_abs.size() );
  assert ( n_p==vmr.size()   );

  // Check that dimensions of xsec are consistent with n_f
  // and n_p. It should be [n_f,n_p]:
  assert ( n_f==xsec.nrows() );
  assert ( n_p==xsec.ncols() );
  
  // Check that suspended water droplet density and specific weight of the droplet
  // are in the correct limits
  if ((w < 0.00) || (w > 10.00e-3) || (fabs(m-1.000e3)> 0.100)) {
    ostringstream os;
    os << "MPM93WaterDropletAbs: \n"
       << "suspended liquid water particle density,       valid range: 0-10.00e-3 kg/m3,  w=" << w << "\n"
       << "specific weight of the liquid water particle,  fixed value:     1.00e3 kg/m3,  m=" << m << "\n"
       << ".";
    throw runtime_error(os.str());
    return;
  }

  // Loop pressure/temperature:
  for ( size_t i=0; i<n_p; ++i )
    {
      // water vapor saturation pressure over liquid water [Pa]
      // Numeric es       = WVSatPressureLiquidWater(t_abs[i]);
      // water vapor partial pressure [Pa]
      // Numeric e        = p_abs[i] * vmr[i];      
      // relative humidity [1]
      // Numeric RH       = e / es;
  
      // relative inverse temperature [1]
      Numeric theta    = 300.000 / t_abs[i];
      // relaxation frequencies [GHz]
      Numeric gamma1   = 20.20 - 146.40*(theta-1.000) + 316.00*(theta-1.000)*(theta-1.000);
      Numeric gamma2   = 39.80 * gamma1; 
      // static and high-frequency permittivities
      Numeric epsilon0 = 103.30 * (theta-1.000) + 77.66;
      Numeric epsilon1 = 0.0671 * epsilon0;
      Numeric epsilon2 = 3.52;

      // Loop frequency:
      for ( size_t s=0; s<n_f; ++s )
	{
	  // real part of the complex permittivity of water (double-debye model)
	  Numeric Reepsilon  = epsilon0 - 
	                       pow((f_mono[s]*1.000e-9),2) *
	                       ( ((epsilon0-epsilon1)/
				  (pow((f_mono[s]*1.000e-9),2) + pow(gamma1,2))) + 
                                 ((epsilon1-epsilon2)/
				  (pow((f_mono[s]*1.000e-9),2) + pow(gamma2,2))) );
	  // imaginary part of the complex permittivity of water (double-debye model)
	  Numeric Imepsilon  = (f_mono[s]*1.000e-9) *
	                       ( (gamma1*(epsilon0-epsilon1)/
				  (pow((f_mono[s]*1.000e-9),2) + pow(gamma1,2))) + 
                                 (gamma2*(epsilon1-epsilon2)/
				 (pow((f_mono[s]*1.000e-9),2) + pow(gamma2,2))) );
	  // the imaginary part of the complex refractivity of suspended liquid water particle [ppm]
	  // In MPM93 w is in g/m3 and m is in g/cm3. Because of the units used in arts,
          // a factor of 1.000e6 must be multiplied with the ratio (w/m):
          // MPM93: (w/m)_MPM93  in   (g/m3)/(g/cm3)
          // arts:  (w/m)_arts   in  (kg/m3)/(kg/m3)
          // ===> (w/m)_MPM93 = 1.0e6 * (w/m)_arts
          // the factor of 1.0e6 is included below in the constant 41.90705.
	  Numeric ImNw = 1.500 * (w/m) * 
 	                ( 3.000 * Imepsilon / ( pow((Reepsilon+2.000),2) + pow(Imepsilon,2) ) );
	  // liquid water particle absorption cross section [1/m]
	  // The vmr of H2O will be multiplied at the stage of absorption 
	  // calculation: abs = vmr * xsec.
          // 41.90705 = (0.182 * 0.001 / (10.000*log10(2.718281828))) * 1.000e6
	  xsec[s][i] += 41.90705 * (f_mono[s]*1.000e-9) * ImNw / vmr[i];
	}
    }

}
//
// #################################################################################
//
//   A) ice particle absorption parameterization from MPM93 model
//      input parameters:
//      w :  suspended ice particle density,    valid range: 0-10.0e-3 kg/m3
//      m :  specific weight of ice particles,  fixed value:   0.916e3 kg/m3
//      The internal numerical values (and units) are the same as in MPM93
//
void MPM93IceCrystalAbs( MATRIX&           xsec,
			 Numeric	      w, // suspended ice particle density
			 Numeric	      m, // specific ice particles weight 
			 const VECTOR&   f_mono,
			 const VECTOR&    p_abs,
			 const VECTOR&    t_abs,
			 const VECTOR&      vmr	 )
{
  const size_t n_p = p_abs.size();	// Number of pressure levels
  const size_t n_f = f_mono.size();	// Number of frequencies

  // Check that dimensions of p_abs, t_abs, and vmr agree:
  assert ( n_p==t_abs.size() );
  assert ( n_p==vmr.size()   );

  // Check that dimensions of xsec are consistent with n_f
  // and n_p. It should be [n_f,n_p]:
  assert ( n_f==xsec.nrows() );
  assert ( n_p==xsec.ncols() );
  
  // Check that suspended water droplet density and specific weight of the droplet
  // are in the correct limits
  if ((w < 0.00) || (w > 10.00e-3) || (fabs(m-0.916e3)> 0.100)) {
    ostringstream os;
    os << "MPM93IceCrystalAbs: \n"
       << "suspended ice particle density,       valid range: 0-10.0e-3 kg/m3,   w=" << w << "\n"
       << "specific weight of the ice particle,  fixed value:   0.916e3 kg/m3,   m=" << m << "\n"
       << ".";
    throw runtime_error(os.str());
    return;
  }


  // Loop pressure/temperature:
  for ( size_t i=0; i<n_p; ++i )
    {
      // water vapor saturation pressure over ice [Pa]
      // Numeric es = WVSatPressureIce(t_abs[i]);
      // water vapor partial pressure [Pa]
      // Numeric e  = p_abs[i] * vmr[i];
      // relative humidity [1]
      // Numeric RH = e / es;
  
      // relative inverse temperature [1]
      Numeric theta = 300.000 / t_abs[i];
      // inverse frequency T-dependency function [Hz]
      Numeric ai = (62.000 * theta - 11.600) * exp(-22.100 * (theta-1.000)) * 1.000e-4;
      // linear frequency T-dependency function [1/Hz]
      Numeric bi = 0.542e-6 * 
                   ( -24.17 + (116.79/theta) + pow((theta/(theta-0.9927)),2) );

      // Loop frequency:
      for ( size_t s=0; s<n_f; ++s )
	{
	  // real part of the complex permittivity of ice
	  Numeric Reepsilon  = 3.15;
	  // imaginary part of the complex permittivity of water
	  Numeric Imepsilon  = ( ( ai/(f_mono[s]*1.000e-9) ) +
				 ( bi*(f_mono[s]*1.000e-9) ) );
	  // the imaginary part of the complex refractivity of suspended ice particles.
	  // In MPM93 w is in g/m3 and m is in g/cm3. Because of the units used in arts,
          // a factor of 1.000e6 must be multiplied with the ratio (w/m):
          // MPM93: (w/m)_MPM93  in   (g/m3)/(g/cm3)
          // arts:  (w/m)_arts   in  (kg/m3)/(kg/m3)
          // ===> (w/m)_MPM93 = 1.0e6 * (w/m)_arts
          // the factor of 1.0e6 is included below in the constant 41.90705.
	  Numeric ImNw = 1.500 * (w/m) * 
 	                ( 3.000 * Imepsilon / ( pow((Reepsilon+2.000),2) + pow(Imepsilon,2) ) );
	  // ice particle absorption cross section [1/m]
	  // The vmr of H2O will be multiplied at the stage of absorption 
	  // calculation: abs = vmr * xsec.
          // 41.90705 = (0.182 * 0.001 / (10.000*log10(2.718281828))) * 1.000e6
	  xsec[s][i] += 41.90705 * (f_mono[s]*1.000e-9) * ImNw / vmr[i];
	}
    }

}
//
// #################################################################################
//
/** Calculates continuum absorption for one continuum tag. Note, that
    only one tag can be taken at a time. That means for water vapor
    you will have to call this function two times, once with the
    self-continuum tag and once with the foreign continuum tag.

    Calculated is the absorption cross section, that means you have to
    multiply this with the VMR in order to get the absorption
    coefficient:

    alpaha [1/m] = xsec * VMR

    \retval xsec       Cross section of one continuum tag.
    \param  name       The name of the continuum to calculate.
    \param  parameters Continuum model parameters, as defined in
                       cont_description_parameters. 
    \param  f_mono     Frequency grid.
    \param  p_abs      Pressure grid.
    \param  t_abs      Temperatures associated with p_abs.
    \param  vmrh2o     Total volume mixing ratio of water vapor. This
                       will be needed only for the oxygen continuum
    \param  vmrn2      Total volume mixing ratio of molecular nitrogen. This
                       will be needed only for the CO2 foreign continuum
    \param  vmr        Volume mixing ratio of the calculated species.

    \date   2001-01-16
    \author Stefan Buehler */
void xsec_continuum_tag( MATRIX&                    xsec,
			 const string&              name,
			 const VECTOR&              parameters,
			 const VECTOR&  	    f_mono,
			 const VECTOR&  	    p_abs,
			 const VECTOR&  	    t_abs,
			 const VECTOR&  	    n2_abs,
			 const VECTOR&  	    h2o_abs,
			 const VECTOR&              vmr )
{
  //
  // out3 << "  Continuum paramters are: " << parameters << "\n";
  //
  // A simple switch statement does not work here, because the
  // switching condition is not a simple value. So we need to use a
  // chain of if-else statements.
  //
  // ============= H2O continuum ================================================
  if ( "H2O-ContRosenkranzSelf"==name )
    {
      // Check if the right number of paramters has been specified:
      if ( 2 != parameters.size() )
	{
	  ostringstream os;
	  os << "Continuum model " << name << " requires two input\n"
	     << "parameters, but you specified " << parameters.size()
	     << ".";
	  throw runtime_error(os.str());
	  return;
	}
      
      // specific continuum parameters:
      // parameters[0] : continuum coefficient (C_s)
      // parameters[1] : temperature exponent  (x_s)
      //
      // units:
      //  a) output 
      //     xsec          : [1/m],
      //  b) input
      //     parameters[0] : [1/m / (Hz^2*Pa^2)]
      //     parameters[1] : [1]
      //     f_mono        : [Hz]
      //     p_abs         : [Pa]
      //     t_abs         : [K]
      //     vmr           : [1]
      //
      Rosenkranz_h2o_self_continuum( xsec,
				     parameters[0],
				     parameters[1],
				     f_mono,
				     p_abs,
				     t_abs,
				     vmr );
    }
  else if ( "H2O-ContRosenkranzForeign"==name ) // ------------------------------
    {
      // Check if the right number of paramters has been specified:
      if ( 2 != parameters.size() )
	{
	  ostringstream os;
	  os << "Continuum model " << name << " requires two input\n"
	     << "parameters, but you specified " << parameters.size()
	     << ".";
	  throw runtime_error(os.str());
	  return;
	}
      
      // specific continuum parameters:
      // parameters[0] : continuum coefficient (C_f)
      // parameters[1] : temperature exponent  (x_f)
      //
      // units:
      //  a) output 
      //     xsec          : [1/m],
      //  b) input
      //     parameters[0] : [1/m / (Hz^2*Pa^2)]
      //     parameters[1] : [1]
      //     f_mono        : [Hz]
      //     p_abs         : [Pa]
      //     t_abs         : [K]
      //     vmr           : [1]
      //
      Rosenkranz_h2o_foreign_continuum( xsec,
					parameters[0],
					parameters[1],
					f_mono,
					p_abs,
					t_abs,
					vmr );
    }
  else if ( "H2O-ContMPM93"==name ) // --------------------------------------
    {
      // only self and foreign continuum term is only simultaneously to calculated
      // since the parameterization is not devided up in these two terms.

      // Check if the right number of paramters has been specified:
      if ( 7 != parameters.size() )
	{
	  ostringstream os;
	  os << "Continuum model " << name << " requires two input\n"
	     << "parameters, but you specified " << parameters.size()
	     << ".";
	  throw runtime_error(os.str());
	  return;
	}
      
      // specific continuum parameters:
      // parameters[0] : pseudo continuum line frequency                      default: 1780.0*10^9 Hz
      // parameters[1] : pseudo continuum line strength parameter             default: 22300.0 Hz/Pa
      // parameters[2] : pseudo continuum line strength temperature parameter default: 0.952
      // parameters[3] : pseudo continuum line broadening parameter           default: 17.6*10^4 Hz/Pa
      // parameters[4] : pseudo continuum line broadening parameter           default: 30.5
      // parameters[5] : pseudo continuum line broadening parameter           default: 2
      // parameters[6] : pseudo continuum line broadening parameter           default: 5
      //
      // units:
      //  a) output 
      //     xsec          : [1/m],
      //  b) input
      //     parameters[0] : [Hz]
      //     parameters[1] : [Hz/Pa]
      //     parameters[2] : [1]
      //     parameters[3] : [Hz/Pa]
      //     parameters[4] : [1]
      //     parameters[5] : [1]
      //     parameters[6] : [1]
      //     f_mono        : [Hz]
      //     p_abs         : [Pa]
      //     t_abs         : [K]
      //     vmr           : [1]
      //
      MPM93_h2o_continuum( xsec,
			   parameters[0],
			   parameters[1],
			   parameters[2],
			   parameters[3],
			   parameters[4],
			   parameters[5],
			   parameters[6],
			   f_mono,
			   p_abs,
			   t_abs,
			   vmr	 );
    }
  else if ( "H2O-ContCKDSelf"==name ) // ----------------------------------------
    {
	  ostringstream os;
	  os << "CKD self continuum model not yet implemented"
	     << ".";
	  throw runtime_error(os.str());
	  return;

    }
  else if ( "H2O-ContCKDForeign"==name ) // -------------------------------------
    {
	  ostringstream os;
	  os << "CKD foreign continuum model not yet implemented"
	     << ".";
	  throw runtime_error(os.str());
	  return;

    }
  // ============= O2 continuum =================================================
  else if ( "O2-ContRosenkranz"==name )
    {
      // data information about this continuum: 
      // P. W. Rosenkranz Chapter 2, pp 74, in M. A. Janssen, 
      // "Atmospheric Remote Sensing by Microwave Radiometry",
      // John Wiley & Sons, Inc., 1993, ISBN 0-471-62891-3
      // (see also JQSRT, Vol.48, No.5/6 pp.629-643, 1992)

      // Check if the right number of paramters has been specified:
      if ( 2 != parameters.size() )
	{
	  ostringstream os;
	  os << "Continuum model " << name << " requires two input\n"
	     << "parameters, but you specified " << parameters.size()
	     << ".";
	  throw runtime_error(os.str());
	  return;
	}
      
      // specific continuum parameters:
      // parameters[0] : continuum coefficient (C)
      // parameters[1] : temperature exponent  (x)
      //
      // units:
      //  a) output 
      //     xsec          : [1/m],
      //  b) input
      //     parameters[0] : [K^2/(Hz*Pa*m)]
      //     parameters[1] : [1]
      //     f_mono        : [Hz]
      //     p_abs         : [Pa]
      //     t_abs         : [K]
      //     vmr           : [1]
      //
      Rosenkranz_o2_continuum( xsec,
			       parameters[0], // coefficient
			       parameters[1], // temp. exponent
			       f_mono,
			       p_abs,
			       t_abs,
			       h2o_abs,
			       vmr );
    }
  // ============= N2 continuum =================================================
  else if ( "N2-ContRosenkranzSelf"==name )
    {
      // data information about this continuum: 
      // P. W. Rosenkranz Chapter 2, pp 74, in M. A. Janssen, 
      // "Atmospheric Remote Sensing by Microwave Radiometry",
      // John Wiley & Sons, Inc., 1993, ISBN 0-471-62891-3

      // Check if the right number of paramters has been specified:
      if ( 2 != parameters.size() )
	{
	  ostringstream os;
	  os << "Continuum model " << name << " requires two input\n"
	     << "parameters, but you specified " << parameters.size()
	     << ".";
	  throw runtime_error(os.str());
	  return;
	}
      
      // specific continuum parameters:
      // parameters[0] : continuum coefficient (C)
      // parameters[1] : temperature exponent  (x)
      //
      // units:
      //  a) output 
      //     xsec          : [1/m],
      //  b) input
      //     parameters[0] : [1/(Hz^2*Pa^2*m)]
      //     parameters[1] : [1]
      //     f_mono        : [Hz]
      //     p_abs         : [Pa]
      //     t_abs         : [K]
      //     vmr           : [1]
      //
      Rosenkranz_n2_self_continuum( xsec,
				    parameters[0], // coefficient
				    parameters[1], // temp. exponent
				    f_mono,
				    p_abs,
				    t_abs,
				    vmr );
    }  
  else if ( "N2-ContBorysowSelf"==name ) // -------------------------------------
    {
      // data information about this continuum: 
      // A. Borysow and L. Frommhold, The Astrophysical Journal,
      // Vol. 311, pp.1043-1057, 1986
      ostringstream os;
      os << "N2 continuum parameterization of A. Borysow and L. Frommhold\n"
         << "is not yet implemented  ==>  no calculation performed!\n";
      /*
      Borysow_Frommhold_n2_continuum( xsec,
                                      parameters[0],
				      parameters[1],
				      f_mono,
				      p_abs,
				      t_abs,
				      vmr );
      */
      return;
    }  
  // ============= CO2 continuum ================================================
  else if ( "CO2-ContRosenkranzSelf"==name )
    {
      // data information about this continuum: 
      // P. W. Rosenkranz Chapter 2, pp 74, in M. A. Janssen, 
      // "Atmospheric Remote Sensing by Microwave Radiometry",
      // John Wiley & Sons, Inc., 1993, ISBN 0-471-62891-3

      // Check if the right number of paramters has been specified:
      if ( 2 != parameters.size() )
	{
	  ostringstream os;
	  os << "Continuum model " << name << " requires two input\n"
	     << "parameters, but you specified " << parameters.size()
	     << ".";
	  throw runtime_error(os.str());
	  return;
	}
      
      // specific continuum parameters:
      // parameters[0] : continuum coefficient (C)
      // parameters[1] : temperature exponent  (x)
      //
      // units:
      //  a) output 
      //     xsec          : [1/m],
      //  b) input
      //     parameters[0] : [1/(Hz^2*Pa^2*m)]
      //     parameters[1] : [1]
      //     f_mono        : [Hz]
      //     p_abs         : [Pa]
      //     t_abs         : [K]
      //     vmr           : [1]
      //
      Rosenkranz_co2_self_continuum( xsec,
				     parameters[0], // coefficient
				     parameters[1], // temp. exponent
				     f_mono,
				     p_abs,
				     t_abs,
				     vmr );
    }
  else if ( "CO2-ContRosenkranzForeign"==name ) // ------------------------------
    {
      // data information about this continuum: 
      // P. W. Rosenkranz Chapter 2, pp 74, in M. A. Janssen, 
      // "Atmospheric Remote Sensing by Microwave Radiometry",
      // John Wiley & Sons, Inc., 1993, ISBN 0-471-62891-3

      // Check if the right number of paramters has been specified:
      if ( 2 != parameters.size() )
	{
	  ostringstream os;
	  os << "Continuum model " << name << " requires two input\n"
	     << "parameters, but you specified " << parameters.size()
	     << ".";
	  throw runtime_error(os.str());
	  return;
	}
      
      // specific continuum parameters:
      // parameters[0] : continuum coefficient (C)
      // parameters[1] : temperature exponent  (x)
      //
      // units:
      //  a) output 
      //     xsec          : [1/m],
      //  b) input
      //     parameters[0] : [1/(Hz^2*Pa^2*m)]
      //     parameters[1] : [1]
      //     f_mono        : [Hz]
      //     p_abs         : [Pa]
      //     t_abs         : [K]
      //     n2_abs        : [1]
      //     vmr           : [1]
      //
      Rosenkranz_co2_foreign_continuum( xsec,
					parameters[0], // coefficient
					parameters[1], // temp. exponent
					f_mono,
					p_abs,
					t_abs,
					n2_abs,
					vmr );
    }
  // ============= cloud and fog absorption from MPM93 ==========================
  else if ( "H2O-MPM93droplet"==name )
    {
      // Suspended water droplet absorption parameterization from MPM93 model
      // H. J. Liebe and G. A. Hufford and M. G. Cotton,
      // "Propagation modeling of moist air and suspended water/ice
      //  particles at frequencies below 1000 GHz",
      // AGARD 52nd Specialists Meeting of the Electromagnetic Wave
      // Propagation Panel, Palma de Mallorca, Spain, 1993, May 17-21 

      // Check if the right number of paramters has been specified:
      if ( 2 != parameters.size() )
	{
	  ostringstream os;
	  os << "MPM93 liquid water particle absorption model " << name << " requires two input\n"
	     << "parameters, but you specified " << parameters.size()
	     << ".";
	  throw runtime_error(os.str());
	  return;
	}
      
      // liquid water droplet parameters:
      // parameters[0] : suspended water droplet density   range: 0-10 g/m3
      // parameters[1] : specific droplet weight           value:    1 g/cm3
      //
      // valid atmospheric condition:
      // temperature      : 233 to 323 K
      // relative humidity:   1 to 100 %
      // 
      // units:
      //  a) output 
      //     xsec          : [1/m],
      //  b) input
      //     parameters[0] : [g/m3]
      //     parameters[1] : [g/m3]
      //     f_mono        : [Hz]
      //     p_abs         : [Pa]
      //     t_abs         : [K]
      //     n2_abs        : [1]
      //     vmr           : [1]
      //
      MPM93WaterDropletAbs(xsec,
			   parameters[0],     // suspended water droplet density
			   parameters[1],     // specific droplet weight 
			   f_mono,
			   p_abs,
			   t_abs,
			   vmr );
    }
  // ============= ice particle absorption from MPM93 ============================
  else if ( "H2O-MPM93ice"==name )
    {
      // Ice particle absorption parameterization from MPM93 model
      // H. J. Liebe and G. A. Hufford and M. G. Cotton,
      // "Propagation modeling of moist air and suspended water/ice
      //  particles at frequencies below 1000 GHz",
      // AGARD 52nd Specialists Meeting of the Electromagnetic Wave
      // Propagation Panel, Palma de Mallorca, Spain, 1993, May 17-21 

      // Check if the right number of paramters has been specified:
      if ( 2 != parameters.size() )
	{
	  ostringstream os;
	  os << "MPM93 ice particle absorption model " << name << " requires two input\n"
	     << "parameters, but you specified " << parameters.size()
	     << ".";
	  throw runtime_error(os.str());
	  return;
	}
      
      // ice crystal parameters:
      // parameters[0] : suspended ice particle density   range:  0-10 g/m3
      // parameters[1] : specific ice particle weight     value: 0.916 g/cm3
      //
      // valid atmospheric condition:
      // temperature      : 233 to 323 K
      // relative humidity:   1 to 100 %
      // 
      // units:
      //  a) output 
      //     xsec          : [1/m],
      //  b) input
      //     parameters[0] : [g/m3]
      //     parameters[1] : [g/cm3]
      //     f_mono        : [Hz]
      //     p_abs         : [Pa]
      //     t_abs         : [K]
      //     n2_abs        : [1]
      //     vmr           : [1]
      //
      MPM93IceCrystalAbs(xsec,
			 parameters[0],     // suspended water droplet density
			 parameters[1],     // specific droplet weight 
			 f_mono,
			 p_abs,
			 t_abs,
			 vmr );
    }
  else // -----------------------------------------------------------------------
    {
      ostringstream os;
      os << "Continuum tag `" << name << "' not implemented.";
      throw runtime_error(os.str());
      return;
    }

  return;
}
//
// #################################################################################
//
/**
   An auxiliary functions that checks if a given continuum model is
   listed in species_data.cc. This is just in order to verify that this
   really represent a valid continuum model.

   The given name should be something like
   `H2O-ContRosenkranzSelf'. The function simply checks if there is a
   species H2O with an isotope ContRosenkranzSelf.

   For user-friendliness, the function also compiles a list of allowed
   continuum models and gives this as an error message if the model is
   not found. 

   The function has no return value, since, if the name does not match
   a valid model an error is thrown anyway.

   \param name The name of the continuum model to check.

   \throw runtime_error The model does not exist.

   \author Stefan Buehler
   \date   2001-03-12
*/
void check_continuum_model(const string& name)
{
  // The species lookup data:
  extern const ARRAY<SpeciesRecord> species_data;

  // For the list of valid continuum models:
  ARRAY<string> valid_models;

  bool found = false;

  // Loop all species:
  for ( ARRAY<SpeciesRecord>::const_iterator i=species_data.begin();
	i<species_data.end();
	++i )
    {
      string specnam = i->Name();

      // Loop all isotopes:
      for ( ARRAY<IsotopeRecord>::const_iterator j=i->Isotope().begin();
	    j<i->Isotope().end();
	    ++j )
	{
	  string isonam = j->Name();

	  // The specified name consists of a species part and an
	  // isotope part, e.g., H2O-ContRosenkranzSelf. We need to
	  // construct a similar string from the species lookup data
	  // by concatenating species name and isotope name.

	  string fullnam = specnam + "-" + isonam;
	  //	  cout << fullnam << "\n";

	  // See if this is a continuum tag, so that we can add it to
	  // the list:
	  if ( 0 > j->Abundance() )
	    {
	      valid_models.push_back(fullnam);	      
	    }
	  
	  if ( name == fullnam )
	    {
	      found = true;
	    }
	}
    }

  // ----------------------------------------------------------------------
  // Have we found it?
  if (!found)
    {
      ostringstream os;
      os << "The string `" << name << "' matches none of the known\n"
	 << "continuum models. Known continuum models are:";
      for ( ARRAY<string>::const_iterator i=valid_models.begin();
	    i<valid_models.end();
	    ++i )
	{
	  os << "\n" << *i;
	}      
      throw runtime_error(os.str());
    }
}
//
// #################################################################################
