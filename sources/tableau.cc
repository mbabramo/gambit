//#
//# FILE: tableau.cc -- tableau and basis class instantiations
//#
//# $Id$
//#

#include "rational.h"
#include "tableau.imp"

#ifdef __GNUG__
#define TEMPLATE template
#elif defined __BORLANDC__
#define TEMPLATE
#pragma option -Jgd
#else
#error Unsupported compiler type
#endif // __GNUG__, __BORLANDC__

TEMPLATE class BaseTableau<double>;
TEMPLATE class BaseTableau<gRational>;

TEMPLATE class Tableau<double>;
TEMPLATE class Tableau<gRational>;

TEMPLATE class LPTableau<double>;
TEMPLATE class LPTableau<gRational>;

/*
TEMPLATE gOutput & operator<< {gOutput&, const Basis<double>&);
TEMPLATE gOutput & operator<< {gOutput&, const Basis<gRational>&);
TEMPLATE gOutput & operator<< {gOutput&, const Tableau<double>&);
TEMPLATE gOutput & operator<< {gOutput&, const Tableau<gRational>&);
*/

