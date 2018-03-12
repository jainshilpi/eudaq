
# IPbus in euDAQ
The code of *src/HGCalProducer.cxx* is using IPbus communication for data-taking and it will be used for the beam tests in July of 2017.
It is based on *RpiProducer*, and *miniTLUProducer* and *Controller* codes.

To build and compile the code:

 * Install cactus (aka ipbus): [follow these instructions](https://svnweb.cern.ch/trac/cactus/wiki/uhalQuickTutorial#HowtoInstalltheIPbusSuite).
 * Import libraries paths if needed:
```
export LD_LIBRARY_PATH=/opt/cactus/lib:$LD_LIBRARY_PATH
export PATH=/opt/cactus/bin:$PATH
```

 * Compile. From eudaq root dir:
```
cd build
cmake -DBUILD_cmshgcal=ON ../
make install -j4
```  

 * To actually use it we need to start an ORM data emulation. See [ipbus-test](https://github.com/asteencern/ipbus-test/tree/hgcal-test) repository for intstructions on how to do that. It can be launched from any computer with ipbus installed. Then, IP address of that computer should be used in *producers/cmshgcal/etc/connection.xml* instead of *localhost*.
 * Run this producer (from root of eudaq):  
 ```
 ./STARTRUN cmshgcal
 ```
   * Load config file from *eudaq/producers/cmshgcal/conf/hgcal.conf*, and start the run. It will readout the data and store them in local file in *eudaq/data* directory.
   
 See [our IPbus wiki page](https://github.com/HGCDAQ/eudaq/wiki/IPBus)  for more information.

# RpiProducer
The code of *src/RpiProducer.cxx* was used for the data-taking during beam tests in May of 2017. See [RpiProducer wiki page](https://github.com/HGCDAQ/eudaq/wiki/RpiProducer) for it's description

