/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/* $Id$ */

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  			ZDC digitizer class                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>

// --- ROOT system
#include <TTree.h>
#include <TFile.h>
#include <TNtuple.h>
#include <TRandom.h>

// --- AliRoot header files
#include "AliLog.h"
#include "AliRun.h"
#include "AliHeader.h"
#include "AliGenHijingEventHeader.h"
#include "AliRunDigitizer.h"
#include "AliRunLoader.h"
#include "AliCDBManager.h"
#include "AliCDBEntry.h"
#include "AliZDCSDigit.h"
#include "AliZDCDigit.h"
#include "AliZDCFragment.h"
#include "AliZDCv3.h"
#include "AliZDCDigitizer.h"

class AliCDBStorage;
class AliZDCPedestals;
class AliZDCCalib;

ClassImp(AliZDCDigitizer)


//____________________________________________________________________________
AliZDCDigitizer::AliZDCDigitizer() :
  fIsCalibration(0), 
  fIsSignalInADCGate(kFALSE),
  fFracLostSignal(0.),
  fPedData(0), 
  fCalibData(0),
  fSpectators2Track(kFALSE)
{
// Default constructor    

}

//____________________________________________________________________________
AliZDCDigitizer::AliZDCDigitizer(AliRunDigitizer* manager):
  AliDigitizer(manager),
  fIsCalibration(0), //By default the simulation doesn't create calib. data
  fIsSignalInADCGate(kFALSE),
  fFracLostSignal(0.),
  fPedData(GetPedData()), 
  fCalibData(GetCalibData()),
  fSpectators2Track(kFALSE)
{
  // Get calibration data
  if(fIsCalibration!=0) printf("\n\t AliZDCDigitizer -> Creating calibration data (pedestals)\n");

}

//____________________________________________________________________________
AliZDCDigitizer::~AliZDCDigitizer()
{
// Destructor
// Not implemented
}


//____________________________________________________________________________
AliZDCDigitizer::AliZDCDigitizer(const AliZDCDigitizer &digitizer):
  AliDigitizer(),
  fIsCalibration(digitizer.fIsCalibration),
  fIsSignalInADCGate(digitizer.fIsSignalInADCGate),
  fFracLostSignal(digitizer.fFracLostSignal),
  fPedData(digitizer.fPedData),
  fCalibData(digitizer.fCalibData),
  fSpectators2Track(digitizer.fSpectators2Track)
{
  // Copy constructor

  for(Int_t i=0; i<6; i++){
     for(Int_t j=0; j<5; j++){
        fPMGain[i][j]   = digitizer.fPMGain[i][j];           
     }
  }
  for(Int_t i=0; i<2; i++) fADCRes[i] = digitizer.fADCRes[i];

}

//____________________________________________________________________________
Bool_t AliZDCDigitizer::Init()
{
  // Initialize the digitizer
  // NB -> PM gain vs. HV & ADC resolutions will move to DCDB ***************
   for(Int_t j = 0; j < 5; j++){
     fPMGain[0][j] = 50000.;
     fPMGain[1][j] = 100000.;
     fPMGain[2][j] = 100000.;
     fPMGain[3][j] = 50000.;
     fPMGain[4][j] = 100000.;
     fPMGain[5][j] = 100000.;
   }
   // ADC Caen V965
  fADCRes[0] = 0.0000008; // ADC Resolution high gain: 200 fC/adcCh
  fADCRes[1] = 0.0000064; // ADC Resolution low gain:  25  fC/adcCh

  return kTRUE;
}

//____________________________________________________________________________
void AliZDCDigitizer::Exec(Option_t* /*option*/)
{
  // Execute digitization

  // ------------------------------------------------------------
  // !!! 2nd ZDC set added 
  // *** 1st 3 arrays are digits from REAL (simulated) hits
  // *** last 2 are copied from simulated digits
  // --- pm[0][...] = light in ZN side C  [C, Q1, Q2, Q3, Q4]
  // --- pm[1][...] = light in ZP side C [C, Q1, Q2, Q3, Q4]
  // --- pm[2][...] = light in ZEM [x, 1, 2, x, x]
  // --- pm[3][...] = light in ZN side A [C, Q1, Q2, Q3, Q4] ->NEW!
  // --- pm[4][...] = light in ZP side A [C, Q1, Q2, Q3, Q4] ->NEW!
  // ------------------------------------------------------------
  Float_t pm[5][5]; 
  for(Int_t iSector1=0; iSector1<5; iSector1++) 
    for(Int_t iSector2=0; iSector2<5; iSector2++){
      pm[iSector1][iSector2] = 0;
    }
    
  // ------------------------------------------------------------
  // ### Out of time ADC added (22 channels)
  // --- same codification as for signal PTMs (see above)
  // ------------------------------------------------------------
  Float_t pmoot[5][5];
  for(Int_t iSector1=0; iSector1<5; iSector1++) 
    for(Int_t iSector2=0; iSector2<5; iSector2++){
      pmoot[iSector1][iSector2] = 0;
    }

  // impact parameter and number of spectators
  Float_t impPar = -1;
  Int_t specNTarg = 0, specPTarg = 0;
  Int_t specNProj = 0, specPProj = 0;
  Float_t signalTime0 = 0.;

  // loop over input streams
  for(Int_t iInput = 0; iInput<fManager->GetNinputs(); iInput++){

    // get run loader and ZDC loader
    AliRunLoader* runLoader = 
      AliRunLoader::GetRunLoader(fManager->GetInputFolderName(iInput));
    AliLoader* loader = runLoader->GetLoader("ZDCLoader");
    if(!loader) continue;

    // load sdigits
    loader->LoadSDigits();
    TTree* treeS = loader->TreeS();
    if(!treeS) continue;
    AliZDCSDigit sdigit;
    AliZDCSDigit* psdigit = &sdigit;
    treeS->SetBranchAddress("ZDC", &psdigit);

    // loop over sdigits
    for(Int_t iSDigit=0; iSDigit<treeS->GetEntries(); iSDigit++){
      treeS->GetEntry(iSDigit);
      //
      if(!psdigit) continue;
      if((sdigit.GetSector(1) < 0) || (sdigit.GetSector(1) > 4)){
	AliError(Form("\nsector[0] = %d, sector[1] = %d\n", 
                      sdigit.GetSector(0), sdigit.GetSector(1)));
	continue;
      }
      // Checking if signal is inside ADC gate
      if(iSDigit==0) signalTime0 = sdigit.GetTrackTime();
      else{
        // Assuming a signal lenght of 20 ns, signal is in gate if 
	// signal ENDS in signalTime0+50. -> sdigit.GetTrackTime()+20<=signalTime0+50.
        if(sdigit.GetTrackTime()<=signalTime0+30.) fIsSignalInADCGate = kTRUE;
        if(sdigit.GetTrackTime()>signalTime0+30.){
	  fIsSignalInADCGate = kFALSE;
	  // Vedi quaderno per spiegazione approx. usata 
	  // nel calcolo della fraz. di segnale perso
	  fFracLostSignal = (sdigit.GetTrackTime()-30)*(sdigit.GetTrackTime()-30)/280.;
	}
      }
      Float_t sdSignal = sdigit.GetLightPM();
      if(fIsSignalInADCGate == kFALSE){
        AliWarning(Form("\t Fraction %f of ZDC signal 4 det.(%d, %d) out of ADC gate\n",
		fFracLostSignal,sdigit.GetSector(0),sdigit.GetSector(1)));
	sdSignal = (1-fFracLostSignal)*sdSignal;
      }
      
      pm[(sdigit.GetSector(0))-1][sdigit.GetSector(1)] += sdigit.GetLightPM();
      /*printf("\n\t Detector %d, Tower %d -> pm[%d][%d] = %.0f \n",
      	  sdigit.GetSector(0), sdigit.GetSector(1),sdigit.GetSector(0)-1,
      	  sdigit.GetSector(1), pm[sdigit.GetSector(0)-1][sdigit.GetSector(1)]); // Chiara debugging!
      */
      
    }

    loader->UnloadSDigits();

    // get the impact parameter and the number of spectators in case of hijing
    if(!runLoader->GetAliRun()) runLoader->LoadgAlice();
    AliHeader* header = runLoader->GetHeader();
    if(!header) continue;
    AliGenEventHeader* genHeader = header->GenEventHeader();
    if(!genHeader) continue;
    if(!genHeader->InheritsFrom(AliGenHijingEventHeader::Class())) continue;
    
    if(fSpectators2Track==kTRUE){
      impPar = ((AliGenHijingEventHeader*) genHeader)->ImpactParameter(); 
      specNProj = ((AliGenHijingEventHeader*) genHeader)->ProjSpectatorsn();
      specPProj = ((AliGenHijingEventHeader*) genHeader)->ProjSpectatorsp();
      specNTarg = ((AliGenHijingEventHeader*) genHeader)->TargSpectatorsn();
      specPTarg = ((AliGenHijingEventHeader*) genHeader)->TargSpectatorsp();
      printf("\n\t AliZDCDigitizer: b = %1.2f fm\n"
      " \t    PROJ.:  #spectator n %d, #spectator p %d\n"
      " \t    TARG.:  #spectator n %d, #spectator p %d\n\n", 
      impPar, specNProj, specPProj, specNTarg, specPTarg);
    }
    
  }

  // Applying fragmentation algorithm and adding spectator signal
  if(fSpectators2Track==kTRUE && impPar) {
    Int_t freeSpecNProj, freeSpecPProj;
    Fragmentation(impPar, specNProj, specPProj, freeSpecNProj, freeSpecPProj);
    Int_t freeSpecNTarg, freeSpecPTarg;
    Fragmentation(impPar, specNTarg, specPTarg, freeSpecNTarg, freeSpecPTarg);
    SpectatorSignal(1, freeSpecNProj, pm);
    //printf("    AliZDCDigitizer -> Signal for %d PROJ free spectator n",freeSpecNProj);
    SpectatorSignal(2, freeSpecPProj, pm);
    //printf(" and %d free spectator p added\n",freeSpecPProj);
    SpectatorSignal(3, freeSpecNTarg, pm);
    //printf("    AliZDCDigitizer -> Signal for %d TARG free spectator n",freeSpecNTarg);
    SpectatorSignal(4, freeSpecPTarg, pm);
    //printf("and %d free spectator p added\n",freeSpecPTarg);
  }


  // get the output run loader and loader
  AliRunLoader* runLoader = 
    AliRunLoader::GetRunLoader(fManager->GetOutputFolderName());
  AliLoader* loader = runLoader->GetLoader("ZDCLoader");
  if(!loader) {
    AliError("no ZDC loader found");
    return;
  }

  // create the output tree
  const char* mode = "update";
  if(runLoader->GetEventNumber() == 0) mode = "recreate";
  loader->LoadDigits(mode);
  loader->MakeTree("D");
  TTree* treeD = loader->TreeD();
  AliZDCDigit digit;
  AliZDCDigit* pdigit = &digit;
  const Int_t kBufferSize = 4000;
  treeD->Branch("ZDC", "AliZDCDigit", &pdigit, kBufferSize);

  // Create digits
  Int_t sector[2];
  Int_t digi[2], digioot[2];
  for(sector[0]=1; sector[0]<6; sector[0]++){
    for(sector[1]=0; sector[1]<5; sector[1]++){
        if((sector[0]==3) && ((sector[1]<1) || (sector[1]>2))) continue;
        for(Int_t res=0; res<2; res++){
           digi[res] = Phe2ADCch(sector[0], sector[1], pm[sector[0]-1][sector[1]], res) 
	            + Pedestal(sector[0], sector[1], res);
      	}
	/*printf("\t DIGIT added -> det %d quad %d - digi[0,1] = [%d, %d]\n",
	     sector[0], sector[1], digi[0], digi[1]); // Chiara debugging!
	*/
	//
	new(pdigit) AliZDCDigit(sector, digi);
        treeD->Fill();
    }
  } // Loop over detector
  // Adding in-time digits for 2 reference PTM signals (after signal ch.)
  // (for the moment the ref. signal is completely invented assuming a PMgain of 5*10^4!)
  Int_t sectorRef[2];
  sectorRef[1] = 5;
  Int_t sigRef[2];
  // Reference signal are set to 100 (high gain chain) and 800 (low gain chain)
  if(fIsCalibration==0) {sigRef[0]=100;  sigRef[1]=800;}
  else {sigRef[0]=0;  sigRef[1]=0;} // calibration -> simulation of pedestal values
  //
  for(Int_t iref=0; iref<2; iref++){
     sectorRef[0] = 3*iref+1;
     for(Int_t res=0; res<2; res++){
       sigRef[res] += Pedestal(sectorRef[0], sectorRef[1], res);
     }
     /*printf("\t RefDigit added -> det = %d, quad = %d - digi[0,1] = [%d, %d]\n",
         sectorRef[0], sectorRef[1], sigRef[0], sigRef[1]); // Chiara debugging!
     */
     new(pdigit) AliZDCDigit(sectorRef, sigRef);
     treeD->Fill();     
  }
  //
  // --- Adding digits for out-of-time channels after signal digits
  for(sector[0]=1; sector[0]<6; sector[0]++){
    for(sector[1]=0; sector[1]<5; sector[1]++){
        if((sector[0]==3) && ((sector[1]<1) || (sector[1]>2))) continue;
        for(Int_t res=0; res<2; res++){
           digioot[res] = Pedestal(sector[0], sector[1], res); // out-of-time ADCs
      	}
	/*printf("\t DIGIToot added -> det = %d, quad = %d - digi[0,1] = [%d, %d]\n",
	     sector[0], sector[1], digioot[0], digioot[1]); // Chiara debugging!
	*/
	//
	new(pdigit) AliZDCDigit(sector, digioot);
        treeD->Fill();
    }
  }
  // Adding out-of-time digits for 2 reference PTM signals (after out-of-time ch.)
  Int_t sigRefoot[2];
  for(Int_t iref=0; iref<2; iref++){
     sectorRef[0] = 3*iref+1;
     for(Int_t res=0; res<2; res++){
       sigRefoot[res] = Pedestal(sectorRef[0], sectorRef[1], res);
     }
     /*printf("\t RefDigitoot added -> det = %d, quad = %d - digi[0,1] = [%d, %d]\n",
         sectorRef[0], sectorRef[1], sigRefoot[0], sigRefoot[1]); // Chiara debugging!
     */
     new(pdigit) AliZDCDigit(sectorRef, sigRefoot);
     treeD->Fill();
  }
  //printf("\t AliZDCDigitizer -> TreeD has %d entries\n",(Int_t) treeD->GetEntries());

  // write the output tree
  loader->WriteDigits("OVERWRITE");
  loader->UnloadDigits();
}


//_____________________________________________________________________________
void AliZDCDigitizer::Fragmentation(Float_t impPar, Int_t specN, Int_t specP,
                                    Int_t &freeSpecN, Int_t &freeSpecP) const
{
// simulate fragmentation of spectators

  AliZDCFragment frag(impPar);

  // Fragments generation
  frag.GenerateIMF();
  Int_t nAlpha = frag.GetNalpha();

  // Attach neutrons
  Int_t ztot = frag.GetZtot();
  Int_t ntot = frag.GetNtot();
  frag.AttachNeutrons();
  freeSpecN = specN-ntot-2*nAlpha;
  freeSpecP = specP-ztot-2*nAlpha;
  // Removing deuterons
  Int_t ndeu = (Int_t) (freeSpecN*frag.DeuteronNumber());
  freeSpecN -= ndeu;
  //
  if(freeSpecN<0) freeSpecN=0;
  if(freeSpecP<0) freeSpecP=0;
  AliDebug(2, Form("FreeSpn = %d, FreeSpp = %d", freeSpecN, freeSpecP));
}

//_____________________________________________________________________________
void AliZDCDigitizer::SpectatorSignal(Int_t SpecType, Int_t numEvents, 
                                      Float_t pm[5][5]) const
{
// add signal of the spectators
  
  TString hfn; 
  if(SpecType == 1) {	   // --- Signal for projectile spectator neutrons
    hfn = "$ALICE_ROOT/ZDC/ZNCSignal.root";
  } 
  else if(SpecType == 2) { // --- Signal for projectile spectator protons
    hfn = "$ALICE_ROOT/ZDC/ZPCSignal.root";
  }
  else if(SpecType == 3) { // --- Signal for target spectator neutrons
    hfn = "$ALICE_ROOT/ZDC/ZNASignal.root";
  }
  else if(SpecType == 4) { // --- Signal for target spectator protons
    hfn = "$ALICE_ROOT/ZDC/ZPASignal.root";
  }
  
  TFile* file = TFile::Open(hfn);
  if(!file || !file->IsOpen()) {
    AliError((Form(" Opening file %s failed\n",hfn.Data())));
    return;
  }

  TNtuple* zdcSignal = (TNtuple*) file->Get("ZDCSignal");
  Int_t nentries = (Int_t) zdcSignal->GetEntries();
  
  Float_t *entry;
  Int_t pl, i, k, iev=0, rnd[125], volume[2];
  for(pl=0;pl<125;pl++) rnd[pl] = 0;
  if(numEvents > 125) {
    AliWarning(Form("numEvents (%d) is larger than 125", numEvents));
    numEvents = 125;
  }
  for(pl=0;pl<numEvents;pl++){
     rnd[pl] = (Int_t) (9999*gRandom->Rndm());
     if(rnd[pl] >= 9999) rnd[pl] = 9998;
     //printf("	rnd[%d] = %d\n",pl,rnd[pl]);     
  }
  // Sorting vector in ascending order with C function QSORT 
  qsort((void*)rnd,numEvents,sizeof(Int_t),comp);
  do{
     for(i=0; i<nentries; i++){  
  	zdcSignal->GetEvent(i);
  	entry = zdcSignal->GetArgs();
  	if(entry[0] == rnd[iev]){
          for(k=0; k<2; k++) volume[k] = (Int_t) entry[k+1];
	  //
	  Float_t lightQ = entry[7];
	  Float_t lightC = entry[8];
	  //
	  if(volume[0] != 3) {  // ZN or ZP
            pm[volume[0]-1][0] += lightC;
            pm[volume[0]-1][volume[1]] += lightQ;
	    //printf("\n   pm[%d][0] = %.0f, pm[%d][%d] = %.0f\n",(volume[0]-1),pm[volume[0]-1][0],
	    //	(volume[0]-1),volume[1],pm[volume[0]-1][volume[1]]);
	  } 
	  else { 
            if(volume[1] == 1) pm[2][1] += lightC; // ZEM 1
            else               pm[2][2] += lightQ; // ZEM 2
	    //printf("\n   pm[2][1] = %.0f, pm[2][2] = %.0f\n",pm[2][1],pm[2][2]);
	  }
  	}
  	else if(entry[0] > rnd[iev]){
	  iev++;
	  continue;
	}
     }
  }while(iev<numEvents);
  
  file->Close();
  delete file;
}


//_____________________________________________________________________________
Int_t AliZDCDigitizer::Phe2ADCch(Int_t Det, Int_t Quad, Float_t Light, 
                                 Int_t Res) const
{
  // Evaluation of the ADC channel corresponding to the light yield Light
  Int_t vADCch = (Int_t) (Light * fPMGain[Det-1][Quad] * fADCRes[Res]);
  //printf("\t Phe2ADCch -> det %d quad %d - phe %.0f  ADC %d\n", Det,Quad,Light,vADCch);

  return vADCch;
}

//_____________________________________________________________________________
Int_t AliZDCDigitizer::Pedestal(Int_t Det, Int_t Quad, Int_t Res) const
{
  // Returns a pedestal for detector det, PM quad, channel with res.
  //
  Float_t pedValue;
  // Normal run
  if(fIsCalibration == 0){
    Int_t index=0, kNch=24;
    if(Quad!=5){
      if(Det==1)	index = Quad+kNch*Res;	  // ZNC
      else if(Det==2)	index = (Quad+5)+kNch*Res;  // ZPC
      else if(Det==3)	index = (Quad+9)+kNch*Res;  // ZEM
      else if(Det==4)	index = (Quad+12)+kNch*Res; // ZNA
      else if(Det==5)	index = (Quad+17)+kNch*Res; // ZPA
    }
    else index = (Det-1)/3+22+kNch*Res; // Reference PMs
    //
    Float_t meanPed = fPedData->GetMeanPed(index);
    Float_t pedWidth = fPedData->GetMeanPedWidth(index);
    pedValue = gRandom->Gaus(meanPed,pedWidth);
    //
    /*printf("\t  AliZDCDigitizer::Pedestal -> det %d quad %d res %d - Ped[%d] = %d\n",
  	Det, Quad, Res, index,(Int_t) pedValue); // Chiara debugging!
    */
  }
  // To create calibration object
  else{
    if(Res == 0) pedValue = gRandom->Gaus((35.+10.*gRandom->Rndm()),(0.5+0.2*gRandom->Rndm())); //High gain
    else  pedValue = gRandom->Gaus((250.+100.*gRandom->Rndm()),(3.5+2.*gRandom->Rndm())); //Low gain
  }

  return (Int_t) pedValue;
}

//_____________________________________________________________________________
AliCDBStorage* AliZDCDigitizer::SetStorage(const char *uri) 
{

  Bool_t deleteManager = kFALSE;
  
  AliCDBManager *manager = AliCDBManager::Instance();
  AliCDBStorage *defstorage = manager->GetDefaultStorage();
  
  if(!defstorage || !(defstorage->Contains("ZDC"))){ 
     AliWarning("No default storage set or default storage doesn't contain ZDC!");
     manager->SetDefaultStorage(uri);
     deleteManager = kTRUE;
  }
 
  AliCDBStorage *storage = manager->GetDefaultStorage();

  if(deleteManager){
    AliCDBManager::Instance()->UnsetDefaultStorage();
    defstorage = 0;   // the storage is killed by AliCDBManager::Instance()->Destroy()
  }

  return storage; 
}

//_____________________________________________________________________________
AliZDCPedestals* AliZDCDigitizer::GetPedData() const
{

  // Getting pedestal calibration object for ZDC set

  AliCDBEntry  *entry = AliCDBManager::Instance()->Get("ZDC/Calib/Pedestals");
  if(!entry) AliFatal("No calibration data loaded!");  

  AliZDCPedestals *calibdata = dynamic_cast<AliZDCPedestals*>  (entry->GetObject());
  if(!calibdata)  AliFatal("Wrong calibration object in calibration  file!");

  return calibdata;
}

//_____________________________________________________________________________
AliZDCCalib* AliZDCDigitizer::GetCalibData() const
{

  // Getting calibration object for ZDC set

  AliCDBEntry  *entry = AliCDBManager::Instance()->Get("ZDC/Calib/EMDCalib");
  if(!entry) AliFatal("No calibration data loaded!");  

  AliZDCCalib *calibdata = dynamic_cast<AliZDCCalib*>  (entry->GetObject());
  if(!calibdata)  AliFatal("Wrong calibration object in calibration  file!");

  return calibdata;
}
