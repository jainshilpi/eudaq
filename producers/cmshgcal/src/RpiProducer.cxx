#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <boost/format.hpp>

const size_t RAW_EV_SIZE_8  = 30787;
const size_t RAW_EV_SIZE_32 = 123152;

const size_t split1 = 62000;
const size_t split2 = 61152;

// A name to identify the raw data format of the events generated
// Modify this to something appropriate for your producer.
static const std::string EVENT_TYPE = "HexaBoard";

// Declare a new class that inherits from eudaq::Producer
class RpiProducer : public eudaq::Producer {
  public:
    // The constructor must call the eudaq::Producer constructor with the name
    // and the runcontrol connection string, and initialize any member variables.
    RpiProducer(const std::string & name, const std::string & runcontrol)
      : eudaq::Producer(name, runcontrol),
        m_run(0), m_ev(0), m_stopping(false), m_done(false),
        m_sockfd1(0), m_sockfd2(0), m_running(false), m_configured(false),
        WARN_MSG("if_you_see_this_the_conf_file_is_incomplete")
  {}
    
    int synchronizeFolder(std::string sourceFolderPath, std::string targetFolderPath, std::string targetRPi) {
      EUDAQ_INFO("Synchronizing files in source directory sv1:" + sourceFolderPath + " with target directory " + targetRPi + ":" + targetFolderPath);
      int synchronizationStatus = system(("rsync --progress -a -v -c -e ssh " + sourceFolderPath + "/ " + targetRPi + ":" + targetFolderPath + "/").data());
      if (synchronizationStatus != 0) {
        EUDAQ_ERROR("Error: unable to synchronize folders between sv1 and " + targetRPi);
      }
      else {
        EUDAQ_INFO("Successfully synchronized files!");
      }
      return synchronizationStatus;
    }

    int executeScript(std::string targetRPi, std::string targetFolderPath, std::string scriptFileName) {
      EUDAQ_INFO("Executing script " + scriptFileName + " on " + targetRPi + ":" + targetFolderPath);
      int executionStatus = system(("ssh -T " + targetRPi + " \"cd " + targetFolderPath + " && source " + scriptFileName + "\"").data());
      if (executionStatus != 0) {
        EUDAQ_ERROR("Error: unable to execute script " + scriptFileName);
      }
      else {
        EUDAQ_INFO("Successfully executed script!");
      }
      return executionStatus;
    }

    // This gets called whenever the DAQ is configured
    virtual void OnConfigure(const eudaq::Configuration & config) {
      std::cout << "Configuring: " << config.Name() << std::endl;

      bool configurationSuccessful = true;

      // Do any configuration of the hardware here
      // Configuration file values are accessible as config.Get(name, default)
      
      // Fetch parameters needed to reset configuration

      m_piSynch_IP = config.Get("piSynch_IP", WARN_MSG);
      for (int RDOUTCounter = 0; RDOUTCounter <= 3; ++RDOUTCounter) {
        m_piRDOUT_IPs[RDOUTCounter] = config.Get("piRDOUT" + eudaq::to_string(1+RDOUTCounter) + "_IP", WARN_MSG);
      }
      m_RDOUTMask = config.Get("RDOUTMask", WARN_MSG);

      m_sourceDirPath_Synch = config.Get("sourceDirPath_Synch", WARN_MSG);
      m_targetDirPath_Synch = config.Get("targetDirPath_Synch", WARN_MSG);
      m_resetConfiguration_Synch_exe_fileName = config.Get("resetConfiguration_Synch_exe_fileName", WARN_MSG);
      m_sourceDirPath_Readout = config.Get("sourceDirPath_Readout", WARN_MSG);
      m_targetDirPath_Readout = config.Get("targetDirPath_Readout", WARN_MSG);
      m_resetConfiguration_Readout_exe_fileName = config.Get("resetConfiguration_Readout_exe_fileName", WARN_MSG);
      m_sourceDirPath_Skirocs = config.Get("sourceDirPath_Skirocs", WARN_MSG);
      m_targetDirPath_Skirocs = config.Get("targetDirPath_Skirocs", WARN_MSG);
      m_resetConfiguration_Skirocs_exe_fileName = config.Get("resetConfiguration_Skirocs_exe_fileName", WARN_MSG);

      for (int RDOUTCounter = 0; RDOUTCounter <= 3; ++RDOUTCounter) {
        m_skirocIndices_RDOUTs[RDOUTCounter] = config.Get("skirocIndices_RDOUT" + eudaq::to_string(1+RDOUTCounter), WARN_MSG);
      }

      // reset configuration by programming FPGAs on the SYNCH and RDOUT boards, and the RDOUT and CTL ORMs

      // synchronize folders:

      int folderSyncStatus;
      // First the Synch board, commented out for now
      // folderSyncStatus = synchronizeFolder(m_sourceDirPath_Synch, m_targetDirPath_Synch, m_piSynch_IP);
      // configurationSuccessful = (folderSyncStatus == 0);
      
      // Next the Readout boards, also commented out for now
      for (int RDOUTCounter = 0; RDOUTCounter <= 3; ++RDOUTCounter) {
        if (m_RDOUTMask.substr(RDOUTCounter, 1) == std::string("1")) {
          // folderSyncStatus = synchronizeFolder(m_sourceDirPath_Readout, m_targetDirPath_Readout, m_piRDOUT_IPs[RDOUTCounter]); // Commented out for now
          // configurationSuccessful = (folderSyncStatus == 0);
          folderSyncStatus = synchronizeFolder(m_sourceDirPath_Skirocs, m_targetDirPath_Skirocs, m_piRDOUT_IPs[RDOUTCounter]);
          configurationSuccessful = (folderSyncStatus == 0);
        }
      }
      
      // Reset configurations:
      
      int scriptExecutionStatus;
      // First on the synch, commented out
      // scriptExecutionStatus = executeScript(m_piSynch_IP, m_targetDirPath_Synch, m_resetConfiguration_Synch_exe_fileName);
      // configurationSuccessful = (scriptExecutionStatus == 0);

      // Next on the Readout boards, commented out
      // for (int RDOUTCounter = 0; RDOUTCounter <= 3; ++RDOUTCounter) {
      //   if (m_RDOUTMask.substr(RDOUTCounter, 1) == std::string("1")) {
      //     scriptExecutionStatus = executeScript(m_piRDOUT_IPs[RDOUTCounter], m_targetDirPath_Readout, m_resetConfiguration_Readout_exe_fileName);
      //     configurationSuccessful = (scriptExecutionStatus == 0);
      //   }
      // }
      
      m_portTCP = config.Get("portTCP", 55511);
      m_portUDP = config.Get("portUDP", 55512);
      m_rpi_1_ip = config.Get("RPI_1_IP", "192.168.222.3");

      std::cout<<"RPI IP address: "<<m_rpi_1_ip<<std::endl;

      if (configurationSuccessful) {
        SetStatus(eudaq::Status::LVL_OK, "Configuration reset!");
      }
      else {
        SetStatus(eudaq::Status::LVL_ERROR, "Unable to reset configuration!");
      }

      // Copied over from onStartRun temporarily

      for (int RDOUTCounter = 0; RDOUTCounter <= 3; ++RDOUTCounter) {
        if (m_RDOUTMask.substr(RDOUTCounter, 1) == std::string("1")) {
          scriptExecutionStatus = executeScript(m_piRDOUT_IPs[RDOUTCounter], m_targetDirPath_Skirocs, m_resetConfiguration_Skirocs_exe_fileName + " " + m_skirocIndices_RDOUTs[RDOUTCounter]);
          if (scriptExecutionStatus != 0) {
            EUDAQ_WARN("Unable to configure Skirocs!");
            return;
          }
        }
      }

      // At the end, set the status that will be displayed in the Run Control.
      // SetStatus(eudaq::Status::LVL_OK, "Configured (" + config.Name() + ")");

      // m_configured=true;
      m_configured = configurationSuccessful;
    }

    // This gets called whenever a new run is started
    // It receives the new run number as a parameter
    virtual void OnStartRun(unsigned param) {
      // Commented out for now
      // int scriptExecutionStatus;
      // for (int RDOUTCounter = 0; RDOUTCounter <= 3; ++RDOUTCounter) {
      //   if (m_RDOUTMask.substr(RDOUTCounter, 1) == std::string("1")) {
      //     scriptExecutionStatus = executeScript(m_piRDOUT_IPs[RDOUTCounter], m_targetDirPath_Skirocs, m_resetConfiguration_Skirocs_exe_fileName + " " + m_skirocIndices_RDOUTs[RDOUTCounter]);
      //     if (scriptExecutionStatus != 0) {
      //       EUDAQ_WARN("Unable to configure Skirocs!");
      //       return;
      //     }
      //   }
      // }
    
      m_run = param;
      m_ev = 0;

      std::cout << "Start Run: " << m_run << std::endl;

      // openinig socket:

      bool con = OpenConnection();
      if (!con) {
	EUDAQ_WARN("Socket conection failed: no server. Can't start the run.");
	SetStatus(eudaq::Status::LVL_ERROR, "No Socket.");
	return;
      }

      // Before starting the new run we shall flush the UDP buffer
      const int bufsize = 62001;
      char buffer[bufsize];	  
      struct sockaddr_in their_addr;
      socklen_t addr_len = sizeof(their_addr);
      //std::cout<<"Before recvfrom. Are you blocking us?  addr_len="<<addr_len<<std::endl;
      while (true){
	int n = recvfrom(m_sockfd2, buffer, bufsize, 0, (struct sockaddr *)&their_addr, &addr_len);
	if (n <= 0) {
	  std::cout<<"n = "<<n<<" Socket timed out. Errno="<<errno<<std::endl;
	  std::cout<<" In this case it means there is no more data in UDP buffer, so we can safely close it."<<std::endl;
	  break;
	}
      }
	

      std::string message = "START_RUN " + eudaq::to_string(m_run);
      SendCommand(message.c_str());

      char answer[20];
      bzero(answer, 20);
      int n = recv(m_sockfd1, answer, 20, 0);
      if (n <= 0) {
	std::cout<<n<<" Something is wrong with socket, we can't start the run..."<<std::endl;
	SetStatus(eudaq::Status::LVL_ERROR, "Can't Start Run on Hardware side.");
	return;
      }
      
      else {
	std::cout<<"Answer to START_RUN: "<<answer<<std::endl;
	if (strncmp(answer,"GOOD_START",10)!=0) {
	  std::cout<<n<<" Not expected answer from server, we can't start the run..."<<std::endl;
	  SetStatus(eudaq::Status::LVL_ERROR, "Can't Start Run on Hardware side.");
	  return;
	}
      }

      // Let's open a file for raw data:
      char rawFilename[256];
      sprintf(rawFilename, "../data/HexaData_Run%04d.raw", m_run); // The path is relative to eudaq/bin
      m_rawFile.open(rawFilename, std::ios::binary);
      
      //fout = fopen("myOUT.txt", "w");
      //fprintf(fout,"Total number of events: NN \n");
      
      // If we're here, then the Run was started on the Hardware side (TCP server will send data)
      
      // It must send a BORE to the Data Collector
      eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(EVENT_TYPE, m_run));
      // You can set tags on the BORE that will be saved in the data file
      // and can be used later to help decoding
      //bore.SetTag("MyTag", eudaq::to_string(m_exampleparam));

      // Send the event to the Data Collector
      SendEvent(bore);

      m_running=true;
      m_stopped=false;
      m_stopping=false;

      m_gotPart1 = false;
      m_gotPart2 = false;
      
      // At the end, set the status that will be displayed in the Run Control.
      SetStatus(eudaq::Status::LVL_OK, "Running");
      std::cout<<"Started it!"<<std::endl;
    }

    // This gets called whenever a run is stopped
    virtual void OnStopRun() {
      std::cout << "Stopping Run" << std::endl;

      // Send command to Hardware:
      SendCommand("STOP_RUN");


      // Set a flag to signal to the polling loop that the run is over
      SetStatus(eudaq::Status::LVL_OK, "Stopping...");

      eudaq::mSleep(1000);

      m_stopping = true;

      while (!m_stopped){
	char answer[20];
	bzero(answer, 20);
	int n = recv(m_sockfd1, answer, 20, 0);
	if (n <= 0) {
	  SetStatus(eudaq::Status::LVL_ERROR, "Can't Stop Run on Hardware side.");
	  return;
	}
	else {
	  std::cout<<"Answer to STOP_RUN: "<<answer<<std::endl;
	  if (strncmp(answer,"STOPPED_OK",10)!=0) {
	    std::cout<<"Something is wrong, we can't stop the run..."<<std::endl;
	    SetStatus(eudaq::Status::LVL_ERROR, "Can't Start Run on Hardware side.");
	    return;
	  }


	  CloseConnection();
	  m_stopping = false;
	  m_running  = false;
	  m_stopped  = true;
	  
	}
      }

      m_rawFile.close();
      // If we were running, send signal to stop:
      //m_running=false;
      //fclose(fout);
      
      // wait until all events have been read out from the hardware
      while (m_stopping) {
        eudaq::mSleep(20);
      }

      m_stopped = true;
      // Send an EORE after all the real events have been sent
      // You can also set tags on it (as with the BORE) if necessary
      SendEvent(eudaq::RawDataEvent::EORE("Test", m_run, ++m_ev));

      SetStatus(eudaq::Status::LVL_OK, "Stopped.");
      std::cout << "Stopped it!" << std::endl;

    }

    // This gets called when the Run Control is terminating,
    // we should also exit.
    virtual void OnTerminate() {
      SetStatus(eudaq::Status::LVL_OK, "Terminating...");
      std::cout << "Terminating..." << std::endl;
      m_done = true;
    }

    // This is just an example, adapt it to your hardware
    void ReadoutLoop() {
      // Loop until Run Control tells us to terminate
      while (!m_done) {

	if (!m_running)
	  {
	    eudaq::mSleep(2000);
	    //EUDAQ_DEBUG("Not Running; but sleeping");
	    SetStatus(eudaq::Status::LVL_USER, "Sleeping");
	    continue;
	  }


	if (m_sockfd1 <= 0 || m_sockfd2 <= 0) {
	  EUDAQ_DEBUG("No sockets. Not Running; but sleeping");
	  SetStatus(eudaq::Status::LVL_USER, "No Sockets yet in Readout Loop.  sfd1="+
		    eudaq::to_string(m_sockfd1)+" sfd2="+eudaq::to_string(m_sockfd2));
	  eudaq::mSleep(100);
	  continue;
	}

	// **********
	// If we are below this point, we listen for data
	// ***********

	SetStatus(eudaq::Status::LVL_DEBUG, "Running");
	//EUDAQ_DEBUG("Running again");

	const int bufsize = split1;
	char buffer[bufsize];
	bzero(buffer, bufsize);

	struct sockaddr_in their_addr;
	socklen_t addr_len = sizeof(their_addr);	
	//std::cout<<"Before recvfrom. Are you blocking us?  addr_len="<<addr_len<<std::endl;
	int n = recvfrom(m_sockfd2, buffer, bufsize, 0, (struct sockaddr *)&their_addr, &addr_len);
	//std::cout<<"After recvfrom. Un-blocked..."<<std::endl;
	if (n <= 0) {
	  if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
	    std::cout<<"n = "<<n<<" Socket timed out. Errno="<<errno<<std::endl;

	    std::time_t current_time = std::time(NULL);

	    if (current_time - m_last_readout_time < 30)
	      continue; // No need to stop the run, just wait
	    else {
	      // It's been too long without data, we shall give a warning
	      SetStatus(eudaq::Status::LVL_WARN, "No Data");
	      EUDAQ_WARN("Sockets: No data for too long..");
	    }

	  eudaq::mSleep(200);

	  }
	  else {
	    std::cout<<" n="<<n<<" errno="<<errno<<std::endl;
	    SetStatus(eudaq::Status::LVL_WARN, "Nothing to read from socket...");
	    EUDAQ_WARN("Sockets: ERROR reading from socket (it's probably disconnected)");
	    continue;
	  }
	}

	// We are here if there is data and it's size is > 0

	std::cout<<" After recv. Size of message recieved: n="<<n<<std::endl;
	//std::cout<<"In ReadoutLoop.  Here is the message from Server: \n"<<buffer<<std::endl;
	

	// If we get here, there must be data to read out
	m_last_readout_time = std::time(NULL);
	//std::cout <<"size = "<<n<< "  m_last_readout_time:"<< m_last_readout_time<<std::endl;
	//std::cout<<"First byte of the RAW event: "<<eudaq::to_hex(buffer[0])<<std::endl;
	//std::cout<<"Last two bytes of the event: "<<eudaq::to_hex(buffer[RAW_EV_SIZE-2])
	//	 <<" "<<eudaq::to_hex(buffer[RAW_EV_SIZE-1])<<std::endl;


	if (n==split1) {// First part of the data
	  m_gotPart1 = true;
	  memcpy (m_raw32bitData, buffer, split1);
	  continue;
	}
	else if (n==split2) {//Second part
	  memcpy (m_raw32bitData+bufsize, buffer, split2);
	  m_gotPart2 = true;
	  if (!m_gotPart1) {
	    m_gotPart2=false; 
	    continue;
	  }
	}
	else {
	  EUDAQ_WARN("The event size is not right! n="+eudaq::to_string(n));
	  SetStatus(eudaq::Status::LVL_WARN, "Wrong event size.");
	  continue;
	}
	

	std::cout<<"First few bytes of the RAW event:"<<std::endl;
	for (int b=0; b<3; b++)
	  //printf(" %d byte: %x\n", b, (unsigned char)m_raw32bitData[b]);
	  std::cout<<b<<" byte = "<<eudaq::to_hex(m_raw32bitData[b])<<std::endl;

	//std::cout<<"Last 8 bytes of the RAW event:"<<std::endl;
	//for (int b=1; b<8; b++)
	//std::cout<<RAW_EV_SIZE_32-b<<" byte = "<<eudaq::to_hex(m_raw32bitData[RAW_EV_SIZE_32-b])<<std::endl;

	
	
	if ((unsigned char)m_raw32bitData[0]==0xff && m_gotPart1 && m_gotPart2){

       	  // This is good data (at first sight)
      
	  // Write it into raw file: 
	  m_rawFile.write(m_raw32bitData, RAW_EV_SIZE_32);

	  	
	  // Create a RawDataEvent to contain the event data to be sent
	  eudaq::RawDataEvent ev(EVENT_TYPE, m_run, m_ev);

	  ev.AddBlock(0, m_raw32bitData, RAW_EV_SIZE_32);
	  SendEvent(ev);

	}
	else {
	  if ((unsigned char)m_raw32bitData[0]!=0xff){
	    EUDAQ_WARN("First byte is not 0xff. It is: "+eudaq::to_hex(m_raw32bitData[0]));
	    SetStatus(eudaq::Status::LVL_WARN, "Corrupted Data");
	  }
	  else if ( !m_gotPart1 || !m_gotPart2){
	    EUDAQ_WARN("Did not receive all parts of data... p1:"+eudaq::to_hex(m_gotPart1)+"  p2:"+eudaq::to_hex(m_gotPart2));
	    SetStatus(eudaq::Status::LVL_WARN, "Corrupted Data");
	  }
	}

	bzero(m_raw32bitData, RAW_EV_SIZE_32);
	m_gotPart1 = false;
	m_gotPart2 = false;

	m_ev++;
	continue;

      }// end of while(done) loop

    } // end of ReadLoop

    bool OpenConnection()
    {
      // This opens a TCP socket connection (available for both in-data and out-data)
      // We are going to set up this producer as CLIENT
      // Using this tutorial as guidence: http://www.linuxhowtos.org/C_C++/socket.htm
      // and code from:
      // https://github.com/EUDAQforLC/eudaq/blob/ahcal_telescope_december2016_v1.6/producers/calice/AHCAL/src/AHCALProducer.cc
      // TCP socket is to be used for messaging only (START, STOP, and confirmations)
      
      m_sockfd1 = socket(AF_INET, SOCK_STREAM, 0);
      if (m_sockfd1<0) {
	EUDAQ_ERROR("Can't open TCP socket: m_sockfd="+eudaq::to_string(m_sockfd1));
	return 0;
      }
      
      struct sockaddr_in tcp_addr;
      bzero((char *) &tcp_addr, sizeof(tcp_addr));
      tcp_addr.sin_family = AF_INET;
      tcp_addr.sin_addr.s_addr = inet_addr(m_rpi_1_ip.c_str());
      tcp_addr.sin_port = htons(m_portTCP);
 
      int ret = connect(m_sockfd1, (struct sockaddr *) &tcp_addr, sizeof(tcp_addr));
      if (ret != 0) {
	SetStatus(eudaq::Status::LVL_WARN, "No Socket.");
	EUDAQ_WARN("Can't connect() to socket: ret="+eudaq::to_string(ret)+"  sockfd="+eudaq::to_string(m_sockfd1));
	return 0;
      }

      // -----
      // And here is an UDP socket.
      // It is to be used for receiving data

      m_sockfd2 = socket(AF_INET, SOCK_DGRAM, 0);
      if (m_sockfd2<0) {
        EUDAQ_ERROR("Can't open UDP socket: m_sockfd="+eudaq::to_string(m_sockfd2));
        return 0;
      }

      struct sockaddr_in udp_addr;
      bzero((char *) &udp_addr, sizeof(udp_addr));
      udp_addr.sin_family = AF_INET;
      udp_addr.sin_addr.s_addr = INADDR_ANY;
      udp_addr.sin_port = htons(m_portUDP);

      if ( bind(m_sockfd2, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0)
	std::cout<<"Can't bind the UDP socket "<<m_sockfd2<<std::endl;
	
      //char buffer[]="HELLO";
      //int n = sendto(m_sockfd2, buffer, strlen(buffer), 0, (const struct sockaddr *)&udp_addr, sizeof(struct sockaddr_in));

      // ***********************
      // This makes the recv() command non-blocking.
      // After the timeout, it will get an error which we can catch and continue the loops

      struct timeval timeout;
      timeout.tv_sec = 5;
      timeout.tv_usec = 0;
      setsockopt(m_sockfd2, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
      //****************

      std::cout << "  Opened TCP socket: "<<m_sockfd1 << std::endl;
      return 1;
    }


    void CloseConnection()
    {
      //std::unique_lock<std::mutex> myLock(m_mufd);
      close(m_sockfd1);
      std::cout << "  Closed TCP socket: "<<m_sockfd1 << std::endl;
      m_sockfd1 = -1;
      close(m_sockfd2);
      std::cout << "  Closed UDP socket: "<<m_sockfd2 << std::endl;
      m_sockfd2 = -1;

    }

    void SendCommand(const char *command, int size=0) {
      
      if (size == 0) size = strlen(command);
      
      if (m_sockfd1 <= 0)
	std::cout << "SendCommand(): cannot send command because connection is not open." << std::endl;
      else {
	size_t bytesWritten = write(m_sockfd1, command, size);
	if (bytesWritten < 0)
	  std::cout << "There was an error writing to the TCP socket" << std::endl;
	else
	  std::cout << bytesWritten << " out of " << size << " bytes is  written to the TCP socket" << std::endl;
      }
    }


    
private:
    unsigned m_run, m_ev;
    std::string WARN_MSG;
    std::string m_piSynch_IP, m_RDOUTMask;
    std::string m_piRDOUT_IPs[4];
    std::string m_sourceDirPath_Synch, m_targetDirPath_Synch, m_resetConfiguration_Synch_exe_fileName, m_sourceDirPath_Readout, m_targetDirPath_Readout, m_resetConfiguration_Readout_exe_fileName, m_sourceDirPath_Skirocs, m_targetDirPath_Skirocs, m_resetConfiguration_Skirocs_exe_fileName;
    std::string m_skirocIndices_RDOUTs[4];
    unsigned m_ski;
    bool m_stopping, m_stopped, m_done, m_started, m_running, m_configured;
    int m_sockfd1, m_sockfd2; //TCP and UDP socket connection file descriptors (fd)
    //std::mutex m_mufd;

    std::string m_rpi_1_ip;
    int m_portTCP, m_portUDP;

    std::time_t m_last_readout_time;

    std::ofstream m_rawFile;

    char m_raw32bitData[RAW_EV_SIZE_32];

    bool m_gotPart1, m_gotPart2;

};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char ** argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("RPI Test producer", "1.0",
      "Just an example, modify it to suit your own needs");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
      "tcp://localhost:44000", "address",
      "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "RPI", "string",
      "The name of this Producer");
  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    RpiProducer producer(name.Value(), rctrl.Value());
    // And set it running...
    producer.ReadoutLoop();
    // When the readout loop terminates, it is time to go
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}
