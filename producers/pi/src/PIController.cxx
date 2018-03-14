#include "eudaq/RawDataEvent.hh"
#include "eudaq/SlowProducer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"

#include "PIWrapper.h"
#include "HexGrid.h"

#include <iostream>
#include <ostream>
#include <vector>
#include <string>
//#include <sched.h>

//PIController::PIController(const std::string &name,
//			     const std::string &runcontrol)
//  : eudaq::Controller(name, runcontrol), m_terminated(false), m_name(name) { }

static const std::string EVENT_TYPE = "PI";

class PIController : public eudaq::SlowProducer {
public:
	PIController(const std::string &name, const std::string &runcontrol)
		: eudaq::SlowProducer(name, runcontrol), m_terminated(false), m_name(name) {
		std::cout << "PIController was started successfully." << std::endl;
	}

	//~PIController();

	virtual void OnInitialise(const eudaq::Configuration & init) {
		try {
			std::cout << "Initalising: " << init.Name() << std::endl;

			std::string m_hostname2 = init.Get("HostName", "192.168.21.5");
			m_hostname = &m_hostname2[0];
			m_portnumber = init.Get("PortNumber", 50001);
			std::cout << m_hostname2 << m_portnumber << m_hostname << std::endl;
			wrapper = std::make_shared<PIWrapper>(m_portnumber, m_hostname);

			// connect
			if (!wrapper->connectTCPIP()) {
				EUDAQ_ERROR("No TCP/IP connection to PI Controller!");
			}
			// check
			if (!wrapper->isConnected()) {
				EUDAQ_ERROR("No TCP/IP connection to PI Controller!");
			}
			else {
				wrapper->printStringID();
				wrapper->printStageTypeAll();
			}

			if (!wrapper->axisIsReferenced(m_axis1)) {
				std::cout << "Axis1 not referenced! Please use PIControl!" << std::endl;
			}
			if (!wrapper->axisIsReferenced(m_axis2)) {
				std::cout << "Axis2 not referenced! Please use PIControl!" << std::endl;
			}
			if (!wrapper->axisIsReferenced(m_axis3)) {
				std::cout << "Axis3 not referenced! Please use PIControl!" << std::endl;
			}
			if (!wrapper->axisIsReferenced(m_axis4)) {
				std::cout << "Axis4 not referenced! Please use PIControl!" << std::endl;
			}

			std::cout << "Set to max. velocity:" << std::endl;
			wrapper->setVelocityStage(m_axis1, m_velocitymax);
			wrapper->setVelocityStage(m_axis2, m_velocitymax);
			wrapper->setVelocityStage(m_axis3, m_velocitymax);
			wrapper->setVelocityStage(m_axis4, m_velocitymax);

			std::cout << "\nGet velocity:" << std::endl;
			wrapper->printVelocityStage(m_axis1);
			wrapper->printVelocityStage(m_axis2);
			wrapper->printVelocityStage(m_axis3);
			wrapper->printVelocityStage(m_axis4);

			// get maximum travel range
			wrapper->maxRangeOfStage(m_axis1, &m_axis1max);
			wrapper->maxRangeOfStage(m_axis2, &m_axis2max);
			wrapper->maxRangeOfStage(m_axis3, &m_axis3max);
			wrapper->maxRangeOfStage(m_axis4, &m_axis4max);
			std::cout << "\nGet maximum range:\n"
				<< m_axis1max << "\n"
				<< m_axis2max << "\n"
				<< m_axis3max << "\n"
				<< m_axis4max << std::endl;
			// get position
			printf("\nCurrent position:\n");
			wrapper->printPosition(m_axis1);
			wrapper->printPosition(m_axis2);
			wrapper->printPosition(m_axis3);
			wrapper->printPosition(m_axis4);


			SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Initialised (" + init.Name() + ")");
		}
		catch (...) {
			// Message as cout in the terminal of your producer
			std::cout << "Unknown exception" << std::endl;
			// Message to the LogCollector
			EUDAQ_ERROR("Unknown error during initialization from PI controller");
			// Otherwise, the State is set to ERROR
			SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Initialisation Error");
		}
	}

	virtual void OnConfigure(const eudaq::Configuration &config) {
		try {
			std::cout << "Configuring: " << config.Name() << std::endl;

			m_velocity1 = config.Get("Velocity1", -1.0);
			m_velocity2 = config.Get("Velocity2", -1.0);
			m_velocity3 = config.Get("Velocity3", -1.0);
			m_velocity4 = config.Get("Velocity4", -1.0);

			m_position1 = config.Get("Position1", -1.0);
			m_position2 = config.Get("Position2", -1.0);
			m_position3 = config.Get("Position3", -1.0);
			m_position4 = config.Get("Position4", -1.0);

			m_nsteps1 = config.Get("Nsteps1", 0);
			m_nsteps2 = config.Get("Nsteps2", 0);
			m_nsteps3 = config.Get("Nsteps3", 0);
			m_nsteps4 = config.Get("Nsteps4", 0);

			m_stepsize1 = config.Get("Stepsize1", 0.);
			m_stepsize2 = config.Get("Stepsize2", 0.);
			m_stepsize3 = config.Get("Stepsize3", 0.);
			m_stepsize4 = config.Get("Stepsize4", 0.);

			/*
			  Move Mode:
			  0 [default] for normal axis movement
			  1 [hexagon] for hexagonal 2d axis movement
			*/
			m_movemode = config.Get("MoveMode", 0);

			if (m_movemode == 1){
			    // Read other parameters for hexagon
			    m_start_position_id = config.Get("StartPosID",-1);

			    m_home_position1 = config.Get("HomePosition1",0);
			    m_home_position2 = config.Get("HomePosition2",0);

			    // If start position ID not given, check for start position X/Y
			    if (m_start_position_id > -1){
				m_start_position1 = config.Get("StartPos1",0);
				m_start_position2 = config.Get("StartPos2",0);
			    }

			    // set parameters of grid
			    hexgrid.setCenterX(m_home_position1);
			    hexgrid.setCenterY(m_home_position2);
			    hexgrid.setStepX(m_stepsize1);
			    hexgrid.setStepY(m_stepsize2);
			    // Create hexagonal grid
			    hexgrid.BuildGrid();
			    m_npositions = hexgrid.getNpos();
			    EUDAQ_INFO("Number of positions " + std::to_string(m_npositions) + ". Starting at " + std::to_string(m_start_position_id));

			}


			// Set velocity
			printf("\nSet individual velocity:\n");
			if (m_velocity1 >= m_velocitymax) { m_velocity1 = m_velocitymax; }
			if (m_velocity1 < 0) { printf("No velocity change for axis1!\n"); }
			else { wrapper->setVelocityStage(m_axis1, m_velocity1); }
			if (m_velocity2 >= m_velocitymax) { m_velocity2 = m_velocitymax; }
			if (m_velocity2 < 0) { printf("No velocity change for axis2!\n"); }
			else { wrapper->setVelocityStage(m_axis2, m_velocity2); }
			if (m_velocity3 >= m_velocitymax) { m_velocity3 = m_velocitymax; }
			if (m_velocity3 < 0) { printf("No velocity change for axis3!\n"); }
			else { wrapper->setVelocityStage(m_axis3, m_velocity3); }
			if (m_velocity4 >= m_velocitymax) { m_velocity4 = m_velocitymax; }
			if (m_velocity4 < 0) { printf("No velocity change for axis4!\n"); }
			else { wrapper->setVelocityStage(m_axis4, m_velocity4); }
			// get velocity // alternative: wrapper->getVelocityStage2(m_axis1, &m_velocity1);
			std::cout << "\nGet velocity:" << std::endl;
			wrapper->printVelocityStage(m_axis1);
			wrapper->printVelocityStage(m_axis2);
			wrapper->printVelocityStage(m_axis3);
			wrapper->printVelocityStage(m_axis4);

			// Move into starting position
			printf("\nMoving...\n");

			// Normal movement
			if (m_movemode == 0){
			    if (m_position1 >= m_axis1max) { m_position1 = m_axis1max; }
			    if (m_position1 < 0) { printf("No movement of axis1!\n"); }
			    else { wrapper->moveTo(m_axis1, m_position1); }
			    if (m_position2 >= m_axis2max) { m_position2 = m_axis2max; }
			    if (m_position2 < 0) { printf("No movement of axis2!\n"); }
			    else { wrapper->moveTo(m_axis2, m_position2); }
			    if (m_position3 >= m_axis3max) { m_position3 = m_axis3max; }
			    if (m_position3 < 0) { printf("No movement of axis3!\n"); }
			    else { wrapper->moveTo(m_axis3, m_position3); }
			    if (m_position4 >= m_axis4max) { m_position4 = m_axis4max; }
			    if (m_position4 < 0) { printf("No movement of axis4!\n"); }
			    else { wrapper->moveTo(m_axis4, m_position4); }

			    if (m_nsteps4 > 0){
				if (m_stepsize4 == 0.){
				    EUDAQ_ERROR("Amount of steps (NstepsX) given, but (StepsizeX) is 0. This cannot end well!");
				}
				else{
				    EUDAQ_INFO("Performing " + std::to_string(m_nsteps4) + " steps of " + std::to_string(m_stepsize4) + " units each. Starting at " + std::to_string(m_position4));
				    m_currstep = 0;
				}
			    }
			}

			// Hexagonal grid
			else if (m_movemode == 1){
			    m_currstep = m_start_position_id;

			    // get position coordinates
			    m_position1 = hexgrid.getPosX(m_currstep);
			    m_position2 = hexgrid.getPosY(m_currstep);

			    // move to position
			    if (m_position1 >= m_axis1max) { m_position1 = m_axis1max; }
			    if (m_position1 < 0) { printf("No movement of axis1!\n"); }
			    else { wrapper->moveTo(m_axis1, m_position1); }

			    if (m_position2 >= m_axis2max) { m_position2 = m_axis2max; }
			    if (m_position2 < 0) { printf("No movement of axis2!\n"); }
			    else { wrapper->moveTo(m_axis2, m_position2); }

			}

			// New position
			printf("\nCurrent position:\n");
			wrapper->printPosition(m_axis1);
			wrapper->printPosition(m_axis2);
			wrapper->printPosition(m_axis3);
			wrapper->printPosition(m_axis4);

			double pos_curr1;
			double pos_curr2;
			double pos_curr3;
			double pos_curr4;
			wrapper->getPosition2(m_axis1, &pos_curr1);
			wrapper->getPosition2(m_axis2, &pos_curr2);
			wrapper->getPosition2(m_axis3, &pos_curr3);
			wrapper->getPosition2(m_axis4, &pos_curr4);

			if (m_movemode == 0){
			    EUDAQ_INFO("Moved to x=" + std::to_string(pos_curr1) + " , y=" + std::to_string(pos_curr2) + " , phi=" + std::to_string(pos_curr4));
			}
			else if (m_movemode == 1){
			    EUDAQ_INFO("Moved to x=" + std::to_string(pos_curr1) + " , y=" + std::to_string(pos_curr2));
			}

			SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + config.Name() + ")");
		}
		catch (...) {
			EUDAQ_ERROR("Unknown exception.");
			SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Unknown exception.");
		}
	}

	virtual void OnStartRun(unsigned runnumber) {

		m_run = runnumber;
		m_ev = 0;

		try {
			// If a stepping is wanted, execute the next step.

		    if(m_movemode == 0){
			if (m_currstep < m_nsteps4 && m_stepsize4!=0){

				if (m_currstep == 0){
					EUDAQ_INFO("Initial step, staying at first position.");
				}else{
					m_position4 += m_stepsize4;
					if (m_position4 >= m_axis4max) {
						EUDAQ_ERROR("Position target value too large: " + std::to_string(m_position4) + "   Moving to maximum value: " + std::to_string(m_axis4max));
						m_position4 = m_axis4max;
						m_currstep = m_nsteps4;
					}
					if (m_position4 < 0) {
						EUDAQ_ERROR("Position target value too small: " + std::to_string(m_position4) + "   Moving to minimum value: " + std::to_string(m_axis4max));
						m_position4 = 0.0;
						m_currstep = m_nsteps4;
					}
					wrapper->moveTo(m_axis4, m_position4);

					double pos_curr4;
					wrapper->getPosition2(m_axis4, &pos_curr4);

					EUDAQ_INFO("Step Nr. " + std::to_string(m_currstep) + " , phi=" + std::to_string(pos_curr4) + " deg");
				}

				m_currstep++;

			}
		    }
		    else if (m_movemode == 1){
			if (m_currstep == m_start_position_id){
			    EUDAQ_INFO("Initial step, staying at first position.");
			}
			else if (m_currstep < m_npositions){
			    // get position coordinates
			    m_position1 = hexgrid.getPosX(m_currstep);
			    m_position2 = hexgrid.getPosY(m_currstep);

			    if (m_position1 <= m_axis1max && m_position1 >= 0. && m_position2 <= m_axis2max && m_position2 >= 0.){
				wrapper->moveTo(m_axis1, m_position1);
				wrapper->moveTo(m_axis2, m_position2);

				// check position
				double pos_curr1;
				double pos_curr2;
				wrapper->getPosition2(m_axis1, &pos_curr1);
				wrapper->getPosition2(m_axis2, &pos_curr2);

				EUDAQ_INFO("Moved to position " + std::to_string(m_currstep)+ " x=" + std::to_string(pos_curr1) + " , y=" + std::to_string(pos_curr2));
			    }
			    else {
				EUDAQ_ERROR("Position target out of range: x= " + std::to_string(m_position1) + " y= " + std::to_string(m_position2));
			    }

			    // Increment step
			    m_currstep += 1;
			}
			else {
			    // Current step greater than number of positions to be scanned
			    // Normally should stop running, but for now move to home position (if not already there)
			    if (m_position1 != m_home_position1 && m_position2 != m_home_position2){
				m_position1 = m_home_position1;
				m_position2 = m_home_position2;

				if (m_position1 <= m_axis1max && m_position1 >= 0. && m_position2 <= m_axis2max && m_position2 >= 0.){
				    wrapper->moveTo(m_axis1, m_position1);
				    wrapper->moveTo(m_axis2, m_position2);

				    // check position
				    double pos_curr1;
				    double pos_curr2;
				    wrapper->getPosition2(m_axis1, &pos_curr1);
				    wrapper->getPosition2(m_axis2, &pos_curr2);

				    EUDAQ_INFO("Moved to x=" + std::to_string(pos_curr1) + " , y=" + std::to_string(pos_curr2));
				}
			    }
			}
		    }


		    // get position
			double pos_curr1;
			double pos_curr2;
			double pos_curr3;
			double pos_curr4;
			wrapper->getPosition2(m_axis1, &pos_curr1);
			wrapper->getPosition2(m_axis2, &pos_curr2);
			wrapper->getPosition2(m_axis3, &pos_curr3);
			wrapper->getPosition2(m_axis4, &pos_curr4);
		    printf("\nCurrent position:\n");
		    wrapper->printPosition(m_axis1);
		    wrapper->printPosition(m_axis2);
		    wrapper->printPosition(m_axis3);
		    wrapper->printPosition(m_axis4);

		    EUDAQ_INFO("PI stage in position 1=" + std::to_string(pos_curr1) +
			       ", 2=" + std::to_string(pos_curr2) +
			       ", 3=" + std::to_string(pos_curr3) +
			       ", 4=" + std::to_string(pos_curr4));


		    // It must send a BORE to the Data Collector
		    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
		   // You can set tags on the BORE that will be saved in the data file
		   // and can be used later to help decoding
		   bore.SetTag("pi_pos_chan1", eudaq::to_string(pos_curr1));
		   bore.SetTag("pi_pos_chan2", eudaq::to_string(pos_curr2));
		   bore.SetTag("pi_pos_chan3", eudaq::to_string(pos_curr3));
		   bore.SetTag("pi_pos_chan4", eudaq::to_string(pos_curr4));

		   // Send the event to the Data Collector
		   SendEvent(bore);

		    SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");
		}
		catch (...) {
			EUDAQ_ERROR("Unknown exception.");
			SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Unknown exception.");
		}
	}

	virtual void OnStopRun() {

		try {
			// No Action

			// get position
			printf("\nCurrent position:\n");
			wrapper->printPosition(m_axis1);
			wrapper->printPosition(m_axis2);
			wrapper->printPosition(m_axis3);
			wrapper->printPosition(m_axis4);

			SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopped");
		}
		catch (const std::exception &e) {
			printf("While Stopping: Caught exception: %s\n", e.what());
			SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error");
		}
		catch (...) {
			printf("While Stopping: Unknown exception\n");
			SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error");
		}
	}

	virtual void OnTerminate() {

		std::cout << "PIController terminating..." << std::endl;

		m_terminated = true;
		std::cout << "PIController " << m_name << " terminated." << std::endl;
	}

	void Loop() {

		// Loop until Run Control tells us to terminate
		while (!m_terminated) {

		    if (GetConnectionState() != eudaq::ConnectionState::STATE_RUNNING)
		    {
			// Now sleep for a bit, to prevent chewing up all the CPU
			eudaq::mSleep(20);
			// Then restart the loop
			continue;
		    }

		    if (m_ev % 100 == 0){
			// get position
			double pos_curr1;
			double pos_curr2;
			double pos_curr3;
			double pos_curr4;
			wrapper->getPosition2(m_axis1, &pos_curr1);
			wrapper->getPosition2(m_axis2, &pos_curr2);
			wrapper->getPosition2(m_axis3, &pos_curr3);
			wrapper->getPosition2(m_axis4, &pos_curr4);

			// It must send a BORE to the Data Collector
			eudaq::RawDataEvent ev(eudaq::RawDataEvent(EVENT_TYPE, m_run, m_ev));
			ev.SetTag("pi_pos_chan1", eudaq::to_string(pos_curr1));
			ev.SetTag("pi_pos_chan2", eudaq::to_string(pos_curr2));
			ev.SetTag("pi_pos_chan3", eudaq::to_string(pos_curr3));
			ev.SetTag("pi_pos_chan4", eudaq::to_string(pos_curr4));

			// Send the event to the Data Collector
			SendEvent(ev);
		    }

		    m_ev++;

		    // Move this thread to the end of the scheduler queue:
		    //sched_yield();
		    eudaq::mSleep(50);
		}
	}

private:
	std::shared_ptr<PIWrapper> wrapper;
	//PIWrapper wrapper();
	bool m_terminated;
	std::string m_name;
	char *m_hostname;
	int m_portnumber;
	int  m_movemode;
	int  m_start_position1;
	int  m_start_position2;
	char *m_axis1 = "1";
	char *m_axis2 = "2";
	char *m_axis3 = "3";
	char *m_axis4 = "4";
	double m_axis1max = 0.0;
	double m_axis2max = 0.0;
	double m_axis3max = 0.0;
	double m_axis4max = 0.0;
	double m_position1 = 0.0;
	double m_position2 = 0.0;
	double m_position3 = 0.0;
	double m_position4 = 0.0;
	double m_velocity1 = 0.0;
	double m_velocity2 = 0.0;
	double m_velocity3 = 0.0;
	double m_velocity4 = 0.0;
	double m_velocitymax = 10.0;
	double m_stepsize1 = 0.0;
	double m_stepsize2 = 0.0;
	double m_stepsize3 = 0.0;
	double m_stepsize4 = 0.0;
	unsigned int m_nsteps1 = 0;
	unsigned int m_nsteps2 = 0;
	unsigned int m_nsteps3 = 0;
	unsigned int m_nsteps4 = 0;
	unsigned int m_currstep = 0;

	// parameters for hexa grid
	unsigned int m_start_position_id = 0;
	double m_home_position1 = 0;
	double m_home_position2 = 0;
	unsigned int m_npositions = 0;

	HexGrid hexgrid;

	unsigned m_run;
	unsigned m_ev;
};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char **argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("PI Stages Controller", "0.0", "Run options");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
				   "tcp://localhost:44000", "address",
				   "The address of the RunControl.");
  eudaq::Option<std::string> level(
      op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name(op, "n", "name", "PIController", "string",
				  "The name of this Producer/Controller");

  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());

    // Create the instance
    PIController controller(name.Value(), rctrl.Value());

    // And set it running...
    controller.Loop();

    // When the keep-alive loop terminates, it is time to go
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}
