/** @file ex.cpp
 *
 *  Implementation of GiNaC's light-weight expression handles. */

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

#include "ex.h"
#include "add.h"
#include "mul.h"
#include "ncmul.h"
#include "numeric.h"
#include "power.h"
#include "debugmsg.h"
#include "utils.h"

#ifndef NO_GINAC_NAMESPACE
namespace GiNaC {
#endif // ndef NO_GINAC_NAMESPACE

//////////
// default constructor, destructor, copy constructor assignment operator and helpers
//////////

// public

#ifndef INLINE_EX_CONSTRUCTORS

ex::ex() : bp(ex0().bp)
{
    debugmsg("ex default constructor",LOGLEVEL_CONSTRUCT);
    GINAC_ASSERT(ex0().bp!=0);
    GINAC_ASSERT(ex0().bp->flags & status_flags::dynallocated);
    GINAC_ASSERT(bp!=0);
    ++bp->refcount;
}

ex::~ex()
{
    debugmsg("ex destructor",LOGLEVEL_DESTRUCT);
    GINAC_ASSERT(bp!=0);
    GINAC_ASSERT(bp->flags & status_flags::dynallocated);
    if (--bp->refcount == 0) {
        delete bp;
    }
}

ex::ex(ex const & other) : bp(other.bp)
{
    debugmsg("ex copy constructor",LOGLEVEL_CONSTRUCT);
    GINAC_ASSERT(bp!=0);
    GINAC_ASSERT((bp->flags) & status_flags::dynallocated);
    ++bp->refcount;
}

ex const & ex::operator=(ex const & other)
{
    debugmsg("ex operator=",LOGLEVEL_ASSIGNMENT);
    GINAC_ASSERT(bp!=0);
    GINAC_ASSERT(bp->flags & status_flags::dynallocated);
    GINAC_ASSERT(other.bp!=0);
    GINAC_ASSERT(other.bp->flags & status_flags::dynallocated);
    ++other.bp->refcount;
    basic * tmpbp=other.bp;
    if (--bp->refcount==0) {
            delete bp;
    }
    bp=tmpbp;
    return *this;
}

#endif // ndef INLINE_EX_CONSTRUCTORS

//////////
// other constructors
//////////

// public

#ifndef INLINE_EX_CONSTRUCTORS
ex::ex(basic const & other)
{
    debugmsg("ex constructor from basic",LOGLEVEL_CONSTRUCT);
    construct_from_basic(other);
}
#endif

ex::ex(int const i)
{
    debugmsg("ex constructor from int",LOGLEVEL_CONSTRUCT);
    switch (i) {  // some tiny efficiency-hack (FIXME: is this ok?)
        case -1:
            bp = _ex_1().bp;
            ++bp->refcount;
            break;
        case 0:
            bp = _ex0().bp;
            ++bp->refcount;
            break;
        case 1:
            bp = _ex1().bp;
            ++bp->refcount;
            break;
        default:
            construct_from_basic(numeric(i));
    }
}

ex::ex(unsigned int const i)
{
    debugmsg("ex constructor from unsigned int",LOGLEVEL_CONSTRUCT);
    construct_from_basic(numeric(i));
}

ex::ex(long const i)
{
    debugmsg("ex constructor from long",LOGLEVEL_CONSTRUCT);
    construct_from_basic(numeric(i));
}

ex::ex(unsigned long const i)
{
    debugmsg("ex constructor from unsigned long",LOGLEVEL_CONSTRUCT);
    construct_from_basic(numeric(i));
}

ex::ex(double const d)
{
    debugmsg("ex constructor from double",LOGLEVEL_CONSTRUCT);
    construct_from_basic(numeric(d));
}
    
//////////
// functions overriding virtual functions from bases classes
//////////

// none

//////////
// new virtual functions which can be overridden by derived classes
//////////

// none

//////////
// non-virtual functions in this class
//////////

// public

/** Swap the contents of two expressions. */
void ex::swap(ex & other)
{
    debugmsg("ex swap",LOGLEVEL_MEMBER_FUNCTION);

    GINAC_ASSERT(bp!=0);
    GINAC_ASSERT(bp->flags & status_flags::dynallocated);
    GINAC_ASSERT(other.bp!=0);
    GINAC_ASSERT(other.bp->flags & status_flags::dynallocated);
    
    basic * tmpbp=bp;
    bp=other.bp;
    other.bp=tmpbp;
}

/** Output formatted to be useful as ginsh input. */
void ex::print(ostream & os, unsigned upper_precedence) const
{
    debugmsg("ex print",LOGLEVEL_PRINT);
    GINAC_ASSERT(bp!=0);
    bp->print(os,upper_precedence);
}

void ex::printraw(ostream & os) const
{
    debugmsg("ex printraw",LOGLEVEL_PRINT);
    GINAC_ASSERT(bp!=0);
    os << "ex(";
    bp->printraw(os);
    os << ")";
}

void ex::printtree(ostream & os, unsigned indent) const
{
    debugmsg("ex printtree",LOGLEVEL_PRINT);
    GINAC_ASSERT(bp!=0);
    // os << "refcount=" << bp->refcount << " ";
    bp->printtree(os,indent);
}

/** Print expression as a C++ statement. The output looks like
 *  "<type> <var_name> = <expression>;". The "type" parameter has an effect
 *  on how number literals are printed.
 *
 *  @param os output stream
 *  @param type variable type (one of the csrc_types)
 *  @param var_name variable name to be printed */
void ex::printcsrc(ostream & os, unsigned type, const char *var_name) const
{
    debugmsg("ex print csrc", LOGLEVEL_PRINT);
    GINAC_ASSERT(bp!=0);
    switch (type) {
        case csrc_types::ctype_float:
            os << "float ";
            break;
        case csrc_types::ctype_double:
            os << "double ";
            break;
        case csrc_types::ctype_cl_N:
            os << "cl_N ";
            break;
    }
    os << var_name << " = ";
    bp->printcsrc(os, type, 0);
    os << ";\n";
}

/** Little wrapper arount print to be called within a debugger. */
void ex::dbgprint(void) const
{
    debugmsg("ex dbgprint",LOGLEVEL_PRINT);
    GINAC_ASSERT(bp!=0);
    bp->dbgprint();
}

/** Little wrapper arount printtree to be called within a debugger. */
void ex::dbgprinttree(void) const
{
    debugmsg("ex dbgprinttree",LOGLEVEL_PRINT);
    GINAC_ASSERT(bp!=0);
    bp->dbgprinttree();
}

bool ex::info(unsigned inf) const
{
    if (inf == info_flags::normal_form) {

    // Polynomials are in normal form
    if (info(info_flags::polynomial))
        return true;

    // polynomial^(-int) is in normal form
    if (is_ex_exactly_of_type(*this, power))
        return op(1).info(info_flags::negint);

    // polynomial^(int) * polynomial^(int) * ... is in normal form
    if (!is_ex_exactly_of_type(*this, mul))
        return false;
    for (int i=0; i<nops(); i++) {
        if (is_ex_exactly_of_type(op(i), power)) {
            if (!op(i).op(1).info(info_flags::integer))
                return false;
            if (!op(i).op(0).info(info_flags::polynomial))
                return false;
        } else
            if (!op(i).info(info_flags::polynomial))
                return false;
    }
    return true;
    } else {
        return bp->info(inf);
    }
}

int ex::nops() const
{
    GINAC_ASSERT(bp!=0);
    return bp->nops();
}

ex ex::expand(unsigned options) const
{
    GINAC_ASSERT(bp!=0);
    return bp->expand(options);
}

bool ex::has(ex const & other) const
{
    GINAC_ASSERT(bp!=0);
    return bp->has(other);
}

int ex::degree(symbol const & s) const
{
    GINAC_ASSERT(bp!=0);
    return bp->degree(s);
}

int ex::ldegree(symbol const & s) const
{
    GINAC_ASSERT(bp!=0);
    return bp->ldegree(s);
}

ex ex::coeff(symbol const & s, int const n) const
{
    GINAC_ASSERT(bp!=0);
    return bp->coeff(s,n);
}

ex ex::numer(bool normalize) const
{
    ex n;
    if (normalize && !info(info_flags::normal_form))
        n = normal();
    else
        n = *this;

    // polynomial
    if (n.info(info_flags::polynomial))
        return n;

    // something^(-int)
    if (is_ex_exactly_of_type(n, power) && n.op(1).info(info_flags::negint))
        return _ex1();

    // something^(int) * something^(int) * ...
    if (!is_ex_exactly_of_type(n, mul))
        return n;
    ex res = _ex1();
    for (int i=0; i<n.nops(); i++) {
        if (!is_ex_exactly_of_type(n.op(i), power) || !n.op(i).op(1).info(info_flags::negint))
            res *= n.op(i);
    }
    return res;
}

ex ex::denom(bool normalize) const
{
    ex n;
    if (normalize && !info(info_flags::normal_form))
        n = normal();
    else
        n = *this;

    // polynomial
    if (n.info(info_flags::polynomial))
        return _ex1();

    // something^(-int)
    if (is_ex_exactly_of_type(n, power) && n.op(1).info(info_flags::negint))
        return power(n.op(0), -(n.op(1)));

    // something^(int) * something^(int) * ...
    if (!is_ex_exactly_of_type(n, mul))
        return _ex1();
    ex res = _ex1();
    for (int i=0; i<n.nops(); i++) {
        if (is_ex_exactly_of_type(n.op(i), power) && n.op(i).op(1).info(info_flags::negint))
            res *= power(n.op(i), -1);
    }
    return res;
}

ex ex::collect(symbol const & s) const
{
    GINAC_ASSERT(bp!=0);
    return bp->collect(s);
}

ex ex::eval(int level) const
{
    GINAC_ASSERT(bp!=0);
    return bp->eval(level);
}

ex ex::evalf(int level) const
{
    GINAC_ASSERT(bp!=0);
    return bp->evalf(level);
}

ex ex::subs(lst const & ls, lst const & lr) const
{
    GINAC_ASSERT(bp!=0);
    return bp->subs(ls,lr);
}

ex ex::subs(ex const & e) const
{
    GINAC_ASSERT(bp!=0);
    return bp->subs(e);
}

exvector ex::get_indices(void) const
{
    GINAC_ASSERT(bp!=0);
    return bp->get_indices();
}

ex ex::simplify_ncmul(exvector const & v) const
{
    GINAC_ASSERT(bp!=0);
    return bp->simplify_ncmul(v);
}

ex ex::operator[](ex const & index) const
{
    debugmsg("ex operator[ex]",LOGLEVEL_OPERATOR);
    GINAC_ASSERT(bp!=0);
    return (*bp)[index];
}

ex ex::operator[](int const i) const
{
    debugmsg("ex operator[int]",LOGLEVEL_OPERATOR);
    GINAC_ASSERT(bp!=0);
    return (*bp)[i];
}

ex ex::op(int const i) const
{
    debugmsg("ex op()",LOGLEVEL_MEMBER_FUNCTION);
    GINAC_ASSERT(bp!=0);
    return bp->op(i);
}

ex & ex::let_op(int const i)
{
    debugmsg("ex let_op()",LOGLEVEL_MEMBER_FUNCTION);
    makewriteable();
    GINAC_ASSERT(bp!=0);
    return bp->let_op(i);
}

#ifndef INLINE_EX_CONSTRUCTORS
int ex::compare(ex const & other) const
{
    GINAC_ASSERT(bp!=0);
    GINAC_ASSERT(other.bp!=0);
    if (bp==other.bp) {
        // special case: both expression point to same basic, trivially equal
        return 0; 
    }
    return bp->compare(*other.bp);
}
#endif // ndef INLINE_EX_CONSTRUCTORS

#ifndef INLINE_EX_CONSTRUCTORS
bool ex::is_equal(ex const & other) const
{
    GINAC_ASSERT(bp!=0);
    GINAC_ASSERT(other.bp!=0);
    if (bp==other.bp) {
        // special case: both expression point to same basic, trivially equal
        return true; 
    }
    return bp->is_equal(*other.bp);
}
#endif // ndef INLINE_EX_CONSTRUCTORS

unsigned ex::return_type(void) const
{
    GINAC_ASSERT(bp!=0);
    return bp->return_type();
}

unsigned ex::return_type_tinfo(void) const
{
    GINAC_ASSERT(bp!=0);
    return bp->return_type_tinfo();
}

unsigned ex::gethash(void) const
{
    GINAC_ASSERT(bp!=0);
    return bp->gethash();
}

ex ex::exadd(ex const & rh) const
{
    return (new add(*this,rh))->setflag(status_flags::dynallocated);
}

ex ex::exmul(ex const & rh) const
{
    return (new mul(*this,rh))->setflag(status_flags::dynallocated);
}

ex ex::exncmul(ex const & rh) const
{
    return (new ncmul(*this,rh))->setflag(status_flags::dynallocated);
}

// private

void ex::makewriteable()
{
    debugmsg("ex makewriteable",LOGLEVEL_MEMBER_FUNCTION);
    GINAC_ASSERT(bp!=0);
    GINAC_ASSERT(bp->flags & status_flags::dynallocated);
    if (bp->refcount > 1) {
        basic * bp2=bp->duplicate();
        ++bp2->refcount;
        bp2->setflag(status_flags::dynallocated);
        --bp->refcount;
        bp=bp2;
    }
    GINAC_ASSERT(bp->refcount == 1);
}    

void ex::construct_from_basic(basic const & other)
{
    if ((other.flags & status_flags::evaluated)==0) {
        // cf. copy constructor
        ex const & tmpex = other.eval(1); // evaluate only one (top) level
        bp = tmpex.bp;
        GINAC_ASSERT(bp!=0);
        GINAC_ASSERT(bp->flags & status_flags::dynallocated);
        ++bp->refcount;
        if ((other.flags & status_flags::dynallocated)&&(other.refcount==0)) {
            delete &const_cast<basic &>(other);
        }
    } else {
        if (other.flags & status_flags::dynallocated) {
            bp=&const_cast<basic &>(other);
        } else {
            bp=other.duplicate();
            bp->setflag(status_flags::dynallocated);
        }
        GINAC_ASSERT(bp!=0);
        // bp->clearflag(status_flags::evaluated);
        ++bp->refcount;
    }
    GINAC_ASSERT(bp!=0);
    GINAC_ASSERT(bp->flags & status_flags::dynallocated);
}

//////////
// static member variables
//////////

// none

//////////
// functions which are not member functions
//////////

// none

//////////
// global functions
//////////

// none


#ifndef NO_GINAC_NAMESPACE
} // namespace GiNaC
#endif // ndef NO_GINAC_NAMESPACE
