#ifndef EUDAQ_INCLUDED_DataCollector
#define EUDAQ_INCLUDED_DataCollector

#include "eudaq/TransportServer.hh"
#include "eudaq/CommandReceiver.hh"
#include "eudaq/Event.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Processor.hh"
#include "eudaq/Factory.hh"

#include <string>
#include <vector>
#include <list>
#include <memory>
#include <atomic>

namespace eudaq {
  class DataCollector;
  
#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<DataCollector>;
  extern template DLLEXPORT std::map<uint32_t, typename Factory<DataCollector>::UP_BASE (*)
				     (const std::string&, const std::string&)>&
  Factory<DataCollector>::Instance<const std::string&, const std::string&>();
#endif
  
  class DLLEXPORT DataCollector : public CommandReceiver {
  public:
    DataCollector(const std::string &name, const std::string &runcontrol);
    void OnConfigure(const Configuration &param) override final;
    void OnServer() override final;
    void OnStartRun(uint32_t) override final;
    void OnStopRun() override final;
    void OnTerminate() override final;
    void OnReset() override final;
    void OnStatus() override final;
    void OnData(const std::string &param) override final{};
    void Exec() override;

    virtual void DoConfigure(const Configuration &param){};
    virtual void DoStartRun(uint32_t){};
    virtual void DoStopRun(){};
    virtual void DoTerminate(){};
    virtual void DoReceive(const ConnectionInfo &id, EventUP ev) = 0;
    void WriteEvent(EventUP ev);
    const Configuration& GetConfiguration()const {return m_config;} 
  private:
    void DataHandler(TransportEvent &ev);
    
  private:
    bool m_done;
    std::unique_ptr<TransportServer> m_dataserver;
    FileWriterUP m_writer;
    std::vector<ConnectionInfo> m_info_pdc;
    uint32_t m_dct_n;
    uint32_t m_run_n;
    uint32_t m_evt_c;
    Configuration m_config;
  };
}

#endif // EUDAQ_INCLUDED_DataCollector