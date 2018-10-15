int PlaneIDtoLayerMapping(int id_plane){

  int ilayer = 0;
  if(id_plane>94) return ilayer;
  
  if(id_plane<28) return ilayer+1; //EE

  ilayer = 25 + (int)(id_plane/7);
  
  return ilayer;
  


}
