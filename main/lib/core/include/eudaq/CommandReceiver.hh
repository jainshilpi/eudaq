#ifndef EUDAQ_INCLUDED_CommandReceiver
#define EUDAQ_INCLUDED_CommandReceiver

#include "eudaq/Status.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"

#include <thread>
#include <memory>
#include <string>
#include <iosfwd>

namespace eudaq {

  class TransportClient;
  class TransportEvent;

  class DLLEXPORT CommandReceiver {
  public:
    CommandReceiver(const std::string & type, const std::string & name,
		    const std::string & runcontrol);
    virtual ~CommandReceiver(){};

    virtual void OnConfigure(const Configuration & param) = 0;
    virtual void OnStartRun(uint32_t /*runnumber*/) = 0;
    virtual void OnStopRun() = 0;
    virtual void OnTerminate(){}
    virtual void OnReset() = 0;
    virtual void OnStatus() {}
    virtual void OnData(const std::string & /*param*/) {}
    virtual void OnLog(const std::string & /*param*/);
    virtual void OnServer() {}
    virtual void OnIdle();
    virtual void OnUnrecognised(const std::string & /*cmd*/, const std::string & /*param*/) {}
    virtual void Exec() = 0;
    
    void Process(int timeout = -1);
    void SetStatus(Status::Level level, const std::string & info = "");
    void SetStatusTag(const std::string &key, const std::string &val){m_status.SetTag(key, val);}
    
  private:
    void CommandHandler(TransportEvent &);

    Status m_status;
    std::unique_ptr<TransportClient> m_cmdclient;
    std::string m_type, m_name;
  };

}

#endif // EUDAQ_INCLUDED_CommandReceiver