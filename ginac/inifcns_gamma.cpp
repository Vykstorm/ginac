/** @file inifcns_gamma.cpp
 *
 *  Implementation of Gamma-function, Beta-function, Polygamma-functions, and
 *  some related stuff. */

/*
 *  GiNaC Copyright (C) 1999-2004 Johannes Gutenberg University Mainz, Germany
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <vector>
#include <stdexcept>

#include "inifcns.h"
#include "constant.h"
#include "pseries.h"
#include "numeric.h"
#include "power.h"
#include "relational.h"
#include "operators.h"
#include "symbol.h"
#include "symmetry.h"
#include "utils.h"

namespace GiNaC {

//////////
// Logarithm of Gamma function
//////////

static ex lgamma_evalf(const ex & x)
{
	if (is_exactly_a<numeric>(x)) {
		try {
			return lgamma(ex_to<numeric>(x));
		} catch (const dunno &e) { }
	}
	
	return lgamma(x).hold();
}


/** Evaluation of lgamma(x), the natural logarithm of the Gamma function.
 *  Knows about integer arguments and that's it.  Somebody ought to provide
 *  some good numerical evaluation some day...
 *
 *  @exception GiNaC::pole_error("lgamma_eval(): logarithmic pole",0) */
static ex lgamma_eval(const ex & x)
{
	if (x.info(info_flags::numeric)) {
		// trap integer arguments:
		if (x.info(info_flags::integer)) {
			// lgamma(n) -> log((n-1)!) for postitive n
			if (x.info(info_flags::posint))
				return log(factorial(x + _ex_1));
			else
				throw (pole_error("lgamma_eval(): logarithmic pole",0));
		}
		//  lgamma_evalf should be called here once it becomes available
	}
	
	return lgamma(x).hold();
}


static ex lgamma_deriv(const ex & x, unsigned deriv_param)
{
	GINAC_ASSERT(deriv_param==0);
	
	// d/dx  lgamma(x) -> psi(x)
	return psi(x);
}


static ex lgamma_series(const ex & arg,
                        const relational & rel,
                        int order,
                        unsigned options)
{
	// method:
	// Taylor series where there is no pole falls back to psi function
	// evaluation.
	// On a pole at -m we could use the recurrence relation
	//   lgamma(x) == lgamma(x+1)-log(x)
	// from which follows
	//   series(lgamma(x),x==-m,order) ==
	//   series(lgamma(x+m+1)-log(x)...-log(x+m)),x==-m,order);
	const ex arg_pt = arg.subs(rel, subs_options::no_pattern);
	if (!arg_pt.info(info_flags::integer) || arg_pt.info(info_flags::positive))
		throw do_taylor();  // caught by function::series()
	// if we got here we have to care for a simple pole of tgamma(-m):
	numeric m = -ex_to<numeric>(arg_pt);
	ex recur;
	for (numeric p = 0; p<=m; ++p)
		recur += log(arg+p);
	return (lgamma(arg+m+_ex1)-recur).series(rel, order, options);
}


REGISTER_FUNCTION(lgamma, eval_func(lgamma_eval).
                          evalf_func(lgamma_evalf).
                          derivative_func(lgamma_deriv).
                          series_func(lgamma_series).
                          latex_name("\\log \\Gamma"));


//////////
// true Gamma function
//////////

static ex tgamma_evalf(const ex & x)
{
	if (is_exactly_a<numeric>(x)) {
		try {
			return tgamma(ex_to<numeric>(x));
		} catch (const dunno &e) { }
	}
	
	return tgamma(x).hold();
}


/** Evaluation of tgamma(x), the true Gamma function.  Knows about integer
 *  arguments, half-integer arguments and that's it. Somebody ought to provide
 *  some good numerical evaluation some day...
 *
 *  @exception pole_error("tgamma_eval(): simple pole",0) */
static ex tgamma_eval(const ex & x)
{
	if (x.info(info_flags::numeric)) {
		// trap integer arguments:
		const numeric two_x = _num2*ex_to<numeric>(x);
		if (two_x.is_even()) {
			// tgamma(n) -> (n-1)! for postitive n
			if (two_x.is_positive()) {
				return factorial(ex_to<numeric>(x).sub(_num1));
			} else {
				throw (pole_error("tgamma_eval(): simple pole",1));
			}
		}
		// trap half integer arguments:
		if (two_x.is_integer()) {
			// trap positive x==(n+1/2)
			// tgamma(n+1/2) -> Pi^(1/2)*(1*3*..*(2*n-1))/(2^n)
			if (two_x.is_positive()) {
				const numeric n = ex_to<numeric>(x).sub(_num1_2);
				return (doublefactorial(n.mul(_num2).sub(_num1)).div(pow(_num2,n))) * sqrt(Pi);
			} else {
				// trap negative x==(-n+1/2)
				// tgamma(-n+1/2) -> Pi^(1/2)*(-2)^n/(1*3*..*(2*n-1))
				const numeric n = abs(ex_to<numeric>(x).sub(_num1_2));
				return (pow(_num_2, n).div(doublefactorial(n.mul(_num2).sub(_num1))))*sqrt(Pi);
			}
		}
		//  tgamma_evalf should be called here once it becomes available
	}
	
	return tgamma(x).hold();
}


static ex tgamma_deriv(const ex & x, unsigned deriv_param)
{
	GINAC_ASSERT(deriv_param==0);
	
	// d/dx  tgamma(x) -> psi(x)*tgamma(x)
	return psi(x)*tgamma(x);
}


static ex tgamma_series(const ex & arg,
                        const relational & rel,
                        int order,
                        unsigned options)
{
	// method:
	// Taylor series where there is no pole falls back to psi function
	// evaluation.
	// On a pole at -m use the recurrence relation
	//   tgamma(x) == tgamma(x+1) / x
	// from which follows
	//   series(tgamma(x),x==-m,order) ==
	//   series(tgamma(x+m+1)/(x*(x+1)*...*(x+m)),x==-m,order);
	const ex arg_pt = arg.subs(rel, subs_options::no_pattern);
	if (!arg_pt.info(info_flags::integer) || arg_pt.info(info_flags::positive))
		throw do_taylor();  // caught by function::series()
	// if we got here we have to care for a simple pole at -m:
	const numeric m = -ex_to<numeric>(arg_pt);
	ex ser_denom = _ex1;
	for (numeric p; p<=m; ++p)
		ser_denom *= arg+p;
	return (tgamma(arg+m+_ex1)/ser_denom).series(rel, order, options);
}


REGISTER_FUNCTION(tgamma, eval_func(tgamma_eval).
                          evalf_func(tgamma_evalf).
                          derivative_func(tgamma_deriv).
                          series_func(tgamma_series).
                          latex_name("\\Gamma"));


//////////
// beta-function
//////////

static ex beta_evalf(const ex & x, const ex & y)
{
	if (is_exactly_a<numeric>(x) && is_exactly_a<numeric>(y)) {
		try {
			return tgamma(ex_to<numeric>(x))*tgamma(ex_to<numeric>(y))/tgamma(ex_to<numeric>(x+y));
		} catch (const dunno &e) { }
	}
	
	return beta(x,y).hold();
}


static ex beta_eval(const ex & x, const ex & y)
{
	if (x.is_equal(_ex1))
		return 1/y;
	if (y.is_equal(_ex1))
		return 1/x;
	if (x.info(info_flags::numeric) && y.info(info_flags::numeric)) {
		// treat all problematic x and y that may not be passed into tgamma,
		// because they would throw there although beta(x,y) is well-defined
		// using the formula beta(x,y) == (-1)^y * beta(1-x-y, y)
		const numeric &nx = ex_to<numeric>(x);
		const numeric &ny = ex_to<numeric>(y);
		if (nx.is_real() && nx.is_integer() &&
			ny.is_real() && ny.is_integer()) {
			if (nx.is_negative()) {
				if (nx<=-ny)
					return pow(_num_1, ny)*beta(1-x-y, y);
				else
					throw (pole_error("beta_eval(): simple pole",1));
			}
			if (ny.is_negative()) {
				if (ny<=-nx)
					return pow(_num_1, nx)*beta(1-y-x, x);
				else
					throw (pole_error("beta_eval(): simple pole",1));
			}
			return tgamma(x)*tgamma(y)/tgamma(x+y);
		}
		// no problem in numerator, but denominator has pole:
		if ((nx+ny).is_real() &&
		    (nx+ny).is_integer() &&
		   !(nx+ny).is_positive())
			 return _ex0;
		// beta_evalf should be called here once it becomes available
	}
	
	return beta(x,y).hold();
}


static ex beta_deriv(const ex & x, const ex & y, unsigned deriv_param)
{
	GINAC_ASSERT(deriv_param<2);
	ex retval;
	
	// d/dx beta(x,y) -> (psi(x)-psi(x+y)) * beta(x,y)
	if (deriv_param==0)
		retval = (psi(x)-psi(x+y))*beta(x,y);
	// d/dy beta(x,y) -> (psi(y)-psi(x+y)) * beta(x,y)
	if (deriv_param==1)
		retval = (psi(y)-psi(x+y))*beta(x,y);
	return retval;
}


static ex beta_series(const ex & arg1,
                      const ex & arg2,
                      const relational & rel,
                      int order,
                      unsigned options)
{
	// method:
	// Taylor series where there is no pole of one of the tgamma functions
	// falls back to beta function evaluation.  Otherwise, fall back to
	// tgamma series directly.
	const ex arg1_pt = arg1.subs(rel, subs_options::no_pattern);
	const ex arg2_pt = arg2.subs(rel, subs_options::no_pattern);
	GINAC_ASSERT(is_a<symbol>(rel.lhs()));
	const symbol &s = ex_to<symbol>(rel.lhs());
	ex arg1_ser, arg2_ser, arg1arg2_ser;
	if ((!arg1_pt.info(info_flags::integer) || arg1_pt.info(info_flags::positive)) &&
	    (!arg2_pt.info(info_flags::integer) || arg2_pt.info(info_flags::positive)))
		throw do_taylor();  // caught by function::series()
	// trap the case where arg1 is on a pole:
	if (arg1.info(info_flags::integer) && !arg1.info(info_flags::positive))
		arg1_ser = tgamma(arg1+s).series(rel, order, options);
	else
		arg1_ser = tgamma(arg1).series(rel,order);
	// trap the case where arg2 is on a pole:
	if (arg2.info(info_flags::integer) && !arg2.info(info_flags::positive))
		arg2_ser = tgamma(arg2+s).series(rel, order, options);
	else
		arg2_ser = tgamma(arg2).series(rel,order);
	// trap the case where arg1+arg2 is on a pole:
	if ((arg1+arg2).info(info_flags::integer) && !(arg1+arg2).info(info_flags::positive))
		arg1arg2_ser = tgamma(arg2+arg1+s).series(rel, order, options);
	else
		arg1arg2_ser = tgamma(arg2+arg1).series(rel,order);
	// compose the result (expanding all the terms):
	return (arg1_ser*arg2_ser/arg1arg2_ser).series(rel, order, options).expand();
}


REGISTER_FUNCTION(beta, eval_func(beta_eval).
                        evalf_func(beta_evalf).
                        derivative_func(beta_deriv).
                        series_func(beta_series).
                        latex_name("\\mbox{B}").
						set_symmetry(sy_symm(0, 1)));


//////////
// Psi-function (aka digamma-function)
//////////

static ex psi1_evalf(const ex & x)
{
	if (is_exactly_a<numeric>(x)) {
		try {
			return psi(ex_to<numeric>(x));
		} catch (const dunno &e) { }
	}
	
	return psi(x).hold();
}

/** Evaluation of digamma-function psi(x).
 *  Somebody ought to provide some good numerical evaluation some day... */
static ex psi1_eval(const ex & x)
{
	if (x.info(info_flags::numeric)) {
		const numeric &nx = ex_to<numeric>(x);
		if (nx.is_integer()) {
			// integer case 
			if (nx.is_positive()) {
				// psi(n) -> 1 + 1/2 +...+ 1/(n-1) - Euler
				numeric rat = 0;
				for (numeric i(nx+_num_1); i>0; --i)
					rat += i.inverse();
				return rat-Euler;
			} else {
				// for non-positive integers there is a pole:
				throw (pole_error("psi_eval(): simple pole",1));
			}
		}
		if ((_num2*nx).is_integer()) {
			// half integer case
			if (nx.is_positive()) {
				// psi((2m+1)/2) -> 2/(2m+1) + 2/2m +...+ 2/1 - Euler - 2log(2)
				numeric rat = 0;
				for (numeric i = (nx+_num_1)*_num2; i>0; i-=_num2)
					rat += _num2*i.inverse();
				return rat-Euler-_ex2*log(_ex2);
			} else {
				// use the recurrence relation
				//   psi(-m-1/2) == psi(-m-1/2+1) - 1 / (-m-1/2)
				// to relate psi(-m-1/2) to psi(1/2):
				//   psi(-m-1/2) == psi(1/2) + r
				// where r == ((-1/2)^(-1) + ... + (-m-1/2)^(-1))
				numeric recur = 0;
				for (numeric p = nx; p<0; ++p)
					recur -= pow(p, _num_1);
				return recur+psi(_ex1_2);
			}
		}
		//  psi1_evalf should be called here once it becomes available
	}
	
	return psi(x).hold();
}

static ex psi1_deriv(const ex & x, unsigned deriv_param)
{
	GINAC_ASSERT(deriv_param==0);
	
	// d/dx psi(x) -> psi(1,x)
	return psi(_ex1, x);
}

static ex psi1_series(const ex & arg,
                      const relational & rel,
                      int order,
                      unsigned options)
{
	// method:
	// Taylor series where there is no pole falls back to polygamma function
	// evaluation.
	// On a pole at -m use the recurrence relation
	//   psi(x) == psi(x+1) - 1/z
	// from which follows
	//   series(psi(x),x==-m,order) ==
	//   series(psi(x+m+1) - 1/x - 1/(x+1) - 1/(x+m)),x==-m,order);
	const ex arg_pt = arg.subs(rel, subs_options::no_pattern);
	if (!arg_pt.info(info_flags::integer) || arg_pt.info(info_flags::positive))
		throw do_taylor();  // caught by function::series()
	// if we got here we have to care for a simple pole at -m:
	const numeric m = -ex_to<numeric>(arg_pt);
	ex recur;
	for (numeric p; p<=m; ++p)
		recur += power(arg+p,_ex_1);
	return (psi(arg+m+_ex1)-recur).series(rel, order, options);
}

unsigned psi1_SERIAL::serial =
	function::register_new(function_options("psi").
	                       eval_func(psi1_eval).
	                       evalf_func(psi1_evalf).
	                       derivative_func(psi1_deriv).
	                       series_func(psi1_series).
	                       latex_name("\\psi").
	                       overloaded(2));

//////////
// Psi-functions (aka polygamma-functions)  psi(0,x)==psi(x)
//////////

static ex psi2_evalf(const ex & n, const ex & x)
{
	if (is_exactly_a<numeric>(n) && is_exactly_a<numeric>(x)) {
		try {
			return psi(ex_to<numeric>(n),ex_to<numeric>(x));
		} catch (const dunno &e) { }
	}
	
	return psi(n,x).hold();
}

/** Evaluation of polygamma-function psi(n,x). 
 *  Somebody ought to provide some good numerical evaluation some day... */
static ex psi2_eval(const ex & n, const ex & x)
{
	// psi(0,x) -> psi(x)
	if (n.is_zero())
		return psi(x);
	// psi(-1,x) -> log(tgamma(x))
	if (n.is_equal(_ex_1))
		return log(tgamma(x));
	if (n.info(info_flags::numeric) && n.info(info_flags::posint) &&
		x.info(info_flags::numeric)) {
		const numeric &nn = ex_to<numeric>(n);
		const numeric &nx = ex_to<numeric>(x);
		if (nx.is_integer()) {
			// integer case 
			if (nx.is_equal(_num1))
				// use psi(n,1) == (-)^(n+1) * n! * zeta(n+1)
				return pow(_num_1,nn+_num1)*factorial(nn)*zeta(ex(nn+_num1));
			if (nx.is_positive()) {
				// use the recurrence relation
				//   psi(n,m) == psi(n,m+1) - (-)^n * n! / m^(n+1)
				// to relate psi(n,m) to psi(n,1):
				//   psi(n,m) == psi(n,1) + r
				// where r == (-)^n * n! * (1^(-n-1) + ... + (m-1)^(-n-1))
				numeric recur = 0;
				for (numeric p = 1; p<nx; ++p)
					recur += pow(p, -nn+_num_1);
				recur *= factorial(nn)*pow(_num_1, nn);
				return recur+psi(n,_ex1);
			} else {
				// for non-positive integers there is a pole:
				throw (pole_error("psi2_eval(): pole",1));
			}
		}
		if ((_num2*nx).is_integer()) {
			// half integer case
			if (nx.is_equal(_num1_2))
				// use psi(n,1/2) == (-)^(n+1) * n! * (2^(n+1)-1) * zeta(n+1)
				return pow(_num_1,nn+_num1)*factorial(nn)*(pow(_num2,nn+_num1) + _num_1)*zeta(ex(nn+_num1));
			if (nx.is_positive()) {
				const numeric m = nx - _num1_2;
				// use the multiplication formula
				//   psi(n,2*m) == (psi(n,m) + psi(n,m+1/2)) / 2^(n+1)
				// to revert to positive integer case
				return psi(n,_num2*m)*pow(_num2,nn+_num1)-psi(n,m);
			} else {
				// use the recurrence relation
				//   psi(n,-m-1/2) == psi(n,-m-1/2+1) - (-)^n * n! / (-m-1/2)^(n+1)
				// to relate psi(n,-m-1/2) to psi(n,1/2):
				//   psi(n,-m-1/2) == psi(n,1/2) + r
				// where r == (-)^(n+1) * n! * ((-1/2)^(-n-1) + ... + (-m-1/2)^(-n-1))
				numeric recur = 0;
				for (numeric p = nx; p<0; ++p)
					recur += pow(p, -nn+_num_1);
				recur *= factorial(nn)*pow(_num_1, nn+_num_1);
				return recur+psi(n,_ex1_2);
			}
		}
		//  psi2_evalf should be called here once it becomes available
	}
	
	return psi(n, x).hold();
}    

static ex psi2_deriv(const ex & n, const ex & x, unsigned deriv_param)
{
	GINAC_ASSERT(deriv_param<2);
	
	if (deriv_param==0) {
		// d/dn psi(n,x)
		throw(std::logic_error("cannot diff psi(n,x) with respect to n"));
	}
	// d/dx psi(n,x) -> psi(n+1,x)
	return psi(n+_ex1, x);
}

static ex psi2_series(const ex & n,
                      const ex & arg,
                      const relational & rel,
                      int order,
                      unsigned options)
{
	// method:
	// Taylor series where there is no pole falls back to polygamma function
	// evaluation.
	// On a pole at -m use the recurrence relation
	//   psi(n,x) == psi(n,x+1) - (-)^n * n! / x^(n+1)
	// from which follows
	//   series(psi(x),x==-m,order) == 
	//   series(psi(x+m+1) - (-1)^n * n! * ((x)^(-n-1) + (x+1)^(-n-1) + ...
	//                                      ... + (x+m)^(-n-1))),x==-m,order);
	const ex arg_pt = arg.subs(rel, subs_options::no_pattern);
	if (!arg_pt.info(info_flags::integer) || arg_pt.info(info_flags::positive))
		throw do_taylor();  // caught by function::series()
	// if we got here we have to care for a pole of order n+1 at -m:
	const numeric m = -ex_to<numeric>(arg_pt);
	ex recur;
	for (numeric p; p<=m; ++p)
		recur += power(arg+p,-n+_ex_1);
	recur *= factorial(n)*power(_ex_1,n);
	return (psi(n, arg+m+_ex1)-recur).series(rel, order, options);
}

unsigned psi2_SERIAL::serial =
	function::register_new(function_options("psi").
	                       eval_func(psi2_eval).
	                       evalf_func(psi2_evalf).
	                       derivative_func(psi2_deriv).
	                       series_func(psi2_series).
	                       latex_name("\\psi").
	                       overloaded(2));


} // namespace GiNaC
