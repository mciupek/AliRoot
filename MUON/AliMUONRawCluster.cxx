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

// Class for the MUON RecPoint
// It contains the propeorties of the physics cluters found in the tracking chambers
// RawCluster contains also the information from the both cathode of the chambers.


#include "AliMUONRawCluster.h"
#include <TArrayF.h>

ClassImp(AliMUONRawCluster);


AliMUONRawCluster::AliMUONRawCluster() {
// Constructor
    fTracks[0]=fTracks[1]=fTracks[2]=-1; 
    for (int j=0;j<2;j++) {
	fQ[j]=0;
	fX[j]=0;
	fY[j]=0;
	fMultiplicity[j]=0;
	fPeakSignal[j]=-1;
	fChi2[j]=-1;
	
	for (int k=0;k<50;k++) {
	    fIndexMap[k][j]=-1;
	    fOffsetMap[k][j]=0;
	    fContMap[k][j]=0;
	    fPhysicsMap[k]=-1;
	}
    }
    fNcluster[0]=fNcluster[1]=-1;
    fGhost=0;
}
//____________________________________________________
Int_t AliMUONRawCluster::Compare(const TObject *obj) const
{
  /*
         AliMUONRawCluster *raw=(AliMUONRawCluster *)obj;
	 Float_t r=GetRadius();
         Float_t ro=raw->GetRadius();
         if (r>ro) return 1;
         else if (r<ro) return -1;
         else return 0;
  */
         AliMUONRawCluster *raw=(AliMUONRawCluster *)obj;
	 Float_t y=fY[0];
         Float_t yo=raw->fY[0];
         if (y>yo) return 1;
         else if (y<yo) return -1;
         else return 0;

}
//____________________________________________________
Int_t AliMUONRawCluster::BinarySearch(Float_t y, TArrayF coord, Int_t from, Int_t upto)
{
   // Find object using a binary search. Array must first have been sorted.
   // Search can be limited by setting upto to desired index.

   Int_t low=from, high=upto-1, half;
   while(high-low>1) {
        half=(high+low)/2;
        if(y>coord[half]) low=half;
        else high=half;
   }
   return low;
}
//____________________________________________________
void AliMUONRawCluster::SortMin(Int_t *idx,Float_t *xdarray,Float_t *xarray,Float_t *yarray,Float_t *qarray, Int_t ntr)
{
  //
  // Get the 3 closest points(cog) one can find on the second cathode 
  // starting from a given cog on first cathode
  //
  
  //
  //  Loop over deltax, only 3 times
  //
  
    Float_t xmin;
    Int_t jmin;
    Int_t id[3] = {-2,-2,-2};
    Float_t jx[3] = {0.,0.,0.};
    Float_t jy[3] = {0.,0.,0.};
    Float_t jq[3] = {0.,0.,0.};
    Int_t jid[3] = {-2,-2,-2};
    Int_t i,j,imax;
  
    if (ntr<3) imax=ntr;
    else imax=3;
    for(i=0;i<imax;i++){
        xmin=1001.;
        jmin=0;
    
        for(j=0;j<ntr;j++){
            if ((i == 1 && j == id[i-1]) 
	          ||(i == 2 && (j == id[i-1] || j == id[i-2]))) continue;
           if (TMath::Abs(xdarray[j]) < xmin) {
	      xmin = TMath::Abs(xdarray[j]);
	      jmin=j;
           }       
        } // j
        if (xmin != 1001.) {    
           id[i]=jmin;
           jx[i]=xarray[jmin]; 
           jy[i]=yarray[jmin]; 
           jq[i]=qarray[jmin]; 
           jid[i]=idx[jmin];
        } 
    
    }  // i
  
    for (i=0;i<3;i++){
        if (jid[i] == -2) {
            xarray[i]=1001.;
            yarray[i]=1001.;
            qarray[i]=1001.;
            idx[i]=-1;
        } else {
            xarray[i]=jx[i];
            yarray[i]=jy[i];
            qarray[i]=jq[i];
            idx[i]=jid[i];
        }
    }

}

//____________________________________________________
Int_t AliMUONRawCluster::PhysicsContribution() const
{
// Evaluate physics contribution to cluster
  Int_t iPhys=0;
  Int_t iBg=0;
  Int_t iMixed=0;
  for (Int_t i=0; i<fMultiplicity[0]; i++) {
    if (fPhysicsMap[i]==2) iPhys++;
    if (fPhysicsMap[i]==1) iMixed++;
    if (fPhysicsMap[i]==0) iBg++;
  }
  if (iMixed==0 && iBg==0) {
    return 2;
  } else if ((iPhys != 0 && iBg !=0) || iMixed != 0) {
    return 1;
  } else {
    return 0;
  }
}



//____________________________________________________
void AliMUONRawCluster::DumpIndex(void)
{
    printf ("-----\n");
    for (Int_t icat=0;icat<2;icat++) {
	printf ("Mult %d\n",fMultiplicity[icat]);
	for (Int_t idig=0;idig<fMultiplicity[icat];idig++){
	    printf("Index %d",fIndexMap[idig][icat]);
	}
	printf("\n");
    }
}

//____________________________________________________
Int_t AliMUONRawCluster::AddCharge(Int_t i, Int_t Q)
{
  if (i==0 || i==1) {
    fQ[i]+=Q;
    return 1;
  }
  else  return 0;
}
//____________________________________________________
Int_t AliMUONRawCluster::AddX(Int_t i, Float_t X)
{
  if (i==0 || i==1) {
    fX[i]+=X;
    return 1;
  }
  else  return 0;
}
//____________________________________________________
Int_t AliMUONRawCluster::AddY(Int_t i, Float_t Y)
{
  if (i==0 || i==1) {
    fY[i]+=Y;
    return 1;
  }
  else return 0;
}
//____________________________________________________
Int_t AliMUONRawCluster::AddZ(Int_t i, Float_t Z)
{
  if (i==0 || i==1) {
    fZ[i]+=Z;
    return 1;
  }
  else return 0;
}
//____________________________________________________
Int_t AliMUONRawCluster::GetCharge(Int_t i) const
{
  if (i==0 || i==1) return fQ[i];
  else  return 99999;
}
//____________________________________________________
Float_t AliMUONRawCluster::GetX(Int_t i)  const
{
  if (i==0 || i==1) return fX[i];
  else  return 99999.;
}
//____________________________________________________
Float_t AliMUONRawCluster::GetY(Int_t i) const 
{
  if (i==0 || i==1) return fY[i];
  else  return 99999.;
}
//____________________________________________________
Float_t AliMUONRawCluster::GetZ(Int_t i) const 
{
  if (i==0 || i==1) return fZ[i];
  else  return 99999.;
}
//____________________________________________________
Int_t AliMUONRawCluster::GetTrack(Int_t i) const 
{
  if (i==0 || i==1 || i==2) return fTracks[i];
  else  return 99999;
}
//____________________________________________________
Int_t AliMUONRawCluster::GetPeakSignal(Int_t i) const 
{
  if (i==0 || i==1 ) return fPeakSignal[i];
  else  return 99999;
}
//____________________________________________________
Int_t AliMUONRawCluster::GetMultiplicity(Int_t i) const 
{
  if (i==0 || i==1 ) return fMultiplicity[i];
  else  return 99999;
}
//____________________________________________________
Int_t AliMUONRawCluster::GetClusterType() const 
{
  return fClusterType;
}

//____________________________________________________
Int_t AliMUONRawCluster::SetCharge(Int_t i, Int_t Q)
{
  if (i==0 || i==1) {
    fQ[i]=Q;
    return 1;
  }
  else  return 0;
}
//____________________________________________________
Int_t AliMUONRawCluster::SetX(Int_t i, Float_t X)
{
  if (i==0 || i==1) {
    fX[i]=X;
    return 1;
  }
  else  return 0;
}
//____________________________________________________
Int_t AliMUONRawCluster::SetY(Int_t i, Float_t Y)
{
  if (i==0 || i==1) {
    fY[i]=Y;
    return 1;
  }
  else return 0;
}
//____________________________________________________
Int_t AliMUONRawCluster::SetZ(Int_t i, Float_t Z)
{
  if (i==0 || i==1) {
    fZ[i]=Z;
    return 1;
  }
  else return 0;
}
//____________________________________________________
Int_t AliMUONRawCluster::SetTrack(Int_t i, Int_t track)
{
  if (i==0 || i==1 || i==2) {
    fTracks[i]=track;
    return 1;
  }
  else return 0;
}
//____________________________________________________
Int_t AliMUONRawCluster::SetPeakSignal(Int_t i, Int_t peaksignal)
{
  if (i==0 || i==1 ) {
    fPeakSignal[i]=peaksignal;
    return 1;
  }
  else return 0;
}
//____________________________________________________
Int_t AliMUONRawCluster::SetMultiplicity(Int_t i, Int_t mul)
{
  if (i==0 || i==1 ) {
    fMultiplicity[i]=mul;
    return 1;
  }
  else return 0;
}
//____________________________________________________
Int_t AliMUONRawCluster::SetClusterType(Int_t type)
{
  fClusterType=type;
  return 1;
}
