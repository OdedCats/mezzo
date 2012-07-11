#include "parameters.h"
#include <iostream>




Parameters::Parameters ()


{  
//#drawing_parameters
//#simulation view
   draw_link_ids = false;
   link_thickness = 3;
   node_thickness = 2;
   node_radius = 6;
   queue_thickness = 8;
   selected_thickness = 6;
   text_size = 12;
   show_background = true;
   background_x=0;
   background_y=0;
   background_scale=5;
#ifndef _NO_GUI
   linkcolor=Qt::gray;
   nodecolor=Qt::gray;
   queuecolor=Qt::red;
   backgroundcolor=Qt::white;
   selectedcolor=Qt::green;
#endif
   gui_update_step= 0.2;
   zerotime= 27000; // simulation starts standard at 7:30
// #output_view
   thickness_width= 20;
   cutoff= 5;
   show_link_names= false;
   show_link_ids= false;
   show_data_values= false;
//#moe_parameters 
   moe_speed_update= 300.0;
   moe_inflow_update= 300.0;
   moe_outflow_update= 300.0;
   moe_queue_update= 300.0;
   moe_density_update= 300.0;
   linktime_alpha= 0.6;    // REPLACES the time_alpha
// #assignment_matrix_parameters
   use_ass_matrix= 0;
   ass_link_period= 900.0;
   ass_od_period= 900.0;
// #turning_parameters
   default_lookback_size= 20;
   turn_penalty_cost= 99999.0;
   use_giveway= false;
   max_wait= 5.0;
   critical_gap= 4.1;

   // NEW
   min_headway_inflow= 1.44;

// #server_parameters
   od_servers_deterministic= 1;
   odserver_sigma= 0.2; // unused for the moment
// NEW:
   implicit_nr_servers= false;
// #vehicle_parameters
   standard_veh_length= 7;
// #route_parameters
   update_interval_routes= 600.0;
   mnl_theta = -0.00417;
   kirchoff_alpha = -1.0;
   delete_bad_routes= false;
   max_rel_route_cost = 2.0; // routes that are more expensive are pruned
   small_od_rate = 3.0;
// NEW:
   use_linktime_disturbances= true;
   linktime_disturbance= 0.1; // adds an amount (factor) of disturbance to link times during route search
   routesearch_random_draws= 3;
   scale_demand= false;
   scale_demand_factor= 0.5; // diminish the demand by this factor to avoid gridlock and crazy routes during route search.
   renum_routes= true; // renumber the routes
   overwrite_histtimes= false; // overwrite the histtimes file after running simulation

 // #mime_parameters
   mime_comm_step= 0.1;
   mime_min_queue_length= 20;
   mime_queue_dis_speed= 6.0;
   vissim_step= 0.1;
   sim_speed_factor= 0.0;

 //#iteration_control
   max_iter=10;
   rel_gap_threshold=0.02;
   // following are not yet implemented in file. testing their use first
   //NEW:
   max_route_iter=3; 
 
   	//state vars
   shortest_paths_initialised = false;
   veh_in_network = 0;
   // stat vars Output view
   viewmode = 0; // show simulation run
   max_thickness_value=1;
   max_colour_value=1;
   min_thickness_value=1;
   min_colour_value=1;
   inverse_colour_scale=false;
   show_period = 1; // maybe later start at 0 with empty results (everything 0)
   
   
   
}

bool Parameters::read_parameters (istream & in )
{
	string keyword, temp;
	in >> keyword;
	if (keyword!= "#drawing_parameters")
	{
		eout << "ERROR reading Parameters file, expecting: #drawing_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "#simulation_view")
	{
		eout << "ERROR reading Parameters file, expecting: #simulation_view, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "draw_link_ids=")
	{
		eout << "ERROR reading Parameters file, expecting: draw_link_ids=, read: " << keyword << endl;
		return false;
	}
	in >> draw_link_ids;
	
	in >> keyword;
	if (keyword!= "link_thickness=")
	{
		eout << "ERROR reading Parameters file, expecting: link_thickness=, read: " << keyword << endl;
		return false;
	}
	in >> link_thickness;

	in >> keyword;
	if (keyword!= "node_thickness=")
	{
		eout << "ERROR reading Parameters file, expecting: node_thickness=, read: " << keyword << endl;
		return false;
	}
	in >> node_thickness;

	in >> keyword;
	if (keyword!= "node_radius=")
	{
		eout << "ERROR reading Parameters file, expecting: node_radius=, read: " << keyword << endl;
		return false;
	}
	in >> node_radius;

	in >> keyword;
	if (keyword!= "queue_thickness=")
	{
		eout << "ERROR reading Parameters file, expecting: queue_thickness=, read: " << keyword << endl;
		return false;
	}
	in >> queue_thickness;

	
	in >> keyword;
	if (keyword!= "selected_thickness=")
	{
		eout << "ERROR reading Parameters file, expecting: selected_thickness=, read: " << keyword << endl;
		return false;
	}
	in >> selected_thickness;
	
	in >> keyword;
	if (keyword!= "text_size=")
	{
		eout << "ERROR reading Parameters file, expecting: text_size=, read: " << keyword << endl;
		return false;
	}
	in >> text_size;

	in >> keyword;
	if (keyword!= "show_background=")
	{
		eout << "ERROR reading Parameters file, expecting: show_background=, read: " << keyword << endl;
		return false;
	}
	in >> show_background;

	in >> keyword;
	if (keyword!= "background_x=")
	{
		eout << "ERROR reading Parameters file, expecting: background_x=, read: " << keyword << endl;
		return false;
	}
	in >> background_x;

	in >> keyword;
	if (keyword!= "background_y=")
	{
		eout << "ERROR reading Parameters file, expecting: background_y=, read: " << keyword << endl;
		return false;
	}
	in >> background_y;

	in >> keyword;
	if (keyword!= "background_scale=")
	{
		eout << "ERROR reading Parameters file, expecting: background_scale=, read: " << keyword << endl;
		return false;
	}
	in >> background_scale;

	in >> keyword;
	if (keyword!= "linkcolor=")
	{
		eout << "ERROR reading Parameters file, expecting: linkcolor=, read: " << keyword << endl;
		return false;
	}
	in >> temp;
#ifndef _NO_GUI
	linkcolor.setNamedColor(QString (temp.c_str()));
#endif 

	in >> keyword;
	if (keyword!= "nodecolor=")
	{
		eout << "ERROR reading Parameters file, expecting: nodecolor=, read: " << keyword << endl;
		return false;
	}
	in >> temp;
#ifndef _NO_GUI
	nodecolor.setNamedColor(QString (temp.c_str()));
#endif 
	
	in >> keyword;
	if (keyword!= "queuecolor=")
	{
		eout << "ERROR reading Parameters file, expecting: queuecolor=, read: " << keyword << endl;
		return false;
	}
	in >> temp;
#ifndef _NO_GUI
	queuecolor.setNamedColor(QString (temp.c_str()));
#endif 

	in >> keyword;
	if (keyword!= "backgroundcolor=")
	{
		eout << "ERROR reading Parameters file, expecting: backgroundcolor=, read: " << keyword << endl;
		return false;
	}
	in >> temp;
#ifndef _NO_GUI
	backgroundcolor.setNamedColor(QString (temp.c_str()));
#endif 

	in >> keyword;
	if (keyword!= "selectedcolor=")
	{
		eout << "ERROR reading Parameters file, expecting: selectedcolor=, read: " << keyword << endl;
		return false;
	}

	in >> temp;
#ifndef _NO_GUI
	selectedcolor.setNamedColor(QString (temp.c_str()));
#endif 

	in >> keyword;
	if (keyword!= "gui_update_step=")
	{
		eout << "ERROR reading Parameters file, expecting: gui_update_step=, read: " << keyword << endl;
		return false;
	}
	in >> gui_update_step;

	in >> keyword;
	if (keyword!= "zerotime=")
	{
		eout << "ERROR reading Parameters file, expecting: zerotime=, read: " << keyword << endl;
		return false;
	}
	in >> zerotime;
	
	in >> keyword;
	if (keyword!= "#output_view")
	{
		eout << "ERROR reading Parameters file, expecting: #output_view, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "thickness_width=")
	{
		eout << "ERROR reading Parameters file, expecting: thickness_width=, read: " << keyword << endl;
		return false;
	}
	in >> thickness_width;

	in >> keyword;
	if (keyword!= "cutoff=")
	{
		eout << "ERROR reading Parameters file, expecting: cutoff=, read: " << keyword << endl;
		return false;
	}
	in >> cutoff;

	in >> keyword;
	if (keyword!= "show_link_names=")
	{
		eout << "ERROR reading Parameters file, expecting: show_link_names=, read: " << keyword << endl;
		return false;
	}
	in >> show_link_names;

	in >> keyword;
	if (keyword!= "show_link_ids=")
	{
		eout << "ERROR reading Parameters file, expecting: show_link_ids=, read: " << keyword << endl;
		return false;
	}
	in >> show_link_ids;

	in >> keyword;
	if (keyword!= "show_data_values=")
	{
		eout << "ERROR reading Parameters file, expecting: show_data_values=, read: " << keyword << endl;
		return false;
	}
	in >> show_data_values;

	in >> keyword;
	if (keyword!= "#moe_parameters")
	{
		eout << "ERROR reading Parameters file, expecting: #moe_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "moe_speed_update=")
	{
		eout << "ERROR reading Parameters file, expecting: moe_speed_update=, read: " << keyword << endl;
		return false;
	}
	in >> moe_speed_update;

	in >> keyword;
	if (keyword!= "moe_inflow_update=")
	{
		eout << "ERROR reading Parameters file, expecting: moe_inflow_update=, read: " << keyword << endl;
		return false;
	}
	in >> moe_inflow_update;

	in >> keyword;
	if (keyword!= "moe_outflow_update=")
	{
		eout << "ERROR reading Parameters file, expecting: moe_outflow_update=, read: " << keyword << endl;
		return false;
	}
	in >> moe_outflow_update;

	in >> keyword;
	if (keyword!= "moe_queue_update=")
	{
		eout << "ERROR reading Parameters file, expecting: moe_queue_update=, read: " << keyword << endl;
		return false;
	}
	in >> moe_queue_update;

	in >> keyword;
	if (keyword!= "moe_density_update=")
	{
		eout << "ERROR reading Parameters file, expecting: moe_density_update=, read: " << keyword << endl;
		return false;
	}
	in >> moe_density_update;

	in >> keyword;
	if (keyword!= "linktime_alpha=")
	{
		eout << "ERROR reading Parameters file, expecting: linktime_alpha=, read: " << keyword << endl;
		return false;
	}
	in >> linktime_alpha;

///////
	in >> keyword;
	if (keyword!= "#assignment_matrix_parameters")
	{
		eout << "ERROR reading Parameters file, expecting: #assignment_matrix_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "use_ass_matrix=")
	{
		eout << "ERROR reading Parameters file, expecting: use_ass_matrix=, read: " << keyword << endl;
		return false;
	}
	in >> use_ass_matrix;

	in >> keyword;
	if (keyword!= "ass_link_period=")
	{
		eout << "ERROR reading Parameters file, expecting: ass_link_period=, read: " << keyword << endl;
		return false;
	}
	in >> ass_link_period;

	in >> keyword;
	if (keyword!= "ass_od_period=")
	{
		eout << "ERROR reading Parameters file, expecting: ass_od_period=, read: " << keyword << endl;
		return false;
	}
	in >> ass_od_period;

	in >> keyword;
	if (keyword!= "#turning_parameters")
	{
		eout << "ERROR reading Parameters file, expecting: #turning_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "default_lookback_size=")
	{
		eout << "ERROR reading Parameters file, expecting: default_lookback_size=, read: " << keyword << endl;
		return false;
	}
	in >> default_lookback_size;

	in >> keyword;
	if (keyword!= "turn_penalty_cost=")
	{
		eout << "ERROR reading Parameters file, expecting: turn_penalty_cost=, read: " << keyword << endl;
		return false;
	}
	in >> turn_penalty_cost;

	in >> keyword;
	if (keyword!= "use_giveway=")
	{
		eout << "ERROR reading Parameters file, expecting: use_giveway=, read: " << keyword << endl;
		return false;
	}
	in >> use_giveway;

	in >> keyword;
	if (keyword!= "max_wait=")
	{
		eout << "ERROR reading Parameters file, expecting: max_wait=, read: " << keyword << endl;
		return false;
	}
	in >> max_wait;


	in >> keyword;
	if (keyword!= "min_headway_inflow=")
	{
		eout << "ERROR reading Parameters file, expecting: min_headway_inflow=, read: " << keyword << endl;
		return false;
	}
	in >> min_headway_inflow;
	

	in >> keyword;
	if (keyword!= "#server_parameters")
	{
		eout << "ERROR reading Parameters file, expecting: #server_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "od_servers_deterministic=")
	{
		eout << "ERROR reading Parameters file, expecting: od_servers_deterministic=, read: " << keyword << endl;
		return false;
	}
	in >> od_servers_deterministic;

	in >> keyword;
	if (keyword!= "odserver_sigma=")
	{
		eout << "ERROR reading Parameters file, expecting: odserver_sigma=, read: " << keyword << endl;
		return false;
	}
	in >> odserver_sigma;

	in >> keyword;
	if (keyword!= "implicit_nr_servers=")
	{
		eout << "ERROR reading Parameters file, expecting: implicit_nr_servers=, read: " << keyword << endl;
		return false;
	}
	in >> implicit_nr_servers;

	in >> keyword;
	if (keyword!= "#vehicle_parameters")
	{
		eout << "ERROR reading Parameters file, expecting: #vehicle_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "standard_veh_length=")
	{
		eout << "ERROR reading Parameters file, expecting: standard_veh_length=, read: " << keyword << endl;
		return false;
	}
	in >> standard_veh_length;

	in >> keyword;
	if (keyword!= "#route_parameters")
	{
		eout << "ERROR reading Parameters file, expecting: #route_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "update_interval_routes=")
	{
		eout << "ERROR reading Parameters file, expecting: update_interval_routes=, read: " << keyword << endl;
		return false;
	}
	in >> update_interval_routes;

	in >> keyword;
	if (keyword!= "mnl_theta=")
	{
		eout << "ERROR reading Parameters file, expecting: mnl_theta=, read: " << keyword << endl;
		return false;
	}
	in >> mnl_theta;

    in >> keyword;
	if (keyword!= "kirchoff_alpha=")
	{
		eout << "ERROR reading Parameters file, expecting: kirchoff_alpha=, read: " << keyword << endl;
		return false;
	}
	in >> kirchoff_alpha;

	in >> keyword;
	if (keyword!= "delete_bad_routes=")
	{
		eout << "ERROR reading Parameters file, expecting: delete_bad_routes=, read: " << keyword << endl;
		return false;
	}
	in >> delete_bad_routes;

    in >> keyword;
	if (keyword!= "max_rel_route_cost=")
	{
		eout << "ERROR reading Parameters file, expecting: max_rel_route_cost=, read: " << keyword << endl;
		return false;
	}
	in >> max_rel_route_cost;

	in >> keyword;
	if (keyword!= "small_od_rate=")
	{
		eout << "ERROR reading Parameters file, expecting: small_od_rate=, read: " << keyword << endl;
		return false;
	}
	in >> small_od_rate;

	in >> keyword;
	if (keyword!= "use_linktime_disturbances=")
	{
		eout << "ERROR reading Parameters file, expecting: use_linktime_disturbances=, read: " << keyword << endl;
		return false;
	}
	in >> use_linktime_disturbances;

	in >> keyword;
	if (keyword!= "linktime_disturbance=")
	{
		eout << "ERROR reading Parameters file, expecting: linktime_disturbance=, read: " << keyword << endl;
		return false;
	}
	in >> linktime_disturbance;

	in >> keyword;
	if (keyword!= "routesearch_random_draws=")
	{
		eout << "ERROR reading Parameters file, expecting: routesearch_random_draws=, read: " << keyword << endl;
		return false;
	}
	in >> routesearch_random_draws;

	in >> keyword;
	if (keyword!= "scale_demand=")
	{
		eout << "ERROR reading Parameters file, expecting: scale_demand=, read: " << keyword << endl;
		return false;
	}
	in >> scale_demand;

	in >> keyword;
	if (keyword!= "scale_demand_factor=")
	{
		eout << "ERROR reading Parameters file, expecting: scale_demand_factor=, read: " << keyword << endl;
		return false;
	}
	in >> scale_demand_factor;

	in >> keyword;
	if (keyword!= "renum_routes=")
	{
		eout << "ERROR reading Parameters file, expecting: renum_routes=, read: " << keyword << endl;
		return false;
	}
	in >> renum_routes;

	in >> keyword;
	if (keyword!= "overwrite_histtimes=")
	{
		eout << "ERROR reading Parameters file, expecting: overwrite_histtimes=, read: " << keyword << endl;
		return false;
	}
	in >> overwrite_histtimes;
	
	in >> keyword;
	if (keyword!= "#mime_parameters")
	{
		eout << "ERROR reading Parameters file, expecting: #mime_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "mime_comm_step=")
	{
		eout << "ERROR reading Parameters file, expecting: mime_comm_step=, read: " << keyword << endl;
		return false;
	}
	in >> mime_comm_step;

	in >> keyword;
	if (keyword!= "mime_min_queue_length=")
	{
		eout << "ERROR reading Parameters file, expecting: mime_min_queue_length=, read: " << keyword << endl;
		return false;
	}
	in >> mime_min_queue_length;

	in >> keyword;
	if (keyword!= "mime_queue_dis_speed=")
	{
		eout << "ERROR reading Parameters file, expecting: mime_queue_dis_speed=, read: " << keyword << endl;
		return false;
	}
	in >> mime_queue_dis_speed;

	in >> keyword;
	if (keyword!= "vissim_step=")
	{
		eout << "ERROR reading Parameters file, expecting: vissim_step=, read: " << keyword << endl;
		return false;
	}
	in >> vissim_step;

	in >> keyword;
	if (keyword!= "sim_speed_factor=")
	{
		eout << "ERROR reading Parameters file, expecting: sim_speed_factor=, read: " << keyword << endl;
		return false;
	}
	in >> sim_speed_factor;

	in >> keyword;
	if (keyword!= "#iteration_control")
	{
		eout << "WARNING reading Parameters file, expecting: #iteration_control, read: " << keyword << endl;
		
	}
	else
	{
		in >> keyword;
		if (keyword!= "max_iter=")
		{
			eout << "ERROR reading Parameters file, expecting: max_iter=, read: " << keyword << endl;
			return false;
		}
		in >> max_iter;
		
		in >> keyword;
		if (keyword!= "rel_gap_threshold=")
		{
			eout << "ERROR reading Parameters file, expecting: rel_gap_threshold=, read: " << keyword << endl;
			return false;
		}
		in >> rel_gap_threshold;
	
		in >> keyword;
		if (keyword!= "max_route_iter=")
		{
			eout << "ERROR reading Parameters file, expecting: max_route_iter=, read: " << keyword << endl;
			return false;
		}
		in >> max_route_iter;
	
	}

	return true;
}


void Parameters::write_parameters(ostream & out)
{
#ifndef _NO_GUI
   out << "#drawing_parameters" << endl;
   out << "#simulation_view" << endl;
   out << "  draw_link_ids= " << draw_link_ids << endl;
   out << "  link_thickness= " << link_thickness << endl;
   out << "  node_thickness= " << node_thickness << endl;
   out << "  node_radius= " << node_radius << endl;
   out << "  queue_thickness= " << queue_thickness << endl;	
   out << "  selected_thickness= " << selected_thickness << endl;
   out << "  text_size= " << text_size << endl;
   out << "  show_background= " << show_background << endl;
   out << "  background_x= " << background_x << endl;
   out << "  background_y= " << background_y << endl;
   out << "  background_scale= " << background_scale << endl;
   out << "  linkcolor= " << qPrintable(linkcolor.name()) << endl;
   out << "  nodecolor= " << qPrintable(nodecolor.name()) << endl;
   out << "  queuecolor= " << qPrintable(queuecolor.name()) << endl;
   out << "  backgroundcolor= " << qPrintable(backgroundcolor.name()) << endl;
   out << "  selectedcolor= " << qPrintable(selectedcolor.name()) << endl;
   out << "  gui_update_step= " << gui_update_step << endl;
   out << "  zerotime= " << zerotime << endl;
   out << "#output_view" << endl;
   out << "  thickness_width= " << thickness_width << endl;
   out << "  cutoff= " << cutoff << endl;
   out << "  show_link_names= " << show_link_names << endl;
   out << "  show_link_ids= " << show_link_ids << endl;
   out << "  show_data_values= " << show_data_values << endl;
   out << "#moe_parameters" << endl;
   out << "  moe_speed_update= " << moe_speed_update << endl;
   out << "  moe_inflow_update= " << moe_inflow_update << endl;
   out << "  moe_outflow_update= " << moe_outflow_update << endl;
   out << "  moe_queue_update= " << moe_queue_update << endl;
   out << "  moe_density_update= " << moe_density_update << endl;
   out << "  linktime_alpha= " << linktime_alpha << endl; // for future use, for now the one in the scenario file is used
   out << "#assignment_matrix_parameters" << endl;
   out << "  use_ass_matrix= " << use_ass_matrix << endl;
   out << "  ass_link_period= " << ass_link_period << endl;
   out << "  ass_od_period= " << ass_od_period << endl;
   out << "#turning_parameters" << endl;
   out << "  default_lookback_size= " << default_lookback_size << endl;
   out << "  turn_penalty_cost= " << turn_penalty_cost << endl;
   out << "  use_giveway= " << use_giveway << endl;
   out << "  max_wait= " << max_wait << endl;
   out << "  min_headway_inflow= " << min_headway_inflow << endl;
   out << "#server_parameters" << endl;
   out << "  od_servers_deterministic= " << od_servers_deterministic << endl;
   out << "  odserver_sigma= " << odserver_sigma << endl;
   out << "  implicit_nr_servers= " << odserver_sigma << endl;
   out << "#vehicle_parameters" << endl;
   out << "  standard_veh_length= " << standard_veh_length << endl;
   out << "#route_parameters" << endl;
   out << "  update_interval_routes= " << update_interval_routes << endl;
   out << "  mnl_theta= " << mnl_theta << endl;
   out << "  kirchoff_alpha= " << kirchoff_alpha << endl;
   out << "  delete_bad_routes= " << delete_bad_routes << endl;
   out << "  max_rel_route_cost= " << max_rel_route_cost << endl;
   out << "  use_linktime_disturbances= " << use_linktime_disturbances << endl;
   out << "  linktime_disturbance= " << linktime_disturbance << endl;
   out << "  routesearch_random_draws= " << routesearch_random_draws << endl;
   out << "  scale_demand= " << scale_demand << endl;
   out << "  scale_demand_factor= " << scale_demand_factor << endl;
   out << "  renum_routes= " << renum_routes << endl;
   out << "  overwrite_histtimes= " << overwrite_histtimes << endl;
   out << "#mime_parameters" << endl;
   out << "  mime_comm_step= " << mime_comm_step << endl;
   out << "  mime_min_queue_length= " << mime_min_queue_length << endl;
   out << "  mime_queue_dis_speed= " << mime_queue_dis_speed << endl;
   out << "  vissim_step= " << vissim_step << endl;
   out << "  sim_speed_factor= " << sim_speed_factor << endl;
   out << "#iteration_control" << endl;
   out << "  max_iter= " << max_iter << endl;
   out << "  rel_gap_threshold= " << rel_gap_threshold << endl;
   out << "  max_route_iter= " << max_route_iter << endl; 

#endif


}