#!/usr/bin/python

#
# This script is to be run on RPI which does data-acquisition.
# It could be tested also from any other machine.

# Usage:
#

import socket
import sys, io, argparse, subprocess
from thread import *
import threading
import time
import numpy as np


t1_stop = threading.Event()

EventRate = 30 # Events per spill
SpillTime = 1  # Duration of the spill in seconds
InterSpill = 1 # Time between spills in seconds

RAW_EV_SIZE = 30787 # 8 bit format

parser =  argparse.ArgumentParser(description='RPI DAQ: a TCP server')
parser.add_argument('-f', '--inputFile', dest="fname", type=str, default=None, required=True,
                    help="Raw data file to open")
parser.add_argument('--bit', dest="bit", type=int,  choices=[8,32], default=32,
                    help="Format of the input data file (32 or 8 bit)")
parser.add_argument('--tport', dest="tport", type=int,  choices=[55511, 55311], default=55511,
                    help="TCP port for connection")
parser.add_argument('--uport', dest="uport", type=int,  choices=[55512, 55312], default=55512,
                    help="UDP port for connection")

opt = parser.parse_args()

#MYIP = '192.168.222.3'
MYIP = '127.0.0.1'
PORTTCP = opt.tport

HOSTUDP = '' # To be determined from connection
PORTUDP = opt.uport

def sendData(sudp, stop_event):

  # Here read the file with some raw data from HexaBoard
  # fname = 'RUN_170317_0912.raw' # This file is not on github, get it elsewhere
  rawData = None
  try:
    f = io.open(opt.fname, "rb")
    print f
  except EnvironmentError:
    print "Can't open file?", opt.fname
    raise
  
  ev=0
  
  while (not stop_event.is_set()):

    if opt.bit==8:
      rawData = f.read(RAW_EV_SIZE)
    else:
      rawData = f.read(RAW_EV_SIZE*4+4)
    
    if ev%EventRate==0:
      stop_event.wait(InterSpill)

    if rawData=='':
      print 'We reached EOF'
      print 'Will wait for a bit, maybe more data will appear'
      time.sleep(3)
      continue
    
    if rawData!=None:
      d = rawData
      print rawData[0:4].encode('hex') , len(d)
      if opt.bit==8:
        # d = rawData[evmod*RAW_EV_SIZE:(evmod+1)*RAW_EV_SIZE]
        npd = np.fromstring(d, dtype=np.uint8)
        #print len(npd), npd.nbytes, npd
        npd32 = npd.astype(np.uint32)
        #print len(npd32), npd32.nbytes, npd32.nbytes/4, npd32
        half1 = npd32[0:15500]
        half2 = np.append(npd32[15500:], [0x0a0b0c0d]).astype(np.uint32)
        
      else:
        npd32 = np.fromstring(d, np.uint32)

        half1 = npd32[0:15500]
        half2 = npd32[15500:]

      print len(half1), half1.nbytes, len(half2), half2.nbytes, half2.dtype
      print half2[-3], half2[-2], half2[-1]
      
    else:
      d = 'Hello World'

    print 'Submitting data, ev: ', ev,  len(d)

    try:
      # print 'UDP host:', HOSTUDP
      # The data is too large now to send in one go. So, plit it in two packets:
      sudp.sendto(half1, (HOSTUDP,PORTUDP))
      time.sleep(0.005)
      sudp.sendto(half2, (HOSTUDP,PORTUDP))
      #conn.sendall('-->>>  Here is your data <<< --')
    except socket.error:
      print 'The client socket is probably closed. We stop sending data.'
      break
    time.sleep(SpillTime/EventRate)
    # stop_event.wait(SpillTime/EventRate)
    ev+=1
    

def clientthread(conn, sudp):
    #Sending message to connected client
    #conn.send('Welcome to the server. Receving Data...\n')

    #infinite loop so that function do not terminate and thread do not end.

    pro = None # To store the Popen instance of the local daq executable
    
    while True:
        #Receiving from client
        try:
            data = conn.recv(1024)
        except socket.error:
            print 'The client socket is probably closed. break this thread'
            break

        if not data:
            break
        
        print 'Command recieved:', str(data), type(data)

        if str(data)[0:9]=='START_RUN':
            t1_stop.clear()
            # First, let's reply to the DAQ that we received her command
            conn.sendall('GOOD_START\n')

            # Now let's start the local daq code
            #pro = subprocess.Popen(['./a.out','5'], stdin=None, stdout=None, stderr=None, shell=False)

            # Let's take a quick nap.
            #time.sleep(0.2)

            # And start a thread to trasmit raw data to DAQ server via UDP:
            start_new_thread(sendData, (sudp,t1_stop))
            
            print 'Sent GOOD_START confirmation'

        elif str(data)=='STOP_RUN':
            t1_stop.set()
            try:
              ret = pro.poll()
            except:
              ret = -1
            #while (ret==None):
            #  print 'We must wait until the daq finishes, because it is too fragile to kill'
            # time.sleep(1)
              
            print 'Return code of the daq:', ret
            
            time.sleep(2)
            conn.sendall('STOPPED_OK')
            #sudp.sendto('STOPPED_OK', (HOST,PORTUDP))
            print 'Sent STOPPED_OK confirmation'
        else:
            conn.sendall('Message Received at the server: %s\n' % str(data))
            #sudp.sendto('Message Received at the server: %s\n' % str(data), (HOST,PORTUDP))


    conn.close()
            

if __name__ == "__main__":
  
  # This socket is to be used for communication:
  sTcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  
  print 'Socket created:', sTcp

  try:
    sTcp.bind((MYIP, PORTTCP))
  except socket.error , msg:
    print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
    sys.exit()

  print 'Socket bind complete'

  sTcp.listen(3)
  print 'Socket now listening'

  # This one is to be used for sending the data
  sUdp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  
  print 'Socket created:', sUdp

  '''
  try:
    sUdp.bind((HOSTUDP, PORTUDP))
  except socket.error , msg:
    print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
    sys.exit()
  '''
  
  try:
    while True:
      # wait to accept a connection
      conn, addr = sTcp.accept()
      print 'Connected with ' + addr[0] + ':' + str(addr[1])

      HOSTUDP=addr[0]

      #start new thread
      start_new_thread(clientthread ,(conn,sUdp,))

  finally:
    sTcp.close()
    sUdp.close()
