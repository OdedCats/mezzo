#include "parameters.h"
#include <iostream>

Parameters::Parameters ()
// Later this will be read from a file

{
   viewmode = 0; // show simulation run
   max_thickness_value=1;
   max_colour_value=1;
   min_thickness_value=1;
   min_colour_value=1;
   inverse_colour_scale=false;
   show_period = 1; // maybe later start at 0 with empty results (everything 0)


   draw_link_ids = false;
   link_thickness = 3;
   node_thickness = 2;
   node_radius = 6;
   queue_thickness = 8;
   selected_thickness = 6;
   text_size = 16;
   show_background = true;
#ifndef _NO_GUI
   linkcolor=Qt::gray;
   nodecolor=Qt::gray;
   queuecolor=Qt::red;
   backgroundcolor=Qt::white;
   selectedcolor=Qt::green;
#endif
   gui_update_step= 0.2;
   zerotime = 27000; // simulation starts standard at 7:30
   moe_speed_update= 300.0;
   moe_inflow_update= 300.0;
   moe_outflow_update= 300.0;
   moe_queue_update= 300.0;
   moe_density_update= 300.0;
   // FOR USE LATER
   linktime_alpha= 0.2;
// #assignment_matrix_parameters
   use_ass_matrix= 1;
   ass_link_period= 1200.0;
   ass_od_period= 1200.0;
// #turning_parameters
   default_lookback_size= 20;
   turn_penalty_cost= 99999.0;
   use_giveway = false;
   max_wait = 5.0;
// #server_parameters
   od_servers_deterministic= 1;
   odserver_sigma= 0.2;
// #vehicle_parameters
   standard_veh_length= 7;
// #route_parameters
   update_interval_routes= 600.0;
   mnl_theta = -0.00417;
   kirchoff_alpha = -1.0;
   delete_bad_routes= false;
   max_rel_route_cost = 2.0;
   small_od_rate = 3.0;
   
	//state var
   shortest_paths_initialised = false;

// #mime_parameters
   mime_comm_step= 0.5;
   mime_min_queue_length= 20;
   mime_queue_dis_speed= 6.0;
   vissim_step= 0.1;
   sim_speed_factor= 2.0;
   
}

bool Parameters::read_parameters (istream & in )
{
	string keyword, temp;
	in >> keyword;
	if (keyword!= "#drawing_parameters")
	{
		cout << "ERROR reading Parameters file, expecting: #drawing_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "draw_link_ids=")
	{
		cout << "ERROR reading Parameters file, expecting: draw_link_ids=, read: " << keyword << endl;
		return false;
	}
	in >> draw_link_ids;
	
	in >> keyword;
	if (keyword!= "link_thickness=")
	{
		cout << "ERROR reading Parameters file, expecting: link_thickness=, read: " << keyword << endl;
		return false;
	}
	in >> link_thickness;

	in >> keyword;
	if (keyword!= "node_thickness=")
	{
		cout << "ERROR reading Parameters file, expecting: node_thickness=, read: " << keyword << endl;
		return false;
	}
	in >> node_thickness;

	in >> keyword;
	if (keyword!= "node_radius=")
	{
		cout << "ERROR reading Parameters file, expecting: node_radius=, read: " << keyword << endl;
		return false;
	}
	in >> node_radius;

	in >> keyword;
	if (keyword!= "queue_thickness=")
	{
		cout << "ERROR reading Parameters file, expecting: queue_thickness=, read: " << keyword << endl;
		return false;
	}
	in >> queue_thickness;

	
	in >> keyword;
	if (keyword!= "selected_thickness=")
	{
		cout << "ERROR reading Parameters file, expecting: selected_thickness=, read: " << keyword << endl;
		return false;
	}
	in >> selected_thickness;
	
	in >> keyword;
	if (keyword!= "show_background_image=")
	{
		cout << "ERROR reading Parameters file, expecting: show_background_image=, read: " << keyword << endl;
		return false;
	}
	in >> show_background;

	in >> keyword;
	if (keyword!= "linkcolor=")
	{
		cout << "ERROR reading Parameters file, expecting: linkcolor=, read: " << keyword << endl;
		return false;
	}
	in >> temp;
#ifndef _NO_GUI
	linkcolor.setNamedColor(QString (temp.c_str()));
#endif 

	in >> keyword;
	if (keyword!= "nodecolor=")
	{
		cout << "ERROR reading Parameters file, expecting: nodecolor=, read: " << keyword << endl;
		return false;
	}
	in >> temp;
#ifndef _NO_GUI
	nodecolor.setNamedColor(QString (temp.c_str()));
#endif 
	
	in >> keyword;
	if (keyword!= "queuecolor=")
	{
		cout << "ERROR reading Parameters file, expecting: queuecolor=, read: " << keyword << endl;
		return false;
	}
	in >> temp;
#ifndef _NO_GUI
	queuecolor.setNamedColor(QString (temp.c_str()));
#endif 

	in >> keyword;
	if (keyword!= "backgroundcolor=")
	{
		cout << "ERROR reading Parameters file, expecting: backgroundcolor=, read: " << keyword << endl;
		return false;
	}
	in >> temp;
#ifndef _NO_GUI
	backgroundcolor.setNamedColor(QString (temp.c_str()));
#endif 

	in >> keyword;
	if (keyword!= "selectedcolor=")
	{
		cout << "ERROR reading Parameters file, expecting: selectedcolor=, read: " << keyword << endl;
		return false;
	}

	in >> temp;
#ifndef _NO_GUI
	selectedcolor.setNamedColor(QString (temp.c_str()));
#endif 

	in >> keyword;
	if (keyword!= "gui_update_step=")
	{
		cout << "ERROR reading Parameters file, expecting: gui_update_step=, read: " << keyword << endl;
		return false;
	}
	in >> gui_update_step;

	in >> keyword;
	if (keyword!= "#moe_parameters")
	{
		cout << "ERROR reading Parameters file, expecting: #moe_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "moe_speed_update=")
	{
		cout << "ERROR reading Parameters file, expecting: moe_speed_update=, read: " << keyword << endl;
		return false;
	}
	in >> moe_speed_update;

	in >> keyword;
	if (keyword!= "moe_inflow_update=")
	{
		cout << "ERROR reading Parameters file, expecting: moe_inflow_update=, read: " << keyword << endl;
		return false;
	}
	in >> moe_inflow_update;

	in >> keyword;
	if (keyword!= "moe_outflow_update=")
	{
		cout << "ERROR reading Parameters file, expecting: moe_outflow_update=, read: " << keyword << endl;
		return false;
	}
	in >> moe_outflow_update;

	in >> keyword;
	if (keyword!= "moe_queue_update=")
	{
		cout << "ERROR reading Parameters file, expecting: moe_queue_update=, read: " << keyword << endl;
		return false;
	}
	in >> moe_queue_update;

	in >> keyword;
	if (keyword!= "moe_density_update=")
	{
		cout << "ERROR reading Parameters file, expecting: moe_density_update=, read: " << keyword << endl;
		return false;
	}
	in >> moe_density_update;

	in >> keyword;
	if (keyword!= "linktime_alpha=")
	{
		cout << "ERROR reading Parameters file, expecting: linktime_alpha=, read: " << keyword << endl;
		return false;
	}
	in >> linktime_alpha;

///////
	in >> keyword;
	if (keyword!= "#assignment_matrix_parameters")
	{
		cout << "ERROR reading Parameters file, expecting: #assignment_matrix_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "use_ass_matrix=")
	{
		cout << "ERROR reading Parameters file, expecting: use_ass_matrix=, read: " << keyword << endl;
		return false;
	}
	in >> use_ass_matrix;

	in >> keyword;
	if (keyword!= "ass_link_period=")
	{
		cout << "ERROR reading Parameters file, expecting: ass_link_period=, read: " << keyword << endl;
		return false;
	}
	in >> ass_link_period;

	in >> keyword;
	if (keyword!= "ass_od_period=")
	{
		cout << "ERROR reading Parameters file, expecting: ass_od_period=, read: " << keyword << endl;
		return false;
	}
	in >> ass_od_period;

	in >> keyword;
	if (keyword!= "#turning_parameters")
	{
		cout << "ERROR reading Parameters file, expecting: #turning_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "default_lookback_size=")
	{
		cout << "ERROR reading Parameters file, expecting: default_lookback_size=, read: " << keyword << endl;
		return false;
	}
	in >> default_lookback_size;

	in >> keyword;
	if (keyword!= "turn_penalty_cost=")
	{
		cout << "ERROR reading Parameters file, expecting: turn_penalty_cost=, read: " << keyword << endl;
		return false;
	}
	in >> turn_penalty_cost;

	in >> keyword;
	if (keyword!= "#server_parameters")
	{
		cout << "ERROR reading Parameters file, expecting: #server_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "od_servers_deterministic=")
	{
		cout << "ERROR reading Parameters file, expecting: od_servers_deterministic=, read: " << keyword << endl;
		return false;
	}
	in >> od_servers_deterministic;

	in >> keyword;
	if (keyword!= "odserver_sigma=")
	{
		cout << "ERROR reading Parameters file, expecting: odserver_sigma=, read: " << keyword << endl;
		return false;
	}
	in >> odserver_sigma;

	in >> keyword;
	if (keyword!= "#vehicle_parameters")
	{
		cout << "ERROR reading Parameters file, expecting: #vehicle_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "standard_veh_length=")
	{
		cout << "ERROR reading Parameters file, expecting: standard_veh_length=, read: " << keyword << endl;
		return false;
	}
	in >> standard_veh_length;

	in >> keyword;
	if (keyword!= "#route_parameters")
	{
		cout << "ERROR reading Parameters file, expecting: #route_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "update_interval_routes=")
	{
		cout << "ERROR reading Parameters file, expecting: update_interval_routes=, read: " << keyword << endl;
		return false;
	}
	in >> update_interval_routes;

	in >> keyword;
	if (keyword!= "mnl_theta=")
	{
		cout << "ERROR reading Parameters file, expecting: mnl_theta=, read: " << keyword << endl;
		return false;
	}
	in >> mnl_theta;

    in >> keyword;
	if (keyword!= "kirchoff_alpha=")
	{
		cout << "ERROR reading Parameters file, expecting: kirchoff_alpha=, read: " << keyword << endl;
		return false;
	}
	in >> kirchoff_alpha;

	in >> keyword;
	if (keyword!= "delete_bad_routes=")
	{
		cout << "ERROR reading Parameters file, expecting: delete_bad_routes=, read: " << keyword << endl;
		return false;
	}
	in >> delete_bad_routes;

    in >> keyword;
	if (keyword!= "max_rel_route_cost=")
	{
		cout << "ERROR reading Parameters file, expecting: max_rel_route_cost=, read: " << keyword << endl;
		return false;
	}
	in >> max_rel_route_cost;

	in >> keyword;
	if (keyword!= "small_od_rate=")
	{
		cout << "ERROR reading Parameters file, expecting: small_od_rate=, read: " << keyword << endl;
		return false;
	}
	in >> small_od_rate;

	
	in >> keyword;
	if (keyword!= "#mime_parameters")
	{
		cout << "ERROR reading Parameters file, expecting: #mime_parameters, read: " << keyword << endl;
		return false;
	}

	in >> keyword;
	if (keyword!= "mime_comm_step=")
	{
		cout << "ERROR reading Parameters file, expecting: mime_comm_step=, read: " << keyword << endl;
		return false;
	}
	in >> mime_comm_step;

	in >> keyword;
	if (keyword!= "mime_min_queue_length=")
	{
		cout << "ERROR reading Parameters file, expecting: mime_min_queue_length=, read: " << keyword << endl;
		return false;
	}
	in >> mime_min_queue_length;

	in >> keyword;
	if (keyword!= "mime_queue_dis_speed=")
	{
		cout << "ERROR reading Parameters file, expecting: mime_queue_dis_speed=, read: " << keyword << endl;
		return false;
	}
	in >> mime_queue_dis_speed;

	in >> keyword;
	if (keyword!= "vissim_step=")
	{
		cout << "ERROR reading Parameters file, expecting: vissim_step=, read: " << keyword << endl;
		return false;
	}
	in >> vissim_step;

	in >> keyword;
	if (keyword!= "sim_speed_factor=")
	{
		cout << "ERROR reading Parameters file, expecting: sim_speed_factor=, read: " << keyword << endl;
		return false;
	}
	in >> sim_speed_factor;


	return true;
}


void Parameters::write_parameters(ostream & out)
{
#ifndef _NO_GUI
   out << "#drawing_parameters" << endl;
   out << "  draw_link_ids= " << draw_link_ids << endl;
   out << "  link_thickness= " << link_thickness << endl;
   out << "  node_thickness= " << node_thickness << endl;
   out << "  node_radius= " << node_radius << endl;
   out << "  queue_thickness= " << queue_thickness << endl;	
   out << "  selected_thickness= " << selected_thickness << endl;
   out << "  show_background_image= " << show_background << endl;
   out << "  linkcolor= " << qPrintable(linkcolor.name()) << endl;
   out << "  nodecolor= " << qPrintable(nodecolor.name()) << endl;
   out << "  queuecolor= " << qPrintable(queuecolor.name()) << endl;
   out << "  backgroundcolor= " << qPrintable(backgroundcolor.name()) << endl;
   out << "  selectedcolor= " << qPrintable(selectedcolor.name()) << endl;
   out << "  gui_update_step= " << gui_update_step << endl;
   out << "#moe_parameters" << endl;
   out << "  moe_speed_update= " << moe_speed_update << endl;
   out << "  moe_inflow_update= " << moe_inflow_update << endl;
   out << "  moe_outflow_update= " << moe_outflow_update << endl;
   out << "  moe_queue_update= " << moe_queue_update << endl;
   out << "  moe_density_update= " << moe_density_update << endl;
   out << "  linktime_alpha= " << linktime_alpha << endl;
   out << "#assignment_matrix_parameters" << endl;
   out << "  use_ass_matrix= " << use_ass_matrix << endl;
   out << "  ass_link_period= " << ass_link_period << endl;
   out << "  ass_od_period= " << ass_od_period << endl;
   out << "#turning_parameters" << endl;
   out << "  default_lookback_size= " << default_lookback_size << endl;
   out << "  turn_penalty_cost= " << turn_penalty_cost << endl;
   out << "#server_parameters" << endl;
   out << "  od_servers_deterministic= " << od_servers_deterministic << endl;
   out << "  odserver_sigma= " << odserver_sigma << endl;
   out << "#vehicle_parameters" << endl;
   out << "  standard_veh_length= " << standard_veh_length << endl;
   out << "#route_parameters" << endl;
   out << "  update_interval_routes= " << update_interval_routes << endl;
   out << "  mnl_theta= " << mnl_theta << endl;
   out << "  kirchoff_alpha= " << kirchoff_alpha << endl;
   out << "  delete_bad_routes= " << delete_bad_routes << endl;
   out << "  max_rel_route_cost= " << max_rel_route_cost << endl;
   out << "  small_od_rate= " << small_od_rate << endl;
   out << "#mime_parameters" << endl;
   out << "  mime_comm_step= " << mime_comm_step << endl;
   out << "  mime_min_queue_length= " << mime_min_queue_length << endl;
   out << "  mime_queue_dis_speed= " << mime_queue_dis_speed << endl;
   out << "  vissim_step= " << vissim_step << endl;
   out << "  sim_speed_factor= " << sim_speed_factor << endl;

#endif


}