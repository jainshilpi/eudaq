#!/usr/bin/env python

# Echo client program
import socket
import sys, time

if __name__ == "__main__":

  print sys.argv
  if len(sys.argv) < 3:
    print 'Usage : python tcpclient.py hostname command'
    sys.exit()

  HOST = sys.argv[1]
  PORT = 55511

  command = sys.argv[2]
  
  s = None
  
  for res in socket.getaddrinfo(HOST, PORT, socket.AF_UNSPEC, socket.SOCK_STREAM):
      af, socktype, proto, canonname, sa = res
      try:
          s = socket.socket(af, socktype, proto)
      except socket.error as msg:
          s = None
          print 'Cant create socket'
          continue
      try:
          s.connect(sa)
      except socket.error as msg:
          s.close()
          s = None
          print 'Cant connect() socket'
          continue
      break
  if s is None:
      print 'could not open socket'
      sys.exit(1)
  


  try :
    #Set the whole string
    print 'Sending command:', command
    s.send(command)
    time.sleep(0.1)
    print 'Message sent successfully'

  except socket.error:
    #Send failed
    print 'Send failed'
    sys.exit()


  data = s.recv(1024)
  s.close()
  print 'Received answer:', repr(data)


