/** @file add.cpp
 *
 *  Implementation of GiNaC's sums of expressions. */

/*
 *  GiNaC Copyright (C) 1999 Johannes Gutenberg University Mainz, Germany
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

#include <iostream>
#include <stdexcept>

#include "add.h"
#include "mul.h"
#include "debugmsg.h"

#ifndef NO_GINAC_NAMESPACE
namespace GiNaC {
#endif // ndef NO_GINAC_NAMESPACE

//////////
// default constructor, destructor, copy constructor assignment operator and helpers
//////////

// public

add::add()
{
    debugmsg("add default constructor",LOGLEVEL_CONSTRUCT);
    tinfo_key = TINFO_add;
}

add::~add()
{
    debugmsg("add destructor",LOGLEVEL_DESTRUCT);
    destroy(0);
}

add::add(add const & other)
{
    debugmsg("add copy constructor",LOGLEVEL_CONSTRUCT);
    copy(other);
}

add const & add::operator=(add const & other)
{
    debugmsg("add operator=",LOGLEVEL_ASSIGNMENT);
    if (this != &other) {
        destroy(1);
        copy(other);
    }
    return *this;
}

// protected

void add::copy(add const & other)
{
    expairseq::copy(other);
}

void add::destroy(bool call_parent)
{
    if (call_parent) expairseq::destroy(call_parent);
}

//////////
// other constructors
//////////

// public

add::add(ex const & lh, ex const & rh)
{
    debugmsg("add constructor from ex,ex",LOGLEVEL_CONSTRUCT);
    tinfo_key = TINFO_add;
    overall_coeff=exZERO();
    construct_from_2_ex(lh,rh);
    GINAC_ASSERT(is_canonical());
}

add::add(exvector const & v)
{
    debugmsg("add constructor from exvector",LOGLEVEL_CONSTRUCT);
    tinfo_key = TINFO_add;
    overall_coeff=exZERO();
    construct_from_exvector(v);
    GINAC_ASSERT(is_canonical());
}

/*
add::add(epvector const & v, bool do_not_canonicalize)
{
    debugmsg("add constructor from epvector,bool",LOGLEVEL_CONSTRUCT);
    tinfo_key = TINFO_add;
    if (do_not_canonicalize) {
        seq=v;
#ifdef EXPAIRSEQ_USE_HASHTAB
        combine_same_terms(); // to build hashtab
#endif // def EXPAIRSEQ_USE_HASHTAB
    } else {
        construct_from_epvector(v);
    }
    GINAC_ASSERT(is_canonical());
}
*/

add::add(epvector const & v)
{
    debugmsg("add constructor from epvector",LOGLEVEL_CONSTRUCT);
    tinfo_key = TINFO_add;
    overall_coeff=exZERO();
    construct_from_epvector(v);
    GINAC_ASSERT(is_canonical());
}

add::add(epvector const & v, ex const & oc)
{
    debugmsg("add constructor from epvector,ex",LOGLEVEL_CONSTRUCT);
    tinfo_key = TINFO_add;
    overall_coeff=oc;
    construct_from_epvector(v);
    GINAC_ASSERT(is_canonical());
}

add::add(epvector * vp, ex const & oc)
{
    debugmsg("add constructor from epvector *,ex",LOGLEVEL_CONSTRUCT);
    tinfo_key = TINFO_add;
    GINAC_ASSERT(vp!=0);
    overall_coeff=oc;
    construct_from_epvector(*vp);
    delete vp;
    GINAC_ASSERT(is_canonical());
}

//////////
// functions overriding virtual functions from bases classes
//////////

// public

basic * add::duplicate() const
{
    debugmsg("add duplicate",LOGLEVEL_DUPLICATE);
    return new add(*this);
}

bool add::info(unsigned inf) const
{
    // TODO: optimize
    if (inf==info_flags::polynomial || inf==info_flags::integer_polynomial || inf==info_flags::rational_polynomial || inf==info_flags::rational_function) {
        for (epvector::const_iterator it=seq.begin(); it!=seq.end(); ++it) {
            if (!(recombine_pair_to_ex(*it).info(inf)))
                return false;
        }
        return true;
    } else {
        return expairseq::info(inf);
    }
}

int add::degree(symbol const & s) const
{
    int deg=INT_MIN;
    if (!overall_coeff.is_equal(exZERO())) {
        deg=0;
    }
    int cur_deg;
    for (epvector::const_iterator cit=seq.begin(); cit!=seq.end(); ++cit) {
        cur_deg=(*cit).rest.degree(s);
        if (cur_deg>deg) deg=cur_deg;
    }
    return deg;
}

int add::ldegree(symbol const & s) const
{
    int deg=INT_MAX;
    if (!overall_coeff.is_equal(exZERO())) {
        deg=0;
    }
    int cur_deg;
    for (epvector::const_iterator cit=seq.begin(); cit!=seq.end(); ++cit) {
        cur_deg=(*cit).rest.ldegree(s);
        if (cur_deg<deg) deg=cur_deg;
    }
    return deg;
}

ex add::coeff(symbol const & s, int const n) const
{
    epvector coeffseq;
    coeffseq.reserve(seq.size());

    epvector::const_iterator it=seq.begin();
    while (it!=seq.end()) {
        coeffseq.push_back(combine_ex_with_coeff_to_pair((*it).rest.coeff(s,n),
                                                         (*it).coeff));
        ++it;
    }
    if (n==0) {
        return (new add(coeffseq,overall_coeff))->setflag(status_flags::dynallocated);
    }
    return (new add(coeffseq))->setflag(status_flags::dynallocated);
}

ex add::eval(int level) const
{
    // simplifications: +(;c) -> c
    //                  +(x;1) -> x

    debugmsg("add eval",LOGLEVEL_MEMBER_FUNCTION);

    epvector * evaled_seqp=evalchildren(level);
    if (evaled_seqp!=0) {
        // do more evaluation later
        return (new add(evaled_seqp,overall_coeff))->
                   setflag(status_flags::dynallocated);
    }

#ifdef DO_GINAC_ASSERT
    for (epvector::const_iterator cit=seq.begin(); cit!=seq.end(); ++cit) {
        GINAC_ASSERT(!is_ex_exactly_of_type((*cit).rest,add));
        if (is_ex_exactly_of_type((*cit).rest,numeric)) {
            dbgprint();
        }
        GINAC_ASSERT(!is_ex_exactly_of_type((*cit).rest,numeric));
    }
#endif // def DO_GINAC_ASSERT

    if (flags & status_flags::evaluated) {
        GINAC_ASSERT(seq.size()>0);
        GINAC_ASSERT((seq.size()>1)||!overall_coeff.is_equal(exZERO()));
        return *this;
    }

    int seq_size=seq.size();
    if (seq_size==0) {
        // +(;c) -> c
        return overall_coeff;
    } else if ((seq_size==1)&&overall_coeff.is_equal(exZERO())) {
        // +(x;0) -> x
        return recombine_pair_to_ex(*(seq.begin()));
    }
    return this->hold();
}

exvector add::get_indices(void) const
{
    // FIXME: all terms in the sum should have the same indices (compatible
    // tensors) however this is not checked, since there is no function yet
    // which compares indices (idxvector can be unsorted)
    if (seq.size()==0) {
        return exvector();
    }
    return (seq.begin())->rest.get_indices();
}    

ex add::simplify_ncmul(exvector const & v) const
{
    if (seq.size()==0) {
        return expairseq::simplify_ncmul(v);
    }
    return (*seq.begin()).rest.simplify_ncmul(v);
}    

// protected

int add::compare_same_type(basic const & other) const
{
    return expairseq::compare_same_type(other);
}

bool add::is_equal_same_type(basic const & other) const
{
    return expairseq::is_equal_same_type(other);
}

unsigned add::return_type(void) const
{
    if (seq.size()==0) {
        return return_types::commutative;
    }
    return (*seq.begin()).rest.return_type();
}
   
unsigned add::return_type_tinfo(void) const
{
    if (seq.size()==0) {
        return tinfo_key;
    }
    return (*seq.begin()).rest.return_type_tinfo();
}

ex add::thisexpairseq(epvector const & v, ex const & oc) const
{
    return (new add(v,oc))->setflag(status_flags::dynallocated);
}

ex add::thisexpairseq(epvector * vp, ex const & oc) const
{
    return (new add(vp,oc))->setflag(status_flags::dynallocated);
}

expair add::split_ex_to_pair(ex const & e) const
{
    if (is_ex_exactly_of_type(e,mul)) {
        mul const & mulref=ex_to_mul(e);
        ex numfactor=mulref.overall_coeff;
        // mul * mulcopyp=static_cast<mul *>(mulref.duplicate());
        mul * mulcopyp=new mul(mulref);
        mulcopyp->overall_coeff=exONE();
        mulcopyp->clearflag(status_flags::evaluated);
        mulcopyp->clearflag(status_flags::hash_calculated);
        return expair(mulcopyp->setflag(status_flags::dynallocated),numfactor);
    }
    return expair(e,exONE());
}

expair add::combine_ex_with_coeff_to_pair(ex const & e,
                                          ex const & c) const
{
    GINAC_ASSERT(is_ex_exactly_of_type(c,numeric));
    if (is_ex_exactly_of_type(e,mul)) {
        mul const & mulref=ex_to_mul(e);
        ex numfactor=mulref.overall_coeff;
        //mul * mulcopyp=static_cast<mul *>(mulref.duplicate());
        mul * mulcopyp=new mul(mulref);
        mulcopyp->overall_coeff=exONE();
        mulcopyp->clearflag(status_flags::evaluated);
        mulcopyp->clearflag(status_flags::hash_calculated);
        if (are_ex_trivially_equal(c,exONE())) {
            return expair(mulcopyp->setflag(status_flags::dynallocated),numfactor);
        } else if (are_ex_trivially_equal(numfactor,exONE())) {
            return expair(mulcopyp->setflag(status_flags::dynallocated),c);
        }
        return expair(mulcopyp->setflag(status_flags::dynallocated),
                          ex_to_numeric(numfactor).mul_dyn(ex_to_numeric(c)));
    } else if (is_ex_exactly_of_type(e,numeric)) {
        if (are_ex_trivially_equal(c,exONE())) {
            return expair(e,exONE());
        }
        return expair(ex_to_numeric(e).mul_dyn(ex_to_numeric(c)),exONE());
    }
    return expair(e,c);
}
    
expair add::combine_pair_with_coeff_to_pair(expair const & p,
                                            ex const & c) const
{
    GINAC_ASSERT(is_ex_exactly_of_type(p.coeff,numeric));
    GINAC_ASSERT(is_ex_exactly_of_type(c,numeric));

    if (is_ex_exactly_of_type(p.rest,numeric)) {
        GINAC_ASSERT(ex_to_numeric(p.coeff).is_equal(numONE())); // should be normalized
        return expair(ex_to_numeric(p.rest).mul_dyn(ex_to_numeric(c)),exONE());
    }

    return expair(p.rest,ex_to_numeric(p.coeff).mul_dyn(ex_to_numeric(c)));
}
    
ex add::recombine_pair_to_ex(expair const & p) const
{
    //if (p.coeff.compare(exONE())==0) {
    //if (are_ex_trivially_equal(p.coeff,exONE())) {
    if (ex_to_numeric(p.coeff).is_equal(numONE())) {
        return p.rest;
    } else {
        return p.rest*p.coeff;
    }
}

ex add::expand(unsigned options) const
{
    epvector * vp=expandchildren(options);
    if (vp==0) {
        return *this;
    }
    return (new add(vp,overall_coeff))->setflag(status_flags::expanded    |
                                                status_flags::dynallocated );
}

//////////
// new virtual functions which can be overridden by derived classes
//////////

// none

//////////
// non-virtual functions in this class
//////////

// none

//////////
// static member variables
//////////

// protected

unsigned add::precedence=40;

//////////
// global constants
//////////

const add some_add;
type_info const & typeid_add=typeid(some_add);

#ifndef NO_GINAC_NAMESPACE
} // namespace GiNaC
#endif // ndef NO_GINAC_NAMESPACE
