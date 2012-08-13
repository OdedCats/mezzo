/*
	Mezzo Mesoscopic Traffic Simulation 
	Copyright (C) 2008  Wilco Burghout

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 Grid class defines a standard grid for storing things of the same type T
 somehow the template instantiation doesnt work @ linktime... so only for doubles


*/

#ifndef GRID_HH
#define GRID_HH
#include <list>
#include <string>
#include <vector>
#include "parameters.h"
#include <iostream>

//#define INCLUDE_FIELD_NAMES  // to include the field names in the output file for the Grids
//#define _DEBUG_MOE
using namespace std;

class Grid
{
    public:
		Grid() {}
      Grid(const int nr_fields_, const vector<string> names_);
	  Grid (const Grid & grid_): nr_fields(grid_.nr_fields), fnames(grid_.fnames), grid (grid_.grid) {}
	  ~Grid () {}
	  void reset(); // resets the values NOT the fieldnames!
      const bool insert_row(const list <double> & values); // reference to a list of values so theres no needless copying
      const bool write_empty(ostream& out);
      const double sum (const int column);
      const double average (const int column);
      const int size() {return grid.size();}
	  const vector<string> & get_fieldnames () {return fnames;}
     private:
      int nr_fields;
      vector<string> fnames;
	  list < list <double> > grid;
};

class MOE
	/* MOE class is used to collect output data (Measures Of Effectiveness), usually from Links
		

	*/
{
 public:
 	MOE (const double val_update, const double default_val_);
 	MOE (const double val_update, const double scale_, const double default_val_);
	void reset(); // resets the value list, NOT the scale_ or value_update
	void fill_missing (const int nr_periods,const double default_value); // fills out missing values for time periods without data
 	void report_value(const double value, const double time); // used to report values that are averaged
 	void report_value(const double time); // used to report counts such as flows
	const double get_value(int index);
	const double get_last_value();
	const double get_min();
	const double get_max();
 	void write_values(ostream & out, const int nrperiods);
 	void write_value(ostream& out, const int index);
 	const int get_size() {return values.size();}
 private:
	
   double value_update;
   double scale;
   double default_val;
   int value_obs;
   int value_period;
   list <double>::iterator value_iter;
   list <double> values;
};

#endif
