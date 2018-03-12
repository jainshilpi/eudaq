#!/usr/bin/python

import sys, io, argparse
import time
import numpy as np
import fnmatch, os

RAW_EV_SIZE = 30787

parser =  argparse.ArgumentParser(description='RPI DAQ: a TCP server')
parser.add_argument('-f', '--inputFile', dest="fname", type=str, default=None,
                    help="Raw data file to open")
opt = parser.parse_args()

def convertData(fname):


  run_pos = fname.find('RUN_')
    
  rawData = None
  try:
    f_in  = io.open(fname, "rb")
    f_out = open('HexaData_Run%s.raw'%fname[run_pos+4:run_pos+8], "wb")
    
    print f_in, f_out
    
  except EnvironmentError:
    print "Can't open file?", opt.fname
    raise
  
  ev=0

  
  while (not rawData==''):

    rawData = f_in.read(RAW_EV_SIZE)
    
    if rawData=='':
      print 'We reached EOF'

      continue
    
    if rawData!=None:
      d = rawData
      # print rawData[0].encode('hex') , len(d)

      npd = np.fromstring(d, dtype=np.uint8)

      npd32 = npd.astype(np.uint32)
      # print len(npd32), npd32.nbytes, npd32.nbytes/4, npd32
      plusOne = np.append(npd32[:], [0x0a0b0c0d]).astype(np.uint32) # this is to check endienness
      
      # print len(plusOne), plusOne.nbytes

      plusOne.tofile(f_out)
      
    else:
      d = 'Hello World'

    ev+=1
    

if __name__ == "__main__":
  
  if opt.fname!=None:
    convertData(opt.fname)
  

  else:

    dirToData = '/disk2_2TB/data/'
    for run in range(8000, 8005+1):
      for fn in os.listdir(dirToData):
        if fnmatch.fnmatch(fn, '*RUN_'+str(run)+'_*.raw.txt'):
          print 'Doing file: ', fn
          convertData(dirToData+fn)
