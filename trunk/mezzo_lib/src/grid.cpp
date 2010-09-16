#include "grid.h"


Grid::Grid(const int nr_fields_,const vector<string> names_):nr_fields(nr_fields_),fnames(names_)
{}

void Grid::reset()
{
	grid.clear();
}
const bool Grid::insert_row(const list <double> & values)
{
    grid.insert(grid.end(), values);
    return true;
}

const bool Grid::write_empty(ostream& out)
{
#ifdef INCLUDE_FIELD_NAMES
	for (vector<string>::iterator iter0=fnames.begin();iter0<fnames.end();iter0++)
		out << *iter0 << "\t";
	out << endl;
#endif //INCLUDE_FIELD_NAMES
	list<double> row;
	while (!grid.empty())
	{
		row=(*grid.begin());
		while (!row.empty())
		{
			out << row.front() << "\t";
			row.pop_front();
		}
		grid.pop_front();
		out << endl;
	}
	return true;
}

const double Grid::sum (const int column)
{
	list<list <double> >::iterator iter0;
	list <double>::iterator iter1;
	double total=0.0;
	iter0=grid.begin();
	while (iter0 != grid.end())
	{
		iter1=(*iter0).begin();
		for (int  i=1; i<column; i++)
		{
			iter1++;
		}
		total+=(*iter1);
		iter0++;
	}
	return total;
}

const double Grid::average (const int column)
{
	return (sum(column)/grid.size());
}


// MOE functions
MOE::MOE (const double val_update, const double default_val_)

{
	values.push_back(default_val_);
	value_iter=values.begin();
	value_update=val_update;
	value_period=1;
	value_obs=0;
	scale=1.0;
	default_val=default_val_;
}

MOE::MOE (const double val_update, const double scale_, const double default_val_)
{
  	values.push_back(default_val_);
	value_iter=values.begin();
	value_update=val_update;
	value_period=1;
	value_obs=0;
	scale=scale_;
	default_val=default_val_;

}

void MOE::reset()
{	
	values.clear();
	values.push_back(default_val);
	value_iter=values.begin();
	value_period=1;
	value_obs=0;

}
void MOE::report_value(const double value, const double time)
{
	 	if (time > (value_period*value_update))
 		 	{
 		 		for (int i=0; i<(static_cast<int>(time/value_update)-value_period+1); i++) // for all previous periods
 		 		{
 		 	 		value_period++;
 		 	 		values.push_back(default_val); // add a new interval to the list, initialized with default value
 		 	 		value_iter++;
 		 	 		value_obs=0;
 		 	 	}
 		 	}
 		 	value_obs++;
 		    if (value_obs== 1) // if this is the first value for this period
 		    	*value_iter=value;
 		    else
 		    	*value_iter=((*value_iter)*(value_obs-1)+value ) /value_obs; 		 	
}


void MOE::report_value(const double time) // for flows
{
	 	if (time > (value_period*value_update))
 		 	{
 		 		for (int i=0; i<(static_cast<int>(time/value_update)-value_period+1); i++)
 		 		{
 		 	 		value_period++;
 		 	 		values.push_back(0.0); // add a new interval to the list
 		 	 		value_iter++;
 		 	 		value_obs=0;
 		 	 	}
 		 	}
 		    value_obs++;
 		    *value_iter=static_cast <double> (value_obs); 		 	
}

const double MOE::get_value(const int index)
{
  if (values.empty())
    return -1.0;
  else
  {
    list <double>::iterator iter=values.begin();
    for (int i=0;i<index;i++)
    {
      if (iter ==values.end() )
        break;
      else
  	 	 iter++;
    }
    if ( iter	!= values.end() )	
        return (*iter);
    else
        return -1.0;
  }
}

const double MOE::get_last_value()
{
 if (!(values.empty()))
   return values.back();
 else
   return -1.0;
}

const double MOE::get_min()
{
	if (values.empty())
		return -1.0;
	else
	{
		double temp = values.front();
		for (list<double>::iterator iter = values.begin(); iter!= values.end(); iter++)
			{
				if ((*iter) < temp)
				{
					temp = (*iter);
				}
			}			
		return temp;
	}
}
const double MOE::get_max()
{
	if (values.empty())
		return -1.0;
	else
	{
		double temp = values.front();
		for (list<double>::iterator iter = values.begin(); iter!= values.end(); iter++)
			{
				if ((*iter) > temp)
				{
					temp = (*iter);
				}
			}			
		return temp;
	}

}
void MOE::fill_missing (const int nr_periods, const double default_value)
{
	if (value_obs==0)
	{
		values.back()=default_value;
	}
	if ((int)values.size() < nr_periods)
	{
		list <double>::iterator iter=values.begin();
		for (int i = values.size()-1; i != nr_periods; i++)
		{	
			values.push_back(default_value);

		}
	}



}

void MOE::write_values(ostream & out, const int nrperiods)
{
  #ifdef _DEBUG_MOE
  eout << "values write size: " << values.size() << endl;
  #endif
  list <double>::iterator iter=values.begin();
  for  (int i = 0; (i < nrperiods) && (iter!=values.end()); i++)
  {
   out << (scale*(*iter)) << "\t";
   iter++;
  }
  out << endl;
}

void MOE::write_value(ostream& out, const int index)
{
  list <double>::iterator iter=values.begin();
  for (int i=0;i<index;i++)
  {  		
  	
  	 if (iter ==values.end() )
  	 	break;
  	 else
  	 	 iter++;
  }		
  		
  if ( iter	!= values.end() )	
	  out << (scale*(*iter)) << "\t" ;
	else
		out << "\t" ;

}



