#ifndef H_IPBUSCONTROLLER_HH
#define H_IPBUSCONTROLLER_HH

#include <vector>
#include <string>
#include <iostream>

namespace uhal{
  class HwInterface;
}
using namespace uhal;

namespace ipbus {

  class IpbusHwController{
  public:
    IpbusHwController(const std::string & connectionFile, const std::string & deviceName);
    ~IpbusHwController();
  private:
    uhal::HwInterface *m_hw;
    std::vector<uint32_t> m_data;
  public:
    uint32_t ReadRegister(const std::string & name);
    void SetRegister(const std::string & name, uint32_t val);
    void ReadDataBlock(const std::string & blkName, uint32_t blkSize);
    void SetUhalLogLevel(unsigned char lvl);
    void ResetTheData();
    //uhal::HwInterface *getInterface() const {return m_hw;};
    
    std::vector<uint32_t> const & getData(){return m_data;}
  private:
    //void CastTheData(const uhal::ValVector<uint32_t> &data);
  };
}

#endif
