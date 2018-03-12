#! /usr/bin/env python

import sys,os,datetime
from ROOT import *

import argparse
parser = argparse.ArgumentParser(description='Ploting my plots', usage="./plot.py -f inputFileName")
parser.add_argument("-f",  dest="fname", type=str, default='../run001192.raw.root',
                    help="File with histograms")
parser.add_argument("-o",  dest="outdir", type=str, default='./RUN_1192_OnlineMon',
                    help="Output directory")
parser.add_argument("-e", "--events",  dest="events", action="store_true", default=False,
                    help="Make event display plots")
parser.add_argument("-v", "--verbosity",  dest="verb", action="store_true", default=False,
                    help="Print out more stuff")

opt = parser.parse_args()

if not opt.verb:
    gROOT.ProcessLine("gErrorIgnoreLevel = 1001;")


gROOT.SetBatch()
gStyle.SetPalette(56)
gStyle.SetNumberContours(50)

c1 = TCanvas("c1", "c1", 500, 500)

def createDir(myDir):
    if opt.verb: print 'Creating a new directory: ', myDir
    if not os.path.exists(myDir):
        try: os.makedirs(myDir)
        except OSError:
            if os.path.isdir(myDir): pass
            else: raise
    else:
        if opt.verb: print "\t OOps, it already exists"


def getall(d, basepath="/"):
    "Generator function to recurse into a ROOT file/dir and yield (path, obj) pairs"
    for key in d.GetListOfKeys():
        kname = key.GetName()
        if key.IsFolder():
            # TODO: -> "yield from" in Py3
            for i in getall(d.Get(kname), basepath+kname+"/"):
                yield i
        else:
            yield basepath+kname, d.Get(kname)

def drawIt(key, obj):

    if not opt.events and  'Display_Event_' in key:
        # Don't make event display plots for now 
        return

    gStyle.SetOptStat(0)
    c1.SetRightMargin(0.06)
    
    drawOpt = ""
    if obj.InheritsFrom("TH2"):
        drawOpt="COLZ2"
        
    if obj.InheritsFrom("TH2Poly"):
        drawOpt="COLZ2 TEXT"
        
    histName = obj.GetName()
                
    if 'WireChamber' in key and 'h_reco' in histName:
        obj.SetStats(1)
        gStyle.SetOptStat(1111)

    obj.Draw(drawOpt)
    
    if obj.InheritsFrom("TH2Poly") or obj.InheritsFrom("TH2"):
        c1.SetRightMargin(0.15)
        gPad.Update();
        palette = obj.GetListOfFunctions().FindObject("palette")
        if palette != None:
                        
            palette.SetX1NDC(0.85)
            palette.SetX2NDC(0.90)
            palette.SetY1NDC(0.1)
            palette.SetY2NDC(0.9)
            gPad.Modified()
            gPad.Update()  


    if 'Hexagon' in key and 'Display_Event_' in histName:
        # Here are the event display plots (opt.events must be true)
        evNum = histName.split('_')[5]
        #print histName
        #print 'Ev number=', evNum
        evDir = opt.outdir+'/EventDisplays_HG_ADC/%05d'%int(evNum)
        #createDir(evDir)
        #c1.SaveAs(evDir+'/'+histName+'.png')
        return
    
    fullDir = opt.outdir+'/'.join(key.split('/')[0:-1])
    # print 'my full dir= ', fullDir
    # print key
    createDir(fullDir)
    c1.SaveAs(fullDir+'/'+histName+'.png')

    # For some specific plots, we save them also in the main directory:
    if any(word in histName for word in ["hexagons_occ_HA_bit", "p_waveform_HG"]):
        fullDir = opt.outdir+'/'.join(key.split('/')[0])
        c1.SaveAs(fullDir+'/'+histName+'.png')
    if any (word in histName for word in ["h_hitmap_Calice", "XYmap_WireChamber"]):
        fullDir = opt.outdir+'/'.join(key.split('/')[0:2])
        c1.SaveAs(fullDir+'/'+histName+'.png')
    
if __name__ == "__main__":

    f = TFile(opt.fname,'OPEN')
    
    print "Making all the plots from root file:", opt.fname
    print "Will put them in directory:", opt.outdir
    for k, o in getall(f):
        if not o.IsFolder():
            drawIt(k,o)
              
    print "Done with all the plots!"


        
