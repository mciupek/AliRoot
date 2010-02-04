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

/* $Id: AliTRDv0Info.cxx 27496 2008-07-22 08:35:45Z cblume $ */

////////////////////////////////////////////////////////////////////////////
//                                                                        //
//  Reconstruction QA                                                     //
//                                                                        //
//  Gathers all information necessary for reference data selection about  //
//  the track and (in case) its corresponding V0.                         //
//  Carries out the selection of electrons (from gamma conversions),      //
//  pions (from K0s decays) and protons (from Lambda and Anti-Lambda      //
//  decays) by cuts specific for the respective decay and particle        //
//  species.                                                              //
//  (M.Heide, 2009/10/06)                                                 //
//                                                                        //
//  Authors:                                                              //
//   Alex Bercuci <A.Bercuci@gsi.de>                                      //
//   Alex Wilk    <wilka@uni-muenster.de>                                 //
//   Markus Heide <mheide@uni-muenster.de>                                //
//                                                                        //
////////////////////////////////////////////////////////////////////////////
#include "TMath.h"

#include "AliESDtrack.h"
#include "AliESDv0.h"
#include "AliESDInputHandler.h"
#include "AliAnalysisManager.h"
#include "AliLog.h"

#include "AliTRDv0Info.h"
#include "AliTRDtrackInfo.h"
#include "AliTRDtrackInfo.h"

ClassImp(AliTRDv0Info)

//_________________________________________________
AliTRDv0Info::AliTRDv0Info()
  : TObject()
  ,fESD(0x0)
  ,fHasV0(0)      
  ,fQuality(0)    
  ,fMomentum(0)
  ,fDCA(10)
  ,fPointingAngle(10)
  ,fOpenAngle(10)
  ,fPsiPair(99)
  ,fMagField(0)
  ,fRadius(0)
  ,fTrackID(0)
  ,fV0Momentum(0)
  ,fTrackP(0x0)
  ,fTrackN(0x0)
  ,fTrack(0x0)
  ,fNindex(0)
  ,fPindex(0)
{
  //
  // Default constructor
  //

  memset(fPplus, 0, 2*kNlayer*sizeof(Float_t));
  memset(fPminus, 0, 2*kNlayer*sizeof(Float_t));
  memset(fDetPID, 0, 2*kNDaughters*kNDetectors*AliPID::kSPECIES*sizeof(Float_t));
  memset(fComPID, 0, 2*kNDaughters*AliPID::kSPECIES*sizeof(Float_t));
  memset(fInvMass, 0, kNMomBins*kNDecays*sizeof(Double_t));

  /////////////////////////////////////////////////////////////////////////////
  //Set Cut values: First specify decay in brackets, then the actual cut value!
  ///////////////////////////////////////////////////////////////////////////// 

  //Upper limit for distance of closest approach of two daughter tracks :
  fUpDCA[kGamma] = 1000.;
  fUpDCA[kK0s] = 0.08;
  fUpDCA[kLambda] = 0.2;
  fUpDCA[kAntiLambda] = 0.2;

  //Upper limit for pointing angle (= angle between between vector from primary to secondary vertex and reconstructed momentum of V0 mother particle) :
  fUpPointingAngle[kGamma] = 0.03;
  fUpPointingAngle[kK0s] = 0.03;
  fUpPointingAngle[kLambda] = 0.04;
  fUpPointingAngle[kAntiLambda] = 0.04;

  //Upper limit for invariant mass of V0 mother :
  fUpInvMass[kGamma][0] = 0.05;// second pair of brackets is for momentum bin: 0: below mother momentm of 2.5 GeV
  fUpInvMass[kGamma][1] = 0.07;//1: above 2.5 GeV
  fUpInvMass[kK0s][0] = fUpInvMass[kK0s][1] = 0.50265;
  fUpInvMass[kLambda][0] = fUpInvMass[kLambda][1] = 1.1207;
  fUpInvMass[kAntiLambda][0] = fUpInvMass[kAntiLambda][1] = 1.1207;

  //Lower limit for invariant mass of V0 mother :
  fDownInvMass[kGamma] = -1.;
  fDownInvMass[kK0s] = 0.49265;
  fDownInvMass[kLambda] = 1.107;
  fDownInvMass[kAntiLambda] = 1.107;

  //Lower limit for distance from secondary vertex to primary vertex in x-y plane :
  fDownRadius[kGamma] = 6.;
  fDownRadius[kK0s] = 0.;
  fDownRadius[kLambda] = 0.;
  fDownRadius[kAntiLambda] = 0.;

  //Upper limit for distance from secondary vertex to primary vertex in x-y plane :
  fUpRadius[kGamma] = 1000.;
  fUpRadius[kK0s] = 20.;
  fUpRadius[kLambda] = 1000.;
  fUpRadius[kAntiLambda] = 1000.;

  //Upper limit for opening angle between two daughter tracks (characteristically near zero for conversions) :
  fUpOpenAngle[kGamma] = 0.1;
  fUpOpenAngle[kK0s] = 3.15;
  fUpOpenAngle[kLambda] = 3.15;
  fUpOpenAngle[kAntiLambda] = 3.15;

  //Upper limit for angle between daughter momentum plane and plane perpendicular to magnetic field (characteristically around zero for conversions) :
  fUpPsiPair[kGamma] = 0.05;
  fUpPsiPair[kK0s] = 1.6;
  fUpPsiPair[kLambda] = 1.6;
  fUpPsiPair[kAntiLambda] = 1.6;

  //Lower limit for likelihood value of TPC PID :
  fDownTPCPIDneg[AliPID::kElectron] = 0.21;
  fDownTPCPIDpos[AliPID::kElectron] = 0.21;

  fDownTPCPIDneg[AliPID::kMuon] = 0.21;
  fDownTPCPIDpos[AliPID::kMuon] = 0.21;

  fDownTPCPIDneg[AliPID::kPion] = 0.21;
  fDownTPCPIDpos[AliPID::kPion] = 0.21;

  fDownTPCPIDneg[AliPID::kKaon] = 0.21;
  fDownTPCPIDpos[AliPID::kKaon] = 0.21;

  fDownTPCPIDneg[AliPID::kProton] = 0.21;
  fDownTPCPIDpos[AliPID::kProton] = 0.21;

 //Lower limit for likelihood value of combined PID :
  fDownComPIDneg[AliPID::kElectron] = 0.21;
  fDownComPIDpos[AliPID::kElectron] = 0.21;

  fDownComPIDneg[AliPID::kMuon] = 0.21;
  fDownComPIDpos[AliPID::kMuon] = 0.21;

  fDownComPIDneg[AliPID::kPion] = 0.9;
  fDownComPIDpos[AliPID::kPion] = 0.9;

  fDownComPIDneg[AliPID::kKaon] = 0.21;
  fDownComPIDpos[AliPID::kKaon] = 0.21;

  fDownComPIDneg[AliPID::kProton] = 0.9;
  fDownComPIDpos[AliPID::kProton] = 0.9;

 //Lower limit for likelihood value of combined PID for daughter track which doesn't enter reference data (here: pion daughters from Lambda decays:
  fDownComPIDnegPart[AliPID::kElectron] = 0.05;
  fDownComPIDposPart[AliPID::kElectron] = 0.05;

  fDownComPIDnegPart[AliPID::kMuon] = 0.05;
  fDownComPIDposPart[AliPID::kMuon] = 0.05;

  fDownComPIDnegPart[AliPID::kPion] = 0.05;
  fDownComPIDposPart[AliPID::kPion] = 0.05;

  fDownComPIDnegPart[AliPID::kKaon] = 0.05;
  fDownComPIDposPart[AliPID::kKaon] = 0.05;

  fDownComPIDnegPart[AliPID::kProton] = 0.05;
  fDownComPIDposPart[AliPID::kProton] = 0.05;

  //////////////////////////////////////////////////////////////////////////////////

}

//_________________________________________________
void AliTRDv0Info::GetESDv0Info(AliESDv0 *esdv0)
{//Gets values of ESDv0 and daughter track properties
  //See header file for description of variables

  Int_t part1 = -1;
  Int_t part2 = -1;

  fQuality = Quality(esdv0);//Attributes an Int_t to the V0 due to quality cuts (= 1 if V0 is accepted, other integers depending on cut which excludes the vertex)    

  fRadius = Radius(esdv0);//distance from secondary vertex to primary vertex in x-y plane
      
  fDCA = esdv0->GetDcaV0Daughters();//distance of closest approach of two daughter tracks
      
  fPointingAngle = TMath::ACos(esdv0->GetV0CosineOfPointingAngle());// pointing angle (= angle between between vector from primary to secondary vertex and reconstructed momentum of V0 mother particle)
      
  fOpenAngle = OpenAngle(esdv0);//Opening angle between two daughter tracks
      
  fPsiPair = PsiPair(esdv0);//Angle between daughter momentum plane and plane perpendicular to magnetic field

  fV0Momentum = V0Momentum(esdv0);//Reconstructed momentum of the mother particle
      
  for(Int_t idecay = 0; idecay < kNDecays; idecay++)//4 decay types : conversions, K0s, Lambda, Anti-Lambda 
    //five particle types: electrons, muons, pions, kaons, protons (muons and kaons not involved)
    {
      if(idecay == kLambda)//protons and pions from Lambda
  {
    part1 = AliPID::kProton;
    part2 = AliPID::kPion;
  }
      else if(idecay == kAntiLambda)//antiprotons and pions from Anti-Lambda
  {
    part1 = AliPID::kPion;
    part2 = AliPID::kProton;
  }
      else if(idecay == kK0s)//pions from K0s
  part1 = part2 = AliPID::kPion;
      else if(idecay == kGamma)//electrons from conversions
  part1 = part2 = AliPID::kElectron;
    
      fInvMass[idecay] = InvMass(part1, part2, esdv0);//Calculate invariant mass for all of our four supposed decays
    }
  GetDetectorPID();//Gets all likelihood values from TPC, TOF and ITS PID for the fDetPID[kNDaughters][kNDetectors][AliPID::kSPECIES] array
  CombinePID();//Bayesian combination of likelihoods from TPC and TOF

  fMomentum = fTrack->GetOuterParam()->P();

    
}
//_________________________________________________
Float_t  AliTRDv0Info::V0Momentum(AliESDv0 *esdv0) const
{
  //
  // Reconstructed momentum of V0 mother particle
  //

  Double_t mn[3] = {0,0,0};
  Double_t mp[3] = {0,0,0};


  esdv0->GetNPxPyPz(mn[0],mn[1],mn[2]);//reconstructed cartesian momentum components of negative daughter; 
  esdv0->GetPPxPyPz(mp[0],mp[1],mp[2]);//reconstructed cartesian momentum components of positive daughter;
  
  
  return TMath::Sqrt((mn[0]+mp[0])*(mn[0]+mp[0]) + (mn[1]+mp[1])*(mn[1]+mp[1])+(mn[2]+mp[2])*(mn[2]+mp[2]));
}

//_________________________________________________
Double_t AliTRDv0Info::InvMass(Int_t part1, Int_t part2, AliESDv0 *esdv0) const
{
  //
  // Invariant mass of reconstructed V0 mother
  //

  const Double_t kpmass[5] = {AliPID::ParticleMass(AliPID::kElectron),AliPID::ParticleMass(AliPID::kMuon),AliPID::ParticleMass(AliPID::kPion),AliPID::ParticleMass(AliPID::kKaon),AliPID::ParticleMass(AliPID::kProton)};
  //Masses of electrons, muons, pions, kaons and protons, as implemented in ROOT


  Double_t mn[3] = {0,0,0};
  Double_t mp[3] = {0,0,0};  

  esdv0->GetNPxPyPz(mn[0],mn[1],mn[2]);//reconstructed cartesian momentum components of negative daughter;
  esdv0->GetPPxPyPz(mp[0],mp[1],mp[2]);//reconstructed cartesian momentum components of positive daughter;
  
  Double_t mass1 = kpmass[part1];//sets supposed rest masses for both daughters
  Double_t mass2 = kpmass[part2];   

  //Calculate daughters' energies :
  Double_t e1    = TMath::Sqrt(mass1*mass1+
            mp[0]*mp[0]+
            mp[1]*mp[1]+
            mp[2]*mp[2]);
  Double_t e2    = TMath::Sqrt(mass2*mass2+
            mn[0]*mn[0]+
            mn[1]*mn[1]+
            mn[2]*mn[2]);  

  //Sum of daughter momenta :   
  Double_t momsum =  
    (mn[0]+mp[0])*(mn[0]+mp[0])+
    (mn[1]+mp[1])*(mn[1]+mp[1])+
    (mn[2]+mp[2])*(mn[2]+mp[2]);

  //invariant mass :	  	     
  Double_t mInv = TMath::Sqrt((e1+e2)*(e1+e2)-momsum);

  return mInv;
  
}
//_________________________________________________
Float_t AliTRDv0Info::OpenAngle(AliESDv0 *esdv0)
{//Opening angle between two daughter tracks
  Double_t mn[3] = {0,0,0};
  Double_t mp[3] = {0,0,0};
    

  esdv0->GetNPxPyPz(mn[0],mn[1],mn[2]);//reconstructed cartesian momentum components of negative daughter;
  esdv0->GetPPxPyPz(mp[0],mp[1],mp[2]);//reconstructed cartesian momentum components of positive daughter;

  
  fOpenAngle = TMath::ACos((mp[0]*mn[0] + mp[1]*mn[1] + mp[2]*mn[2])/(TMath::Sqrt(mp[0]*mp[0] + mp[1]*mp[1] + mp[2]*mp[2])*TMath::Sqrt(mn[0]*mn[0] + mn[1]*mn[1] + mn[2]*mn[2])));
  
  return fOpenAngle;
}

//_________________________________________________
Float_t AliTRDv0Info::PsiPair(AliESDv0 *esdv0)
{//Angle between daughter momentum plane and plane perpendicular to magnetic field
  Double_t x, y, z;
  esdv0->GetXYZ(x,y,z);//Reconstructed coordinates of V0; to be replaced by Markus Rammler's method in case of conversions!
  
  Double_t mn[3] = {0,0,0};
  Double_t mp[3] = {0,0,0};
  

  esdv0->GetNPxPyPz(mn[0],mn[1],mn[2]);//reconstructed cartesian momentum components of negative daughter;
  esdv0->GetPPxPyPz(mp[0],mp[1],mp[2]);//reconstructed cartesian momentum components of positive daughter; 


  Double_t deltat = 1.;
  deltat = TMath::ATan(mp[2]/(TMath::Sqrt(mp[0]*mp[0] + mp[1]*mp[1])+1.e-13)) -  TMath::ATan(mn[2]/(TMath::Sqrt(mn[0]*mn[0] + mn[1]*mn[1])+1.e-13));//difference of angles of the two daughter tracks with z-axis

  Double_t radiussum = TMath::Sqrt(x*x + y*y) + 50;//radius to which tracks shall be propagated

  Double_t momPosProp[3];
  Double_t momNegProp[3];
    
  AliExternalTrackParam nt(*fTrackN), pt(*fTrackP);
    
  fPsiPair = 4.;

  if(nt.PropagateTo(radiussum,fMagField) == 0)//propagate tracks to the outside
    fPsiPair =  -5.;
  if(pt.PropagateTo(radiussum,fMagField) == 0)
    fPsiPair = -5.;
  pt.GetPxPyPz(momPosProp);//Get momentum vectors of tracks after propagation
  nt.GetPxPyPz(momNegProp);
  
  Double_t pEle =
    TMath::Sqrt(momNegProp[0]*momNegProp[0]+momNegProp[1]*momNegProp[1]+momNegProp[2]*momNegProp[2]);//absolute momentum value of negative daughter
  Double_t pPos =
    TMath::Sqrt(momPosProp[0]*momPosProp[0]+momPosProp[1]*momPosProp[1]+momPosProp[2]*momPosProp[2]);//absolute momentum value of positive daughter
    
  Double_t scalarproduct =
    momPosProp[0]*momNegProp[0]+momPosProp[1]*momNegProp[1]+momPosProp[2]*momNegProp[2];//scalar product of propagated positive and negative daughters' momenta
    
  Double_t chipair = TMath::ACos(scalarproduct/(pEle*pPos));//Angle between propagated daughter tracks

  fPsiPair =  TMath::Abs(TMath::ASin(deltat/chipair));  

  return fPsiPair; 

}

//_________________________________________________
void AliTRDv0Info::V0fromTrack(AliTRDtrackInfo * const track, Int_t ivertex)
{//Checks if track is a secondary vertex daughter (due to V0 finder)
  
  fMagField = fESD->GetMagneticField();

  fTrackID =  track->GetTrackId();//index of the track

  fTrack = fESD->GetTrack(fTrackID);//sets track information

  fMomentum = fTrack->GetOuterParam()->P();//Get track momentum from outer parameters

  fHasV0 = 0;

  //comparing index of track with indices of pos./neg. V0 daughter :
  AliESDv0 * esdv0 = fESD->GetV0(ivertex);
  if((esdv0->GetIndex(0) == fTrackID)||(esdv0->GetIndex(1) == fTrackID))
    {
      fHasV0 = 1;//track belongs to vertex found by V0 finder!
      fNindex = esdv0->GetIndex(0);
      fPindex = esdv0->GetIndex(1);
      fTrackN = fESD->GetTrack(esdv0->GetIndex(0));//providing information about the other of the two daughter tracks 
      fTrackP = fESD->GetTrack(esdv0->GetIndex(1));
      GetESDv0Info(esdv0);//gets all the relevant information about our V0
    }
}
//_________________________________________________
void AliTRDv0Info::GetDetectorPID()
{//PID likelihoods from TPC, TOF, and ITS, for all particle species

  fTrackN->GetTPCpid(fDetPID[kNeg][kTPC]);
  fTrackP->GetTPCpid(fDetPID[kPos][kTPC]);
  fTrackN->GetTOFpid(fDetPID[kNeg][kTOF]);
  fTrackP->GetTOFpid(fDetPID[kPos][kTOF]);
  fTrackN->GetITSpid(fDetPID[kNeg][kITS]);
  fTrackP->GetITSpid(fDetPID[kPos][kITS]);

  Long_t statusN = fTrackN->GetStatus(); 
  Long_t statusP = fTrackP->GetStatus(); 
  
  if(!(statusN & AliESDtrack::kTPCpid)){
         for(Int_t iPart = 0; iPart < AliPID::kSPECIES; iPart++){
	fDetPID[kNeg][kTPC][iPart] = 0.2;
      }    
  }
  if(!(statusN & AliESDtrack::kTOFpid)){
    for(Int_t iPart = 0; iPart < AliPID::kSPECIES; iPart++){
      fDetPID[kNeg][kTOF][iPart] = 0.2;
    }    
    
  }
  if(!(statusN & AliESDtrack::kITSpid)){
    for(Int_t iPart = 0; iPart < AliPID::kSPECIES; iPart++){
      fDetPID[kNeg][kITS][iPart] = 0.2;
    }    
  }
  if(!(statusP & AliESDtrack::kTPCpid)){
    for(Int_t iPart = 0; iPart < AliPID::kSPECIES; iPart++){
      fDetPID[kPos][kTPC][iPart] = 0.2;
    }    
  }
  if(!(statusP & AliESDtrack::kTOFpid)){
    for(Int_t iPart = 0; iPart < AliPID::kSPECIES; iPart++){
      fDetPID[kPos][kTOF][iPart] = 0.2;
    }    
    
  }
  if(!(statusP & AliESDtrack::kITSpid)){
    for(Int_t iPart = 0; iPart < AliPID::kSPECIES; iPart++){
      fDetPID[kPos][kITS][iPart] = 0.2;
    }    
  }

}
//____________________________________________________________________________________
void AliTRDv0Info::CombinePID()
{
  Double_t partrat[AliPID::kSPECIES] = {0.208, 0.010, 0.662, 0.019, 0.101};
  
  for(Int_t iSign = 0; iSign < kNDaughters; iSign++)
    {
    for(Int_t iPart = 0; iPart < AliPID::kSPECIES; iPart++)
      {
      fComPID[iSign][iPart] = (partrat[iPart]*fDetPID[iSign][kTPC][iPart]*fDetPID[iSign][kTOF][iPart])/((partrat[0]*fDetPID[iSign][kTPC][0]*fDetPID[iSign][kTOF][0])+(partrat[1]*fDetPID[iSign][kTPC][1]*fDetPID[iSign][kTOF][1])+(partrat[2]*fDetPID[iSign][kTPC][2]*fDetPID[iSign][kTOF][2])+(partrat[3]*fDetPID[iSign][kTPC][3]*fDetPID[iSign][kTOF][3])+(partrat[4]*fDetPID[iSign][kTPC][4]*fDetPID[iSign][kTOF][4]));
      
      }
    }
}
//_________________________________________________
Float_t AliTRDv0Info::Radius(AliESDv0 *esdv0)
{//distance from secondary vertex to primary vertex in x-y plane
  Double_t x, y, z;
  esdv0->GetXYZ(x,y,z); //Reconstructed coordinates of V0; to be replaced by Markus Rammler's method in case of conversions!
  fRadius = TMath::Sqrt(x*x + y*y);
  return fRadius;

}

//_________________________________________________
Int_t AliTRDv0Info::Quality(AliESDv0 *const esdv0)
{ 
  //
  // Checking track and V0 quality status in order to exclude vertices based on poor information
  //

  Float_t nClsN;
  nClsN = fTrackN->GetTPCNcls();//number of found clusters in TPC for negative track
  Float_t nClsFN;
  nClsFN = fTrackN->GetTPCNclsF();//number of findable clusters in TPC for negative track
  Float_t nClsP;
  nClsP = fTrackP->GetTPCNcls();//number of found clusters in TPC for positive track
  Float_t nClsFP;
  nClsFP = fTrackP->GetTPCNclsF();//number of findable clusters in TPC for positive track
  
  fQuality = 0;


  Float_t clsRatioN; 
  Float_t clsRatioP;

  if((nClsFN == 0) || (nClsFP == 0))
    return 2;
    
  clsRatioN = nClsN/nClsFN; //ratios of found to findable clusters in TPC 
  clsRatioP = nClsP/nClsFP;
    
  if (!(esdv0->GetOnFlyStatus()))//accept only vertices from online V0 finder
    return 3;
  if (!((fTrackP->GetStatus() &
  AliESDtrack::kTPCrefit)))//accept only vertices in which both tracks have TPC refit
    return 4;
  if (!((fTrackN->GetStatus() &
  AliESDtrack::kTPCrefit)))
    return 5;	
  if (fTrackP->GetKinkIndex(0)>0  ||
      fTrackN->GetKinkIndex(0)>0 )//exclude tracks with kinks
    return 6;
  if((clsRatioN < 0.6)||(clsRatioP < 0.6))//exclude tracks with low ratio of found to findable TPC clusters
    return 7;
  fQuality = 1;
  return fQuality;
}
//_________________________________________________
Bool_t AliTRDv0Info::GetV0PID(Int_t ipart, AliTRDtrackInfo *track)
{//decides if track is accepted for one of the reference data samples
  
  Int_t iDecay = -1;

  //decide which decay has to be considered for which particle sample (Anti-Lambda will be treated separately)
  if(ipart == AliPID::kElectron)
    iDecay = kGamma;
  else if(ipart == AliPID::kPion)
    iDecay = kK0s;
  else if(ipart == AliPID::kProton)
    iDecay = kLambda;

  Int_t iPSlot;//Mother momentum slots above/below 2.5 GeV

  
  Bool_t pid = 0;//return value for V0 PID decision

  if(!(track))
    {
      AliError("AliTRDv0Info::GetV0PID(Int_t ipart, AliTRDtrackInfo *track) : No track info found.\n");
      return 0;
    }

  AliESDInputHandler *esdH = dynamic_cast<AliESDInputHandler*>(AliAnalysisManager::GetAnalysisManager()->GetInputEventHandler());
  if(!esdH)
    {
      AliError("AliTRDv0Info::GetV0PID(Int_t ipart, AliTRDtrackInfo *track) : ERROR - ESD input handler not found");
      return 0;
    } 
      
  
  fESD = esdH->GetEvent();
  
  for(Int_t ivertex=0; ivertex<fESD->GetNumberOfV0s(); ivertex++)
    {
    
      if(pid == 0)
	{     
	  V0fromTrack(track, ivertex);//Get the V0 corresponding to the track (if there is a V0)
    
	  if(fV0Momentum > 2.5)//divide into slots according to reconstructed momentum of the mother particle
	    {iPSlot = 1;}
	  else
	    {iPSlot = 0;}
	  //Accept track for a sample only if...

	  if(!(fHasV0))//... there is a V0 found for it
	    continue;
	  if(!(fQuality == 1))//... it fulfills our quality criteria
	    continue;
	  if(!(fDCA < fUpDCA[iDecay]))//... distance of closest approach between daughters is reasonably small
	    continue;
	  if(!(fPointingAngle < fUpPointingAngle[iDecay]))//... pointing angle between momentum of mother particle and vector from prim. to sec. vertex is small
	    continue;				  
	  if(!(fRadius > fDownRadius[iDecay]))//... x-y plane distance of decay point to prim. vertex is bigger than a certain minimum value (for conversions)
	    continue;
	  if(!(fOpenAngle < fUpOpenAngle[iDecay]))//... opening angle is close enough to zero (for conversions)
	    continue;
	  if(!(TMath::Abs(fPsiPair) < fUpPsiPair[iDecay]))//... Psi-pair angle is close enough to zero(for conversions)
	    continue;

	  //specific cut criteria :
	  if(ipart == AliPID::kProton)
	    {//for proton sample: separate treatment of Lamba and Anti-Lambda decays:
	      //for Anti-Lambda:
	      //Combined PID likelihoods high enough for pi+ and anti-proton ; invariant mass calculated postulating these two particle species...
	      if((fComPID[kNeg][AliPID::kProton] > fDownComPIDneg[AliPID::kProton]) && (fComPID[kPos][AliPID::kPion] > fDownComPIDposPart[AliPID::kPion]))
		{
		  if(fNindex == fTrackID)
		    {
		      if((fInvMass[kAntiLambda] < fUpInvMass[kAntiLambda][iPSlot]) && (fInvMass[kAntiLambda] > fDownInvMass[kAntiLambda]))
			{
			  pid = 1;
			}
		    }
		}
	      //for Lambda:
	      //TPC PID likelihoods high enough for pi- and proton ; invariant mass calculated accordingly
	      if((fComPID[kNeg][AliPID::kPion] > fDownComPIDnegPart[AliPID::kPion]) && (fComPID[kPos][AliPID::kProton] > fDownComPIDpos[AliPID::kProton]))
		{
		  if(fPindex == fTrackID)
		    {
		      if((fInvMass[kLambda] < fUpInvMass[kLambda][iPSlot]) && (fInvMass[kLambda] > fDownInvMass[kLambda]))
			{
			  pid = 1;
			}
		    }
		}
	    }
	  //Invariant mass cut for K0s and photons, assuming two pions/two electrons as daughters:
	  if((fInvMass[iDecay] > fUpInvMass[iDecay][iPSlot]) || (fInvMass[iDecay] < fDownInvMass[iDecay]))
	    continue;
	  //for K0s decays: equal TPC PID likelihood criteria for both daughters ; invariant mass calculated postulating two pions
	  if(ipart == AliPID::kPion)
	    {
	      if((fDetPID[kNeg][kTPC][ipart] > fDownTPCPIDneg[ipart]) && (fDetPID[kPos][kTPC][ipart] > fDownTPCPIDpos[ipart]))
		{
		  pid = 1;						  
		}
	    }
	  //for photon conversions: equal combined PID likelihood criteria for both daughters ; invariant mass calculated postulating two electrons
	  if(ipart == AliPID::kElectron)
	    {
	      if(fMomentum > 1.75)
		{//since combined PID performs a little worse in simulations than TPC standalone for higher momenta, ONLY TPC PID is used here
		  if((fDetPID[kNeg][kTPC][ipart] > fDownTPCPIDneg[ipart]) && (fDetPID[kPos][kTPC][ipart] > fDownTPCPIDpos[ipart]))
		    {
		      pid = 1;						  
		    }
		}
	      else
		{//for low momenta, combined PID from TOF and TPC is used to get rid of proton contamination
		  if((fComPID[kNeg][ipart] > fDownComPIDneg[ipart]) && (fComPID[kPos][ipart] > fDownComPIDpos[ipart]))
		    {
		      pid = 1;						  
		    }
		}
	    }	  
	} 
    }
  return pid;
  
}
//_________________________________________________
void AliTRDv0Info::Print(Option_t */*opt*/) const
{

}
