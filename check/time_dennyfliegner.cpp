/** @file time_dennyfliegner.cpp
 *
 *  The first test routine implements Denny Fliegner's quick consistency check:
 *  1)  e = (a0 + a1 + a2 + a3 + ...)^2, in expanded form
 *  2)  substitute a0 by (-a2 - a3 - ...) in e
 *  3)  expand e
 *  after which e should be just a1^2. */

/*
 *  GiNaC Copyright (C) 1999-2000 Johannes Gutenberg University Mainz, Germany
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

#include "times.h"

#define VECSIZE 200

static unsigned expand_subs(unsigned size)
{
    unsigned result = 0;
    symbol a1("a1");
    symbol a[VECSIZE];
    ex e, aux;
    
    a[1] = a1;
    for (unsigned i=0; i<size; ++i) {
        e = e + a[i];
    }
    
    // prepare aux so it will swallow anything but a1^2:
    aux = -e + a[0] + a[1];
    e = expand(subs(expand(pow(e, 2)), a[0] == aux));
    
    if (e != pow(a1,2)) {
        clog << "Denny Fliegner's quick consistency check erroneously returned "
             << e << "." << endl;
        ++result;
    }
    
    return result;
}

unsigned time_dennyfliegner(void)
{
    unsigned result = 0;
    
    cout << "timing commutative expansion and substitution" << flush;
    clog << "-------commutative expansion and substitution:" << endl;
    
    vector<unsigned> sizes;
    vector<double> times;
    timer rolex;
    
    sizes.push_back(25);
    sizes.push_back(50);
    sizes.push_back(100);
    sizes.push_back(200);
    
    for (vector<unsigned>::iterator i=sizes.begin(); i!=sizes.end(); ++i) {
        rolex.start();
        result += expand_subs(*i);
        times.push_back(rolex.read());
        cout << '.' << flush;
    }
    
    if (!result) {
        cout << " passed ";
        clog << "(no output)" << endl;
    } else {
        cout << " failed ";
    }
    // print the report:
    cout << endl << "    size:  ";
    for (vector<unsigned>::iterator i=sizes.begin(); i!=sizes.end(); ++i)
        cout << '\t' << (*i);
    cout << endl << "    time/s:";
    for (vector<double>::iterator i=times.begin(); i!=times.end(); ++i)
        cout << '\t' << int(1000*(*i))*0.001;
    cout << endl;
    
    return result;
}